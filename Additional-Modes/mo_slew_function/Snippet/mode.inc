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
void handleModeSlewFunction() {
  static int32_t currentV = 0;
  static unsigned long lastMicros = 0;
  static bool inited = false;

  if (enteredMode == MODE_SLEW_FUNCTION) {
    enteredMode = -1;
    currentV = 0;
    inited = false;
    setOutA(0);
    setOutB(4095);
    digitalWrite(GATEOUT, LOW);
  }

  unsigned long nowUs = micros();
  if (!inited) {
    lastMicros = nowUs;
    inited = true;
  }
  uint32_t dtUs = nowUs - lastMicros;
  lastMicros = nowUs;
  if (dtUs > 50000UL) dtUs = 50000UL;

  bool trigLevel = (resetLevelNow || manualLevelNow);
  int32_t target = trigLevel ? 4095 : 0;
  int32_t rise = map(ctrl1Val, 0, 1023, 1, 350);
  int32_t fall = map(ctrl2Val, 0, 1023, 1, 350);

  if (target > currentV) {
    currentV += (rise * (int32_t)dtUs) / 1000;
    if (currentV > target) currentV = target;
  } else if (target < currentV) {
    currentV -= (fall * (int32_t)dtUs) / 1000;
    if (currentV < target) currentV = target;
  }

  uint16_t shaped = unipolarTo12(shapeUniByPot3((float)currentV / 4095.0f));
  setOutA(shaped);
  setOutB(4095U - shaped);
  digitalWrite(GATEOUT, trigLevel ? HIGH : LOW);

  if (resetTriggered && !trigLevel) (void)checkReset();
}
