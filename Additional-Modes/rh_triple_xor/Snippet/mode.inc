static void modeTripleXorLfo(unsigned long t) {
  static unsigned long next1, next2, next3;
  static bool l1, l2, l3;

  if (modeJustChanged) {
    next1 = next2 = next3 = t;
    l1 = l2 = l3 = false;
    schClearAll();
    resetOutputs();
  }

  unsigned long half1 = (unsigned long)mapQuadraticTo(1023 - ctrl1Val, 5, 4000);
  unsigned long half2 = (unsigned long)mapQuadraticTo(1023 - ctrl2Val, 5, 4000);
  unsigned long half3 = (unsigned long)mapQuadraticTo(1023 - ctrl3Val, 5, 4000);

  if (checkReset()) {
    l1 = l2 = l3 = false;
    next1 = t + half1;
    next2 = t + half2;
    next3 = t + half3;
  }

  uint8_t guard = 0;
  while ((long)(t - next1) >= 0 && guard++ < 8) { l1 = !l1; next1 += half1; }
  guard = 0;
  while ((long)(t - next2) >= 0 && guard++ < 8) { l2 = !l2; next2 += half2; }
  guard = 0;
  while ((long)(t - next3) >= 0 && guard++ < 8) { l3 = !l3; next3 += half3; }

  bool x12 = l1 ^ l2;
  bool x23 = l2 ^ l3;
  bool x31 = l3 ^ l1;

  dacSetA(x12 ? 4095 : 0);
  dacSetB(x23 ? 4095 : 0);
  setGate(x31);
}
