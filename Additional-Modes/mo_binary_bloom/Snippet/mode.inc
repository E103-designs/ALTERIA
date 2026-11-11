enum MotionMode {
  MODE_AD_ENVELOPE = 0,
  MODE_SLEW_FUNCTION,
  MODE_PENDULUM,
  MODE_RADIOACTIVE_GEIGER,
  MODE_SWARM,
  MODE_NEURAL_GHOST,
  MODE_PHASE_FORGE,
  MODE_VECTOR_DRIFT,
  MODE_ENERGY_FIELD,
  MODE_BINARY_BLOOM,
  MODE_CHAOS_MAP,
  MODE_SAMPLE_HOLD_MORPH,
  MODE_MIRROR_LFO,
  MODE_HARMONIC_STACK,
  MODE_RANDOM_WALK_CONSTRAIN,
  MODE_CV_CROSS_MOD
};


int enteredMode=-1;
void handleModeBinaryBloom(unsigned long t) {
  static uint16_t pattern = 0x00F3;
  static unsigned long lastTick = 0;
  static bool gOn = false;
  static unsigned long gStart = 0;

  if (enteredMode == MODE_BINARY_BLOOM) {
    enteredMode = -1;
    pattern = (uint16_t)random(1, 65535);
    lastTick = t;
    gOn = false;
    setOutA(0);
    setOutB(0);
    digitalWrite(GATEOUT, LOW);
  }

  if (checkReset()) pattern = (uint16_t)random(1, 65535);

  float p3 = pot3f();
  unsigned long tickMs = (unsigned long)lerpf(220.0f, 22.0f, p3);

  if ((unsigned long)(t - lastTick) >= tickMs) {
    lastTick = t;
    uint8_t popBefore = popcount16(pattern);
    uint16_t newP = 0;
    uint8_t birthBias = (uint8_t)map(ctrl1Val, 0, 1023, 0, 255);
    uint8_t decayBias = (uint8_t)map(ctrl2Val, 0, 1023, 0, 255);
    uint8_t mutation = (uint8_t)lerpf(0.0f, 55.0f, p3);

    for (uint8_t i = 0; i < 16; i++) {
      uint8_t l = (i == 0) ? 15 : (i - 1);
      uint8_t r = (i == 15) ? 0 : (i + 1);
      bool self = (pattern >> i) & 1U;
      uint8_t neigh = (((pattern >> l) & 1U) ? 1 : 0) + (((pattern >> r) & 1U) ? 1 : 0);
      bool nextState = self;

      if (!self) {
        uint16_t threshold = (uint16_t)birthBias + (uint16_t)neigh * 70U;
        if (random(0, 256) < (int)(threshold > 255 ? 255 : threshold)) nextState = true;
      } else {
        int decayTh = (int)decayBias - (int)neigh * 40;
        if (decayTh < 0) decayTh = 0;
        if (random(0, 256) < decayTh) nextState = false;
      }

      if (random(0, 256) < mutation) nextState = !nextState;
      if (nextState) newP |= (uint16_t)(1U << i);
    }

    pattern = newP;
    uint8_t popAfter = popcount16(pattern);
    setOutA((uint16_t)map(popAfter, 0, 16, 0, 4095));
    setOutB(quantize12ByPot3((uint16_t)((pattern & 0x00FFU) * 16U)));

    if (popAfter > popBefore) {
      digitalWrite(GATEOUT, HIGH);
      gStart = t;
      gOn = true;
    }
  }

  if (gOn && (t - gStart >= 8UL)) {
    digitalWrite(GATEOUT, LOW);
    gOn = false;
  }
}
