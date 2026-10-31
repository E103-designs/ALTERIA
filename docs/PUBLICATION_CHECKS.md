# Publication verification — 27 October 2026

Recompiled locally with Arduino AVR core 1.8.6, FQBN `arduino:avr:nano:cpu=atmega328` and `--warnings all`. No upload to a physical module was performed.

| Sketch | Result | Flash / 30720 | Static SRAM / 2048 |
| --- | --- | --- | --- |
| ALTERIA_ALLROUNDER 2.0.2 | PASS | 29448 | 1094 |
| ALTERIA_DRUM_LAB 1.0.0 | PASS | 18458 | 966 |
| SINCRIA_Rev2_OK | PASS | 6520 | 209 |

The Arduino core reports unused `tag` parameters in `new.cpp`. SINCRIA also reports a signed/unsigned comparison at its clock step check (`currentMillis - lastStepTime >= stepInterval`). Its source is preserved unchanged; future cleanup should use a deliberate unsigned interval type and test timing/rollover.

All five existing ALTERIA host suites passed under Clang C++11 with ASan/UBSan: Allrounder mode/clock/drum tests, demo regression, Allrounder calibration, DRUM LAB engine/UI/kit tests and DRUM LAB calibration. The included shell runner reproduces those suites.

The complete Allrounder source matches its versioned archive/report hash; the truncated loose local file was not published. DRUM LAB matches its archive/report hash. SINCRIA source was copied unchanged. No new simulation results are claimed beyond the pre-existing detailed reports, and compilation/software tests do not certify analog behavior or PCB compatibility.

## Schematic publication update - 28 October 2026

The updated maintainer-supplied schematic PDF is now in [hardware/schematics](../hardware/schematics/README.md). The original bytes and title blocks were preserved; all pages were opened and rendered for readability. Native CAD and a matching manufacturing package remain pending. Firmware and its recorded software test results were not changed; this upload does not claim new bench verification.

## FULL VOICE and quick references - 31 October 2026

Added FULL VOICE 1.0.0 as **NON TESTATO SU HARDWARE / UNTESTED ON HARDWARE**,
not a stable or physically approved firmware. Source SHA-256:
`248a7a62e211e88f59981d5b991aafaff8f4942266af6ba4845ab83b3c8bfc63`.
The source is unchanged from its recorded AVR build: 14536/30720 bytes flash,
513/2048 bytes static SRAM. Its host and AVR simulation reports are included;
no upload, listening or voltage/tracking measurements are claimed.

Re-ran **all seven host suites** under Clang C++11 ASan/UBSan from the assembled
publication tree: PASS, including the original five suites plus FULL VOICE
functional/DSP/UI/patch and ACAL suites. `tools/test_host.sh` includes all three
firmware families. ALLROUNDER 2.0.2 and DRUM LAB 1.0.0 sketch hashes are unchanged.

[Quick references](quick-reference/README.md) cover the exact versions published:
ALLROUNDER 2.0.2 (mode 10 Mutant Drums), DRUM LAB 1.0.0 and experimental FULL VOICE
1.0.0. They include pots, LEDs, buttons, CV IN, I/O direction and outputs.
They do not add new hardware specifications or replace bench validation.
