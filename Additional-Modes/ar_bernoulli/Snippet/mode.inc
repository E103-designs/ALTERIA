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
