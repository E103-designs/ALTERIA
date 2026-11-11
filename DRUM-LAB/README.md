# DRUM LAB 1.0.0

Due voci percussive indipendenti e otto motori per voce. CV IN è il trigger A, I/O è sempre ingresso trigger B. OUT1 e OUT2 sono audio, con livello nominale circa ±2,5 V.

[Sketch Arduino](Firmware/ALTERIA_DRUM_LAB/ALTERIA_DRUM_LAB.ino) · [Manuale e controlli](Firmware/ALTERIA_DRUM_LAB/README.md) · [Test e limitazioni](Firmware/ALTERIA_DRUM_LAB/TEST_REPORT.md)

Target: **ALTERIA REV5-2, Nano ATmega328P a 16 MHz**. Aprire lo sketch dalla propria cartella con Arduino IDE; installare Arduino AVR Boards 1.8.6 e scegliere Arduino Nano / ATmega328P. SPI ed EEPROM sono incluse nel core. Per compilare dalla radice:

```sh
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328 DRUM-LAB/Firmware/ALTERIA_DRUM_LAB
```

Prima di caricare un altro firmware, scollegare eventuali driver esterni da I/O. Vedere il manuale per taratura, tensioni nominali e stato del collaudo fisico.
