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
void handleModeSwarm() {
  static bool inited = false;
  static unsigned long lastUs = 0;
  static float phase = 0.0f;
  static float follower = 0.5f;

  if (enteredMode == MODE_SWARM) {
    enteredMode = -1;
    inited = false;
    phase = 0.0f;
    follower = 0.5f;
    digitalWrite(GATEOUT, LOW);
  }

  if (checkReset()) {
    phase = 0.0f;
    follower = 0.5f;
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
  float freq = lerpf(0.03f, 12.0f, (float)ctrl1Val / 1023.0f);
  float response = lerpf(0.2f, 12.0f, (float)ctrl2Val / 1023.0f) * lerpf(0.35f, 2.2f, p3);

  phase = fract1(phase + freq * dt);

  float spread = lerpf(1.3f, 4.1f, p3);
  float leader = 0.5f + 0.35f * sinf(2.0f * PI * phase) + 0.15f * sinf(2.0f * PI * (phase * spread));
  leader = clampf(leader, 0.0f, 1.0f);

  follower += (leader - follower) * response * dt;
  follower = clampf(follower, 0.0f, 1.0f);

  setOutA(unipolarTo12(leader));
  setOutB(unipolarTo12(follower));

  float dist = fabsf(leader - follower);
  float thr = lerpf(0.025f, 0.18f, p3);
  digitalWrite(GATEOUT, (dist < thr) ? HIGH : LOW);
}
