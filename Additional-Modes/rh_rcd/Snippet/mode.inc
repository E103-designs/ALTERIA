static void modeRCDRotation(unsigned long t) {
  static const uint8_t setSimple[4] = {1, 2, 4, 8};
  static const uint8_t setFull[8]   = {1, 2, 3, 4, 6, 8, 12, 16};

  static uint32_t masterCount;
  static unsigned long lastClk, clkP;

  if (modeJustChanged) {
    masterCount = 0;
    lastClk = 0;
    clkP = 0;
    schClearAll();
    resetOutputs();
  }

  if (checkReset()) {
    if (lastClk) clkP = t - lastClk;
    lastClk = t;

    masterCount++;

    bool complex = (ctrl1Val > 512);
    uint8_t len = complex ? 8 : 4;
    uint8_t rot = (uint8_t)map(ctrl2Val, 0, 1023, 0, (int)(len - 1));
    uint8_t spread = (uint8_t)map(ctrl3Val, 0, 1023, 1, (int)(len - 1));

    uint8_t d0 = complex ? setFull[(uint8_t)(rot % len)] : setSimple[(uint8_t)(rot % len)];
    uint8_t d1 = complex ? setFull[(uint8_t)((rot + spread) % len)] : setSimple[(uint8_t)((rot + spread) % len)];
    uint8_t d2 = complex ? setFull[(uint8_t)((rot + (spread * 2U)) % len)] : setSimple[(uint8_t)((rot + (spread * 2U)) % len)];

    unsigned long pw = pulseWidthFromClock(clkP ? clkP : 40);

    if ((masterCount % d0) == 0) schPulseA(t, 0, pw, 4095);
    if ((masterCount % d1) == 0) schPulseB(t, 0, pw, 4095);
    if ((masterCount % d2) == 0) schPulseG(t, 0, pw);
  }

  schService(t);
}
