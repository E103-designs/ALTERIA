uint16_t turingRegister=0xACE1;
static uint8_t turingFlipPct(uint16_t pot){
  uint16_t edge=pot<512?pot:1023-pot;
  uint8_t slip=edge>=500?50:mapQuadraticTo(2*edge+1,0,50);
  return pot<512?100-slip:slip;
}
static uint16_t turingTopBit(){
  uint8_t length=1+map(ctrl2Val,0,1023,0,15);
  return (uint16_t)1<<(length-1);
}
static void outputTuring(uint16_t w,uint16_t mask){
  uint16_t v=(uint32_t)w*4095UL/mask;
  dacSetA(quantizeByPot(v,ctrl3Val));
  dacSetB((w&1U)?4095:0);
}
static void modeTuring(unsigned long t) {

  if (modeJustChanged) {
    turingRegister = 0xACE1;
    dacSetA(0);
    dacSetB(0);
    setGate(false);
  }

  if (checkReset()) {
    uint16_t top=turingTopBit(),mask=(top-1)|top;
    uint16_t w=turingRegister&mask;
    // Inverted recirculation complements all N bits after N clocks.
    bool bit=(w&1U)^(random(0,100)<turingFlipPct(ctrl1Val));
    w=(w>>1)|(bit?top:0);
    turingRegister=(turingRegister&~mask)|w;
    outputTuring(w,mask);
  }

  // Latch the new CV before emitting the clock edge to an external envelope.
  latchDac();
  setGate(cvGate || manualHeld());
  (void)t;
}
