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
void handleModePhaseForge() {
  static bool inited = false;
  static unsigned long lastUs = 0;
  static float phase = 0.0f;
  static bool gOn = false;
  static unsigned long gStart = 0;

  if (enteredMode == MODE_PHASE_FORGE) {
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

  float rate = lerpf(0.03f, 20.0f, (float)ctrl1Val / 1023.0f);
  float k = (float)ctrl2Val / 1023.0f;
  float oldPhase = phase;
  phase = fract1(phase + rate * dt);
  bool wrapped = (phase < oldPhase);

  float warpA = lerpf(0.15f, 0.85f, k);
  float offset = lerpf(0.0f, 0.95f, k);
  float a = shapeUniByPot3(asymTri01(phase, warpA));
  float b = shapeUniByPot3(asymTri01(fract1(phase + offset), clampf(1.0f - warpA, 0.15f, 0.85f)));

  setOutA(unipolarTo12(a));
  setOutB(unipolarTo12(b));

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
