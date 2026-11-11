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
void handleModeCvCrossMod() {
  static bool inited = false;
  static unsigned long lastUs = 0;
  static float p1 = 0.0f, p2 = 0.31f;
  static bool prevNearSync = false;
  static bool gOn = false;
  static unsigned long gStart = 0;

  if (enteredMode == MODE_CV_CROSS_MOD) {
    enteredMode = -1;
    inited = false;
    p1 = 0.0f;
    p2 = 0.31f;
    prevNearSync = false;
    gOn = false;
    digitalWrite(GATEOUT, LOW);
  }

  if (checkReset()) {
    p1 = 0.0f;
    p2 = 0.25f;
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
  float base = lerpf(0.05f, 18.0f, (float)ctrl1Val / 1023.0f);
  float depth = lerpf(0.0f, 1.2f, (float)ctrl2Val / 1023.0f);

  float y1 = 0.5f + 0.5f * sinf(2.0f * PI * p1);
  float y2 = 0.5f + 0.5f * sinf(2.0f * PI * p2);
  float f1 = base * (1.0f + depth * ((y2 - 0.5f) * 0.9f));
  float f2 = (base * 1.37f) * (1.0f + depth * ((y1 - 0.5f) * 0.9f));

  if (f1 < 0.01f) f1 = 0.01f;
  if (f2 < 0.01f) f2 = 0.01f;

  p1 = fract1(p1 + f1 * dt);
  p2 = fract1(p2 + f2 * dt);

  float sine1 = sinf(2.0f * PI * p1);
  float sine2 = sinf(2.0f * PI * p2);
  float tri1 = tri01(p1) * 2.0f - 1.0f;
  float tri2 = tri01(p2) * 2.0f - 1.0f;
  float wave1 = lerpf(sine1, foldBiByPot3(tri1), p3);
  float wave2 = lerpf(sine2, foldBiByPot3(tri2), p3);

  setOutA(bipolarTo12(wave1));
  setOutB(bipolarTo12(wave2));

  float d = fabsf(p1 - p2);
  if (d > 0.5f) d = 1.0f - d;
  bool nearSync = (d < lerpf(0.008f, 0.055f, p3));

  unsigned long t = millis();
  if (nearSync && !prevNearSync) {
    digitalWrite(GATEOUT, HIGH);
    gStart = t;
    gOn = true;
  }
  prevNearSync = nearSync;

  if (gOn && (t - gStart >= 8UL)) {
    digitalWrite(GATEOUT, LOW);
    gOn = false;
  }
}
