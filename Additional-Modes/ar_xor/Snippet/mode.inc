static void modeLogicXOR(unsigned long t) {
  static unsigned long n1, n2;
  static bool s1, s2;
  static uint16_t lastSkew;

  unsigned long i1 = periodFromSpeedPotMs(ctrl1Val, 10, 2000);
  unsigned long i2 = periodFromSpeedPotMs(ctrl2Val, 10, 2000);
  uint16_t skew = (uint16_t)map(ctrl3Val, 0, 1023, 0, (int)i2);

  if (modeJustChanged) {
    n1 = t;
    n2 = t + skew;
    lastSkew = skew;
    s1 = s2 = false;
    dacSetA(0); dacSetB(0); setGate(false);
  }
  if (checkReset()) {
    n1 = t;
    n2 = t + skew;
    lastSkew = skew;
    s1 = s2 = false;
    dacSetA(0); dacSetB(0); setGate(false);
  }

  if (skew != lastSkew) {
    long delta = (long)skew - (long)lastSkew;
    n2 = (unsigned long)((long)n2 + delta);
    lastSkew = skew;
  }

  if ((long)(t - n1) >= 0) { n1 = t + (i1 / 2UL); s1 = !s1; dacSetA(s1 ? 4095 : 0); }
  if ((long)(t - n2) >= 0) { n2 = t + (i2 / 2UL); s2 = !s2; dacSetB(s2 ? 4095 : 0); }

  setGate((s1 != s2) ? HIGH : LOW);
}
