# ACPM 1.0.0

Addressable or Clocked Preset Manager. Gestore autonomo di 1–16 step indirizzabili o sincronizzati dal clock in CV IN. OUT1 e OUT2 riproducono le CV memorizzate; I/O è sempre uscita gate.

[Sketch Arduino](Firmware/ALTERIA_ACPM/ALTERIA_ACPM.ino) · [Manuale e controlli](Firmware/ALTERIA_ACPM/README.md) · [Test e limitazioni](Firmware/ALTERIA_ACPM/TEST_REPORT.md)

Target: **ALTERIA REV5-2, Nano ATmega328P a 16 MHz**. Aprire lo sketch dalla propria cartella con Arduino IDE; installare Arduino AVR Boards 1.8.6 e scegliere Arduino Nano / ATmega328P. SPI ed EEPROM sono incluse nel core. Per compilare dalla radice:

```sh
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328 ACPM/Firmware/ALTERIA_ACPM
```

Prima di caricare un altro firmware, scollegare eventuali driver esterni da I/O. Vedere il manuale per taratura, tensioni nominali e stato del collaudo fisico.
