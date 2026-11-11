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
