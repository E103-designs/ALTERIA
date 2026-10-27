/*
  ALTERIA_ALLROUNDER 2.0.2 - 2026-10-27 - Nano ATmega328P / REV5-2
  1 AD Shaper, 2 AR Gate, 3 Dual Decay, 4 Ducking,
  5 Burst, 6 Acid Generator, 7 Tremolo/AutoPan, 8 Grids,
  9 Triple Clock Tap/Div/Mult, 10 Mutant Drums, 11 Logic XOR, 12 Turing,
  13 Humanizer, 14 Bernoulli 3, 15 Trance, 16 Presets.
  Modes 1-9 and 11-15: 0..+5V DAC outputs.
  Mode 10: drum audio about +/-2.5V. Mode 16: -5..+5V. Trance: 24 patterns.
  Digital I/O always 0/+5V, INPUT in modes 3 and 10 (independent trigger B).
  Disconnect external I/O gate before leaving modes 3/10 or rebooting.
  MODE short release: next mode. Only in mode 16, hold 650ms to toggle
  edit/play. MANUAL selects preset; P1/P2 voltage, P3 gate with pickup.
  Edits saved on leaving edit or leaving mode 16, two-bank checked EEPROM.
  Hold MODE+MANUAL at boot to calibrate 0/+5/-5V on both channels.
  P1/P2: center to acquire; CW raises volts, CCW lowers volts.
  MANUAL short: next point; MODE short: previous; MODE long: cancel.
  Final flashing LED4: hold MANUAL 1s to commit EEPROM.
  Calibration defaults are theoretical, not measured values.
  Sources: Envelope 1.1 REV5-2, Rhythm Engine 328P, Motion Engine POT3.
*/

#include <Arduino.h>
#include <SPI.h>
#include <EEPROM.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>

// Set to 1 for Serial Monitor diagnostic values (115200 baud).
#define ALTERIA_DEBUG_SERIAL 0
// Mode 10 only: serial heartbeat distinguishes timer, triggers and audio data.
#define ALTERIA_DRUM_DIAGNOSTICS 1

// ================== HARDWARE CONFIGURATION ======================
#define PIN_OUT3            2    // Digital gate INPUT in modes 3/10, OUTPUT otherwise
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
  MODE_DIVMULT = 8, MODE_DRUMS = 9, MODE_LOGIC_XOR = 10,
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

// ============================= MODES ==============================

EngineMode mode = MODE_AD;
bool pendingTrigger = false; // preserves edges until the next processing tick

// -------- MODE 1 AD SHAPER -----------------------------------------

ADPhase adPhase = AD_IDLE;
uint32_t adPhaseStarted = 0;
uint32_t adAttackUs = 10000UL, adDecayUs = 100000UL;
uint16_t adValue = 0, adStartValue = 0;
uint8_t adAttackShape = CURVE_LIN, adDecayShape = CURVE_LIN;
Pulse adEor = {false,0}, adEof = {false,0};

static void advanceAD(uint32_t nowUs) {
  if (adPhase == AD_ATTACK) {
    uint32_t elapsed = (uint32_t)(nowUs - adPhaseStarted);
    if (elapsed >= adAttackUs) {
      adValue = 4095;
      adPhase = AD_DECAY;
      adPhaseStarted += adAttackUs; // stable timing even if a tick is late
      firePulse(adEor, nowUs);
    } else {
      uint16_t e = easeCurve(phaseFromElapsed(elapsed,adAttackUs),adAttackShape);
      adValue = adStartValue + (uint32_t)(4095U-adStartValue)*e/4095UL;
    }
  }
  if (adPhase == AD_DECAY) {
    uint32_t elapsed = (uint32_t)(nowUs - adPhaseStarted);
    if (elapsed >= adDecayUs) {
      adValue = 0;
      adPhase = AD_IDLE;
      firePulse(adEof, nowUs);
    } else {
      uint16_t e = easeCurve(phaseFromElapsed(elapsed,adDecayUs),adDecayShape);
      adValue = 4095U-e;
    }
  }
}
static void triggerAD(uint32_t nowUs) {
  adStartValue = adValue; // retrigger smoothly from current CV
  adAttackUs = timeFromPotMs(pot1,2,5000)*1000UL;
  adDecayUs = timeFromPotMs(pot2,2,5000)*1000UL;
  adAttackShape = shapeAttack(shapeSector);
  adDecayShape  = shapeDecay(shapeSector);
  adPhaseStarted = nowUs;
  adPhase = AD_ATTACK;
  adEor.active = false;
  adEof.active = false;
}
static void serviceAD(uint32_t nowUs, bool trigger) {
  advanceAD(nowUs);
  if (trigger) triggerAD(nowUs);
  servicePulse(adEor,nowUs);
  servicePulse(adEof,nowUs);
  out1(adValue);
  out2(adEor.active ? 4095 : 0);
  digitalWrite(PIN_OUT3,adEof.active ? HIGH : LOW);
}

// -------- MODE 2 AR GATE -------------------------------------------

ARPhase arPhase = AR_IDLE;
uint32_t arPhaseStarted = 0, arAttackUs = 10000UL, arReleaseUs = 100000UL;
uint16_t arValue = 0, arStartValue = 0, arReleaseStart = 4095;
uint8_t arShape = CURVE_LIN;
Pulse arEor = {false,0}; // here EOR = end of RELEASE, not end of rise

static void serviceAR(uint32_t nowUs, bool trigger) {
  if (arPhase == AR_ATTACK) {
    uint32_t elapsed = nowUs-arPhaseStarted;
    if (elapsed >= arAttackUs) { arValue = 4095; arPhase = AR_SUSTAIN; }
    else {
      uint16_t e=easeCurve(phaseFromElapsed(elapsed,arAttackUs),arShape);
      arValue = arStartValue+(uint32_t)(4095U-arStartValue)*e/4095UL;
    }
  } else if (arPhase == AR_SUSTAIN) arValue = 4095;
  else if (arPhase == AR_RELEASE) {
    uint32_t elapsed = nowUs-arPhaseStarted;
    if (elapsed >= arReleaseUs) {
      arValue=0; arPhase=AR_IDLE;
      firePulse(arEor,nowUs);
    } else {
      uint16_t e=easeCurve(phaseFromElapsed(elapsed,arReleaseUs),arShape);
      arValue=(uint32_t)arReleaseStart*(4095U-e)/4095UL;
    }
  }
  // A new rising event forces a new attack, even if another gate stays high.
  if (trigger) {
    arStartValue = arValue;
    arAttackUs = timeFromPotMs(pot1,2,5000)*1000UL;
    arReleaseUs = timeFromPotMs(pot2,2,5000)*1000UL;
    uint16_t p=pot3;
    arShape=(p<341)?CURVE_LOG : (p<683)?CURVE_LIN : CURVE_EXP;
    arPhaseStarted=nowUs;
    arPhase=AR_ATTACK;
    arEor.active=false;
  }
  // Hold -> release on BOTH sources going low, even during ATTACK.
  if (!(cvGate || manualBtn.stable) &&
      (arPhase==AR_ATTACK || arPhase==AR_SUSTAIN)) {
    arReleaseStart=arValue;
    arPhase=AR_RELEASE;
    arPhaseStarted=nowUs;
  }
  servicePulse(arEor,nowUs);
  out1(arValue);
  out2((uint16_t)(4095U-arValue));
  digitalWrite(PIN_OUT3,arEor.active?HIGH:LOW);
}

// -------- MODE 3 DUAL VACTROL DECAY --------------------------------

Vactrol vA={0,0,0,false,150,0}, vB={0,0,0,false,250,0};
// MODE 3 has TWO independent gate inputs. No end-of-both output.
// Gate A comes from CV IN (A2); gate B from I/O (D2).
// Manual button fires both envelopes simultaneously for auditioning.
// I/O must be verified electrically as a bidirectional, protected jack.
bool ioGate = false;
bool ioRising = false;
bool pendingTriggerA = false;
bool pendingTriggerB = false;

static void scanIOGate() {
  ioRising = false;
  bool newGate = (digitalRead(PIN_OUT3) == HIGH);
  ioRising = (!ioGate && newGate);
  ioGate = newGate;
}

static uint16_t vactrolOutput(const Vactrol &v) {
  uint16_t f=q8to12(v.fastQ8);
  uint16_t s=q8to12(v.slowQ8);
  uint16_t slowMix=(uint16_t)(((uint32_t)v.character*570UL)/1023UL); // max ~56% slow component
  return (uint16_t)(((uint32_t)f*(1023U-slowMix)+(uint32_t)s*slowMix)/1023UL);
}

static void strikeVactrol(Vactrol &v,uint32_t nowUs,uint16_t decayMs,uint16_t character) {
  v.decayMs=decayMs;
  v.character=character;
  v.strikeStartedUs=nowUs;
  v.striking=true;
  // Intentionally DO NOT zero fastQ8 / slowQ8: previous hits have memory.
}

static void advanceVactrol(Vactrol &v,uint32_t nowUs,uint16_t dtMs) {
  uint16_t strikeMs=(uint16_t)(3UL+((uint32_t)v.character*15UL)/1023UL);
  if (v.striking && elapsedUs(nowUs,v.strikeStartedUs,(uint32_t)strikeMs*1000UL))
    v.striking=false;

  uint16_t target=v.striking?4095:0;
  uint16_t tauFast, tauSlow;
  if (v.striking) {
    tauFast=(uint16_t)(1UL+((uint32_t)v.character*11UL)/1023UL);
    tauSlow=(uint16_t)(3UL+((uint32_t)v.character*19UL)/1023UL);
  } else {
    tauFast=v.decayMs;
    tauSlow=(uint16_t)minU16(6000U, (uint16_t)((uint32_t)v.decayMs * (1100UL+(uint32_t)v.character*1400UL/1023UL)/1000UL));
  }
  onePoleQ8(v.fastQ8,target,tauFast,dtMs);
  onePoleQ8(v.slowQ8,target,tauSlow,dtMs);
}

static void serviceDualDecay(uint32_t nowUs, bool triggerA, bool triggerB, uint16_t dtMs) {
  // The character pot is shared, but each channel retains its own decay,
  // state, input trigger and retrigger memory.
  uint16_t c=pot3;
  uint16_t cB=(uint16_t)minU16(1023,(uint16_t)((uint32_t)c*9UL/10UL+65UL));
  if (triggerA) strikeVactrol(vA,nowUs,(uint16_t)timeFromPotMs(pot1,7,1400),c);
  if (triggerB) strikeVactrol(vB,nowUs,(uint16_t)timeFromPotMs(pot2,7,1400),cB);
  advanceVactrol(vA,nowUs,dtMs);
  advanceVactrol(vB,nowUs,dtMs);
  uint16_t levelA=vactrolOutput(vA);
  uint16_t levelB=vactrolOutput(vB);
  out1(levelA);
  out2(levelB);
  // Important: DO NOT digitalWrite(PIN_OUT3) here. D2 is an INPUT in MODE 3.
}

// -------- MODE 4 DUCKING / CV FOLLOWER ------------------------------
int32_t followQ8=0;
bool followGate=false;
static void serviceDucking(uint32_t nowUs,uint16_t dtMs) {
  (void)nowUs;
  // Manual button is an independent CV test source (5V while held).
  uint16_t target=manualBtn.stable?4095:cvLevelQ12;
  uint16_t level=q8to12(followQ8);
  uint16_t tau=(uint16_t)((target>level)?timeFromPotMs(pot1,1,300):timeFromPotMs(pot2,4,2500));
  onePoleQ8(followQ8,target,tau,dtMs);
  level=q8to12(followQ8);
  uint16_t amount=(uint16_t)((uint32_t)level*pot3/1023UL);
  uint16_t duck=(uint16_t)(4095U-amount);
  out1(duck);              // at silence: +5 V; dips toward 0 V
  out2(level);             // at silence: 0 V; follows CV
  if (!followGate && level>=820) followGate=true;
  if (followGate && level<=620) followGate=false;
  digitalWrite(PIN_OUT3,followGate?HIGH:LOW);
}


#define ctrl1Val pot1
#define ctrl2Val pot2
#define ctrl3Val pot3
#define GATEOUT PIN_OUT3
#define SHORT_PULSE_MS 10UL
bool modeJustChanged = true;
bool resetTriggered = false;
int8_t enteredMode = -1;
static bool checkReset() { bool r = resetTriggered; resetTriggered = false; return r; }
static void dacSetA(uint16_t v) { out1(v); }
static void dacSetB(uint16_t v) { out2(v); }
static void setGate(bool on) { digitalWrite(PIN_OUT3, on ? HIGH : LOW); }
static void resetOutputs() { setGate(false); out1(0); out2(0); }
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












static void modeGridsXY(unsigned long t) {
  static const uint16_t baseK[4] = {
    0b1000100010001000,
    0b1000100000101000,
    0b1000001010000010,
    0b1000101000101000
  };
  static const uint16_t baseS[4] = {
    0b0000100000001000,
    0b0000100000101000,
    0b0010000000100000,
    0b0000101000001000
  };
  static const uint16_t baseH[4] = {
    0b1010101010101010,
    0b1110111011101110,
    0b1011101110111011,
    0b1101110111011101
  };

  static uint8_t step;
  static unsigned long lastClk, clkP;

  if (modeJustChanged) {
    step = 0;
    lastClk = 0;
    clkP = 0;
    schClearAll();
    resetOutputs();
  }

  if (checkReset()) {
    if (lastClk) clkP = t - lastClk;
    lastClk = t;

    uint8_t x = (uint8_t)((uint32_t)ctrl1Val * 4 / 1024);
    uint8_t y = (uint8_t)((uint32_t)ctrl2Val * 4 / 1024);
    uint8_t density = (uint8_t)map(ctrl3Val, 0, 1023, 0, 100);

    uint16_t patK = rot16(baseK[x], (uint8_t)(y * 2));
    uint16_t patS = rot16(baseS[y], (uint8_t)(x * 2));
    uint16_t patH = rot16(baseH[(uint8_t)((x + y) & 3)], (uint8_t)(x + y));

    bool bk = (patK >> step) & 1U;
    bool bs = (patS >> step) & 1U;
    bool bh = (patH >> step) & 1U;

    uint8_t hkRnd = (uint8_t)((step * 37U + x * 11U + y * 7U + 13U) % 100U);
    uint8_t hsRnd = (uint8_t)((step * 41U + x * 17U + y * 5U + 29U) % 100U);
    uint8_t hhRnd = (uint8_t)((step * 43U + x * 19U + y * 3U + 47U) % 100U);

    bool hk, hs, hh;
    if (density < 50) {
      uint8_t keep = (uint8_t)(density * 2U);
      hk = bk && (hkRnd < keep);
      hs = bs && (hsRnd < keep);
      hh = bh && (hhRnd < keep);
    } else {
      uint8_t add = (uint8_t)((density - 50U) * 2U);
      hk = bk || (hkRnd < add / 2U);
      hs = bs || (hsRnd < add / 2U);
      hh = bh || (hhRnd < add);
    }

    unsigned long pw = pulseWidthFromClock(clkP ? clkP : 40);

    if (hk) schPulseA(t, 0, pw, 4095);
    if (hs) schPulseB(t, 0, pw, 4095);
    if (hh) schPulseG(t, 0, pw);

    step = (uint8_t)((step + 1) & 0x0F);
  }

  schService(t);
}


static void modeLogicXOR(unsigned long t) {
  static unsigned long n1, n2;
  static bool s1, s2;
  static uint16_t lastSkew;

  unsigned long i1 = (unsigned long)mapQuadraticTo(ctrl1Val, 10, 2000);
  unsigned long i2 = (unsigned long)mapQuadraticTo(ctrl2Val, 10, 2000);
  uint16_t skew = (uint16_t)map(ctrl3Val, 0, 1023, 0, (int)i2);

  if (modeJustChanged) {
    n1 = t;
    n2 = t + skew;
    lastSkew = skew;
    s1 = s2 = false;
    dacSetA(0); dacSetB(0); setGate(false);
  }
  if (checkReset()) {
    n1 = t;
    n2 = t + skew;
    lastSkew = skew;
    s1 = s2 = false;
    dacSetA(0); dacSetB(0); setGate(false);
  }

  if (skew != lastSkew) {
    long delta = (long)skew - (long)lastSkew;
    n2 = (unsigned long)((long)n2 + delta);
    lastSkew = skew;
  }

  if ((long)(t - n1) >= 0) { n1 = t + (i1 / 2UL); s1 = !s1; dacSetA(s1 ? 4095 : 0); }
  if ((long)(t - n2) >= 0) { n2 = t + (i2 / 2UL); s2 = !s2; dacSetB(s2 ? 4095 : 0); }

  setGate((s1 != s2) ? HIGH : LOW);
}
static void modeTuring(unsigned long t) {
  static uint16_t reg;

  if (modeJustChanged) {
    reg = 0xACE1;
    dacSetA(0);
    dacSetB(4095);
    setGate(false);
  }

  if (checkReset()) {
    uint8_t len = (uint8_t)map(ctrl2Val, 0, 1023, 1, 16);
    uint8_t mutatePct = (uint8_t)map(ctrl1Val, 0, 1023, 100, 0);

    uint16_t mask = (len >= 16) ? 0xFFFF : (uint16_t)((1UL << len) - 1UL);
    uint16_t w = (uint16_t)(reg & mask);

    bool bit0 = (w & 1U) ? true : false;
    if (random(0, 100) < mutatePct) bit0 = !bit0;

    w >>= 1;
    if (bit0) w |= (len >= 16) ? 0x8000 : (uint16_t)(1UL << (len - 1));
    reg = (uint16_t)((reg & ~mask) | (w & mask));

    uint16_t v;
    if (mask == 0xFFFF) v = (uint16_t)(((uint32_t)w * 4095UL) / 65535UL);
    else v = (uint16_t)(((uint32_t)w * 4095UL) / (uint32_t)mask);

    v = quantizeByPot(v, ctrl3Val);

    dacSetA(v);
    dacSetB((uint16_t)(4095 - v));
    setGate((w & 1U) ? HIGH : LOW);
  }

  (void)t;
}

static void modeBernoulli3(unsigned long t) {
  static unsigned long lastClk, clkP;
  static uint8_t lastRoute;

  if (modeJustChanged) {
    lastClk = 0;
    clkP = 0;
    lastRoute = 0;
    schClearAll();
    resetOutputs();
  }

  if (checkReset()) {
    if (lastClk) clkP = t - lastClk;
    lastClk = t;
    unsigned long pw = pulseWidthFromClock(clkP ? clkP : 40);

    uint8_t stick = (uint8_t)map(ctrl3Val, 0, 1023, 0, 95);
    uint8_t route;

    if (random(0, 100) < stick) {
      route = lastRoute;
    } else {
      uint8_t pGate = (uint8_t)map(ctrl2Val, 0, 1023, 0, 95);
      uint8_t rem = (uint8_t)(100 - pGate);
      uint8_t pA = (uint8_t)(((uint32_t)rem * (uint32_t)(1023 - ctrl1Val)) / 1023UL);
      uint8_t pB = (uint8_t)(rem - pA);

      uint8_t r = (uint8_t)random(0, 100);
      if (r < pA) route = 0;
      else if (r < (uint8_t)(pA + pB)) route = 1;
      else route = 2;
    }

    lastRoute = route;

    if (route == 0) schPulseA(t, 0, pw, 4095);
    else if (route == 1) schPulseB(t, 0, pw, 4095);
    else schPulseG(t, 0, pw);
  }

  schService(t);
}
const uint8_t TRANCE_PATTERN_COUNT = 24;
const uint16_t TRANCE_MAIN[TRANCE_PATTERN_COUNT] PROGMEM = {
    0b1010101010101010,
    0b1000100010001000,
    0b1110111011101110,
    0b1000111010001110,
    0b1010010110100101,
    0b1111000011110000,
    0b1101110011011100,
    0b1011101010111010,
    0x4444, // 9: Offbeat quarters
    0x4949, // 10: 3-3-2 repeated
    0x9491, // 11: Syncopated drive
    0xCCCC, // 12: Offbeat doublets
    0x7777, // 13: Three-on one-off
    0x3333, // 14: Double chop
    0x00FF, // 15: Half-bar open
    0x0FFF, // 16: Three-quarter open
    0x2929, // 17: Staggered pulse
    0x8441, // 18: Sparse syncopation
    0xF911, // 19: End-bar fill
    0xFF11, // 20: Fill build
    0x6525, // 21: Broken eighths
    0xB2CB, // 22: Machine syncopation
    0x4B2D, // 23: Call and response
    0xFFFF  // 24: Full sixteenths
};

const uint16_t TRANCE_ACCENT[TRANCE_PATTERN_COUNT] PROGMEM = {
    0b1000100010001000,
    0b1000000010000000,
    0b1000100010001000,
    0b1000001010000010,
    0b1000010010000100,
    0b1000000010000000,
    0b1001000010010000,
    0b1000101000101000,
    0x0404, 0x0101, 0x0401, 0x0404,
    0x0101, 0x0101, 0x0011, 0x0101,
    0x0101, 0x0401, 0x9101, 0x1101,
    0x4101, 0x2201, 0x4101, 0x1111
};

static uint8_t trancePatternIndex(uint16_t pot) {
  if (pot > 1023) pot = 1023;
  return (uint8_t)(((uint32_t)pot * TRANCE_PATTERN_COUNT) >> 10);
}

static void modeTranceGate(unsigned long t) {
  static uint8_t step;
  static unsigned long lastClk, clkP;

  if (modeJustChanged) {
    step = 0;
    lastClk = 0;
    clkP = 0;
    schClearAll();
    resetOutputs();
  }

  if (checkReset()) {
    if (lastClk) clkP = t - lastClk;
    lastClk = t;

    uint8_t p = trancePatternIndex(ctrl1Val);
    uint16_t mainP = pgm_read_word(&TRANCE_MAIN[p]);
    uint16_t accP  = pgm_read_word(&TRANCE_ACCENT[p]);

    bool on  = (mainP >> step) & 1U;
    bool acc = (accP >> step) & 1U;

    unsigned long base = clkP ? clkP : 120;
    unsigned long gateLen = map(ctrl2Val, 0, 1023, (int)max(2UL, base / 20UL), (int)max(4UL, (base * 95UL) / 100UL));
    if (gateLen >= base && base > 2) gateLen = base - 1;

    uint16_t accentDepth = (uint16_t)map(ctrl3Val, 0, 1023, 0, 4095);
    uint16_t lowLevel = (uint16_t)(4095 - accentDepth);
    if (lowLevel < 400) lowLevel = 400;

    if (on) {
      schPulseA(t, 0, gateLen, 4095);
      schPulseB(t, 0, gateLen, acc ? 4095 : lowLevel);
    }
    if (acc) {
      unsigned long pw = pulseWidthFromClock(base);
      schPulseG(t, 0, pw);
    }

    step = (uint8_t)((step + 1) & 0x0F);
  }

  schService(t);
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
static void modeBurst(unsigned long t) {
  static bool active;
  static unsigned long tStart, spacingMs, nextPulse;
  static uint8_t totalP, doneP;

  if (modeJustChanged) {
    active = false;
    tStart = 0;
    spacingMs = 50;
    nextPulse = 0;
    totalP = 1;
    doneP = 0;
    schClearAll();
    resetOutputs();
  }

  if (checkReset()) {
    spacingMs = (unsigned long)mapQuadraticTo(ctrl1Val, 8, 600);
    totalP = (uint8_t)(1+((uint32_t)ctrl2Val*16/1024));
    active = true;
    tStart = t;
    doneP = 0;
    nextPulse = t;
    schClearChannel(CH_G);
    dacSetA(4095);
    dacSetB(0);
    setGate(false);
  }

  if (active) {
    unsigned long totalDur = spacingMs * (unsigned long)totalP;
    unsigned long el = t - tStart;

    if (totalDur > 0 && el <= totalDur) {
      uint16_t x = (uint16_t)(((uint32_t)el * 4095UL) / totalDur);
      uint16_t y = shapeProgressByPot(x, ctrl3Val);
      uint16_t a = (uint16_t)(4095 - y);
      dacSetA(a);
      dacSetB(y);
    } else {
      dacSetA(0);
      dacSetB(4095);
      active = false;
    }

    if (doneP < totalP && (long)(t - nextPulse) >= 0) {
      unsigned long pw = spacingMs / 3UL;
      if (pw < 2) pw = 2;
      if (pw > 20) pw = 20;
      schPulseG(t, 0, pw);
      doneP++;
      nextPulse += spacingMs;
    }
  }

  schService(t);
}
static void modeHumanizer(unsigned long t) {
  static unsigned long lastClk, clkP;
  static bool haveClk, odd;

  if (modeJustChanged) {
    lastClk = 0;
    clkP = 120;
    haveClk = false;
    odd = false;
    schClearAll();
    resetOutputs();
  }

  if (checkReset()) {
    if (lastClk) { clkP = t - lastClk; if (clkP < 6) clkP = 6; haveClk = true; }
    lastClk = t;

    uint8_t jitterPct = (uint8_t)map(ctrl1Val, 0, 1023, 0, 25);
    uint8_t swingPct  = (uint8_t)map(ctrl2Val, 0, 1023, 0, 33);
    uint16_t coupling = (uint16_t)(((uint32_t)ctrl1Val * (uint32_t)ctrl2Val) / 1023UL);

    unsigned long base = haveClk ? clkP : 120;
    unsigned long pw = pulseWidthFromClock(base);

    unsigned long maxJ = (base * jitterPct) / 100UL;
    unsigned long dA = maxJ ? (unsigned long)random(0, (long)maxJ + 1) : 0;

    unsigned long swingDelay = odd ? ((base * swingPct) / 100UL) : 0;
    unsigned long coupled = coupling ? (unsigned long)random(0, (long)((base * (coupling / 8UL)) / 1023UL) + 1) : 0;
    unsigned long dB = swingDelay + (coupled / 3UL);

    unsigned long maxDelay = (base > 4) ? (base - 2) : 2;
    if (dA > maxDelay) dA = maxDelay;
    if (dB > maxDelay) dB = maxDelay;

    schPulseA(t, dA, pw, 4095);
    schPulseB(t, dB, pw, 4095);

    unsigned long minGW = 2;
    unsigned long maxGW = base / 2UL;
    if (maxGW < 4) maxGW = 4;
    if (maxGW > 100) maxGW = 100;

    unsigned long varAmt = ((maxGW - minGW) * (unsigned long)ctrl3Val) / 1023UL;
    unsigned long gW = minGW + (varAmt ? (unsigned long)random(0, (int)varAmt + 1) : 0);
    if (gW > maxGW) gW = maxGW;

    schPulseG(t, 0, gW);
    odd = !odd;
  }

  schService(t);
}
static void modeTremoloAutoPan(unsigned long t) {
  static uint16_t phase;
  static uint16_t currentX;
  static uint16_t targetX;
  static unsigned long lastT;
  static unsigned long nextRandomT;
  static uint8_t lastHalf;
  static uint16_t currentInc;

  if (modeJustChanged) {
    phase = 0;
    currentX = 2048;
    targetX = 2048;
    lastT = t;
    nextRandomT = t;
    lastHalf = 0;
    currentInc = (uint16_t)mapQuadraticTo(ctrl1Val, 2, 2200);
    schClearAll();
    dacSetA(2048);
    dacSetB(2048);
    setGate(false);
  }

  if (checkReset()) {
    phase = 0;
    currentX = 2048;
    targetX = 2048;
    lastT = t;
    nextRandomT = t;
    lastHalf = 0;
    currentInc = (uint16_t)mapQuadraticTo(ctrl1Val, 2, 2200);
    schClearAll();
    dacSetA(2048);
    dacSetB(2048);
    schPulseG(t, 0, 4);
  }

  unsigned long dt = t - lastT;
  if (dt > 30) dt = 30;
  lastT = t;

  uint16_t baseInc = (uint16_t)mapQuadraticTo(ctrl1Val, 2, 2200);
  uint16_t amount = (uint16_t)mapQuadraticTo(ctrl2Val, 0, 2047);
  uint16_t randomAmt = (uint16_t)ctrl3Val;

  if (randomAmt < 650) {
    if(randomAmt==0)currentInc=baseInc;
    // Regular to drifting LFO.
    uint8_t halfNow = (phase & 0x8000) ? 1 : 0;

    if (halfNow != lastHalf) {
      lastHalf = halfNow;
      unsigned long pw = 4;
      schPulseG(t, 0, pw);

      long spread = ((long)baseInc * (long)randomAmt) / 1600L;
      long r = spread ? random(-spread, spread + 1) : 0;
      long ni = (long)baseInc + r;
      if (ni < 1) ni = 1;
      if (ni > 3000) ni = 3000;
      currentInc = (uint16_t)ni;
    }

    if (currentInc == 0) currentInc = baseInc;
    phase = (uint16_t)(phase + (uint16_t)((uint32_t)currentInc * dt));
    currentX = triangleFromPhase16(phase);
  } else {
    // Random wave with slew to new target values.
    unsigned long halfPeriod = 32768UL / (baseInc ? baseInc : 1);
    if (halfPeriod < 6) halfPeriod = 6;
    if (halfPeriod > 5000) halfPeriod = 5000;

    if ((long)(t - nextRandomT) >= 0) {
      targetX = (uint16_t)random(0, 4096);
      unsigned long minHold = halfPeriod / 4UL;
      unsigned long maxHold = halfPeriod;
      if (minHold < 4) minHold = 4;
      if (maxHold < minHold + 1) maxHold = minHold + 1;
      unsigned long chaos = (unsigned long)map(randomAmt, 650, 1023, 0, 100);
      unsigned long hold = random((int)minHold, (int)maxHold + 1);
      if (chaos > 60 && random(0, 100) < (int)(chaos - 50)) hold = minHold;
      nextRandomT = t + hold;
      schPulseG(t, 0, 4);
    }

    int32_t err = (int32_t)targetX - (int32_t)currentX;
    uint32_t slew = ((uint32_t)baseInc * (uint32_t)dt * (uint32_t)map(randomAmt, 650, 1023, 1, 8));
    slew /= 4UL;
    if (slew < 1) slew = 1;

    if (err > 0) {
      if ((uint32_t)err < slew) currentX = targetX;
      else currentX = (uint16_t)(currentX + slew);
    } else if (err < 0) {
      if ((uint32_t)(-err) < slew) currentX = targetX;
      else currentX = (uint16_t)(currentX - slew);
    }
  }

  int32_t bipolar = (int32_t)currentX - 2048L;
  int32_t dev = (bipolar * (int32_t)amount) / 2047L;
  uint16_t a = clamp12(2048L + dev);
  uint16_t b = clamp12(2048L - dev);

  dacSetA(a);
  dacSetB(b);

  schService(t);
}
// Three free-running clocks, referenced to one tap/external master.

ClockLane clockLane[3];
const uint8_t CLOCK_DIV[11]={8,6,4,3,2,1,1,1,1,1,1};
const uint8_t CLOCK_MUL[11]={1,1,1,1,1,1,2,3,4,6,8};
uint32_t clockPeriodUs=500000UL,clockMasterLast=0,clockLastExt=0,clockLastTap=0;
uint32_t clockTapIntervals[2]={0,0};
uint8_t clockBeat=0,clockTapCount=0,clockTapSlot=0;
bool clockExternal=false;
static uint8_t clockIndex(uint16_t p){return (uint32_t)p*11/1024;}
static void clockWrite(uint8_t ch,bool on) {
  clockLane[ch].high=on;
  if(ch==0)dacSetA(on?4095:0);else if(ch==1)dacSetB(on?4095:0);else setGate(on);
}
static void clockAlign(uint8_t ch,uint32_t now) {
  uint16_t p=ch==0?pot1:ch==1?pot2:pot3;
  uint8_t i=clockIndex(p);clockLane[ch].ratio=i;
  uint32_t cycle=clockPeriodUs*CLOCK_DIV[i]/CLOCK_MUL[i];
  uint32_t offset=(clockPeriodUs*(clockBeat%CLOCK_DIV[i])+(uint32_t)(now-clockMasterLast))%cycle;
  clockLane[ch].halfUs=cycle/2;
  bool on=offset<cycle/2;clockWrite(ch,on);
  clockLane[ch].nextUs=now+(on?cycle/2-offset:cycle-offset);
}
static void clockAlignAll(uint32_t now){for(uint8_t ch=0;ch<3;ch++)clockAlign(ch,now);}
static void clockStart(uint32_t now) {
  clockExternal=false;clockBeat=0;clockMasterLast=now;clockTapCount=0;
  clockAlignAll(now);
}
static void serviceClock3(uint32_t now,bool externalRise,bool tap) {
  if(modeJustChanged){clockStart(now);modeJustChanged=false;}
  if(clockExternal&&(uint32_t)(now-clockLastExt)>clockPeriodUs*3) {
    clockExternal=false;clockTapCount=0;
    uint32_t n=(uint32_t)(now-clockMasterLast)/clockPeriodUs;
    clockBeat=(clockBeat+n)%24;clockMasterLast+=n*clockPeriodUs;
  }
  if(externalRise&&clockExternal) {
    uint32_t interval=now-clockLastExt;
    if(interval<20000UL)externalRise=false;
    else if(interval>4000000UL)clockExternal=false;
  }
  if(externalRise) {
    if(clockExternal) {
      uint32_t interval=now-clockLastExt;
      if(interval>=20000UL&&interval<=4000000UL)clockPeriodUs=interval;
      clockBeat=(clockBeat+1)%24;
    } else clockBeat=0;
    clockExternal=true;clockLastExt=clockMasterLast=now;clockTapCount=0;
    clockAlignAll(now);
  } else if(!clockExternal) {
    uint32_t n=(uint32_t)(now-clockMasterLast)/clockPeriodUs;
    if(n){clockBeat=(clockBeat+n)%24;clockMasterLast+=n*clockPeriodUs;}
  }
  if(tap&&!clockExternal) {
    uint32_t interval=now-clockLastTap;
    if(!clockTapCount||interval>4000000UL){clockTapCount=1;clockTapSlot=0;clockLastTap=now;}
    else if(interval>=80000UL) {
      clockLastTap=now;clockTapIntervals[clockTapSlot]=interval;clockTapSlot^=1;
      if(clockTapCount<3)clockTapCount++;
      if(clockTapCount>=3){clockPeriodUs=(clockTapIntervals[0]+clockTapIntervals[1])/2;clockBeat=0;clockMasterLast=now;clockAlignAll(now);}
    }
  }
  for(uint8_t ch=0;ch<3;ch++) {
    uint16_t p=ch==0?pot1:ch==1?pot2:pot3;
    if(clockIndex(p)!=clockLane[ch].ratio){clockAlign(ch,now);continue;}
    uint32_t half=clockLane[ch].halfUs;
    if((int32_t)(now-clockLane[ch].nextUs)>=0) {
      uint32_t missed=(uint32_t)(now-clockLane[ch].nextUs)/half+1;
      if(missed&1)clockWrite(ch,!clockLane[ch].high);
      clockLane[ch].nextUs+=missed*half;
    }
  }
}
// Native AVR adaptation inspired by drakh/acid-generator (CC0).
// Selected sparse steps, note pool, octaves, accents and slides; no Tone.js/audio.
uint8_t acidNotes[16],acidStep=0;
uint16_t acidAccents=0,acidSlides=0,acidValue=0,acidTarget=0,acidFrom=0;
uint32_t acidLastClock=0,acidPeriod=125,acidOff=0,acidGlideStart=0;
uint16_t acidGlideMs=0;
bool acidGate=false,acidPreviousSlide=false,pendingAcidGenerate=false,acidHaveClock=false;
const uint8_t ACID_SCALE[7] PROGMEM={0,1,3,5,7,8,10}; // Phrygian
static void acidGenerate() {
  uint8_t positions[16],degrees[7];for(uint8_t i=0;i<16;i++){positions[i]=i;acidNotes[i]=255;}
  for(uint8_t i=0;i<7;i++)degrees[i]=i;
  for(uint8_t i=15;i>0;i--){uint8_t j=random(i+1);uint8_t v=positions[i];positions[i]=positions[j];positions[j]=v;}
  for(uint8_t i=6;i>0;i--){uint8_t j=random(i+1);uint8_t v=degrees[i];degrees[i]=degrees[j];degrees[j]=v;}
  uint8_t maxNotes=((uint32_t)pot1*16+511)/1023;
  uint8_t count=maxNotes?random((maxNotes+1)/2,maxNotes+1):0;
  uint8_t pool=(uint32_t)pot2*7/1023,maxOct=(uint32_t)pot2*3/1024;
  acidAccents=acidSlides=0;
  for(uint8_t i=0;i<count;i++) {
    uint8_t pos=positions[i];uint8_t degree=(pool&&random(2))?degrees[random(pool)]:0;
    acidNotes[pos]=pgm_read_byte(&ACID_SCALE[degree])+12*random(maxOct+1);
    if(random(1024)<pot3/2)acidAccents|=(uint16_t)1<<pos;
    if(random(1024)<pot3/2)acidSlides|=(uint16_t)1<<pos;
  }
  acidStep=0;acidPreviousSlide=false;acidGate=false;setGate(false);dacSetB(0);
}
static void modeAcid(uint32_t now,bool trigger) {
  if(modeJustChanged){acidValue=acidTarget=0;acidHaveClock=false;acidPeriod=125;acidGlideMs=0;acidGenerate();dacSetA(0);}
  if(pendingAcidGenerate){pendingAcidGenerate=false;acidGenerate();acidGlideMs=0;}
  if(trigger) {
    if(acidHaveClock){acidPeriod=now-acidLastClock;if(acidPeriod<10)acidPeriod=10;if(acidPeriod>4000)acidPeriod=4000;}
    acidHaveClock=true;acidLastClock=now;
    uint8_t note=acidNotes[acidStep];bool slide=(acidSlides>>acidStep)&1;
    if(note!=255) {
      acidTarget=((uint32_t)note*4095+30)/60; // 12 semitones = 1 V over 0..5 V
      bool glide=acidPreviousSlide&&acidGate;
      acidFrom=acidValue;acidGlideStart=now;acidGlideMs=glide?(uint16_t)(acidPeriod/3):0;
      if(acidGlideMs>120)acidGlideMs=120;
      if(!acidGlideMs)acidValue=acidTarget;
      acidGate=true;setGate(true);acidOff=now+(slide?acidPeriod+2:acidPeriod/2);
      dacSetB(((acidAccents>>acidStep)&1)?4095:0);
    } else {acidGate=false;setGate(false);dacSetB(0);acidGlideMs=0;}
    acidPreviousSlide=note!=255&&slide;acidStep=(acidStep+1)&15;
  }
  if(acidGlideMs){uint32_t dt=now-acidGlideStart;if(dt>=acidGlideMs){acidValue=acidTarget;acidGlideMs=0;}else acidValue=(int32_t)acidFrom+((int32_t)acidTarget-acidFrom)*(int32_t)dt/acidGlideMs;}
  if(acidGate&&(int32_t)(now-acidOff)>=0){acidGate=false;setGate(false);dacSetB(0);}
  dacSetA(acidValue);
}
const int8_t DRUM_SINE[256] PROGMEM={
  0,3,6,9,12,16,19,22,25,28,31,34,37,40,43,46,
  49,51,54,57,60,63,65,68,71,73,76,78,81,83,85,88,
  90,92,94,96,98,100,102,104,106,107,109,111,112,113,115,116,
  117,118,120,121,122,122,123,124,125,125,126,126,126,127,127,127,
  127,127,127,127,126,126,126,125,125,124,123,122,122,121,120,118,
  117,116,115,113,112,111,109,107,106,104,102,100,98,96,94,92,
  90,88,85,83,81,78,76,73,71,68,65,63,60,57,54,51,
  49,46,43,40,37,34,31,28,25,22,19,16,12,9,6,3,
  0,-3,-6,-9,-12,-16,-19,-22,-25,-28,-31,-34,-37,-40,-43,-46,
  -49,-51,-54,-57,-60,-63,-65,-68,-71,-73,-76,-78,-81,-83,-85,-88,
  -90,-92,-94,-96,-98,-100,-102,-104,-106,-107,-109,-111,-112,-113,-115,-116,
  -117,-118,-120,-121,-122,-122,-123,-124,-125,-125,-126,-126,-126,-127,-127,-127,
  -127,-127,-127,-127,-126,-126,-126,-125,-125,-124,-123,-122,-122,-121,-120,-118,
  -117,-116,-115,-113,-112,-111,-109,-107,-106,-104,-102,-100,-98,-96,-94,-92,
  -90,-88,-85,-83,-81,-78,-76,-73,-71,-68,-65,-63,-60,-57,-54,-51,
  -49,-46,-43,-40,-37,-34,-31,-28,-25,-22,-19,-16,-12,-9,-6,-3
};
const uint16_t DRUM_RATE=6400; // CPU margin for the conservative GPIO/SPI path
volatile bool drumAudioActive=false;
volatile uint8_t drumPending=0,drumMutation=0,drumMix=128;
volatile uint16_t drumKickInc=450,drumSnareInc=1600,drumSnareInc2=2400;
volatile uint16_t drumFrames=0,drumKickHits=0,drumSnareHits=0;
volatile uint16_t drumPeak[2]={0,0};
uint32_t drumReportAt=0;
uint16_t drumPreviousFrames=0;
uint16_t drumZero[2],drumPos[2],drumNeg[2];
uint16_t drumKPhase=0,drumSPhase=0,drumSPhase2=0,drumKAmp=0,drumSAmp=0,drumSBodyAmp=0,drumPitch=0;
uint16_t drumRng=0xACE1;
int16_t drumNoiseLow=0,drumWire=0;
uint32_t drumControlsAt=0;
#if !defined(__AVR__)
uint16_t drumWritten[2];
#endif
static int16_t drumClip(int16_t v){return v>127?127:v<-127?-127:v;}
static int16_t drumSine(uint8_t phase){return (int8_t)pgm_read_byte(&DRUM_SINE[phase]);}
static uint16_t drumCode(uint8_t ch,int16_t sample) {
  if(sample>=0)return drumZero[ch]-(((uint32_t)sample*drumPos[ch])>>7);
  return drumZero[ch]+(((uint32_t)(-sample)*drumNeg[ch])>>7);
}
static void drumWrite(uint16_t a,uint16_t b) {
#if defined(__AVR__)
  // Use the same pin/SPI path as the working CV driver, not raw port writes.
  digitalWrite(PIN_DAC_CS,LOW);
  SPI.transfer((uint8_t)((0x1000|a)>>8));SPI.transfer((uint8_t)a);
  digitalWrite(PIN_DAC_CS,HIGH);
  digitalWrite(PIN_DAC_CS,LOW);
  SPI.transfer((uint8_t)((0x9000|b)>>8));SPI.transfer((uint8_t)b);
  digitalWrite(PIN_DAC_CS,HIGH);
  digitalWrite(PIN_DAC_LDAC,LOW);
  __builtin_avr_delay_cycles(F_CPU/1000000UL); // >=1 us, before GPIO-call overhead
  digitalWrite(PIN_DAC_LDAC,HIGH);
#else
  drumWritten[0]=a;drumWritten[1]=b;
#endif
  drumFrames++;
  uint16_t da=a>drumZero[0]?a-drumZero[0]:drumZero[0]-a;
  uint16_t db=b>drumZero[1]?b-drumZero[1]:drumZero[1]-b;
  if(da>drumPeak[0])drumPeak[0]=da;if(db>drumPeak[1])drumPeak[1]=db;
}
static void drumAudioTick() {
  uint8_t hits=drumPending;drumPending=0;
  uint8_t mutation=drumMutation;
  if(hits&1){drumKickHits++;drumKAmp=32767;drumPitch=drumKickInc*3;drumKPhase=0;}
  if(hits&2){drumSnareHits++;drumSAmp=drumSBodyAmp=32767;drumSPhase=drumSPhase2=0;}
  // Maximal 16-bit LFSR; no division, floats or library random in the ISR.
  drumRng=(drumRng>>1)^((uint16_t)(-(int16_t)(drumRng&1))&0xB400);
  int16_t noise=(int8_t)(drumRng>>8);
  drumNoiseLow+=(noise-drumNoiseLow)>>2;
  int16_t high=drumClip(noise-drumNoiseLow);
  drumWire+=((high-drumWire)*3)>>2;
  int16_t k=0,s=0;
  if(drumKAmp) {
    drumKPhase+=drumKickInc+drumPitch;
    uint8_t phase=drumKPhase>>8;
    int16_t pm=(drumSine((uint8_t)(phase*2))*mutation)>>9;
    // Crossfade into saturation: no switched drive or sample-rate reduction.
    k=drumSine((uint8_t)(phase+pm));
    uint8_t drive=((uint16_t)mutation*mutation)>>8;
    k+=((drumClip(k*3)-k)*drive)>>8;
    k=((int32_t)k*drumKAmp)>>15;
    // Scale first to keep these smooth decay products within 16 bits on AVR.
    uint16_t decay=((drumKAmp>>8)*(512U-mutation))>>10;drumKAmp-=decay?decay:1;
    uint16_t pd=drumPitch>>7;drumPitch-=pd?pd:(drumPitch?1:0);
  }
  if(drumSAmp) {
    drumSPhase+=drumSnareInc;drumSPhase2+=drumSnareInc2;
    int16_t body=(drumSine(drumSPhase>>8)+drumSine(drumSPhase2>>8))/2;
    // Short drum head followed by a longer, filtered snare-wire tail.
    body=((int32_t)body*drumSBodyAmp)>>15;
    int16_t wire=((int32_t)drumWire*drumSAmp)>>15;
    s=body+(((int32_t)wire-body)*drumMix>>8);
    uint16_t bd=drumSBodyAmp>>7;drumSBodyAmp-=bd?bd:(drumSBodyAmp?1:0);
    uint16_t decay=((drumSAmp>>8)*(512U-mutation))>>9;drumSAmp-=decay?decay:1;
  }
  drumWrite(drumCode(0,k),drumCode(1,s));
}
#if defined(__AVR__)
ISR(TIMER1_COMPA_vect){drumAudioTick();}
#endif
static void drumsStop() {
  bool wasActive=drumAudioActive;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
#if defined(__AVR__)
    TIMSK1&=(uint8_t)~_BV(OCIE1A);TCCR1B=0;
#endif
    drumAudioActive=false;drumPending=0;
  }
#if ALTERIA_DRUM_DIAGNOSTICS && !ALTERIA_DEBUG_SERIAL
  if(wasActive)Serial.end();
#else
  (void)wasActive;
#endif
}
static void drumsControls(uint32_t now,bool force=false) {
  if(!force&&!elapsedMs(now,drumControlsAt,12))return;
  drumControlsAt=now;
  uint16_t k=(uint32_t)(45+(uint32_t)pot1*90/1023)*65536UL/DRUM_RATE;
  uint16_t s=(uint32_t)(185+(uint32_t)pot2*70/1023)*65536UL/DRUM_RATE;
  uint16_t s2=(uint32_t)(330+(uint32_t)pot2*110/1023)*65536UL/DRUM_RATE;
  uint8_t mix=112+(uint32_t)pot2*96/1023;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){drumKickInc=k;drumSnareInc=s;drumSnareInc2=s2;drumMutation=pot3>>2;drumMix=mix;}
}
static void drumsStart() {
  drumsStop();drumKAmp=drumSAmp=drumSBodyAmp=drumPitch=0;drumNoiseLow=drumWire=0;
  drumFrames=drumKickHits=drumSnareHits=0;drumPeak[0]=drumPeak[1]=0;
  drumPreviousFrames=0;drumReportAt=millis();
  for(uint8_t ch=0;ch<2;ch++){drumZero[ch]=DAC_ZERO_CODE[ch];drumPos[ch]=(DAC_ZERO_CODE[ch]-DAC_PLUS5_CODE[ch])/2;drumNeg[ch]=(DAC_MINUS5_CODE[ch]-DAC_ZERO_CODE[ch])/2;}
  drumsControls(millis(),true);
  SPI.beginTransaction(SPISettings(8000000UL,MSBFIRST,SPI_MODE0));SPI.endTransaction();
#if ALTERIA_DRUM_DIAGNOSTICS
  Serial.begin(115200);
#endif
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    drumAudioActive=true;
#if defined(__AVR__)
    TCCR1A=0;TCCR1B=0;TCNT1=0;OCR1A=F_CPU/DRUM_RATE-1;
    TIFR1=_BV(OCF1A);TIMSK1|=_BV(OCIE1A);TCCR1B=_BV(WGM12)|_BV(CS10);
#endif
  }
}
static void drumsTrigger(bool kick,bool snare) {
  uint8_t mask=(kick?1:0)|(snare?2:0);
  if(!mask)return;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){drumPending|=mask;}
}
static void drumsDiagnostics(uint32_t now) {
#if ALTERIA_DRUM_DIAGNOSTICS
  if(!elapsedMs(now,drumReportAt,500))return;
  uint16_t frames,k,s,pa,pb;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){frames=drumFrames;k=drumKickHits;s=drumSnareHits;pa=drumPeak[0];pb=drumPeak[1];}
  uint16_t rate=(uint32_t)(uint16_t)(frames-drumPreviousFrames)*1000/(now-drumReportAt);
  drumPreviousFrames=frames;drumReportAt=now;
  Serial.print(F("DRUM Hz="));Serial.print(rate);Serial.print(F(" K="));Serial.print(k);
  Serial.print(F(" S="));Serial.print(s);Serial.print(F(" peakA="));Serial.print(pa);
  Serial.print(F(" peakB="));Serial.println(pb);
#else
  (void)now;
#endif
}

// Presets occupy EEPROM 128..189; two records survive interrupted writes.

PresetSlot presets[4];
uint8_t selectedPreset = 0;
bool presetEditing = false, presetDirty = false;
bool pickup[3] = {false,false,false};
uint16_t previousPot[3];
const uint16_t MODE_HOLD_MS = 650;
const uint16_t EEPROM_BASE = 128;
const uint8_t RECORD_BYTES = 30, RECORD_STRIDE = 32;
uint8_t activeBank = 1, saveBank = 0;
uint16_t presetSequence = 0;
uint8_t saveRecord[RECORD_BYTES];
int8_t saveStep = -1;
uint32_t lastUpdateUs = 0;
bool modePressArmed = false, modeLongHandled = false;
uint32_t modePressedMs = 0;

static uint16_t recordCrc(const uint8_t *p) {
  uint16_t c = 0xFFFF;
  for (uint8_t i=0;i<27;i++) {
    c ^= (uint16_t)p[i] << 8;
    for (uint8_t b=0;b<8;b++) c = (c & 0x8000) ? (uint16_t)((c<<1)^0x1021) : (uint16_t)(c<<1);
  }
  return c;
}
static bool validRecord(const uint8_t *r) {
  if (r[0]!='A'||r[1]!='D'||r[2]!='M'||r[3]!='O'||r[4]!=1||r[29]!=0xA5) return false;
  uint16_t crc = recordCrc(r);
  if (r[27]!=(uint8_t)crc || r[28]!=(uint8_t)(crc>>8)) return false;
  for (uint8_t i=0;i<4;i++) {
    uint8_t p=7+i*5;
    if (r[p+1]>15||r[p+3]>15||r[p+4]>1) return false;
  }
  return true;
}
static void loadPresets() {
  uint8_t r[2][RECORD_BYTES];
  for(uint8_t b=0;b<2;b++) for(uint8_t i=0;i<RECORD_BYTES;i++) r[b][i]=EEPROM.read(EEPROM_BASE+b*RECORD_STRIDE+i);
  bool a=validRecord(r[0]), b=validRecord(r[1]);
  if (!a && !b) {
    for(uint8_t i=0;i<4;i++) { presets[i].a=(uint32_t)i*4095/3; presets[i].b=4095-presets[i].a; presets[i].gate=i&1; }
    return;
  }
  uint16_t sa=(uint16_t)r[0][5]|((uint16_t)r[0][6]<<8);
  uint16_t sb=(uint16_t)r[1][5]|((uint16_t)r[1][6]<<8);
  activeBank = !a ? 1 : !b ? 0 : ((int16_t)(sb-sa)>0 ? 1 : 0);
  presetSequence = activeBank ? sb : sa;
  for(uint8_t i=0;i<4;i++) {
    uint8_t p=7+i*5; uint8_t *v=r[activeBank];
    presets[i].a=(uint16_t)v[p]|((uint16_t)v[p+1]<<8);
    presets[i].b=(uint16_t)v[p+2]|((uint16_t)v[p+3]<<8);
    presets[i].gate=v[p+4];
  }
}
static void startPresetSave() {
  if (!presetDirty || saveStep>=0) return;
  saveBank = activeBank^1;
  uint16_t seq=presetSequence+1;
  saveRecord[0]='A';saveRecord[1]='D';saveRecord[2]='M';saveRecord[3]='O';saveRecord[4]=1;
  saveRecord[5]=(uint8_t)seq;saveRecord[6]=(uint8_t)(seq>>8);
  for(uint8_t i=0;i<4;i++) {
    uint8_t p=7+i*5;
    saveRecord[p]=(uint8_t)presets[i].a;saveRecord[p+1]=presets[i].a>>8;
    saveRecord[p+2]=(uint8_t)presets[i].b;saveRecord[p+3]=presets[i].b>>8;
    saveRecord[p+4]=presets[i].gate;
  }
  uint16_t crc=recordCrc(saveRecord);saveRecord[27]=(uint8_t)crc;saveRecord[28]=crc>>8;saveRecord[29]=0xA5;
  presetDirty=false;saveStep=0;
}
static void servicePresetSave() {
  if (saveStep<0) {
    if (!presetEditing) startPresetSave();
    return;
  }
  if (!eeprom_is_ready()) return;
  uint16_t addr=EEPROM_BASE+saveBank*RECORD_STRIDE;
  if (saveStep==0) EEPROM.update(addr+29,0); // invalidate before overwriting
  else if (saveStep<=29) EEPROM.update(addr+saveStep-1,saveRecord[saveStep-1]);
  else if (saveStep==30) EEPROM.update(addr+29,0xA5); // commit last
  else {
    activeBank=saveBank;presetSequence=(uint16_t)saveRecord[5]|((uint16_t)saveRecord[6]<<8);
    saveStep=-1;return;
  }
  saveStep++;
}
static void resetPickup() {
  uint16_t target[2]={(uint16_t)((uint32_t)presets[selectedPreset].a*1023/4095),
                      (uint16_t)((uint32_t)presets[selectedPreset].b*1023/4095)};
  previousPot[0]=pot1;previousPot[1]=pot2;previousPot[2]=pot3;
  pickup[0]=abs((int)pot1-(int)target[0])<=8;
  pickup[1]=abs((int)pot2-(int)target[1])<=8;
  pickup[2]=presets[selectedPreset].gate ? pot3>604 : pot3<420;
}
static void servicePresetMode() {
  if (!presetEditing) return;
  uint16_t values[2]={pot1,pot2};
  uint16_t *dest[2]={&presets[selectedPreset].a,&presets[selectedPreset].b};
  for(uint8_t i=0;i<2;i++) {
    uint16_t target=(uint32_t)*dest[i]*1023/4095;
    if (!pickup[i] && (abs((int)values[i]-(int)target)<=8 ||
        (previousPot[i]<target && values[i]>=target) || (previousPot[i]>target && values[i]<=target))) pickup[i]=true;
    if (pickup[i] && values[i]!=previousPot[i]) {
      uint16_t v=(uint32_t)values[i]*4095/1023;
      if (v!=*dest[i]) { *dest[i]=v;presetDirty=true; }
    }
    previousPot[i]=values[i];
  }
  uint8_t &gate=presets[selectedPreset].gate;
  if (!pickup[2] && (gate ? pot3>604 : pot3<420)) pickup[2]=true;
  if (pickup[2]) {
    uint8_t next=pot3>604 ? 1 : pot3<420 ? 0 : gate;
    if(next!=gate) {gate=next;presetDirty=true;}
  }
  previousPot[2]=pot3;
}
static void outputPreset() {
  out1(presets[selectedPreset].a);out2(presets[selectedPreset].b);setGate(presets[selectedPreset].gate);
}
static void displayModes(uint32_t nowMs) {
  uint8_t bits = mode==MODE_PRESETS ? (uint8_t)(1U<<selectedPreset) : (uint8_t)(((uint8_t)mode+1)&15);
  if(mode==MODE_PRESETS && presetEditing && ((nowMs/140)&1)) bits=0;
  for(uint8_t i=0;i<4;i++) digitalWrite(PIN_LED_MODE_1+i,(bits&(1U<<i))?HIGH:LOW);
}
void enterMode(EngineMode next) {
  drumsStop();
  pendingAcidGenerate=false;
  if (mode==MODE_PRESETS && presetEditing) {presetEditing=false;startPresetSave();}
  schClearAll();
  mode=next;
  bipolarOutputs=mode==MODE_DRUMS || mode==MODE_PRESETS;
  // A cached code from the old range must never suppress the first output write.
  dacLastA=dacLastB=0xFFFF;
  pendingTrigger=pendingTriggerA=pendingTriggerB=resetTriggered=false;
  modeJustChanged=true;enteredMode=(int8_t)mode;
  digitalWrite(PIN_OUT3,LOW);
  pinMode(PIN_OUT3,(mode==MODE_DUAL_DECAY||mode==MODE_DRUMS)?INPUT:OUTPUT);
  ioGate=((mode==MODE_DUAL_DECAY||mode==MODE_DRUMS) && digitalRead(PIN_OUT3)==HIGH);ioRising=false;
  adPhase=AD_IDLE;adValue=adStartValue=0;adEor.active=adEof.active=false;
  arPhase=AR_IDLE;arValue=arStartValue=0;arEor.active=false;
  vA.fastQ8=vA.slowQ8=vB.fastQ8=vB.slowQ8=0;vA.striking=vB.striking=false;
  followQ8=0;followGate=false;
  out1(bipolarOutputs?2048:mode==MODE_DUCKING?4095:0);
  out2(bipolarOutputs?2048:mode==MODE_AR?4095:0);
  if(mode==MODE_PRESETS) {presetEditing=false;outputPreset();resetPickup();}
  latchDac();lastUpdateUs=micros();displayModes(millis());
  if(mode==MODE_DRUMS)drumsStart();
}
static bool serviceModeButton(uint32_t nowMs) {
  if(modeBtn.rising) {modePressArmed=true;modeLongHandled=false;modePressedMs=nowMs;}
  if(modePressArmed && modeBtn.stable && !modeLongHandled && elapsedMs(nowMs,modePressedMs,MODE_HOLD_MS)) {
    modeLongHandled=true;
    if(mode==MODE_PRESETS) {
      presetEditing=!presetEditing;
      if(presetEditing) resetPickup(); else startPresetSave();
    }
  }
  if(modeBtn.falling && modePressArmed) {
    modePressArmed=false;
    if(!modeLongHandled) {enterMode((EngineMode)(((uint8_t)mode+1)&15));return true;}
  }
  return false;
}
// Hidden boot calibration. This record format is independent of preset EEPROM.
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
  enterMode(MODE_AD);
}
static void serviceCalBoot(uint32_t now) {
  if(digitalRead(PIN_MODE_BUTTON)!=HIGH||digitalRead(PIN_MANUAL_BUTTON)!=HIGH) {
    calBootPending=false;enterMode(MODE_AD);return;
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

void setup() {
  pinMode(PIN_MODE_BUTTON,INPUT);     // external hardware pulldown assumed
  pinMode(PIN_MANUAL_BUTTON,INPUT);  // external hardware pulldown assumed
  pinMode(PIN_CV_INPUT,INPUT);       // analog, must NOT exceed 0..5V at Nano pin
  pinMode(PIN_OUT3,INPUT);
  pinMode(PIN_IO_LED,OUTPUT);
  pinMode(PIN_INPUT_LED,OUTPUT);
  pinMode(PIN_LED_MODE_1,OUTPUT);
  pinMode(PIN_LED_MODE_2,OUTPUT);
  pinMode(PIN_LED_MODE_3,OUTPUT);
  pinMode(PIN_LED_MODE_4,OUTPUT);
  pinMode(PIN_DAC_CS,OUTPUT);
  digitalWrite(PIN_DAC_CS,HIGH);
  digitalWrite(PIN_OUT3,LOW);
  SPI.begin();
  digitalWrite(PIN_DAC_LDAC,HIGH);
  pinMode(PIN_DAC_LDAC,OUTPUT); // SPI.begin() may configure MISO/D12 as input
#if ALTERIA_DEBUG_SERIAL
  Serial.begin(115200);
#endif
  // Start with hardware input states instead of creating false rising edges.
  manualBtn.raw=manualBtn.stable=(digitalRead(PIN_MANUAL_BUTTON)==HIGH);
  modeBtn.raw=modeBtn.stable=(digitalRead(PIN_MODE_BUTTON)==HIGH);
  uint32_t nowMs=millis();
  manualBtn.rawSinceMs=modeBtn.rawSinceMs=nowMs;
  pot1=(uint16_t)analogRead(PIN_POT_1);
  pot2=(uint16_t)analogRead(PIN_POT_2);
  pot3=(uint16_t)analogRead(PIN_POT_3);
  shapeSector=(uint8_t)((uint32_t)pot3*12UL/1024UL);
  if (shapeSector>11) shapeSector=11;
  cvLevelQ12=normalizeCv(analogRead(PIN_CV_INPUT));
  cvGate=(cvLevelQ12>=CV_GATE_ON_Q12);
  lastPotsMs=nowMs;
  lastCvPollUs=micros();
  randomSeed(((uint32_t)analogRead(PIN_POT_1)<<20) ^ micros() ^ analogRead(PIN_POT_2));
  loadCalibration();
  loadPresets();
  if(digitalRead(PIN_MODE_BUTTON)==HIGH && digitalRead(PIN_MANUAL_BUTTON)==HIGH) {
    calBootPending=true;calBootStart=millis();
    out1(0);out2(0);latchDac();
  } else enterMode(MODE_AD);
}

void loop() {
  uint32_t nowMs=millis();
  scanButton(manualBtn,nowMs);scanButton(modeBtn,nowMs);readPots(nowMs);
  if(calBootPending){serviceCalBoot(nowMs);return;}
  if(calibrationActive){serviceCalibration(nowMs);return;}
  uint32_t nowUs=micros();readCvInput(nowUs);
  if(mode==MODE_DUAL_DECAY||mode==MODE_DRUMS)scanIOGate();
  servicePresetSave();if(serviceModeButton(nowMs))return;
  digitalWrite(PIN_INPUT_LED,(manualBtn.stable||cvGate)?HIGH:LOW);
  digitalWrite(PIN_IO_LED,((mode==MODE_DUAL_DECAY||mode==MODE_DRUMS)?ioGate:digitalRead(PIN_OUT3)==HIGH)?HIGH:LOW);
  displayModes(nowMs);
  if(mode==MODE_DRUMS) {
    drumsControls(nowMs);drumsTrigger(cvRising||manualBtn.rising,ioRising||manualBtn.rising);
    drumsDiagnostics(nowMs);
    modeJustChanged=false;return; // ISR exclusively owns SPI/DAC in this mode
  }
  if(mode==MODE_DIVMULT){serviceClock3(micros(),cvRising,manualBtn.rising);latchDac();return;}
  if(mode==MODE_PRESETS) {
    if(manualBtn.rising||(!presetEditing&&cvRising)){selectedPreset=(selectedPreset+1)&3;resetPickup();}
    servicePresetMode();outputPreset();latchDac();return;
  }
  if(mode==MODE_DUAL_DECAY) {
    if(cvRising||manualBtn.rising)pendingTriggerA=true;
    if(ioRising||manualBtn.rising)pendingTriggerB=true;
  } else if(mode==MODE_ACID) {
    if(manualBtn.rising)pendingAcidGenerate=true;
    if(cvRising)pendingTrigger=true;
  } else if(cvRising||manualBtn.rising)pendingTrigger=true;
  schService(nowMs);latchDac();nowUs=micros();uint32_t dtUs=nowUs-lastUpdateUs;
  if(dtUs<PROCESS_PERIOD_US)return;
  lastUpdateUs=nowUs;uint16_t dtMs=dtUs/1000UL;if(dtMs>20)dtMs=20;if(dtMs<1)dtMs=1;
  bool trig=pendingTrigger,a=pendingTriggerA,b=pendingTriggerB;
  pendingTrigger=pendingTriggerA=pendingTriggerB=false;resetTriggered=trig;
  uint32_t t=millis();
  switch(mode) {
    case MODE_AD:serviceAD(nowUs,trig);break;
    case MODE_AR:serviceAR(nowUs,trig);break;
    case MODE_DUAL_DECAY:serviceDualDecay(nowUs,a,b,dtMs);break;
    case MODE_DUCKING:serviceDucking(nowUs,dtMs);break;
    case MODE_BURST:modeBurst(t);break;
    case MODE_ACID:modeAcid(t,trig);break;
    case MODE_TREMOLO:modeTremoloAutoPan(t);break;
    case MODE_GRIDS:modeGridsXY(t);break;
    case MODE_LOGIC_XOR:modeLogicXOR(t);break;
    case MODE_TURING:modeTuring(t);break;
    case MODE_HUMANIZER:modeHumanizer(t);break;
    case MODE_BERNOULLI:modeBernoulli3(t);break;
    case MODE_TRANCE:modeTranceGate(t);break;
    default:break;
  }
  modeJustChanged=false;resetTriggered=false;latchDac();
}
