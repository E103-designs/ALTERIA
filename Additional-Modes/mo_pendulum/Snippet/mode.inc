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
void handleModePendulum() {
  static bool inited = false;
  static unsigned long lastUs = 0;
  static float phase = 0.0f;
  static float amp = 0.8f;
  static float prevPos = 0.0f;
  static bool gOn = false;
  static unsigned long gStart = 0;

  if (enteredMode == MODE_PENDULUM) {
    enteredMode = -1;
    inited = false;
    phase = 0.0f;
    amp = 0.9f;
    prevPos = 0.0f;
    gOn = false;
    digitalWrite(GATEOUT, LOW);
  }

  if (checkReset()) {
    amp = 1.0f;
    phase = 0.0f;
  }

  unsigned long nowUs = micros();
  if (!inited) {
    lastUs = nowUs;
    inited = true;
  }
  float dt = (float)(nowUs - lastUs) * 1e-6f;
  lastUs = nowUs;
  if (dt > 0.05f) dt = 0.05f;

  float freq = lerpf(0.05f, 10.0f, (float)ctrl1Val / 1023.0f);
  float damp = lerpf(0.03f, 1.80f, (float)ctrl2Val / 1023.0f);
  float tension = pot3f();

  phase += 2.0f * PI * freq * dt;
  if (phase > 1000.0f) phase = fmodf(phase, 2.0f * PI);

  amp *= expf(-damp * dt);
  if (amp < lerpf(0.005f, 0.08f, tension)) amp = lerpf(0.005f, 0.08f, tension);

  float pos = amp * sinf(phase);
  float vel = amp * cosf(phase);
  pos = lerpf(pos, foldBiByPot3(pos), tension);
  vel = lerpf(vel, shapeBiByPot3(vel), tension);

  setOutA(bipolarTo12(pos));
  setOutB(bipolarTo12(vel));

  if (prevPos < 0.0f && pos >= 0.0f) {
    digitalWrite(GATEOUT, HIGH);
    gStart = millis();
    gOn = true;
  }
  prevPos = pos;

  unsigned long t = millis();
  if (gOn && (t - gStart >= 8UL)) {
    digitalWrite(GATEOUT, LOW);
    gOn = false;
  }
}
