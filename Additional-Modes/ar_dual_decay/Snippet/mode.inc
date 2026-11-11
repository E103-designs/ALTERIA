Vactrol vA={0,0,0,false,150,0}, vB={0,0,0,false,250,0};
// MODE 3 has TWO independent gate inputs. No end-of-both output.
// Gate A comes from CV IN (A2); gate B from I/O (D2).
// Manual button fires both envelopes simultaneously for auditioning.
// I/O must be verified electrically as a bidirectional, protected jack.





static void scanIOGate() {
  ioRising = false;
  bool newGate = (digitalRead(PIN_OUT3) == HIGH);
  ioRising = (!ioGate && newGate);
  ioGate = newGate;
}

static uint16_t vactrolOutput(const Vactrol &v) {
  uint16_t f=q8to12(v.fastQ8);
  uint16_t s=q8to12(v.slowQ8);
  uint16_t slowMix=(uint16_t)(((uint32_t)v.character*570UL)/1023UL); // max ~56% slow component
  return (uint16_t)(((uint32_t)f*(1023U-slowMix)+(uint32_t)s*slowMix)/1023UL);
}

static void strikeVactrol(Vactrol &v,uint32_t nowUs,uint16_t decayMs,uint16_t character) {
  v.decayMs=decayMs;
  v.character=character;
  v.strikeStartedUs=nowUs;
  v.striking=true;
  // Intentionally DO NOT zero fastQ8 / slowQ8: previous hits have memory.
}

static void advanceVactrol(Vactrol &v,uint32_t nowUs,uint16_t dtMs) {
  uint16_t strikeMs=(uint16_t)(3UL+((uint32_t)v.character*15UL)/1023UL);
  if (v.striking && elapsedUs(nowUs,v.strikeStartedUs,(uint32_t)strikeMs*1000UL))
    v.striking=false;

  uint16_t target=v.striking?4095:0;
  uint16_t tauFast, tauSlow;
  if (v.striking) {
    tauFast=(uint16_t)(1UL+((uint32_t)v.character*11UL)/1023UL);
    tauSlow=(uint16_t)(3UL+((uint32_t)v.character*19UL)/1023UL);
  } else {
    tauFast=v.decayMs;
    tauSlow=(uint16_t)minU16(6000U, (uint16_t)((uint32_t)v.decayMs * (1100UL+(uint32_t)v.character*1400UL/1023UL)/1000UL));
  }
  onePoleQ8(v.fastQ8,target,tauFast,dtMs);
  onePoleQ8(v.slowQ8,target,tauSlow,dtMs);
}

static void serviceDualDecay(uint32_t nowUs, bool triggerA, bool triggerB, uint16_t dtMs) {
  // The character pot is shared, but each channel retains its own decay,
  // state, input trigger and retrigger memory.
  uint16_t c=pot3;
  uint16_t cB=(uint16_t)minU16(1023,(uint16_t)((uint32_t)c*9UL/10UL+65UL));
  if (triggerA) strikeVactrol(vA,nowUs,(uint16_t)timeFromPotMs(pot1,7,1400),c);
  if (triggerB) strikeVactrol(vB,nowUs,(uint16_t)timeFromPotMs(pot2,7,1400),cB);
  advanceVactrol(vA,nowUs,dtMs);
  advanceVactrol(vB,nowUs,dtMs);
  uint16_t levelA=vactrolOutput(vA);
  uint16_t levelB=vactrolOutput(vB);
  out1(levelA);
  out2(levelB);
  // Important: DO NOT digitalWrite(PIN_OUT3) here. D2 is an INPUT in MODE 3.
}

// -------- MODE 4 DUCKING / CV FOLLOWER ------------------------------
