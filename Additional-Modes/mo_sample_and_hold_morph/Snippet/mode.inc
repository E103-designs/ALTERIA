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
void handleModeSampleHoldMorph(unsigned long t) {
  static uint16_t raw = 0;
  static float smooth = 0.0f;
  static unsigned long lastSample = 0;
  static bool inited = false;
  static unsigned long lastUs = 0;
  static bool gOn = false;
  static unsigned long gStart = 0;

  if (enteredMode == MODE_SAMPLE_HOLD_MORPH) {
    enteredMode = -1;
    raw = quantize12ByPot3((uint16_t)random(0, 4096));
    smooth = (float)raw;
    lastSample = t;
    inited = false;
    gOn = false;
    setOutA(raw);
    setOutB(raw);
    digitalWrite(GATEOUT, LOW);
  }

  bool sampled = false;

  if (checkReset()) {
    sampled = true;
  } else {
    unsigned long intMs = (unsigned long)map(ctrl1Val, 0, 1023, 30, 1800);
    if ((unsigned long)(t - lastSample) >= intMs) sampled = true;
  }

  if (sampled) {
    lastSample = t;
    uint16_t n = analogRead(RANDOM_SEED_PIN);
    long mix = random(0, 4096) ^ ((long)n << 2) ^ (long)micros();
    raw = quantize12ByPot3((uint16_t)(mix & 0x0FFF));
    digitalWrite(GATEOUT, HIGH);
    gStart = t;
    gOn = true;
  }

  unsigned long nowUs = micros();
  if (!inited) {
    lastUs = nowUs;
    inited = true;
  }
  float dt = (float)(nowUs - lastUs) * 1e-6f;
  lastUs = nowUs;
  if (dt > 0.05f) dt = 0.05f;

  float glide = lerpf(40.0f, 1.0f, (float)ctrl2Val / 1023.0f);
  smooth += ((float)raw - smooth) * (1.0f / glide) * (dt * 120.0f);

  setOutA(raw);
  setOutB(clamp12i((int32_t)smooth));

  if (gOn && (t - gStart >= TRIG_PULSE_MS)) {
    digitalWrite(GATEOUT, LOW);
    gOn = false;
  }
}
