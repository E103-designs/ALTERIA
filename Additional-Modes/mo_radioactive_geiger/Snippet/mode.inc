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
void handleModeRadioactiveGeiger(unsigned long t) {
  static bool inited = false;
  static unsigned long lastUs = 0;
  static float charge = 1.0f;
  static uint16_t sh = 0;
  static bool gOn = false;
  static unsigned long gStart = 0;
  static unsigned long lastEntropySample = 0;
  static uint16_t entropyMix = 0;

  if (enteredMode == MODE_RADIOACTIVE_GEIGER) {
    enteredMode = -1;
    inited = false;
    charge = 1.0f;
    sh = (uint16_t)random(0, 4096);
    gOn = false;
    setOutA(4095);
    setOutB(sh);
    digitalWrite(GATEOUT, LOW);
  }

  if (checkReset()) charge = 1.0f;

  if ((unsigned long)(t - lastEntropySample) >= 25UL) {
    lastEntropySample = t;
    uint16_t n = analogRead(RANDOM_SEED_PIN);
    entropyMix ^= (uint16_t)((n << 2) ^ (n >> 1) ^ (uint16_t)t);
  }

  unsigned long nowUs = micros();
  if (!inited) {
    lastUs = nowUs;
    inited = true;
  }
  float dt = (float)(nowUs - lastUs) * 1e-6f;
  lastUs = nowUs;
  if (dt > 0.05f) dt = 0.05f;

  float chaos = pot3f();
  float rateHz = lerpf(0.1f, 80.0f, (float)ctrl1Val / 1023.0f) * lerpf(0.35f, 2.4f, chaos);
  float recSpeed = lerpf(0.05f, 4.0f, (float)ctrl2Val / 1023.0f);

  charge += (1.0f - charge) * recSpeed * dt;
  if (charge > 1.0f) charge = 1.0f;

  float p = rateHz * dt;
  if (p > 0.95f) p = 0.95f;

  if (rand01() < p) {
    uint16_t n = analogRead(RANDOM_SEED_PIN);
    long mix = random(0, 4096) ^ ((long)n << 2) ^ entropyMix ^ (long)micros();
    sh = quantize12ByPot3((uint16_t)(mix & 0x0FFF));

    float dropMax = lerpf(0.20f, 0.02f, chaos);
    charge = lerpf(0.01f, dropMax, rand01());

    digitalWrite(GATEOUT, HIGH);
    gStart = t;
    gOn = true;
  }

  setOutA(unipolarTo12(shapeUniByPot3(charge)));
  setOutB(sh);

  if (gOn && (t - gStart >= TRIG_PULSE_MS)) {
    digitalWrite(GATEOUT, LOW);
    gOn = false;
  }
}
