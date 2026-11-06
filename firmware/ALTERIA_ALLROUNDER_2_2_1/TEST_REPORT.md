# ALLROUNDER 2.2.1 - software verification

2026-11-03. **UNTESTED ON HARDWARE.** No upload or electrical measurement.

## Build identity

Nano ATmega328P 16 MHz / REV5-2, Arduino AVR core 1.8.6, avr-gcc 7.3.0.
FQBN `arduino:avr:nano:cpu=atmega328`, `--warnings all`.

- Sketch SHA256: `c539ad7523042a8c67aa10d10bb06c48bb91b54503e84c1dbeff98bbc3ba0872`.
- Flash **28992/30720 bytes (94.4%)**, 1728 free.
- Static SRAM **1117/2048 bytes (54.5%)**, 931 before stack.
- Build PASS. Only existing Arduino core `new.cpp` unused `tag` warnings.
- Allrounder 2.0.2 / 2.1.0 / 2.2.0 sketches preserved unchanged.

## Host tests, Clang C++11 ASan / UBSan

Five suites PASS, actual sketch included with host adapters:

1. `allrounder_test`: original clock/tap/rollover, Acid Generator and Burst.
2. `demo_regression`: 16-mode ranges and direction, Envelope, UI, pickup,
   preset persistence, scheduler and all 24 Trance patterns; expectations
   explicitly updated for ORBIT direction, v2 record stride and preset LEDs.
3. `calibration_test`: boot chord, center acquisition, code/rate/bounds,
   three points, back/cancel, commit/readback/CRC/interruption and isolation.
4. `orbit_presets_test`: ORBIT gate ordering and timeout; 128 clocks with P3=0
   preserve the motif exactly; all four scales / 12 roots / three ranges / 15
   degree positions (2160 combinations) quantize into the selected scale;
   1000 generations/evolutions stay bounded with root cadence preserved.
   Manual short queues theme, long queues scale without a short action;
   clock-period rollover. Eight preset LED identities, old-format migration,
   every pre-commit boundary of first migration and second-bank save (102
   interruptions), CRC fallback, failed-write/readback with no automatic retry
   loop, and isolation of bytes 112..1023 including old presets/ACAL/other kits.

From this firmware directory:

```sh
for t in allrounder_test demo_regression calibration_test orbit_presets_test turing_routing_test; do
  clang++ -std=c++11 -fsanitize=address,undefined -Itests/stubs \
    tests/$t.cpp -o /tmp/alteria-$t && /tmp/alteria-$t || exit 1
done
arduino-cli compile --warnings all --fqbn arduino:avr:nano:cpu=atmega328 \
  --build-path /tmp/alteria-orbit/build .
```

5. `turing_routing_test`: 64 reference register rotations keep OUT1 unchanged,
   OUT2 matches the post-shift bit at calibrated 0/+5 V including consecutive
   high bits; I/O follows CV OR MANUAL independently. Real loop edge/hold and
   Schmitt deadband checks, manual debounce/overlap, sub-1ms dispatch and mode exit.

## Compiled AVR binary simulation

avr8js 0.21.1, actual HEX/ELF and ADC, GPIO, SPI, UART, EEPROM, Timer0/1/2.
PASS: nine real MODE presses enter ORBIT; external ADC clock high for 5 ms
advances 48 steps, gate states match the rhythm and close after stop; two
locked phrases produce identical DAC values. Real MANUAL short/long actions
queue theme/scale. Six MODE presses enter preset manager, eight MANUAL presses
cycle through all slots, long MODE enters/exits edit and commits the edited
record. Unrelated EEPROM remains erased. Timer1 audio IRQ stays disabled.

Turing is entered by two additional MODE presses after ORBIT. With locked
16-bit register and actual ADC clocks, 32 reference rotations preserve pitch
conversion while OUT2 follows the post-shift bit and I/O follows each high/low
clock independently. Held clock does not advance again. MANUAL debounce/hold
and overlap with CV are verified. DAC LDAC edges are checked to precede all
33 observed Turing gate rises (overlapped sources intentionally merge gates).

368 DAC commands observed, 193 changes A / 156 B. Maximum stack observed via
SP **211 bytes**, minimum observed static-to-stack gap **719 bytes**; not an
exhaustive worst-case guarantee or a measurement on the Nano.

```sh
npm install --prefix tests --ignore-scripts --no-audit --no-fund
AVR_NM=/path/to/avr-nm node tests/avr_firmware_test.cjs /tmp/alteria-orbit/build
```

No audio renderer is delivered: ORBIT produces **CV / gate only**. Nominal
V/oct output scaling, DAC calibration, electrical ranges, jitter, latency and
minimum trigger widths still require physical tests. Use initially positive
clock pulses >=5 ms within the measured safe input range of the board.
Disconnect any old external source from I/O before upload: mode 10 now drives
that jack as an OUTPUT. Turing routing is changed only as explicitly approved; register CV/quantization algorithm is unchanged.
