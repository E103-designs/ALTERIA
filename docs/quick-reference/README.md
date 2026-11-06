# ALTERIA - Quick Reference

2026-11-06. Arduino Nano ATmega328P / PCB REV5-2. Quattro famiglie di firmware separate:
caricarne uno alla volta. `1` = LED acceso; sequenze sempre nell'ordine fisico
LED1, LED2, LED3, LED4, non lette come numeri da sinistra.

| Firmware | Versione di questa guida | Riferimento rapido |
| --- | --- | --- |
| ALLROUNDER | 2.2.1 **NON TESTATO**: ORBIT, Turing e otto preset; confronto con 2.0.2 e 2.1.0 | [16 modi, controlli e differenze fra versioni](ALLROUNDER.md) |
| DRUMLAB | 1.0.0, due voci / otto motori | [SOUND, FX, tasti e uscite](DRUMLAB.md) |
| FULLVOICE | 1.0.0, **NON TESTATO SU HARDWARE / UNTESTED** | [Quattro pagine e inviluppo AD](FULLVOICE.md) |
| MACRO VOICE | 1.0.0, **NON TESTATO SU HARDWARE / UNTESTED** | [Cinque modelli, pot e demo stereo](MACRO_VOICE.md) |

Le guide non certificano voltaggi/protezioni o tracking V/oct. Valori delle
uscite = target software da verificare sulla propria scheda. Prima di cambiare
firmware scollegare i driver esterni dal jack I/O. Per audio usare mixer/VCA con
attenuazione adeguata, non cuffie/altoparlanti direttamente.

Manuali completi: [ALLROUNDER 2.2.1](../../firmware/ALTERIA_ALLROUNDER_2_2_1/README.md),
[ALLROUNDER 2.0.2](../../firmware/ALTERIA_ALLROUNDER/README.md),
[ALLROUNDER 2.1.0 NON TESTATO](../../firmware/ALTERIA_ALLROUNDER_2_1_0/README.md),
[DRUM LAB](../../firmware/ALTERIA_DRUM_LAB/README.md),
[FULL VOICE](../../firmware/ALTERIA_FULL_VOICE/README.md),
[MACRO VOICE](../../firmware/ALTERIA_MACRO_VOICE/README.md).
