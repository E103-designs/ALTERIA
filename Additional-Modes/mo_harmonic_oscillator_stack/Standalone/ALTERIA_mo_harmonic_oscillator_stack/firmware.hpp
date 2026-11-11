// ALTERIA Builder 3.1.0. Firmware 1.0.0. Slots are defined by configuration.json.
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

enum { CH_A = 0, CH_B = 1, CH_G = 2 };

const uint8_t SCH_DEPTH = 8;
const uint8_t SCH_MASK = SCH_DEPTH - 1;



SchEvent schQueue[3][SCH_DEPTH];
uint8_t schHead[3]  = {0,0,0};
uint8_t schTail[3]  = {0,0,0};
uint8_t schCount[3] = {0,0,0};
bool schOn[3]       = {false,false,false};
unsigned long schOffUs[3] = {0,0,0};
uint16_t schCurrentLevel[3] = {0,0,0};

static inline unsigned long msToUs(unsigned long ms) {
  return ms * 1000UL;
}

static inline bool timeReachedUs(unsigned long nowUs, unsigned long targetUs) {
  return (long)(nowUs - targetUs) >= 0;
}

static inline bool timeAfterUs(unsigned long aUs, unsigned long bUs) {
  return (long)(aUs - bUs) > 0;
}

static inline void schWriteOff(uint8_t ch) {
  if (ch == CH_A) dacSetA(0);
  else if (ch == CH_B) dacSetB(0);
  else setGate(false);
}

static inline void schWriteOn(uint8_t ch, uint16_t level) {
  if (ch == CH_A) dacSetA(level);
  else if (ch == CH_B) dacSetB(level);
  else setGate(true);
}

static inline void schClearChannel(uint8_t ch) {
  if (schOn[ch]) schWriteOff(ch);
  schHead[ch] = 0;
  schTail[ch] = 0;
  schCount[ch] = 0;
  schOn[ch] = false;
  schOffUs[ch] = 0;
  schCurrentLevel[ch] = 0;
}

static inline void schClearAll() {
  for (uint8_t i = 0; i < 3; i++) {
    schClearChannel(i);
  }
}

static inline void schEnqueue(uint8_t ch, unsigned long startUs, unsigned long endUs, uint16_t level) {
  if (schCount[ch] >= SCH_DEPTH) {
    schTail[ch] = (uint8_t)((schTail[ch] + 1) & SCH_MASK);
    schCount[ch]--;
  }

  SchEvent &ev = schQueue[ch][schHead[ch]];
  ev.startUs = startUs;
  ev.endUs = endUs;
  ev.level = level;
  schHead[ch] = (uint8_t)((schHead[ch] + 1) & SCH_MASK);
  schCount[ch]++;
}

static inline void schPulseA(unsigned long now, unsigned long delayMs, unsigned long widthMs, uint16_t level) {
  unsigned long startUs = micros() + (uint32_t)((int32_t)(now - millis()) * 1000L) + msToUs(delayMs);
  schEnqueue(CH_A, startUs, startUs + msToUs(widthMs ? widthMs : 1), level);
}

static inline void schPulseB(unsigned long now, unsigned long delayMs, unsigned long widthMs, uint16_t level) {
  unsigned long startUs = micros() + (uint32_t)((int32_t)(now - millis()) * 1000L) + msToUs(delayMs);
  schEnqueue(CH_B, startUs, startUs + msToUs(widthMs ? widthMs : 1), level);
}

static inline void schPulseG(unsigned long now, unsigned long delayMs, unsigned long widthMs) {
  unsigned long startUs = micros() + (uint32_t)((int32_t)(now - millis()) * 1000L) + msToUs(delayMs);
  schEnqueue(CH_G, startUs, startUs + msToUs(widthMs ? widthMs : 1), 4095);
}

static inline void schServiceChannel(uint8_t ch, unsigned long nowUs) {
  if (schOn[ch] && timeReachedUs(nowUs, schOffUs[ch])) {
    schWriteOff(ch);
    schOn[ch] = false;
    schCurrentLevel[ch] = 0;
  }

  uint8_t guard = 0;
  while (schCount[ch] > 0 && guard < SCH_DEPTH) {
    SchEvent ev = schQueue[ch][schTail[ch]];
    if (!timeReachedUs(nowUs, ev.startUs)) break;

    schTail[ch] = (uint8_t)((schTail[ch] + 1) & SCH_MASK);
    schCount[ch]--;
    guard++;

    if (!timeAfterUs(ev.endUs, nowUs)) {
      ev.endUs = nowUs + 1000UL;
    }

    if (schOn[ch]) {
      if (ch != CH_G && schCurrentLevel[ch] != ev.level) {
        schWriteOn(ch, ev.level);
        schCurrentLevel[ch] = ev.level;
      }
      if (timeAfterUs(ev.endUs, schOffUs[ch])) {
        schOffUs[ch] = ev.endUs;
      }
    } else {
      schWriteOn(ch, ev.level);
      schCurrentLevel[ch] = ev.level;
      schOffUs[ch] = ev.endUs;
      schOn[ch] = true;
    }
  }
}

static inline void schService(unsigned long now) {
  (void)now;
  unsigned long nowUs = micros();
  schServiceChannel(CH_A, nowUs);
  schServiceChannel(CH_B, nowUs);
  schServiceChannel(CH_G, nowUs);
}


static inline int mapQuadraticTo(int value, int toLow, int toHigh) {
  unsigned long s = (unsigned long)value * 1024UL / 1023UL;
  unsigned long q = (s * s) / 1024UL;
  return (int)(toLow + (q * (unsigned long)(toHigh - toLow)) / 1024UL);
}
static inline unsigned long pulseWidthFromClock(unsigned long cMs) {
  if (cMs == 0) return 10UL;
  unsigned long pw = cMs / 4UL;
  if (pw < 2) pw = 2;
  if (pw > 12) pw = 12;
  return pw;
}
static inline unsigned long pulseWidthFromPot3(unsigned long baseMs, unsigned long absoluteMaxMs) {
  if (baseMs == 0) baseMs = 40;
  unsigned long maxW = baseMs - 1;
  if (maxW < 2) maxW = 2;
  if (maxW > absoluteMaxMs) maxW = absoluteMaxMs;
  if (maxW < 2) maxW = 2;
  return (unsigned long)map(ctrl3Val, 0, 1023, 2, (int)maxW);
}
static inline uint16_t quantizeByPot(uint16_t v, int pot) {
  if (pot < 150) return v;

  uint8_t steps;
  if (pot < 340) steps = 24;
  else if (pot < 520) steps = 12;
  else if (pot < 700) steps = 8;
  else if (pot < 880) steps = 5;
  else steps = 3;

  uint32_t idx = ((uint32_t)v * (uint32_t)(steps - 1) + 2047UL) / 4095UL;
  return (uint16_t)((idx * 4095UL) / (uint32_t)(steps - 1));
}
static inline uint16_t rot16(uint16_t p, uint8_t r) {
  r &= 15;
  if (r == 0) return p;
  return (uint16_t)((p >> r) | (p << (16 - r)));
}













static inline uint16_t periodFromSpeedPotMs(uint16_t pot, uint16_t minMs, uint16_t maxMs) {
  // A rate knob shortens its period clockwise; duration knobs stay unchanged.
  return (uint16_t)mapQuadraticTo(1023-pot, minMs, maxMs);
}

static inline uint16_t shapeProgressByPot(uint16_t x, int pot) {
  if (x > 4095) x = 4095;

  if (pot < 341) {
    // Fast start, slow end. Approx log-like curve.
    uint16_t inv = (uint16_t)(4095 - x);
    uint32_t y = 4095UL - ((uint32_t)inv * (uint32_t)inv) / 4095UL;
    return (uint16_t)y;
  }

  if (pot < 682) {
    // Linear region.
    return x;
  }

  // Slow start, fast end. Approx expo-like curve.
  uint32_t y = ((uint32_t)x * (uint32_t)x) / 4095UL;
  return (uint16_t)y;
}
static inline uint16_t triangleFromPhase16(uint16_t phase) {
  uint16_t p = phase & 0x7FFF;
  uint32_t x = ((uint32_t)p * 4095UL) / 32767UL;
  if (phase & 0x8000) x = 4095UL - x;
  return (uint16_t)x;
}
static inline uint16_t clamp12(int32_t v) {
  if (v < 0) return 0;
  if (v > 4095) return 4095;
  return (uint16_t)v;
}

inline float clampf(float x, float lo, float hi) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

inline float fract1(float x) {
  x = x - floorf(x);
  if (x < 0.0f) x += 1.0f;
  return x;
}

inline float lerpf(float a, float b, float t) {
  return a + (b - a) * t;
}

inline float pot3f() {
  return (float)ctrl3Val / 1023.0f;
}

inline float rand01() {
  return (float)random(0, 10000) / 9999.0f;
}

inline uint16_t clamp12i(int32_t v) {
  if (v < 0) return 0;
  if (v > 4095) return 4095;
  return (uint16_t)v;
}

inline uint16_t unipolarTo12(float x01) {
  x01 = clampf(x01, 0.0f, 1.0f);
  return (uint16_t)(x01 * 4095.0f + 0.5f);
}

inline uint16_t bipolarTo12(float xBi) {
  xBi = clampf(xBi, -1.0f, 1.0f);
  return (uint16_t)((xBi * 0.5f + 0.5f) * 4095.0f + 0.5f);
}

inline float shapeUniByPot3(float x) {
  x = clampf(x, 0.0f, 1.0f);
  float k = pot3f();

  if (k < 0.48f) {
    float amt = (0.48f - k) / 0.48f;
    float e = 1.0f + 4.0f * amt;
    return 1.0f - powf(1.0f - x, e); // fast start
  }

  if (k > 0.52f) {
    float amt = (k - 0.52f) / 0.48f;
    float e = 1.0f + 4.0f * amt;
    return powf(x, e); // slow start
  }

  return x;
}

inline float shapeBiByPot3(float x) {
  float sign = (x < 0.0f) ? -1.0f : 1.0f;
  float y = shapeUniByPot3(fabsf(x));
  return sign * y;
}

inline float foldBiByPot3(float x) {
  float amount = pot3f();
  float drive = lerpf(1.0f, 4.0f, amount);
  x *= drive;
  while (x > 1.0f || x < -1.0f) {
    if (x > 1.0f) x = 2.0f - x;
    if (x < -1.0f) x = -2.0f - x;
  }
  return clampf(x, -1.0f, 1.0f);
}

inline uint16_t quantize12ByPot3(uint16_t v) {
  int pot = ctrl3Val;
  if (pot < 150) return v;

  uint8_t steps;
  if (pot < 340) steps = 24;
  else if (pot < 520) steps = 12;
  else if (pot < 700) steps = 8;
  else if (pot < 880) steps = 5;
  else steps = 3;

  uint32_t idx = ((uint32_t)v * (uint32_t)(steps - 1) + 2047UL) / 4095UL;
  return (uint16_t)((idx * 4095UL) / (uint32_t)(steps - 1));
}


int mapQuadratic(int value, int fromLow, int fromHigh, int toLow, int toHigh) {
  unsigned long scale = (unsigned long)(value - fromLow) * 1024UL / (unsigned long)(fromHigh - fromLow);
  return toLow + (int)((((scale * scale) / 1024UL) * (unsigned long)(toHigh - toLow)) / 1024UL);
}
unsigned long mapLogMs(int value, unsigned long toLow, unsigned long toHigh) {
  float minLog = logf((float)toLow);
  float maxLog = logf((float)toHigh);
  float s = (float)value / 1023.0f;
  float r = expf(minLog + (maxLog - minLog) * s);
  return (unsigned long)(r + 0.5f);
}
uint8_t popcount16(uint16_t x) {
  uint8_t c = 0;
  while (x) {
    c += (x & 1U);
    x >>= 1;
  }
  return c;
}
float tri01(float p) {
  p = fract1(p);
  if (p < 0.5f) return p * 2.0f;
  return 2.0f - p * 2.0f;
}
float asymTri01(float p, float warp) {
  p = fract1(p);
  warp = clampf(warp, 0.05f, 0.95f);
  if (p < warp) return p / warp;
  return 1.0f - ((p - warp) / (1.0f - warp));
}

volatile uint8_t audioOwner=0;bool chordTracked=false;uint32_t chordSince=0;

// 1: mo_harmonic_oscillator_stack; algorithm SHA256 bf9c0e5edf9d4d93dad0363799a098b47970cdf95acfee937fd8ca34a344c2db
namespace mo_harmonic_oscillator_stack {
enum MotionMode {
  MODE_AD_ENVELOPE = 0,
  MODE_SLEW_FUNCTION,
  MODE_PENDULUM,
  MODE_RADIOACTIVE_GEIGER,
  MODE_SWARM,
  MODE_NEURAL_GHOST,
  MODE_PHASE_FORGE,
  MODE_VECTOR_DRIFT,
  MODE_ENERGY_FIELD,
  MODE_BINARY_BLOOM,
  MODE_CHAOS_MAP,
  MODE_SAMPLE_HOLD_MORPH,
  MODE_MIRROR_LFO,
  MODE_HARMONIC_STACK,
  MODE_RANDOM_WALK_CONSTRAIN,
  MODE_CV_CROSS_MOD
};


int enteredMode=-1;
void handleModeHarmonicStack() {
  static bool inited = false;
  static unsigned long lastUs = 0;
  static float phase = 0.0f;
  static bool gOn = false;
  static unsigned long gStart = 0;

  if (enteredMode == MODE_HARMONIC_STACK) {
    enteredMode = -1;
    inited = false;
    phase = 0.0f;
    gOn = false;
    digitalWrite(GATEOUT, LOW);
  }

  if (checkReset()) phase = 0.0f;

  unsigned long nowUs = micros();
  if (!inited) {
    lastUs = nowUs;
    inited = true;
  }
  float dt = (float)(nowUs - lastUs) * 1e-6f;
  lastUs = nowUs;
  if (dt > 0.05f) dt = 0.05f;

  float base = lerpf(0.03f, 15.0f, (float)ctrl1Val / 1023.0f);
  float mix = (float)ctrl2Val / 1023.0f;
  float oldPhase = phase;
  phase = fract1(phase + base * dt);
  bool wrapped = (phase < oldPhase);

  float p = 2.0f * PI * phase;
  float s1 = sinf(p);
  float s2 = sinf(2.0f * p);
  float s3 = sinf(3.0f * p);
  float s5 = sinf(5.0f * p);

  float a = (1.00f * s1 + (0.65f + 0.25f * mix) * s2 + (0.30f + 0.50f * mix) * s3) / (1.95f + 0.75f * mix);
  float b = (0.90f * s2 + (0.45f + 0.35f * mix) * s3 + (0.20f + 0.70f * mix) * s5) / (1.55f + 1.05f * mix);

  a = foldBiByPot3(a);
  b = foldBiByPot3(b);

  setOutA(bipolarTo12(a));
  setOutB(bipolarTo12(b));

  unsigned long t = millis();
  if (wrapped) {
    digitalWrite(GATEOUT, HIGH);
    gStart = t;
    gOn = true;
  }
  if (gOn && (t - gStart >= 8UL)) {
    digitalWrite(GATEOUT, LOW);
    gOn = false;
  }
}

}

uint32_t slotEnteredAt=0;
const uint16_t CAL_BASE=256;
const uint8_t CAL_BYTES=22,CAL_STRIDE=24;
const uint16_t CAL_FACTORY[3][2]={{2048,2048},{87,87},{4008,4008}};
const uint16_t CAL_CENTER_LOW=442,CAL_CENTER_HIGH=582;
bool calibrationActive=false,calBootPending=false,calNeedRelease=false;
bool calReady[2]={false,false},calSaved=false,calError=false;
bool calManualArmed=false,calModeArmed=false,calModeLong=false;
bool calResetTracked=false;
uint8_t calStage=0,calBank=1,calWriteBank=0;
uint16_t calSequence=0,calWork[3][2],calAnchor[3][2];
uint8_t calRecord[CAL_BYTES];
int8_t calWriteStep=-1;
uint32_t calBootStart=0,calManualSince=0,calModeSince=0;
uint32_t calNext[2]={0,0},calSavedAt=0,calReportAt=0;
uint32_t calResetSince=0;
bool calHasStored=false;

static uint16_t calCrc(const uint8_t *p) {
  uint16_t c=0xFFFF;
  for(uint8_t i=0;i<19;i++) {
    c^=(uint16_t)p[i]<<8;
    for(uint8_t b=0;b<8;b++)c=(c&0x8000)?(uint16_t)((c<<1)^0x1021):(uint16_t)(c<<1);
  }
  return c;
}
static bool calValid(const uint8_t *r) {
  if(r[0]!='A'||r[1]!='C'||r[2]!='A'||r[3]!='L'||r[4]!=1||r[21]!=0xA5)return false;
  uint16_t crc=calCrc(r);
  if(r[19]!=(uint8_t)crc||r[20]!=(uint8_t)(crc>>8))return false;
  uint16_t p[3][2];
  for(uint8_t s=0;s<3;s++)for(uint8_t ch=0;ch<2;ch++) {
    uint8_t i=7+s*4+ch*2;p[s][ch]=(uint16_t)r[i]|((uint16_t)r[i+1]<<8);
    if(p[s][ch]>4095)return false;
  }
  for(uint8_t ch=0;ch<2;ch++)
    if(p[0][ch]<1024||p[0][ch]>3072||p[1][ch]+512>=p[0][ch]||p[0][ch]+512>=p[2][ch])return false;
  return true;
}
static uint16_t calActivePoint(uint8_t stage,uint8_t ch) {
  return stage==0?DAC_ZERO_CODE[ch]:stage==1?DAC_PLUS5_CODE[ch]:DAC_MINUS5_CODE[ch];
}
static void calApplyRecord(const uint8_t *r) {
  for(uint8_t ch=0;ch<2;ch++) {
    DAC_ZERO_CODE[ch]=(uint16_t)r[7+ch*2]|((uint16_t)r[8+ch*2]<<8);
    DAC_PLUS5_CODE[ch]=(uint16_t)r[11+ch*2]|((uint16_t)r[12+ch*2]<<8);
    DAC_MINUS5_CODE[ch]=(uint16_t)r[15+ch*2]|((uint16_t)r[16+ch*2]<<8);
  }
}
static void loadCalibration() {
  for(uint8_t ch=0;ch<2;ch++) {
    DAC_ZERO_CODE[ch]=CAL_FACTORY[0][ch];DAC_PLUS5_CODE[ch]=CAL_FACTORY[1][ch];DAC_MINUS5_CODE[ch]=CAL_FACTORY[2][ch];
  }
  uint8_t r[2][CAL_BYTES];
  for(uint8_t b=0;b<2;b++)for(uint8_t i=0;i<CAL_BYTES;i++)r[b][i]=EEPROM.read(CAL_BASE+b*CAL_STRIDE+i);
  bool a=calValid(r[0]),b=calValid(r[1]);
  calHasStored=a||b;calBank=1;calSequence=0;
  if(!calHasStored)return;
  uint16_t sa=(uint16_t)r[0][5]|((uint16_t)r[0][6]<<8);
  uint16_t sb=(uint16_t)r[1][5]|((uint16_t)r[1][6]<<8);
  calBank=!a?1:!b?0:((int16_t)(sb-sa)>0?1:0);
  calSequence=calBank?sb:sa;calApplyRecord(r[calBank]);
}
static void calSelectStage(uint8_t next,uint32_t now) {
  calStage=next;calReady[0]=calReady[1]=false;
  calNext[0]=calNext[1]=now;calManualArmed=false;calModeArmed=false;
}
static void startCalibration(uint32_t now) {
  calibrationActive=true;calNeedRelease=true;calSaved=false;calError=false;
  calWriteStep=-1;calModeLong=false;calResetTracked=false;
  pinMode(PIN_OUT3,INPUT);digitalWrite(PIN_OUT3,LOW);pinMode(A5,INPUT);
  for(uint8_t s=0;s<3;s++)for(uint8_t ch=0;ch<2;ch++)calWork[s][ch]=calAnchor[s][ch]=calActivePoint(s,ch);
  calSelectStage(0,now);calReportAt=now;
  Serial.begin(115200);
  Serial.println(F("CAL: release buttons. Center P1/P2; CW raises volts."));
}
static void calExit() {
  calibrationActive=false;calBootPending=false;calNeedRelease=false;
  calManualArmed=calModeArmed=false;modePressArmed=false;
  cvLevelQ12=normalizeCv(analogRead(PIN_CV_INPUT));
  cvGate=cvLevelQ12>=CV_GATE_ON_Q12;cvRising=cvFalling=false;lastCvPollUs=micros();
#if !ALTERIA_DEBUG_SERIAL
  Serial.end();
#endif
  enterSlot(0);
}
static void serviceCalBoot(uint32_t now) {
  if(digitalRead(PIN_MODE_BUTTON)!=HIGH||digitalRead(PIN_MANUAL_BUTTON)!=HIGH) {
    calBootPending=false;enterSlot(0);return;
  }
  if(elapsedMs(now,calBootStart,100)) {calBootPending=false;startCalibration(now);}
}
static void calWriteOutput() {
  uint8_t s=calStage<3?calStage:0;
  uint16_t a=calWork[s][0],b=calWork[s][1];
  if(dacLastA!=a){dacWriteRaw(0,a);dacLastA=a;}
  if(dacLastB!=b){dacWriteRaw(1,b);dacLastB=b;}
  latchDac();
}
static void calBuildRecord() {
  calRecord[0]='A';calRecord[1]='C';calRecord[2]='A';calRecord[3]='L';calRecord[4]=1;
  uint16_t seq=calSequence+1;calRecord[5]=(uint8_t)seq;calRecord[6]=seq>>8;
  for(uint8_t s=0;s<3;s++)for(uint8_t ch=0;ch<2;ch++) {
    uint8_t p=7+s*4+ch*2;calRecord[p]=(uint8_t)calWork[s][ch];calRecord[p+1]=calWork[s][ch]>>8;
  }
  uint16_t crc=calCrc(calRecord);calRecord[19]=(uint8_t)crc;calRecord[20]=crc>>8;calRecord[21]=0xA5;
}
static void calStartSave(uint32_t now) {
  calBuildRecord();calError=false;
  if(!calValid(calRecord)){calError=true;return;}
  bool equal=true;
  for(uint8_t s=0;s<3;s++)for(uint8_t ch=0;ch<2;ch++)if(calWork[s][ch]!=calActivePoint(s,ch))equal=false;
  if(equal&&calHasStored){calSaved=true;calSavedAt=now;return;}
  calWriteBank=calBank^1;calWriteStep=0;
}
static void serviceCalSave(uint32_t now) {
  if(!eeprom_is_ready())return;
  uint16_t addr=CAL_BASE+calWriteBank*CAL_STRIDE;
  if(calWriteStep==0)EEPROM.update(addr+21,0);
  else if(calWriteStep<=21)EEPROM.update(addr+calWriteStep-1,calRecord[calWriteStep-1]);
  else if(calWriteStep==22)EEPROM.update(addr+21,0xA5);
  else {
    uint8_t verify[CAL_BYTES];for(uint8_t i=0;i<CAL_BYTES;i++)verify[i]=EEPROM.read(addr+i);
    calWriteStep=-1;
    if(!calValid(verify)){calError=true;return;}
    for(uint8_t i=0;i<CAL_BYTES;i++)if(verify[i]!=calRecord[i]){calError=true;return;}
    calBank=calWriteBank;calSequence=(uint16_t)verify[5]|((uint16_t)verify[6]<<8);
    calApplyRecord(verify);calHasStored=true;calSaved=true;calSavedAt=now;
    Serial.print(F("SAVED ZERO="));Serial.print(DAC_ZERO_CODE[0]);Serial.print(',');Serial.print(DAC_ZERO_CODE[1]);
    Serial.print(F(" PLUS5="));Serial.print(DAC_PLUS5_CODE[0]);Serial.print(',');Serial.print(DAC_PLUS5_CODE[1]);
    Serial.print(F(" MINUS5="));Serial.print(DAC_MINUS5_CODE[0]);Serial.print(',');Serial.println(DAC_MINUS5_CODE[1]);
    return;
  }
  calWriteStep++;
}
static void calAdjust(uint8_t ch,uint16_t pot,uint32_t now) {
  bool centered=pot>=CAL_CENTER_LOW&&pot<=CAL_CENTER_HIGH;
  if(!calReady[ch]) {if(centered){calReady[ch]=true;calNext[ch]=now;}return;}
  if(centered){calNext[ch]=now;return;}
  uint16_t distance=pot<CAL_CENTER_LOW?CAL_CENTER_LOW-pot:pot-CAL_CENTER_HIGH;
  uint16_t interval=300-(uint32_t)distance*260/442;
  if(!elapsedMs(now,calNext[ch],interval))return;
  calNext[ch]=now;
  int16_t code=(int16_t)calWork[calStage][ch]+(pot>CAL_CENTER_HIGH?-1:1);
  int16_t lo=(int16_t)calAnchor[calStage][ch]-256;
  int16_t hi=(int16_t)calAnchor[calStage][ch]+256;
  if(lo<0)lo=0;if(hi>4095)hi=4095;
  if(code<lo)code=lo;if(code>hi)code=hi;
  calWork[calStage][ch]=(uint16_t)code;
}
static void serviceCalibration(uint32_t now) {
  calWriteOutput();
  uint8_t bits=calSaved?15:(calWriteStep>=0||calError)?(((now/100)&1)?15:0):
    (calStage==3?(((now/250)&1)?8:0):(1U<<calStage));
  for(uint8_t i=0;i<4;i++)digitalWrite(PIN_LED_MODE_1+i,(bits&(1U<<i))?HIGH:LOW);
  digitalWrite(PIN_INPUT_LED,calReady[0]?HIGH:LOW);digitalWrite(PIN_IO_LED,calReady[1]?HIGH:LOW);
  if(calSaved){if(elapsedMs(now,calSavedAt,800))calExit();return;}
  if(calWriteStep>=0){serviceCalSave(now);return;}
  if(calNeedRelease) {if(!manualBtn.stable&&!modeBtn.stable)calNeedRelease=false;return;}
  if(modeBtn.stable&&manualBtn.stable) {
    if(!calResetTracked){calResetTracked=true;calResetSince=now;calManualArmed=calModeArmed=false;}
    if(elapsedMs(now,calResetSince,3000)) {
      for(uint8_t s=0;s<3;s++)for(uint8_t ch=0;ch<2;ch++)calWork[s][ch]=calAnchor[s][ch]=CAL_FACTORY[s][ch];
      calResetTracked=false;calNeedRelease=true;calError=false;calSelectStage(0,now);
      Serial.println(F("CAL factory defaults: confirm all points to save"));
    }
    return;
  }
  if(calResetTracked) {
    if(!modeBtn.stable&&!manualBtn.stable){calResetTracked=false;calManualArmed=calModeArmed=false;}
    return;
  }
  if(modeBtn.rising){calModeArmed=true;calModeLong=false;calModeSince=now;}
  if(calModeArmed&&modeBtn.stable&&!calModeLong&&elapsedMs(now,calModeSince,1200)) {
    calModeLong=true;Serial.println(F("CAL cancelled"));calExit();return;
  }
  if(modeBtn.falling&&calModeArmed){calModeArmed=false;if(!calModeLong){calError=false;calSelectStage(calStage>0?calStage-1:0,now);}return;}
  if(manualBtn.rising){calManualArmed=true;calManualSince=now;}
  if(calStage==3&&calManualArmed&&manualBtn.stable&&elapsedMs(now,calManualSince,1000)) {
    calManualArmed=false;calStartSave(now);return;
  }
  if(manualBtn.falling&&calManualArmed) {
    calManualArmed=false;
    if(calStage<3&&calReady[0]&&calReady[1]&&
       pot1>=CAL_CENTER_LOW&&pot1<=CAL_CENTER_HIGH&&pot2>=CAL_CENTER_LOW&&pot2<=CAL_CENTER_HIGH)
      calSelectStage(calStage+1,now);
    return;
  }
  if(calStage<3) {
    calAdjust(0,pot1,now);calAdjust(1,pot2,now);
    if(elapsedMs(now,calReportAt,500)) {
      calReportAt=now;Serial.print(F("CAL step="));Serial.print(calStage);
      Serial.print(F(" A="));Serial.print(calWork[calStage][0]);Serial.print(F(" B="));Serial.println(calWork[calStage][1]);
    }
  }
}

static bool selectedFast(){switch(slot){default:return false;}}
static bool selectedInput(){switch(slot){default:return false;}}
static bool selectedComposite(){switch(slot){default:return false;}}
static void displayModes(uint32_t nowMs){
 uint8_t bits=(slot+1)&15;


 setLedPattern(bits);
}
static void enterSlot(uint8_t next){
 audioOwner=0;

 schClearAll();pinMode(PIN_OUT3,INPUT);digitalWrite(PIN_OUT3,LOW);
 slot=next%1;slotEnteredAt=millis();modeJustChanged=true;
 pendingTrigger=pendingTriggerA=pendingTriggerB=pendingCv=pendingManual=resetTriggered=false;
 bipolarOutputs=false;
 switch(slot){case 0:bipolarOutputs=true;break;default:break;}
 dacLastA=dacLastB=0xFFFF;out1(bipolarOutputs?2048:0);out2(bipolarOutputs?2048:0);latchDac();
 pinMode(PIN_OUT3,selectedInput()?INPUT:OUTPUT);
 ioGate=selectedInput()&&digitalRead(PIN_OUT3)==HIGH;ioRising=false;
 cvLevelQ12=normalizeCv(analogRead(PIN_CV_INPUT));cvGate=cvLevelQ12>=CV_GATE_ON_Q12;cvRising=cvFalling=false;
 switch(slot){case 0:{using namespace mo_harmonic_oscillator_stack;enteredMode=MODE_HARMONIC_STACK;break;}default:break;}
 lastUpdateUs=micros();displayModes(millis());
}
static bool serviceModeButton(uint32_t nowMs){
 if(modeBtn.rising){modePressArmed=true;modeLongHandled=false;modePressedMs=nowMs;}
 if(modePressArmed&&modeBtn.stable&&!modeLongHandled&&elapsedMs(nowMs,modePressedMs,650)){
  modeLongHandled=true;

 }
 if(modeBtn.falling&&modePressArmed){modePressArmed=false;if(!modeLongHandled&&1>1){enterSlot((slot+1)%1);return true;}}
 return false;
}
void setup(){
 pinMode(PIN_MODE_BUTTON,INPUT);pinMode(PIN_MANUAL_BUTTON,INPUT);pinMode(PIN_CV_INPUT,INPUT);pinMode(PIN_OUT3,INPUT);digitalWrite(PIN_OUT3,LOW);pinMode(A5,INPUT);
 pinMode(PIN_INPUT_LED,OUTPUT);pinMode(PIN_IO_LED,OUTPUT);for(uint8_t i=0;i<4;i++)pinMode(PIN_LED_MODE_1+i,OUTPUT);
 pinMode(PIN_DAC_CS,OUTPUT);digitalWrite(PIN_DAC_CS,HIGH);SPI.begin();digitalWrite(PIN_DAC_LDAC,HIGH);pinMode(PIN_DAC_LDAC,OUTPUT);
 manualBtn.raw=manualBtn.stable=digitalRead(PIN_MANUAL_BUTTON)==HIGH;modeBtn.raw=modeBtn.stable=digitalRead(PIN_MODE_BUTTON)==HIGH;
 manualBtn.rawSinceMs=modeBtn.rawSinceMs=millis();
 pot1=analogRead(PIN_POT_1);pot2=analogRead(PIN_POT_2);pot3=analogRead(PIN_POT_3);shapeSector=(uint32_t)pot3*12/1024;
 randomSeed(((uint32_t)pot1<<20)^micros()^pot2);loadCalibration();
 if(modeBtn.stable&&manualBtn.stable){calBootPending=true;calBootStart=millis();out1(0);out2(0);latchDac();}else enterSlot(0);
}
void loop(){
 uint32_t nowMs=millis();scanButton(manualBtn,nowMs);scanButton(modeBtn,nowMs);readPots(nowMs);
 if(calBootPending){serviceCalBoot(nowMs);return;}if(calibrationActive){serviceCalibration(nowMs);return;}


 if(selectedComposite()){
  if(1>1&&modeBtn.stable&&manualBtn.stable){if(!chordTracked){chordTracked=true;chordSince=nowMs;}else if(elapsedMs(nowMs,chordSince,2000)){chordTracked=false;enterSlot((slot+1)%1);return;}}else chordTracked=false;
  switch(slot){default:break;}
  if(!elapsedMs(nowMs,slotEnteredAt,800))displayModes(nowMs);return;
 }
 if(serviceModeButton(nowMs))return;
 uint32_t nowUs=micros();readCvInput(nowUs);
 ioRising=false;if(selectedInput()){bool level=digitalRead(PIN_OUT3)==HIGH;ioRising=level&&!ioGate;ioGate=level;}
 digitalWrite(PIN_INPUT_LED,(manualBtn.stable||cvGate)?HIGH:LOW);digitalWrite(PIN_IO_LED,selectedInput()?ioGate:digitalRead(PIN_OUT3)==HIGH);displayModes(nowMs);
 pendingCv|=cvRising;pendingManual|=manualBtn.rising;pendingTrigger|=cvRising||manualBtn.rising;
 pendingTriggerA|=cvRising||manualBtn.rising;pendingTriggerB|=ioRising||manualBtn.rising;
 if(!audioOwner){schService(nowMs);latchDac();}uint32_t dtUs=nowUs-lastUpdateUs;
 if(!selectedFast()&&dtUs<PROCESS_PERIOD_US)return;
 lastUpdateUs=nowUs;uint16_t dtMs=dtUs/1000UL;if(dtMs>20)dtMs=20;if(dtMs<1)dtMs=1;
 bool trig=pendingTrigger,a=pendingTriggerA,b=pendingTriggerB;resetTriggered=trig;
 switch(slot){case 0:{using namespace mo_harmonic_oscillator_stack;handleModeHarmonicStack();break;}default:break;}
 modeJustChanged=false;resetTriggered=pendingTrigger=pendingTriggerA=pendingTriggerB=pendingCv=pendingManual=false;if(!audioOwner)latchDac();
}
