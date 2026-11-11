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
void handleModeHarmonicStack() {
  static bool inited = false;
  static unsigned long lastUs = 0;
  static float phase = 0.0f;
  static bool gOn = false;
  static unsigned long gStart = 0;

  if (enteredMode == MODE_HARMONIC_STACK) {
    enteredMode = -1;
    inited = false;
    phase = 0.0f;
    gOn = false;
    digitalWrite(GATEOUT, LOW);
  }

  if (checkReset()) phase = 0.0f;

  unsigned long nowUs = micros();
  if (!inited) {
    lastUs = nowUs;
    inited = true;
  }
  float dt = (float)(nowUs - lastUs) * 1e-6f;
  lastUs = nowUs;
  if (dt > 0.05f) dt = 0.05f;

  float base = lerpf(0.03f, 15.0f, (float)ctrl1Val / 1023.0f);
  float mix = (float)ctrl2Val / 1023.0f;
  float oldPhase = phase;
  phase = fract1(phase + base * dt);
  bool wrapped = (phase < oldPhase);

  float p = 2.0f * PI * phase;
  float s1 = sinf(p);
  float s2 = sinf(2.0f * p);
  float s3 = sinf(3.0f * p);
  float s5 = sinf(5.0f * p);

  float a = (1.00f * s1 + (0.65f + 0.25f * mix) * s2 + (0.30f + 0.50f * mix) * s3) / (1.95f + 0.75f * mix);
  float b = (0.90f * s2 + (0.45f + 0.35f * mix) * s3 + (0.20f + 0.70f * mix) * s5) / (1.55f + 1.05f * mix);

  a = foldBiByPot3(a);
  b = foldBiByPot3(b);

  setOutA(bipolarTo12(a));
  setOutB(bipolarTo12(b));

  unsigned long t = millis();
  if (wrapped) {
    digitalWrite(GATEOUT, HIGH);
    gStart = t;
    gOn = true;
  }
  if (gOn && (t - gStart >= 8UL)) {
    digitalWrite(GATEOUT, LOW);
    gOn = false;
  }
}
