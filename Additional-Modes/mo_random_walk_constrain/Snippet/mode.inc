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
void handleModeRandomWalkConstrain(unsigned long t) {
  static float x = 0.5f;
  static float v = 0.0f;
  static unsigned long lastTick = 0;
  static bool gOn = false;
  static unsigned long gStart = 0;

  if (enteredMode == MODE_RANDOM_WALK_CONSTRAIN) {
    enteredMode = -1;
    x = 0.5f;
    v = 0.0f;
    lastTick = t;
    gOn = false;
    setOutA(unipolarTo12(x));
    setOutB(bipolarTo12(v));
    digitalWrite(GATEOUT, LOW);
  }

  if (checkReset()) {
    x = 0.5f;
    v = 0.0f;
  }

  unsigned long tickMs = (unsigned long)map(ctrl1Val, 0, 1023, 15, 300);
  if ((unsigned long)(t - lastTick) >= tickMs) {
    lastTick = t;
    float p3 = pot3f();
    float oldV = v;
    float constrain = lerpf(0.0f, 1.4f, (float)ctrl2Val / 1023.0f);
    float step = lerpf(0.035f, 0.28f, p3);
    float delta = (rand01() * 2.0f - 1.0f) * step;
    float pull = (0.5f - x) * 0.22f * constrain;

    v = delta + pull;
    x += v;

    bool hit = false;
    float bounce = lerpf(0.35f, 1.0f, p3);
    if (x < 0.0f) { x = 0.0f; v = fabsf(v) * bounce; hit = true; }
    if (x > 1.0f) { x = 1.0f; v = -fabsf(v) * bounce; hit = true; }

    bool reversed = ((oldV > 0.0f && v < 0.0f) || (oldV < 0.0f && v > 0.0f));
    setOutA(unipolarTo12(shapeUniByPot3(x)));
    setOutB(bipolarTo12(clampf(v * lerpf(2.0f, 6.0f, p3), -1.0f, 1.0f)));

    if (hit || reversed) {
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
