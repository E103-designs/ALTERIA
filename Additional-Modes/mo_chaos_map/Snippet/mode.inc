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
void handleModeChaosMap(unsigned long t) {
  static float x = 0.314f;
  static float xPrev = 0.271f;
  static unsigned long lastTick = 0;
  static bool gOn = false;
  static unsigned long gStart = 0;

  if (enteredMode == MODE_CHAOS_MAP) {
    enteredMode = -1;
    x = 0.314f;
    xPrev = 0.271f;
    lastTick = t;
    gOn = false;
    digitalWrite(GATEOUT, LOW);
  }

  if (checkReset()) {
    x = clampf(rand01(), 0.05f, 0.95f);
    xPrev = clampf(rand01(), 0.05f, 0.95f);
  }

  unsigned long tickMs = (unsigned long)map(ctrl2Val, 0, 1023, 5, 350);
  if ((unsigned long)(t - lastTick) >= tickMs) {
    lastTick = t;
    float r = lerpf(2.8f, 4.0f, (float)ctrl1Val / 1023.0f);
    float thr = lerpf(0.35f, 0.92f, pot3f());

    xPrev = x;
    x = r * x * (1.0f - x);
    x = clampf(x, 0.0f, 1.0f);

    setOutA(unipolarTo12(shapeUniByPot3(x)));
    setOutB(unipolarTo12(shapeUniByPot3(xPrev)));

    if (xPrev < thr && x >= thr) {
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
