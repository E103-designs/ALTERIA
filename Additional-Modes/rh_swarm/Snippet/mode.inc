static void modeSwarmTrigger(unsigned long t) {
  static unsigned long lastT;
  static uint16_t phaseA, phaseB, phaseG;
  static uint16_t incA, incB, incG;

  if (modeJustChanged) {
    lastT = t;
    phaseA = 0x0000;
    phaseB = 0x5555;
    phaseG = 0xAAAA;
    incA = 120;
    incB = 160;
    incG = 210;
    schClearAll();
    resetOutputs();
  }

  if (checkReset()) {
    phaseA = (uint16_t)random(0, 65536);
    phaseB = (uint16_t)random(0, 65536);
    phaseG = (uint16_t)random(0, 65536);
    schPulseG(t, 0, 6);
  }

  unsigned long dt = t - lastT;
  if (dt > 25) dt = 25;
  lastT = t;

  uint16_t base = (uint16_t)mapQuadraticTo(ctrl1Val, 8, 1800);
  uint16_t spread = (uint16_t)map(ctrl2Val, 0, 1023, 0, 1200);
  uint16_t chaos = (uint16_t)map(ctrl3Val, 0, 1023, 0, 900);

  long jitterA = chaos ? random(-(long)chaos, (long)chaos + 1L) : 0;
  long jitterB = chaos ? random(-(long)chaos, (long)chaos + 1L) : 0;
  long jitterG = chaos ? random(-(long)chaos, (long)chaos + 1L) : 0;

  long nextA = (long)base + jitterA;
  long nextB = (long)base + (long)(spread / 2U) + jitterB;
  long nextG = (long)base + (long)spread + jitterG;
  if (nextA < 1) nextA = 1;
  if (nextB < 1) nextB = 1;
  if (nextG < 1) nextG = 1;
  if (nextA > 3200) nextA = 3200;
  if (nextB > 3200) nextB = 3200;
  if (nextG > 3200) nextG = 3200;

  incA = (uint16_t)((incA * 7UL + (uint16_t)nextA) >> 3);
  incB = (uint16_t)((incB * 7UL + (uint16_t)nextB) >> 3);
  incG = (uint16_t)((incG * 7UL + (uint16_t)nextG) >> 3);

  uint16_t oldA = phaseA;
  uint16_t oldB = phaseB;
  uint16_t oldG = phaseG;
  phaseA = (uint16_t)(phaseA + (uint16_t)((uint32_t)incA * dt));
  phaseB = (uint16_t)(phaseB + (uint16_t)((uint32_t)incB * dt));
  phaseG = (uint16_t)(phaseG + (uint16_t)((uint32_t)incG * dt));

  uint8_t drop = (uint8_t)map(ctrl3Val, 0, 1023, 0, 45);
  unsigned long pw = (unsigned long)map(ctrl3Val, 0, 1023, 3, 14);

  if (phaseA < oldA && random(0, 100) >= drop) {
    schPulseA(t, 0, pw, 4095);
  }
  if (phaseB < oldB && random(0, 100) >= drop) {
    schPulseB(t, 0, pw, 4095);
  }
  if (phaseG < oldG && random(0, 100) >= drop) {
    schPulseG(t, 0, pw);
  }

  schService(t);
}
