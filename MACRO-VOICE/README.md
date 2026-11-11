# MACRO VOICE 1.0.0

Cinque modelli di oscillatore: Fold, FM, Chord, Wavetable e Karplus. CV IN controlla il pitch, I/O è sempre ingresso sync/pizzico. Due uscite audio; in Chord OUT2 mantiene la fondamentale.

[Sketch Arduino](Firmware/ALTERIA_MACRO_VOICE/ALTERIA_MACRO_VOICE.ino) · [Manuale e controlli](Firmware/ALTERIA_MACRO_VOICE/README.md) · [Test e limitazioni](Firmware/ALTERIA_MACRO_VOICE/TEST_REPORT.md)

Target: **ALTERIA REV5-2, Nano ATmega328P a 16 MHz**. Aprire lo sketch dalla propria cartella con Arduino IDE; installare Arduino AVR Boards 1.8.6 e scegliere Arduino Nano / ATmega328P. SPI ed EEPROM sono incluse nel core. Per compilare dalla radice:

```sh
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328 MACRO-VOICE/Firmware/ALTERIA_MACRO_VOICE
```

Prima di caricare un altro firmware, scollegare eventuali driver esterni da I/O. Vedere il manuale per taratura, tensioni nominali e stato del collaudo fisico.

[Ascolta il demo stereo](ALTERIA_MACRO_VOICE_1_0_0.wav): render software, non registrazione dal modulo.
