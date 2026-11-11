static inline uint16_t shapeProgressByPot(uint16_t x, int pot) {
  if (x > 4095) x = 4095;

  if (pot < 341) {
    // Fast start, slow end. Approx log-like curve.
    uint16_t inv = (uint16_t)(4095 - x);
    uint32_t y = 4095UL - ((uint32_t)inv * (uint32_t)inv) / 4095UL;
    return (uint16_t)y;
  }

  if (pot < 682) {
    // Linear region.
    return x;
  }

  // Slow start, fast end. Approx expo-like curve.
  uint32_t y = ((uint32_t)x * (uint32_t)x) / 4095UL;
  return (uint16_t)y;
}
static inline uint16_t triangleFromPhase16(uint16_t phase) {
  uint16_t p = phase & 0x7FFF;
  uint32_t x = ((uint32_t)p * 4095UL) / 32767UL;
  if (phase & 0x8000) x = 4095UL - x;
  return (uint16_t)x;
}
static inline uint16_t clamp12(int32_t v) {
  if (v < 0) return 0;
  if (v > 4095) return 4095;
  return (uint16_t)v;
}
