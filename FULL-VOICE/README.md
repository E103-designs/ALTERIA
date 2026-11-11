# FULL VOICE 1.0.0

Voce di sintesi monofonica con quattro pagine OSC, FILTER, VCA ed ENV. CV IN controlla il pitch; I/O è sempre ingresso trigger. OUT1 è audio e OUT2 è un inviluppo positivo.

[Sketch Arduino](Firmware/ALTERIA_FULL_VOICE/ALTERIA_FULL_VOICE.ino) · [Manuale e controlli](Firmware/ALTERIA_FULL_VOICE/README.md) · [Test e limitazioni](Firmware/ALTERIA_FULL_VOICE/TEST_REPORT.md)

Target: **ALTERIA REV5-2, Nano ATmega328P a 16 MHz**. Aprire lo sketch dalla propria cartella con Arduino IDE; installare Arduino AVR Boards 1.8.6 e scegliere Arduino Nano / ATmega328P. SPI ed EEPROM sono incluse nel core. Per compilare dalla radice:

```sh
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328 FULL-VOICE/Firmware/ALTERIA_FULL_VOICE
```

Prima di caricare un altro firmware, scollegare eventuali driver esterni da I/O. Vedere il manuale per taratura, tensioni nominali e stato del collaudo fisico.
