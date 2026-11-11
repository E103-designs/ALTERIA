static void modeGateLengthSculptor(unsigned long t) {
  static const uint16_t accents[4] = {
    0b1000100010001000,
    0b1010001010100010,
    0b1110100011101000,
    0b1011011010110110
  };

  static unsigned long lastClk, clkP;
  static uint8_t step;

  if (modeJustChanged) {
    lastClk = 0;
    clkP = 0;
    step = 0;
    schClearAll();
    resetOutputs();
  }

  if (checkReset()) {
    if (lastClk) clkP = t - lastClk;
    lastClk = t;

    unsigned long base = clkP ? clkP : 120;
    unsigned long minLen = 2;
    unsigned long maxLen = (base > 3) ? (base - 1) : 2;
    if (maxLen > 900) maxLen = 900;

    unsigned long lenA = map(ctrl1Val, 0, 1023, (int)minLen, (int)maxLen);

    uint8_t patternIdx = (uint8_t)map(ctrl2Val, 0, 1023, 0, 3);
    uint8_t rotation = (uint8_t)map(ctrl2Val, 0, 1023, 0, 15);
    bool accent = (rot16(accents[patternIdx], rotation) >> step) & 1U;

    uint8_t variationPct = (uint8_t)map(ctrl3Val, 0, 1023, 0, 65);
    unsigned long var = (lenA * variationPct) / 100UL;
    if (var > 0) {
      long delta = random(-(long)var, (long)var + 1L);
      long moved = (long)lenA + delta;
      if (moved < (long)minLen) moved = (long)minLen;
      if (moved > (long)maxLen) moved = (long)maxLen;
      lenA = (unsigned long)moved;
    }

    unsigned long lenB = accent ? ((lenA * 3UL) / 2UL) : (lenA / 2UL);
    if (lenB < 2) lenB = 2;
    if (lenB > maxLen) lenB = maxLen;

    uint16_t levelB = accent ? 4095 : (uint16_t)map(ctrl2Val, 0, 1023, 900, 3000);

    schPulseA(t, 0, lenA, 4095);
    schPulseB(t, 0, lenB, levelB);
    schPulseG(t, 0, pulseWidthFromClock(base));

    step = (uint8_t)((step + 1) & 0x0F);
  }

  schService(t);
}
