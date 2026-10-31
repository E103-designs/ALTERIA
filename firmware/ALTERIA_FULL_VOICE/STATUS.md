# FULL VOICE 1.0.0 - UNTESTED ON HARDWARE

**NON TESTATO SU HARDWARE / UNTESTED ON HARDWARE**, 31 October 2026.

Experimental standalone firmware for ALTERIA REV5-2 / Arduino Nano ATmega328P
at 16 MHz. Publication is not a stable release or hardware approval.

| Check | Status |
| --- | --- |
| Arduino AVR 1.8.6 compilation | PASS: 14536 bytes flash, 513 bytes static SRAM |
| Host functional/calibration tests with ASan/UBSan | PASS |
| Compiled AVR binary simulation, audio/UI/EEPROM and boot calibration | PASS |
| Upload and operation on a physical module | **NOT TESTED for this firmware** |
| Real audio/listening, levels, loading, noise, jitter and input protection | **NOT VERIFIED** |
| Physical 1 V/oct tracking and ADC/reference calibration | **NOT VERIFIED** |

Software tests and generated WAV previews do not validate the analog circuit.
The existing output ACAL does not calibrate pitch input. Use safe positive input
voltages verified for your own board and attenuated audio connections.
I/O remains INPUT in this sketch, but other firmware can drive it: disconnect
external gate sources before changing firmware.

See [test report](TEST_REPORT.md), [manual](README.md),
[quick reference](../../docs/quick-reference/FULLVOICE.md) and
[calibration procedure](GUIDA_CALIBRAZIONE.md).
