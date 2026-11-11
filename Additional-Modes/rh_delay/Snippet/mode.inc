static void modeDelayTrig(unsigned long t) {
  static unsigned long lastClk, clkP;

  if (modeJustChanged) {
    lastClk = 0;
    clkP = 0;
    schClearAll();
    resetOutputs();
  }

  if (checkReset()) {
    if (lastClk) clkP = t - lastClk;
    lastClk = t;

    unsigned long d1 = (unsigned long)mapQuadraticTo(ctrl1Val, 0, 1800);
    unsigned long d2 = (unsigned long)mapQuadraticTo(ctrl2Val, 0, 1800);
    unsigned long base = clkP ? clkP : 120;
    unsigned long pw = pulseWidthFromPot3(base, 300);

    schPulseG(t, 0, pulseWidthFromClock(base));
    schPulseA(t, d1, pw, 4095);
    schPulseB(t, d2, pw, 4095);
  }

  schService(t);
}
