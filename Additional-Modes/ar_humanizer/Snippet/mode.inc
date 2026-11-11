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
