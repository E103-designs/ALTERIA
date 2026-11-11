int32_t followQ8=0;
bool followGate=false;
static void serviceDucking(uint32_t nowUs,uint16_t dtMs) {
  (void)nowUs;
  // Manual button is an independent CV test source (5V while held).
  uint16_t target=manualBtn.stable?4095:cvLevelQ12;
  uint16_t level=q8to12(followQ8);
  uint16_t tau=(uint16_t)((target>level)?timeFromPotMs(pot1,1,300):timeFromPotMs(pot2,4,2500));
  onePoleQ8(followQ8,target,tau,dtMs);
  level=q8to12(followQ8);
  uint16_t amount=(uint16_t)((uint32_t)level*pot3/1023UL);
  uint16_t duck=(uint16_t)(4095U-amount);
  out1(duck);              // at silence: +5 V; dips toward 0 V
  out2(level);             // at silence: 0 V; follows CV
  if (!followGate && level>=820) followGate=true;
  if (followGate && level<=620) followGate=false;
  digitalWrite(PIN_OUT3,followGate?HIGH:LOW);
}
