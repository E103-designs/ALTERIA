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
