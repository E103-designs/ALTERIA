static void modeSHGlide(unsigned long t) {
  static uint16_t target = 2048;
  static uint16_t out2 = 2048;
  static unsigned long lastClk, clkP;
  static unsigned long lastMove;

  if (modeJustChanged) {
    target = 2048;
    out2 = 2048;
    lastClk = 0;
    clkP = 0;
    lastMove = t;
    schClearAll();
    dacSetA(2048);
    dacSetB(2048);
    setGate(false);
  }

  unsigned long now = t;
  unsigned long dt = now - lastMove;
  if (dt > 25) dt = 25;
  lastMove = now;

  uint16_t glide = (uint16_t)map(ctrl2Val, 0, 1023, 0, 450);
  if (glide == 0) {
    out2 = target;
  } else {
    int32_t err = (int32_t)target - (int32_t)out2;
    int32_t step = (err * (int32_t)dt) / (int32_t)glide;

    if (step == 0 && err != 0) {
      step = (err > 0) ? 1 : -1;
    }

    if ((err > 0 && step > err) || (err < 0 && step < err)) {
      step = err;
    }

    out2 = clamp12((int32_t)out2 + step);
  }
  dacSetB(out2);

  if (checkReset()) {
    if (lastClk) clkP = t - lastClk;
    lastClk = t;
    unsigned long pw = pulseWidthFromClock(clkP ? clkP : 40);

    int range = map(ctrl1Val, 0, 1023, 0, 2047);
    int raw = 2048 + (int)random(-range, range + 1);
    if (raw < 0) raw = 0;
    if (raw > 4095) raw = 4095;

    target = quantizeByPot((uint16_t)raw, ctrl3Val);
    dacSetA(target);
    schPulseG(t, 0, pw);
  }

  schService(t);
}
