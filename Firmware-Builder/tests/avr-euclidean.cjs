const assert = require('node:assert/strict');
const simulate = require('./simulator.cjs');
const s = simulate(process.argv[2]);
const composed = process.argv.includes('--composed');
s.adc.channelValues[0] = 0.93;
s.adc.channelValues[1] = 1.51;
s.adc.channelValues[6] = 2.39;
s.run(70);
if (composed) for (let i = 0; i < 9; i++) s.mode();
else s.mode(); // A standalone has no next slot.
assert.equal(s.read8('slot'), composed ? 9 : 0);
assert(!s.input());
const masks = [0x0841, 0x2491, 0x5555];
const state = () => [s.values[0] === 87, s.values[1] === 87, s.gate()];
for (let step = 0; step < 16; step++) {
  s.adc.channelValues[2] = 5;
  s.run(3);
  assert.deepEqual(state(), masks.map(mask => !!(mask & (1 << step))));
  s.run(22);
  assert.deepEqual(state(), [false, false, false]);
  s.adc.channelValues[2] = 0;
  s.run(25);
}
// Move beyond step 1, then reset with CV still held high.
s.adc.channelValues[2] = 5;
s.run(3);
s.C.setPin(3, true);
s.run(30);
assert.deepEqual(state(), [false, false, false]);
s.adc.channelValues[2] = 0;
s.run(25);
s.adc.channelValues[2] = 5;
s.run(3);
assert.deepEqual(state(), [true, true, true]);
console.log('PASS: 16-step patterns, DAC A/B and D2, CV clock, MANUAL reset (' + (composed ? 'composed' : 'standalone') + ')');
