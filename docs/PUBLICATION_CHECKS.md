# Firmware validation

Target: Arduino Nano, ATmega328P, 16 MHz; Arduino AVR Boards 1.8.6; PCB REV5-2.

| Firmware | Flash / 30,720 bytes | Static SRAM / 2,048 bytes | Validation |
| --- | --- | --- | --- |
| ALLROUNDER 2.0.2 | 29,448 | 1,094 | AVR build; host and AVR simulation reports |
| ALLROUNDER 2.1.0 | 30,012 | 1,081 | AVR build; host and AVR simulation reports; hardware qualification pending |
| DRUM LAB 1.0.0 | 18,458 | 966 | AVR build; host and AVR simulation reports |
| FULL VOICE 1.0.0 | 14,536 | 513 | AVR build; host and AVR simulation reports; hardware qualification pending |

The eleven host suites pass under Clang C++11 with AddressSanitizer and UndefinedBehaviorSanitizer. Reproduce them with `sh tools/test_host.sh`. Per-firmware `TEST_REPORT.md` files describe coverage, optional AVR simulation and remaining bench measurements. Static SRAM excludes runtime stack.

Arduino core `new.cpp` emits known unused-parameter warnings for `tag`. Electrical levels, analog sound, pitch tracking, loading and timing require physical measurements on the corresponding board revision.
