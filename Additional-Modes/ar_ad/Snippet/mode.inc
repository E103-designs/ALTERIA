ADPhase adPhase = AD_IDLE;
uint32_t adPhaseStarted = 0;
uint32_t adAttackUs = 10000UL, adDecayUs = 100000UL;
uint16_t adValue = 0, adStartValue = 0;
uint8_t adAttackShape = CURVE_LIN, adDecayShape = CURVE_LIN;
Pulse adEor = {false,0}, adEof = {false,0};

static void advanceAD(uint32_t nowUs) {
  if (adPhase == AD_ATTACK) {
    uint32_t elapsed = (uint32_t)(nowUs - adPhaseStarted);
    if (elapsed >= adAttackUs) {
      adValue = 4095;
      adPhase = AD_DECAY;
      adPhaseStarted += adAttackUs; // stable timing even if a tick is late
      firePulse(adEor, nowUs);
    } else {
      uint16_t e = easeCurve(phaseFromElapsed(elapsed,adAttackUs),adAttackShape);
      adValue = adStartValue + (uint32_t)(4095U-adStartValue)*e/4095UL;
    }
  }
  if (adPhase == AD_DECAY) {
    uint32_t elapsed = (uint32_t)(nowUs - adPhaseStarted);
    if (elapsed >= adDecayUs) {
      adValue = 0;
      adPhase = AD_IDLE;
      firePulse(adEof, nowUs);
    } else {
      uint16_t e = easeCurve(phaseFromElapsed(elapsed,adDecayUs),adDecayShape);
      adValue = 4095U-e;
    }
  }
}
static void triggerAD(uint32_t nowUs) {
  adStartValue = adValue; // retrigger smoothly from current CV
  adAttackUs = timeFromPotMs(pot1,2,5000)*1000UL;
  adDecayUs = timeFromPotMs(pot2,2,5000)*1000UL;
  adAttackShape = shapeAttack(shapeSector);
  adDecayShape  = shapeDecay(shapeSector);
  adPhaseStarted = nowUs;
  adPhase = AD_ATTACK;
  adEor.active = false;
  adEof.active = false;
}
static void serviceAD(uint32_t nowUs, bool trigger) {
  advanceAD(nowUs);
  if (trigger) triggerAD(nowUs);
  servicePulse(adEor,nowUs);
  servicePulse(adEof,nowUs);
  out1(adValue);
  out2(adEor.active ? 4095 : 0);
  digitalWrite(PIN_OUT3,adEof.active ? HIGH : LOW);
}

// -------- MODE 2 AR GATE -------------------------------------------
