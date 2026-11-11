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
