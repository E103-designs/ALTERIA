static void modeClockDivMult(unsigned long t) {
  static const uint8_t numTbl[11] = {8,6,4,3,2,1,1,1,1,1,1};
  static const uint8_t denTbl[11] = {1,1,1,1,1,1,2,3,4,6,8};

  static unsigned long lastClk, clkP;
  static bool haveClk;
  static uint8_t idxA, idxB;
  static uint8_t lastIdxA, lastIdxB;
  static uint8_t divCntA, divCntB;
  static unsigned long nextSubA, nextSubB;
  static uint8_t subsA, subsB;

  if (modeJustChanged) {
    lastClk = 0; clkP = 0; haveClk = false;
    idxA = 5; idxB = 5; lastIdxA = 5; lastIdxB = 5;
    divCntA = divCntB = 0;
    nextSubA = nextSubB = 0;
    subsA = subsB = 0;
    schClearAll();
    resetOutputs();
  }

  if (checkReset()) {
    if (lastClk) { clkP = t - lastClk; if (clkP < 2) clkP = 2; haveClk = true; }
    lastClk = t;

    idxA = (uint8_t)map(ctrl1Val, 0, 1023, 0, 10);
    idxB = (uint8_t)map(ctrl2Val, 0, 1023, 0, 10);

    if (idxA != lastIdxA) { divCntA = 0; subsA = 0; nextSubA = 0; lastIdxA = idxA; }
    if (idxB != lastIdxB) { divCntB = 0; subsB = 0; nextSubB = 0; lastIdxB = idxB; }

    unsigned long base = haveClk ? clkP : 40;
    unsigned long refPw = pulseWidthFromPot3(base, 80);
    schPulseG(t, 0, refPw);

    {
      uint8_t num = numTbl[idxA], den = denTbl[idxA];
      unsigned long seg = (den > 1 && haveClk) ? (clkP / den) : base;
      if (seg < 2) seg = 2;
      unsigned long pw = pulseWidthFromPot3(seg, 80);

      if (den == 1 && num > 1) {
        divCntA++;
        if (divCntA >= num) { divCntA = 0; schPulseA(t, 0, pw, 4095); }
        subsA = 0;
      } else {
        schPulseA(t, 0, pw, 4095);
        if (haveClk && den > 1) {
          unsigned long subI = clkP / den; if (subI < 1) subI = 1;
          nextSubA = t + subI;
          subsA = (uint8_t)(den - 1);
        } else subsA = 0;
      }
    }

    {
      uint8_t num = numTbl[idxB], den = denTbl[idxB];
      unsigned long seg = (den > 1 && haveClk) ? (clkP / den) : base;
      if (seg < 2) seg = 2;
      unsigned long pw = pulseWidthFromPot3(seg, 80);

      if (den == 1 && num > 1) {
        divCntB++;
        if (divCntB >= num) { divCntB = 0; schPulseB(t, 0, pw, 4095); }
        subsB = 0;
      } else {
        schPulseB(t, 0, pw, 4095);
        if (haveClk && den > 1) {
          unsigned long subI = clkP / den; if (subI < 1) subI = 1;
          nextSubB = t + subI;
          subsB = (uint8_t)(den - 1);
        } else subsB = 0;
      }
    }
  }

  if (haveClk) {
    while (subsA > 0 && (long)(t - nextSubA) >= 0) {
      unsigned long subI = clkP / denTbl[idxA]; if (subI < 1) subI = 1;
      unsigned long pw = pulseWidthFromPot3(subI, 80);
      schPulseA(nextSubA, 0, pw, 4095);
      nextSubA += subI;
      subsA--;
    }

    while (subsB > 0 && (long)(t - nextSubB) >= 0) {
      unsigned long subI = clkP / denTbl[idxB]; if (subI < 1) subI = 1;
      unsigned long pw = pulseWidthFromPot3(subI, 80);
      schPulseB(nextSubB, 0, pw, 4095);
      nextSubB += subI;
      subsB--;
    }
  }

  schService(t);
}
