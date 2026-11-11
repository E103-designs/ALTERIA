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
void handleModeMirrorLFO() {
  static bool inited = false;
  static unsigned long lastUs = 0;
  static float phase = 0.0f;
  static bool gOn = false;
  static unsigned long gStart = 0;

  if (enteredMode == MODE_MIRROR_LFO) {
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

  float rate = lerpf(0.02f, 25.0f, (float)ctrl1Val / 1023.0f);
  float oldPhase = phase;
  phase = fract1(phase + rate * dt);
  bool wrapped = (phase < oldPhase);

  float k = (float)ctrl2Val / 1023.0f;
  float s = 0.5f + 0.5f * sinf(2.0f * PI * phase);
  float tri = tri01(phase);
  float sq = (s >= 0.5f) ? 1.0f : 0.0f;
  float wave = (k < 0.5f) ? lerpf(s, tri, k * 2.0f) : lerpf(tri, sq, (k - 0.5f) * 2.0f);
  wave = shapeUniByPot3(wave);

  uint16_t a = unipolarTo12(wave);
  setOutA(a);
  setOutB(4095U - a);

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
