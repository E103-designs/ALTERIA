# ALLROUNDER 2.1.0 - UNTESTED ON HARDWARE

**NON TESTATO SU HARDWARE / UNTESTED ON HARDWARE**, 31 October 2026.

Experimental version for REV5-2 / Nano ATmega328P 16 MHz, published separately
from 2.0.2. Only mode 10 changes: Acid Voice replaces Mutant Drums. This is not
a stable/hardware-approved release. The positive user report for the older
drum audio path does not validate this new voice firmware.

| Check | Status |
| --- | --- |
| Arduino AVR 1.8.6 compilation | PASS: 30012 bytes flash, 1081 bytes static SRAM |
| Four host suites with ASan/UBSan | PASS |
| Compiled AVR binary simulation | PASS; see the recorded test report |
| Upload/audio/listening on physical module for 2.1.0 | **NOT TESTED** |
| Real output levels, input range, jitter and 1 V/oct tracking | **NOT VERIFIED for this version** |

Mode 10: CV IN pitch, I/O gate/trigger INPUT, MANUAL saw/square (not a note),
OUT1 audio about +/-2.5 V, OUT2 positive envelope 0..+5 V. Output ACAL does
not calibrate pitch input. **Disconnect the I/O gate source before leaving
modes 3/10 or rebooting: other modes drive that jack as an output.**

See [manual](README.md), [test report](TEST_REPORT.md),
[quick reference for both versions](../../docs/quick-reference/ALLROUNDER.md).
