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
void handleModeVectorDrift() {
  static bool inited = false;
  static unsigned long lastUs = 0;
  static float x = 0.1f, y = -0.1f;
  static float prevX = 0.1f;
  static bool gOn = false;
  static unsigned long gStart = 0;

  if (enteredMode == MODE_VECTOR_DRIFT) {
    enteredMode = -1;
    inited = false;
    x = 0.1f;
    y = -0.1f;
    prevX = x;
    gOn = false;
    digitalWrite(GATEOUT, LOW);
  }

  if (checkReset()) {
    x = (rand01() * 2.0f - 1.0f) * 0.4f;
    y = (rand01() * 2.0f - 1.0f) * 0.4f;
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
  float speed = lerpf(0.2f, 8.0f, (float)ctrl1Val / 1023.0f);
  float chaos = lerpf(0.0f, 1.2f, (float)ctrl2Val / 1023.0f);

  float nx = (rand01() - 0.5f) * 0.8f;
  float ny = (rand01() - 0.5f) * 0.8f;
  float gravity = lerpf(0.02f, 0.85f, p3);

  float dx = (-(0.4f + chaos) * y + 0.25f * sinf(2.5f * y) + chaos * nx - gravity * x);
  float dy = ( (0.4f + chaos) * x + 0.25f * sinf(2.0f * x) + chaos * ny - gravity * y);

  x += dx * speed * dt;
  y += dy * speed * dt;
  x *= (1.0f - lerpf(0.05f, 0.35f, p3) * dt);
  y *= (1.0f - lerpf(0.05f, 0.35f, p3) * dt);
  x = clampf(x, -1.0f, 1.0f);
  y = clampf(y, -1.0f, 1.0f);

  setOutA(bipolarTo12(x));
  setOutB(bipolarTo12(y));

  unsigned long t = millis();
  bool cross = (prevX < 0.0f && x >= 0.0f);
  bool nearCenter = fabsf(x) < lerpf(0.005f, 0.09f, p3);
  if (cross || nearCenter) {
    digitalWrite(GATEOUT, HIGH);
    gStart = t;
    gOn = true;
  }
  prevX = x;

  if (gOn && (t - gStart >= 8UL)) {
    digitalWrite(GATEOUT, LOW);
    gOn = false;
  }
}
