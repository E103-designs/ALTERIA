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
void handleModeEnergyField() {
  static bool inited = false;
  static unsigned long lastUs = 0;
  static float x = 0.0f, y = 0.0f;
  static float vx = 0.0f, vy = 0.0f;

  if (enteredMode == MODE_ENERGY_FIELD) {
    enteredMode = -1;
    inited = false;
    x = 0.0f; y = 0.0f; vx = 0.0f; vy = 0.0f;
    digitalWrite(GATEOUT, LOW);
  }

  if (checkReset()) {
    x = rand01() * 2.0f - 1.0f;
    y = rand01() * 2.0f - 1.0f;
    vx = 0.0f;
    vy = 0.0f;
  }

  unsigned long nowUs = micros();
  if (!inited) {
    lastUs = nowUs;
    inited = true;
  }
  float dt = (float)(nowUs - lastUs) * 1e-6f;
  lastUs = nowUs;
  if (dt > 0.03f) dt = 0.03f;

  const float ax1 = -0.75f, ay1 = -0.15f;
  const float ax2 =  0.70f, ay2 = -0.55f;
  const float ax3 =  0.10f, ay3 =  0.85f;

  float p3 = pot3f();
  float attract = lerpf(0.2f, 5.0f, (float)ctrl2Val / 1023.0f);
  float energy  = lerpf(0.0f, 1.5f, (float)ctrl1Val / 1023.0f);

  float dx1 = ax1 - x, dy1 = ay1 - y;
  float dx2 = ax2 - x, dy2 = ay2 - y;
  float dx3 = ax3 - x, dy3 = ay3 - y;
  float r1 = dx1*dx1 + dy1*dy1 + 0.05f;
  float r2 = dx2*dx2 + dy2*dy2 + 0.05f;
  float r3 = dx3*dx3 + dy3*dy3 + 0.05f;

  float fx = 0.0f, fy = 0.0f;
  fx += attract * dx1 / r1; fy += attract * dy1 / r1;
  fx += attract * dx2 / r2; fy += attract * dy2 / r2;
  fx += attract * dx3 / r3; fy += attract * dy3 / r3;
  fx += (rand01() - 0.5f) * energy * lerpf(0.25f, 2.0f, p3);
  fy += (rand01() - 0.5f) * energy * lerpf(0.25f, 2.0f, p3);

  vx += fx * dt;
  vy += fy * dt;
  float drag = lerpf(0.45f, 3.4f, p3);
  vx *= (1.0f - drag * dt);
  vy *= (1.0f - drag * dt);

  x += vx * dt;
  y += vy * dt;

  if (x < -1.0f) { x = -1.0f; vx *= -0.7f; }
  if (x >  1.0f) { x =  1.0f; vx *= -0.7f; }
  if (y < -1.0f) { y = -1.0f; vy *= -0.7f; }
  if (y >  1.0f) { y =  1.0f; vy *= -0.7f; }

  setOutA(bipolarTo12(x));
  setOutB(bipolarTo12(y));

  float hotRadius = lerpf(0.035f, 0.16f, p3);
  bool hot = (r1 < hotRadius) || (r2 < hotRadius) || (r3 < hotRadius);
  digitalWrite(GATEOUT, hot ? HIGH : LOW);
}
