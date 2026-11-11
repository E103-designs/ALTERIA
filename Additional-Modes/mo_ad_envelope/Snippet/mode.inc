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
void handleModeADEnvelope(unsigned long t) {
  static bool envActive = false;
  static unsigned long envStart = 0;
  static unsigned long atkMs = 50, decMs = 150, totalMs = 200;
  static bool eocPulse = false;
  static unsigned long eocStart = 0;

  if (enteredMode == MODE_AD_ENVELOPE) {
    enteredMode = -1;
    envActive = false;
    eocPulse = false;
    setOutA(0);
    setOutB(4095);
    digitalWrite(GATEOUT, LOW);
  }

  if (checkReset()) {
    atkMs = mapLogMs(ctrl1Val, 5, 4000);
    decMs = mapLogMs(ctrl2Val, 5, 4000);
    totalMs = atkMs + decMs;
    envStart = t;
    envActive = true;
    eocPulse = false;
    digitalWrite(GATEOUT, LOW);
  }

  if (envActive) {
    unsigned long el = t - envStart;
    float x = 0.0f;
    uint16_t v = 0;

    if (el <= atkMs) {
      x = atkMs ? (float)el / (float)atkMs : 1.0f;
      v = unipolarTo12(shapeUniByPot3(x));
    } else if (el <= totalMs) {
      unsigned long de = el - atkMs;
      x = decMs ? (float)de / (float)decMs : 1.0f;
      v = unipolarTo12(1.0f - shapeUniByPot3(x));
    } else {
      envActive = false;
      setOutA(0);
      setOutB(4095);
      digitalWrite(GATEOUT, HIGH);
      eocStart = t;
      eocPulse = true;
      return;
    }

    setOutA(v);
    setOutB(4095U - v);
  }

  if (eocPulse && (t - eocStart >= DEFAULT_PULSE_MS)) {
    digitalWrite(GATEOUT, LOW);
    eocPulse = false;
  }
}
