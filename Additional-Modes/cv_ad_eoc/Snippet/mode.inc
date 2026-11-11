static void modeAD_EOC(unsigned long t) {
  static bool env;
  static unsigned long tStart, attMs, decMs;

  if (modeJustChanged) {
    env = false;
    tStart = 0;
    attMs = 50;
    decMs = 200;
    schClearAll();
    dacSetA(0);
    dacSetB(4095);
    setGate(false);
  }

  if (checkReset()) {
    attMs = (unsigned long)mapQuadraticTo(ctrl1Val, 5, 2500);
    decMs = (unsigned long)mapQuadraticTo(ctrl2Val, 5, 2500);
    tStart = t;
    env = true;
    setGate(false);
    dacSetA(0);
    dacSetB(4095);
  }

  if (env) {
    unsigned long el = t - tStart;
    unsigned long total = attMs + decMs;
    uint16_t v = 0;

    if (el <= attMs) {
      uint16_t x = attMs ? (uint16_t)((el * 4095UL) / attMs) : 4095;
      v = shapeProgressByPot(x, ctrl3Val);
    } else if (el <= total) {
      unsigned long de = el - attMs;
      uint16_t x = decMs ? (uint16_t)((de * 4095UL) / decMs) : 4095;
      uint16_t shaped = shapeProgressByPot(x, ctrl3Val);
      v = (uint16_t)(4095 - shaped);
    } else {
      env = false;
      v = 0;
      schPulseG(t, 0, 10);
    }

    dacSetA(v);
    dacSetB((uint16_t)(4095 - v));
  }

  schService(t);
}
