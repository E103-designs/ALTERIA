static void modeTremoloAutoPan(unsigned long t) {
  static uint16_t phase;
  static uint16_t currentX;
  static uint16_t targetX;
  static unsigned long lastT;
  static unsigned long nextRandomT;
  static uint8_t lastHalf;
  static uint16_t currentInc;

  if (modeJustChanged) {
    phase = 0;
    currentX = 2048;
    targetX = 2048;
    lastT = t;
    nextRandomT = t;
    lastHalf = 0;
    currentInc = (uint16_t)mapQuadraticTo(ctrl1Val, 2, 2200);
    schClearAll();
    dacSetA(2048);
    dacSetB(2048);
    setGate(false);
  }

  if (checkReset()) {
    phase = 0;
    currentX = 2048;
    targetX = 2048;
    lastT = t;
    nextRandomT = t;
    lastHalf = 0;
    currentInc = (uint16_t)mapQuadraticTo(ctrl1Val, 2, 2200);
    schClearAll();
    dacSetA(2048);
    dacSetB(2048);
    schPulseG(t, 0, 4);
  }

  unsigned long dt = t - lastT;
  if (dt > 30) dt = 30;
  lastT = t;

  uint16_t baseInc = (uint16_t)mapQuadraticTo(ctrl1Val, 2, 2200);
  uint16_t amount = (uint16_t)mapQuadraticTo(ctrl2Val, 0, 2047);
  uint16_t randomAmt = (uint16_t)ctrl3Val;

  if (randomAmt < 650) {
    if(randomAmt==0)currentInc=baseInc;
    // Regular to drifting LFO.
    uint8_t halfNow = (phase & 0x8000) ? 1 : 0;

    if (halfNow != lastHalf) {
      lastHalf = halfNow;
      unsigned long pw = 4;
      schPulseG(t, 0, pw);

      long spread = ((long)baseInc * (long)randomAmt) / 1600L;
      long r = spread ? random(-spread, spread + 1) : 0;
      long ni = (long)baseInc + r;
      if (ni < 1) ni = 1;
      if (ni > 3000) ni = 3000;
      currentInc = (uint16_t)ni;
    }

    if (currentInc == 0) currentInc = baseInc;
    phase = (uint16_t)(phase + (uint16_t)((uint32_t)currentInc * dt));
    currentX = triangleFromPhase16(phase);
  } else {
    // Random wave with slew to new target values.
    unsigned long halfPeriod = 32768UL / (baseInc ? baseInc : 1);
    if (halfPeriod < 6) halfPeriod = 6;
    if (halfPeriod > 5000) halfPeriod = 5000;

    if ((long)(t - nextRandomT) >= 0) {
      targetX = (uint16_t)random(0, 4096);
      unsigned long minHold = halfPeriod / 4UL;
      unsigned long maxHold = halfPeriod;
      if (minHold < 4) minHold = 4;
      if (maxHold < minHold + 1) maxHold = minHold + 1;
      unsigned long chaos = (unsigned long)map(randomAmt, 650, 1023, 0, 100);
      unsigned long hold = random((int)minHold, (int)maxHold + 1);
      if (chaos > 60 && random(0, 100) < (int)(chaos - 50)) hold = minHold;
      nextRandomT = t + hold;
      schPulseG(t, 0, 4);
    }

    int32_t err = (int32_t)targetX - (int32_t)currentX;
    uint32_t slew = ((uint32_t)baseInc * (uint32_t)dt * (uint32_t)map(randomAmt, 650, 1023, 1, 8));
    slew /= 4UL;
    if (slew < 1) slew = 1;

    if (err > 0) {
      if ((uint32_t)err < slew) currentX = targetX;
      else currentX = (uint16_t)(currentX + slew);
    } else if (err < 0) {
      if ((uint32_t)(-err) < slew) currentX = targetX;
      else currentX = (uint16_t)(currentX - slew);
    }
  }

  int32_t bipolar = (int32_t)currentX - 2048L;
  int32_t dev = (bipolar * (int32_t)amount) / 2047L;
  uint16_t a = clamp12(2048L + dev);
  uint16_t b = clamp12(2048L - dev);

  dacSetA(a);
  dacSetB(b);

  schService(t);
}
