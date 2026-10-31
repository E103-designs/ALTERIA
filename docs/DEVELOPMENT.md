# Build, understand and modify ALTERIA

## Source layout and baseline

- `firmware/ALTERIA_ALLROUNDER/ALTERIA_ALLROUNDER.ino`: complete **2.0.2** baseline, recovered byte-for-byte from the versioned distribution ZIP. SHA-256: `6243c5deb29e7082eb011cea992f7d42888b365861dce2ebfaccb5a795f985e9`.
- `firmware/ALTERIA_DRUM_LAB/ALTERIA_DRUM_LAB.ino`: **1.0.0**, SHA-256 `fd3f506c6b6d2cbcda7af0ccba06d537417051523a8e54d3c3fb6dfeeb932826`.
- `firmware/ALTERIA_FULL_VOICE/ALTERIA_FULL_VOICE.ino`: **1.0.0, UNTESTED ON HARDWARE**, SHA-256 `248a7a62e211e88f59981d5b991aafaff8f4942266af6ba4845ab83b3c8bfc63`. Compiled/software-tested, not physically validated. See its `STATUS.md`.
- Each firmware has its own manual, attribution, test report, host stubs and optional AVR simulator. Keep the three sketches in separate directories. [Quick references](quick-reference/README.md) identify exact versions.

The local loose Allrounder file found during publication was only a tail fragment. It was not published or used as a baseline; the complete archive matches the pre-existing build report hash. The original local files were left untouched.

## Toolchain

Arduino CLI or IDE, **Arduino AVR core 1.8.6**, Nano **ATmega328P, 16 MHz**, PCB **REV5-2**. Only built-in `SPI.h` and `EEPROM.h`; do not install an MCP48xx or audio library for these sketches.

From repository root:

```sh
arduino-cli core update-index
arduino-cli core install arduino:avr@1.8.6
arduino-cli compile --warnings all --fqbn arduino:avr:nano:cpu=atmega328 --build-path /tmp/alteria-allrounder-build firmware/ALTERIA_ALLROUNDER
arduino-cli compile --warnings all --fqbn arduino:avr:nano:cpu=atmega328 --build-path /tmp/alteria-drum-lab-build firmware/ALTERIA_DRUM_LAB
arduino-cli compile --warnings all --fqbn arduino:avr:nano:cpu=atmega328 --build-path /tmp/alteria-full-voice-build firmware/ALTERIA_FULL_VOICE
arduino-cli board list
# Replace PORT with the serial port reported above; choose one sketch:
arduino-cli upload --port PORT --fqbn arduino:avr:nano:cpu=atmega328 firmware/ALTERIA_ALLROUNDER
```

Use `cpu=atmega328old` for a Nano that requires the old bootloader. Check USB/external supply compatibility for the actual hardware. Disconnect the I/O trigger source before changing firmware. Compilation does not upload or erase EEPROM; programmer/chip-erase operations may do so.

## REV5-2 hardware mapping

| Function | Nano pin |
| --- | --- |
| I/O digital (shared jack) | D2 |
| CV IN LED / I/O LED | D3 / D5 |
| MODE | D4 |
| Mode LEDs 1–4 | D6–D9 |
| MCP4822 CS / MOSI / LDAC / SCK | D10 / D11 / **D12** / D13 |
| P1 / P2 / P3 | A0 / A1 / A6 |
| CV IN / MANUAL / I/O analog | A2 / A3 / A5 |

The output analog stage is inverting: increasing DAC code lowers jack voltage. Route physical voltages through calibration-aware conversion. D3/D5 are LEDs, not legacy PWM DAC mirrors. I/O is not a third analog DAC. The [updated schematic PDF](../hardware/schematics/README.md) supplied on 28 October 2026 is available. Check exact board conditioning and safe jack input limits against the drawing and the assembled board; the PDF upload adds no new bench measurements.

## Where to change behavior

In Allrounder, `setup()`/`loop()` own input scanning and dispatch. `enterMode()` and `serviceModeButton()` control transitions, LEDs, cleanup and I/O direction. The `serviceAD`, `serviceAR`, `serviceDualDecay`, `serviceDucking`, `modeBurst`, `modeAcid`, `serviceClock3`, `modeHumanizer` and other mode functions implement behavior. `acidGenerate()` builds the clocked melody. Update both the dispatch and documented controls when adding a mode.

`physical5toDacCode()` / `outputToDacCode()` implement output conversion; `dacWriteRaw()` / `latchDac()` drive the MCP4822. `drumAudioTick()` is the audio hot path. `drumsStart()` / `drumsStop()` manage Timer1 ownership. `loadCalibration()` and the `cal*` functions handle ACAL; preset persistence has separate CRC records. Preserve their layouts unless implementing an explicit migration.

For DRUM LAB follow `setup()`/`loop()` into the selected engine and audio tick. Each voice keeps six parameters per engine: SOUND plus FX. Voice selection changes editing, not input routing. Retain pot pickup, independent retrigger and closed/open-hat choke. Change DSP in bounded fixed-point steps; keep UI and EEPROM work outside the audio ISR.

## Resource and interface constraints

Allrounder uses about 95% of application flash. Static SRAM figures exclude stack. Avoid heap allocations and large buffers; keep constant tables in PROGMEM where appropriate. Timer1 and SPI belong to the 6.4 kHz audio engine while active. Added Servo/audio/SPI/interrupt libraries can break that ownership and timing. DRUM LAB's recorded worst observed ISR is 2368 of 2500 cycles; that is a narrow observed margin, not an exhaustive guarantee.

Use rollover-safe unsigned time differences, non-blocking services, bounded arithmetic and atomic access for shared ISR state. Stop the audio engine before restoring the CV driver. In Allrounder I/O must be INPUT in modes 3/10 and calibration, OUTPUT only where intended; in DRUM LAB it remains INPUT. Document direction changes and test them without external driver contention.

## Tests

From repository root, `sh tools/test_host.sh` runs all seven host suites.
FULL VOICE adds `tests/full_voice_test.cpp` and `tests/calibration_test.cpp`;
its AVR harness also accepts `--cal` after the build directory. These tests do
not change its **UNTESTED ON HARDWARE** status. See its `TEST_REPORT.md`.

Run the supplied existing tests from each firmware directory with Clang or GCC supporting C++11 and sanitizers:

```sh
cd firmware/ALTERIA_ALLROUNDER
clang++ -std=c++11 -fsanitize=address,undefined -Itests/stubs tests/allrounder_test.cpp -o /tmp/alteria-allrounder-test
/tmp/alteria-allrounder-test
clang++ -std=c++11 -fsanitize=address,undefined -Itests/stubs tests/demo_regression.cpp -o /tmp/alteria-demo-test
/tmp/alteria-demo-test
clang++ -std=c++11 -fsanitize=address,undefined -Itests/stubs tests/calibration_test.cpp -o /tmp/alteria-cal-test
/tmp/alteria-cal-test
```

For DRUM LAB use its directory and `tests/drum_lab_test.cpp` plus `tests/calibration_test.cpp` with the same flags. Each `tests/package.json` pins optional `avr8js`; install with `npm install --prefix tests`, then run `AVR_NM=/path/to/avr-nm node tests/avr_firmware_test.cjs BUILD_DIRECTORY`. Full commands and simulator coverage are in each `TEST_REPORT.md`. Node/avr8js are test dependencies only and are not flashed.

Verify mode transitions, retrigger, overlapping pulses, timer rollover, debounce, calibrated DAC limits, I/O direction, preset/kit CRC and interrupted EEPROM writes. Recompile AVR and record flash/static SRAM after every firmware change. On the real module check levels, idle zero, jitter, SPI/LDAC, trigger widths, loading and listening. Host stubs and AVR simulation do not model the analog stage.

## Contributions and licensing

Keep original attribution and identify modified versions. The Acid implementation is inspired by drakh/acid-generator; DRUM LAB is original AVR synthesis, not copied LXR DSP. Read the per-firmware attribution files before incorporating upstream code. Submit a focused pull request with code, control table, test evidence and remaining bench checks.
