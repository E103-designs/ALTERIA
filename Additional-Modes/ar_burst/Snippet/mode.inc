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
    spacingMs = periodFromSpeedPotMs(ctrl1Val, 8, 600);
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
