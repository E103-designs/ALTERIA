static void modeBouncingBall(unsigned long t) {
  static bool active;
  static unsigned long nextBounce;
  static unsigned long intervalMs;
  static uint16_t amp;
  static uint8_t bouncesLeft;
  static unsigned long lastClk, clkP;

  if (modeJustChanged) {
    active = false;
    nextBounce = 0;
    intervalMs = 60;
    amp = 0;
    bouncesLeft = 0;
    lastClk = 0;
    clkP = 0;
    schClearAll();
    resetOutputs();
  }

  if (checkReset()) {
    if (lastClk) clkP = t - lastClk;
    lastClk = t;

    intervalMs = (unsigned long)mapQuadraticTo(ctrl1Val, 12, 260);
    bouncesLeft = (uint8_t)map(ctrl2Val, 0, 1023, 2, 18);

    amp = 4095;
    active = true;
    nextBounce = t;

    dacSetA(amp);
    dacSetB((uint16_t)(4095 - amp));
  }

  if (active) {
    if ((long)(t - nextBounce) >= 0 && bouncesLeft > 0) {
      unsigned long pw = pulseWidthFromClock(clkP ? clkP : 50);
      schPulseG(t, 0, pw);

      // Low POT3 = strong damping. High POT3 = elastic / long decay.
      uint16_t r = (uint16_t)map(ctrl3Val, 0, 1023, 1200, 3850);
      amp = (uint16_t)(((uint32_t)amp * (uint32_t)r) >> 12);
      if (amp < 40) amp = 0;

      dacSetA(amp);
      dacSetB((uint16_t)(4095 - amp));

      uint16_t g = (uint16_t)map(ctrl1Val, 0, 1023, 1100, 1600);
      intervalMs = (unsigned long)(((uint32_t)intervalMs * (uint32_t)g) >> 10);
      if (intervalMs < 6) intervalMs = 6;
      if (intervalMs > 1200) intervalMs = 1200;

      nextBounce = t + intervalMs;
      bouncesLeft--;

      if (bouncesLeft == 0 || amp == 0) {
        active = false;
      }
    }
  }

  schService(t);
}
