// ALTERIA Builder runtime for REV5-2 / Nano ATmega328P.
#include <Arduino.h>
#include <SPI.h>
#include <EEPROM.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#if defined(__AVR__) && (!defined(__AVR_ATmega328P__) || F_CPU!=16000000UL)
#error "ALTERIA Builder requires an ATmega328P at 16 MHz."
#endif

// Set to 1 for Serial Monitor diagnostic values (115200 baud).
#define ALTERIA_DEBUG_SERIAL 0

// ================== HARDWARE CONFIGURATION ======================
#define PIN_OUT3            2    // Digital gate INPUT in mode 3, OUTPUT otherwise
#define PIN_INPUT_LED       3    // REV5-2 IN1_LED
#define PIN_MODE_BUTTON     4    // active HIGH + hardware pulldown
#define PIN_IO_LED          5    // REV5-2 IO_LED
#define PIN_LED_MODE_1      6
#define PIN_LED_MODE_2      7
#define PIN_LED_MODE_3      8
#define PIN_LED_MODE_4      9
#define PIN_DAC_CS         10
#define PIN_DAC_LDAC       12
#define PIN_POT_1          A0
#define PIN_POT_2          A1
#define PIN_CV_INPUT       A2    // CV input through protective input circuitry
#define PIN_MANUAL_BUTTON  A3    // active HIGH + hardware pulldown
#define PIN_POT_3          A6    // REV5-2 third pot

// Theoretical calibration for REV5-2 inverting TL074 stages.
// Measure and adjust independently before connecting downstream modules.
int16_t DAC_ZERO_CODE[2]  = {2048, 2048};
int16_t DAC_PLUS5_CODE[2] = {87, 87};
int16_t DAC_MINUS5_CODE[2] = {4008, 4008};
bool bipolarOutputs = false;
const bool DAC_GAIN_2X = true;

// Set CV_INPUT ADC calibration according to REV5-2 input conditioner.
// Example 0..5V -> 0..5V MCU: zero=0, plus5=1023.
// Example -5..+5V -> 0..5V MCU: zero=512, plus5=1023.
const int16_t CV_IN_ADC_ZERO  = 0;
const int16_t CV_IN_ADC_PLUS5 = 1023;

// Digital CV-gate hysteresis is applied *after* CV calibration.
// 0..4095 corresponds to 0..+5V external input.
const uint16_t CV_GATE_ON_Q12  = 1550; // approx 1.9V
const uint16_t CV_GATE_OFF_Q12 =  900; // approx 1.1V


// UI / event parameters
const uint16_t MANUAL_DEBOUNCE_MS = 18;
const uint16_t MODE_DEBOUNCE_MS = 35;
const uint16_t POT_POLL_MS = 12;
const uint16_t CV_POLL_US = 550;     // external input edge observation
const uint16_t PROCESS_PERIOD_US = 1000; // envelope/DAC update every ~1 ms
const uint32_t EVENT_PULSE_US = 10000UL;
const uint16_t SHAPE_HYSTERESIS_ADC = 7;
const uint16_t OUTPUT_MAX = 4095;

enum CurveType : uint8_t { CURVE_LOG = 0, CURVE_LIN = 1, CURVE_EXP = 2, CURVE_S = 3 };

struct Pulse {
  bool active;
  uint32_t startedUs;
};

struct Button {
  uint8_t pin;
  uint16_t debounceMs;
  bool raw, stable, rising, falling;
  uint32_t rawSinceMs;
};

enum EngineMode : uint8_t {
  MODE_AD = 0, MODE_AR = 1, MODE_DUAL_DECAY = 2, MODE_DUCKING = 3, MODE_BURST = 4,
  MODE_ACID = 5, MODE_TREMOLO = 6, MODE_GRIDS = 7,
  MODE_DIVMULT = 8, MODE_ORBIT = 9, MODE_LOGIC_XOR = 10,
  MODE_TURING = 11, MODE_HUMANIZER = 12, MODE_BERNOULLI = 13,
  MODE_TRANCE = 14, MODE_PRESETS = 15
};

enum ADPhase : uint8_t { AD_IDLE, AD_ATTACK, AD_DECAY };

enum ARPhase : uint8_t { AR_IDLE, AR_ATTACK, AR_SUSTAIN, AR_RELEASE };

struct Vactrol {
  int32_t fastQ8;
  int32_t slowQ8;
  uint32_t strikeStartedUs;
  bool striking;
  uint16_t decayMs;
  uint16_t character;
};

struct SchEvent {
  unsigned long startUs;
  unsigned long endUs;
  uint16_t level;
};
struct PresetSlot { uint16_t a, b; uint8_t gate; };
struct ClockLane { uint32_t halfUs,nextUs; uint8_t ratio; bool high; };
// ====================== GENERAL HELPERS ===========================
static inline bool elapsedMs(uint32_t now, uint32_t since, uint32_t ms) {
  return (uint32_t)(now - since) >= ms;
}
static inline bool elapsedUs(uint32_t now, uint32_t since, uint32_t us) {
  return (uint32_t)(now - since) >= us;
}
static inline uint16_t minU16(uint16_t a, uint16_t b) { return a < b ? a : b; }
static inline uint16_t maxU16(uint16_t a, uint16_t b) { return a > b ? a : b; }

// Quadratic potentiometer response: fine adjustment of short times.
static uint32_t timeFromPotMs(uint16_t pot, uint16_t minMs, uint16_t maxMs) {
  uint32_t curved = (uint32_t)pot * (uint32_t)pot / 1023UL;
  return (uint32_t)minMs + curved * (uint32_t)(maxMs - minMs) / 1023UL;
}

// Fraction of duration in 0..4095; 100 us resolution.
// All configured envelope durations below 10 seconds.
static uint16_t phaseFromElapsed(uint32_t sinceUs, uint32_t durationUs) {
  if (sinceUs >= durationUs || durationUs < 100UL) return 4095;
  uint32_t units = sinceUs / 100UL;
  uint32_t dur = durationUs / 100UL;
  if (dur == 0) return 4095;
  uint32_t result = units * 4095UL / dur;
  return (uint16_t)(result > 4095UL ? 4095UL : result);
}



// LOG = fast at beginning (ease-out); EXP = slow at beginning (ease-in).
static uint16_t easeCurve(uint16_t x, uint8_t curve) {
  uint32_t x2 = (uint32_t)x * x / 4095UL;
  switch (curve) {
    case CURVE_LOG: return (uint16_t)(2UL*x - x2);
    case CURVE_EXP: return (uint16_t)x2;
    case CURVE_S:   return (uint16_t)(x2 * (12285UL - 2UL*x) / 4095UL);
    default:        return x;
  }
}

// Purely for envelopes (12 discrete Attack x Decay combinations).
static uint8_t shapeAttack(uint8_t s) { return (uint8_t)(s / 4); }
static uint8_t shapeDecay(uint8_t s) {
  // LOG / LIN / EXP / S for each group of 4.
  return (uint8_t)(s % 4);
}

// Fixed point one-pole smoothing, Q8 internal precision.
// dtMs is expected to be 1..20 ms; integer arithmetic remains safe.
static void onePoleQ8(int32_t &stateQ8, uint16_t target, uint16_t tauMs, uint16_t dtMs) {
  int32_t goal = (int32_t)target * 256L;
  int32_t diff = goal - stateQ8;
  if (diff > -255 && diff < 255) { stateQ8 = goal; return; }
  if (tauMs < 1) tauMs = 1;
  int32_t amount = (int32_t)((diff * (int32_t)dtMs) / ((int32_t)tauMs + dtMs));
  if (amount == 0) amount = (diff > 0) ? 1 : -1;
  stateQ8 += amount;
  if (stateQ8 < 0) stateQ8 = 0;
  if (stateQ8 > 1048320L) stateQ8 = 1048320L;
}

static uint16_t q8to12(int32_t q8) {
  if (q8 <= 0) return 0;
  if (q8 >= 1048320L) return 4095;
  return (uint16_t)(q8 / 256L);
}

// ============================ DAC =================================
uint16_t dacLastA = 0xFFFF, dacLastB = 0xFFFF;
uint16_t output1Q12 = 0, output2Q12 = 0;

static uint16_t physical5toDacCode(uint8_t channel, uint16_t voltsQ12) {
  if (voltsQ12 > 4095) voltsQ12 = 4095;
  int32_t span = (int32_t)DAC_PLUS5_CODE[channel] - (int32_t)DAC_ZERO_CODE[channel];
  int32_t code = (int32_t)DAC_ZERO_CODE[channel] + span * (int32_t)voltsQ12 / 4095L;
  if (code < 0) return 0;
  if (code > 4095) return 4095;
  return (uint16_t)code;
}

static void dacWriteRaw(uint8_t channel, uint16_t data12) {
  // MCP4822: bit15 = channel; bit13 = !gain2x; bit12 = active.
  uint16_t cmd = (uint16_t)(channel ? 0x8000U : 0U) |
                 (uint16_t)(DAC_GAIN_2X ? 0U : 0x2000U) |
                 0x1000U | (data12 & 0x0FFFU);
  SPI.beginTransaction(SPISettings(8000000UL, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_DAC_CS, LOW);
  SPI.transfer((uint8_t)(cmd >> 8));
  SPI.transfer((uint8_t)(cmd & 0xFF));
  digitalWrite(PIN_DAC_CS, HIGH);
  SPI.endTransaction();
}

static void latchDac() {
  digitalWrite(PIN_DAC_LDAC, LOW);
  digitalWrite(PIN_DAC_LDAC, HIGH);
}

static uint16_t outputToDacCode(uint8_t ch, uint16_t value) {
  if (!bipolarOutputs) return physical5toDacCode(ch, value);
  int32_t code;
  if (value <= 2048) code = DAC_ZERO_CODE[ch] +
    ((int32_t)DAC_MINUS5_CODE[ch] - DAC_ZERO_CODE[ch]) * (2048 - value) / 2048L;
  else code = DAC_ZERO_CODE[ch] +
    ((int32_t)DAC_PLUS5_CODE[ch] - DAC_ZERO_CODE[ch]) * (value - 2048) / 2047L;
  return (uint16_t)(code < 0 ? 0 : code > 4095 ? 4095 : code);
}

static void out1(uint16_t val) {
  if (val > 4095) val = 4095;
  output1Q12 = val;
  uint16_t code = outputToDacCode(0, val);
  if (code != dacLastA) { dacLastA = code; dacWriteRaw(0,code); }
}
static void out2(uint16_t val) {
  if (val > 4095) val = 4095;
  output2Q12 = val;
  uint16_t code = outputToDacCode(1, val);
  if (code != dacLastB) { dacLastB = code; dacWriteRaw(1,code); }
}



static void firePulse(Pulse &p, uint32_t nowUs) {
  p.active = true;
  p.startedUs = nowUs;
}
static void servicePulse(Pulse &p, uint32_t nowUs) {
  if (p.active && elapsedUs(nowUs,p.startedUs,EVENT_PULSE_US)) p.active = false;
}

// ============================ INPUTS ==============================

Button manualBtn = { PIN_MANUAL_BUTTON,MANUAL_DEBOUNCE_MS,false,false,false,false,0 };
Button modeBtn   = { PIN_MODE_BUTTON,MODE_DEBOUNCE_MS,false,false,false,false,0 };

static void scanButton(Button &b,uint32_t nowMs) {
  b.rising = b.falling = false;
  bool raw = (digitalRead(b.pin) == HIGH);
  if (raw != b.raw) { b.raw = raw; b.rawSinceMs = nowMs; }
  if (raw != b.stable && elapsedMs(nowMs,b.rawSinceMs,b.debounceMs)) {
    b.stable = raw;
    b.rising = raw;
    b.falling = !raw;
  }
}

uint16_t pot1 = 0, pot2 = 0, pot3 = 0;
uint8_t shapeSector = 5; // initial 06: LIN/LIN (will update on actual pot position)
uint32_t lastPotsMs = 0;
uint16_t cvLevelQ12 = 0;
bool cvGate = false, cvRising = false, cvFalling = false;
uint32_t lastCvPollUs = 0;

static uint16_t normalizeCv(int adc) {
  int32_t denominator = (int32_t)CV_IN_ADC_PLUS5 - CV_IN_ADC_ZERO;
  if (denominator == 0) return 0;
  int32_t val = ((int32_t)adc - CV_IN_ADC_ZERO)*4095L / denominator;
  if (val < 0) val = 0;
  if (val > 4095) val = 4095;
  return (uint16_t)val;
}

static void readCvInput(uint32_t nowUs) {
  cvRising = cvFalling = false;
  if (!elapsedUs(nowUs,lastCvPollUs,CV_POLL_US)) return;
  lastCvPollUs = nowUs;
  cvLevelQ12 = normalizeCv(analogRead(PIN_CV_INPUT));
  bool next = cvGate;
  if (!cvGate && cvLevelQ12 >= CV_GATE_ON_Q12) next = true;
  if (cvGate && cvLevelQ12 <= CV_GATE_OFF_Q12) next = false;
  cvRising = (!cvGate && next);
  cvFalling = (cvGate && !next);
  cvGate = next;
}

static void selectShapeWithHysteresis(uint16_t p) {
  uint8_t target = (uint8_t)((uint32_t)p*12UL/1024UL);
  if (target > 11) target = 11;
  if (target == shapeSector) return;
  if (target > shapeSector) {
    uint16_t threshold = (uint16_t)(((uint32_t)(shapeSector+1)*1024UL)/12UL);
    if (p >= threshold + SHAPE_HYSTERESIS_ADC) shapeSector = target;
  } else {
    int16_t threshold = (int16_t)(((uint32_t)shapeSector*1024UL)/12UL);
    if ((int16_t)p <= threshold - (int16_t)SHAPE_HYSTERESIS_ADC) shapeSector = target;
  }
}

static void readPots(uint32_t nowMs) {
  if (!elapsedMs(nowMs,lastPotsMs,POT_POLL_MS)) return;
  lastPotsMs = nowMs;
  pot1 = (uint16_t)analogRead(PIN_POT_1);
  pot2 = (uint16_t)analogRead(PIN_POT_2);
  pot3 = (uint16_t)analogRead(PIN_POT_3);
  selectShapeWithHysteresis(pot3);
}


#include <math.h>
bool modeJustChanged=true,resetTriggered=false;
bool modePressArmed=false,modeLongHandled=false; uint32_t modePressedMs=0,lastUpdateUs=0;
bool pendingTrigger=false,pendingTriggerA=false,pendingTriggerB=false,pendingCv=false,pendingManual=false;
bool ioGate=false,ioRising=false; uint8_t slot=0;
static bool checkReset(){bool r=resetTriggered;resetTriggered=false;return r;}
static void dacSetA(uint16_t v){out1(v);} static void dacSetB(uint16_t v){out2(v);}
static void setOutA(uint16_t v){out1(v);} static void setOutB(uint16_t v){out2(v);}
static void setGate(bool v){digitalWrite(PIN_OUT3,v?HIGH:LOW);}
static void resetOutputs(){setGate(false);out1(bipolarOutputs?2048:0);out2(bipolarOutputs?2048:0);}
#define ctrl1Val pot1
#define ctrl2Val pot2
#define ctrl3Val pot3
#define GATEOUT PIN_OUT3
#define SHORT_PULSE_MS 10UL
#define DEFAULT_PULSE_MS 10UL
#define TRIG_PULSE_MS 15UL
#define RANDOM_SEED_PIN A7
#define resetLevelNow cvGate
#define manualLevelNow manualBtn.stable
static void setLedPattern(uint8_t bits){for(uint8_t i=0;i<4;i++)digitalWrite(PIN_LED_MODE_1+i,(bits>>i)&1);}
static void enterSlot(uint8_t next);

static bool manualHeld(){return manualBtn.stable;}
