static void modeLogicAND(unsigned long t) {
  static unsigned long n1, n2;
  static bool s1, s2;

  if (modeJustChanged) {
    n1 = n2 = t;
    s1 = s2 = false;
    dacSetA(0); dacSetB(0); setGate(false);
  }
  if (checkReset()) {
    n1 = n2 = t;
    s1 = s2 = false;
    dacSetA(0); dacSetB(0); setGate(false);
  }

  unsigned long p1 = (unsigned long)mapQuadraticTo(ctrl1Val, 10, 2000);
  unsigned long p2 = (unsigned long)mapQuadraticTo(ctrl2Val, 10, 2000);
  uint8_t duty = (uint8_t)map(ctrl3Val, 0, 1023, 5, 95);

  unsigned long h1 = (p1 * duty) / 100UL; if (h1 < 1) h1 = 1;
  unsigned long l1 = p1 - h1; if (l1 < 1) l1 = 1;
  unsigned long h2 = (p2 * duty) / 100UL; if (h2 < 1) h2 = 1;
  unsigned long l2 = p2 - h2; if (l2 < 1) l2 = 1;

  if ((long)(t - n1) >= 0) {
    s1 = !s1;
    n1 = t + (s1 ? h1 : l1);
    dacSetA(s1 ? 4095 : 0);
  }

  if ((long)(t - n2) >= 0) {
    s2 = !s2;
    n2 = t + (s2 ? h2 : l2);
    dacSetB(s2 ? 4095 : 0);
  }

  setGate((s1 && s2) ? HIGH : LOW);
}
