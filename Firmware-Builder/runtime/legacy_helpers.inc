inline float clampf(float x, float lo, float hi) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

inline float fract1(float x) {
  x = x - floorf(x);
  if (x < 0.0f) x += 1.0f;
  return x;
}

inline float lerpf(float a, float b, float t) {
  return a + (b - a) * t;
}

inline float pot3f() {
  return (float)ctrl3Val / 1023.0f;
}

inline float rand01() {
  return (float)random(0, 10000) / 9999.0f;
}

inline uint16_t clamp12i(int32_t v) {
  if (v < 0) return 0;
  if (v > 4095) return 4095;
  return (uint16_t)v;
}

inline uint16_t unipolarTo12(float x01) {
  x01 = clampf(x01, 0.0f, 1.0f);
  return (uint16_t)(x01 * 4095.0f + 0.5f);
}

inline uint16_t bipolarTo12(float xBi) {
  xBi = clampf(xBi, -1.0f, 1.0f);
  return (uint16_t)((xBi * 0.5f + 0.5f) * 4095.0f + 0.5f);
}

inline float shapeUniByPot3(float x) {
  x = clampf(x, 0.0f, 1.0f);
  float k = pot3f();

  if (k < 0.48f) {
    float amt = (0.48f - k) / 0.48f;
    float e = 1.0f + 4.0f * amt;
    return 1.0f - powf(1.0f - x, e); // fast start
  }

  if (k > 0.52f) {
    float amt = (k - 0.52f) / 0.48f;
    float e = 1.0f + 4.0f * amt;
    return powf(x, e); // slow start
  }

  return x;
}

inline float shapeBiByPot3(float x) {
  float sign = (x < 0.0f) ? -1.0f : 1.0f;
  float y = shapeUniByPot3(fabsf(x));
  return sign * y;
}

inline float foldBiByPot3(float x) {
  float amount = pot3f();
  float drive = lerpf(1.0f, 4.0f, amount);
  x *= drive;
  while (x > 1.0f || x < -1.0f) {
    if (x > 1.0f) x = 2.0f - x;
    if (x < -1.0f) x = -2.0f - x;
  }
  return clampf(x, -1.0f, 1.0f);
}

inline uint16_t quantize12ByPot3(uint16_t v) {
  int pot = ctrl3Val;
  if (pot < 150) return v;

  uint8_t steps;
  if (pot < 340) steps = 24;
  else if (pot < 520) steps = 12;
  else if (pot < 700) steps = 8;
  else if (pot < 880) steps = 5;
  else steps = 3;

  uint32_t idx = ((uint32_t)v * (uint32_t)(steps - 1) + 2047UL) / 4095UL;
  return (uint16_t)((idx * 4095UL) / (uint32_t)(steps - 1));
}


int mapQuadratic(int value, int fromLow, int fromHigh, int toLow, int toHigh) {
  unsigned long scale = (unsigned long)(value - fromLow) * 1024UL / (unsigned long)(fromHigh - fromLow);
  return toLow + (int)((((scale * scale) / 1024UL) * (unsigned long)(toHigh - toLow)) / 1024UL);
}
unsigned long mapLogMs(int value, unsigned long toLow, unsigned long toHigh) {
  float minLog = logf((float)toLow);
  float maxLog = logf((float)toHigh);
  float s = (float)value / 1023.0f;
  float r = expf(minLog + (maxLog - minLog) * s);
  return (unsigned long)(r + 0.5f);
}
uint8_t popcount16(uint16_t x) {
  uint8_t c = 0;
  while (x) {
    c += (x & 1U);
    x >>= 1;
  }
  return c;
}
float tri01(float p) {
  p = fract1(p);
  if (p < 0.5f) return p * 2.0f;
  return 2.0f - p * 2.0f;
}
float asymTri01(float p, float warp) {
  p = fract1(p);
  warp = clampf(warp, 0.05f, 0.95f);
  if (p < warp) return p / warp;
  return 1.0f - ((p - warp) / (1.0f - warp));
}
