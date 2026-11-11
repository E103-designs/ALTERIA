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
void handleModeNeuralGhost(unsigned long t) {
  static bool inited = false;
  static unsigned long lastUs = 0;
  static float V = 0.0f;
  static float refr = 0.0f;
  static bool gOn = false;
  static unsigned long gStart = 0;

  if (enteredMode == MODE_NEURAL_GHOST) {
    enteredMode = -1;
    inited = false;
    V = 0.0f;
    refr = 0.0f;
    gOn = false;
    setOutA(0);
    setOutB(unipolarTo12(0.5f));
    digitalWrite(GATEOUT, LOW);
  }

  unsigned long nowUs = micros();
  if (!inited) {
    lastUs = nowUs;
    inited = true;
  }
  float dt = (float)(nowUs - lastUs) * 1e-6f;
  lastUs = nowUs;
  if (dt > 0.05f) dt = 0.05f;

  float p3 = pot3f();
  float memory = (float)ctrl1Val / 1023.0f;
  float leak = lerpf(3.5f, 0.12f, memory);
  float threshold = lerpf(0.12f, 0.95f, (float)ctrl2Val / 1023.0f);

  V -= leak * dt;
  if (V < 0.0f) V = 0.0f;

  if (refr > 0.0f) {
    refr -= dt;
    if (refr < 0.0f) refr = 0.0f;
  }

  if (checkReset()) {
    float n = ((float)(analogRead(RANDOM_SEED_PIN) & 31) / 31.0f) * lerpf(0.02f, 0.18f, p3);
    V += lerpf(0.18f, 0.70f, p3) + n;
  }

  V += (rand01() - 0.5f) * lerpf(0.001f, 0.025f, p3);
  V = clampf(V, 0.0f, 1.1f);

  if (refr <= 0.0f && V >= threshold) {
    digitalWrite(GATEOUT, HIGH);
    gStart = t;
    gOn = true;
    V *= lerpf(0.42f, 0.08f, p3);
    refr = lerpf(0.06f, 0.012f, p3);
  }

  setOutA(unipolarTo12(clampf(V, 0.0f, 1.0f)));
  setOutB(unipolarTo12(threshold));

  if (gOn && (t - gStart >= DEFAULT_PULSE_MS)) {
    digitalWrite(GATEOUT, LOW);
    gOn = false;
  }
}
