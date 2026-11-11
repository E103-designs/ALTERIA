#include <cassert>
#include <cstdio>
#include "../ALTERIA_ALLROUNDER.ino"

static void tick(uint32_t t) { clockUs = t * 1000; loop(); }
static void run(uint32_t from, uint32_t to) {
  for (uint32_t t = from; t <= to; ++t) tick(t);
}
static uint8_t outputs() {
  assert(output1Q12 == 0 || output1Q12 == 4095);
  assert(output2Q12 == 0 || output2Q12 == 4095);
  return (!!output1Q12) | ((!!output2Q12) << 1) | ((!!pins[PIN_OUT3]) << 2);
}
static void expectPattern(uint16_t a, uint16_t b, uint16_t c, uint32_t start) {
  for (unsigned step = 0; step < 16; ++step) {
    const uint32_t t = start + step * 50;
    adc[PIN_CV_INPUT] = 1023;
    tick(t);
    assert(outputs() == (((a >> step) & 1) | (((b >> step) & 1) << 1) | (((c >> step) & 1) << 2)));
    run(t + 1, t + 24);  // Held clock; all pulses must expire without advancing.
    assert(!outputs());
    adc[PIN_CV_INPUT] = 0;
    run(t + 25, t + 49);
  }
}
int main() {
  setup();
  adc[PIN_POT_1] = 190; // 3 hits: steps 1, 7, 12
  adc[PIN_POT_2] = 310; // 5 hits: steps 1, 5, 8, 11, 14
  adc[PIN_POT_3] = 490; // 8 hits: odd steps
  enterMode(MODE_EUCLIDEAN_TRIGS);
  assert(dirs[PIN_OUT3] == OUTPUT);
  run(1, 49);
  assert(!outputs());
  expectPattern(0x0841, 0x2491, 0x5555, 50);
  // P2 changes only its own lane; two complete loops align.
  adc[PIN_POT_2] = 0;
  run(850, 899);
  expectPattern(0x0841, 0, 0x5555, 900);
  // Advance once, reset while clock is still high, and check that neither
  // MANUAL nor the held CV creates an event. The next edge replays step 1.
  adc[PIN_CV_INPUT] = 1023; tick(1700); assert(outputs() == 5);
  pins[PIN_MANUAL_BUTTON] = HIGH; run(1701, 1750); assert(!outputs());
  adc[PIN_CV_INPUT] = 0; run(1751, 1770);
  adc[PIN_CV_INPUT] = 1023; tick(1771); assert(outputs() == 5);
  // Keeping MANUAL held does not cause continuous resets.
  adc[PIN_CV_INPUT] = 0; run(1772, 1800);
  adc[PIN_CV_INPUT] = 1023; tick(1801); assert(!outputs());
  // Endpoints: zero hits, one hit, all sixteen hits.
  pins[PIN_MANUAL_BUTTON] = LOW;
  adc[PIN_CV_INPUT] = 0;
  adc[PIN_POT_1] = 0; adc[PIN_POT_2] = 65; adc[PIN_POT_3] = 1023;
  enterMode(MODE_EUCLIDEAN_TRIGS);run(1802, 1850);
  expectPattern(0, 1, 0xFFFF, 1851);
  enterMode(MODE_SH_GLIDE);
  assert(!bipolarOutputs && output1Q12 == 2048 && output2Q12 == 2048 && !pins[PIN_OUT3]);
  adc[PIN_POT_1] = 1023; adc[PIN_POT_2] = 0; adc[PIN_POT_3] = 0;
  run(2651, 2700);
  adc[PIN_CV_INPUT] = 1023;tick(2701);assert(pins[PIN_OUT3]);
  run(2702, 2730);assert(!pins[PIN_OUT3] && output1Q12 == output2Q12);
  puts("PASS: 16-step Euclidean patterns, independent densities, external clock, manual reset, extremes, S&H Glide pulse and zero-glide tracking");
}
