ARPhase arPhase = AR_IDLE;
uint32_t arPhaseStarted = 0, arAttackUs = 10000UL, arReleaseUs = 100000UL;
uint16_t arValue = 0, arStartValue = 0, arReleaseStart = 4095;
uint8_t arShape = CURVE_LIN;
Pulse arEor = {false,0}; // here EOR = end of RELEASE, not end of rise

static void serviceAR(uint32_t nowUs, bool trigger) {
  if (arPhase == AR_ATTACK) {
    uint32_t elapsed = nowUs-arPhaseStarted;
    if (elapsed >= arAttackUs) { arValue = 4095; arPhase = AR_SUSTAIN; }
    else {
      uint16_t e=easeCurve(phaseFromElapsed(elapsed,arAttackUs),arShape);
      arValue = arStartValue+(uint32_t)(4095U-arStartValue)*e/4095UL;
    }
  } else if (arPhase == AR_SUSTAIN) arValue = 4095;
  else if (arPhase == AR_RELEASE) {
    uint32_t elapsed = nowUs-arPhaseStarted;
    if (elapsed >= arReleaseUs) {
      arValue=0; arPhase=AR_IDLE;
      firePulse(arEor,nowUs);
    } else {
      uint16_t e=easeCurve(phaseFromElapsed(elapsed,arReleaseUs),arShape);
      arValue=(uint32_t)arReleaseStart*(4095U-e)/4095UL;
    }
  }
  // A new rising event forces a new attack, even if another gate stays high.
  if (trigger) {
    arStartValue = arValue;
    arAttackUs = timeFromPotMs(pot1,2,5000)*1000UL;
    arReleaseUs = timeFromPotMs(pot2,2,5000)*1000UL;
    uint16_t p=pot3;
    arShape=(p<341)?CURVE_LOG : (p<683)?CURVE_LIN : CURVE_EXP;
    arPhaseStarted=nowUs;
    arPhase=AR_ATTACK;
    arEor.active=false;
  }
  // Hold -> release on BOTH sources going low, even during ATTACK.
  if (!(cvGate || manualBtn.stable) &&
      (arPhase==AR_ATTACK || arPhase==AR_SUSTAIN)) {
    arReleaseStart=arValue;
    arPhase=AR_RELEASE;
    arPhaseStarted=nowUs;
  }
  servicePulse(arEor,nowUs);
  out1(arValue);
  out2((uint16_t)(4095U-arValue));
  digitalWrite(PIN_OUT3,arEor.active?HIGH:LOW);
}

// -------- MODE 3 DUAL VACTROL DECAY --------------------------------
