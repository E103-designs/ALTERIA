static inline bool euclid16Hit(uint8_t step, uint8_t fills, uint8_t rotation) {
  if (fills == 0) return false;
  if (fills >= 16) return true;
  uint8_t s = (uint8_t)((step + rotation) & 0x0F);
  return (uint8_t)(((uint16_t)s * (uint16_t)fills) & 0x0F) < fills;
}

static void modeEuclideanTrigs(uint32_t now) {
  static uint8_t step;
  static bool haveClock;
  static uint32_t lastClock;
  if (modeJustChanged || manualBtn.rising) {
    step = 0;
    haveClock = false;
    schClearAll();
    resetOutputs();
    if (manualBtn.rising) return; // Reset wins if both inputs rise together.
  }
  if (cvRising) {
    const uint32_t period = haveClock ? now - lastClock : 120;
    haveClock = true;
    lastClock = now;
    const uint32_t width = pulseWidthFromClock(period);
    // Each knob sets 0..16 hits independently; include a full-width 16-hit sector.
    const uint8_t fillA = (uint32_t)ctrl1Val * 17 / 1024;
    const uint8_t fillB = (uint32_t)ctrl2Val * 17 / 1024;
    const uint8_t fillG = (uint32_t)ctrl3Val * 17 / 1024;
    if (euclid16Hit(step, fillA, 0)) schPulseA(now, 0, width, 4095);
    if (euclid16Hit(step, fillB, 0)) schPulseB(now, 0, width, 4095);
    if (euclid16Hit(step, fillG, 0)) schPulseG(now, 0, width);
    step = (step + 1) & 15;
  }
  schService(now);
}
