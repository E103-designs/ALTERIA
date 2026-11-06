# MACRO VOICE 1.0.0 - Quick Reference

**NON TESTATO SU HARDWARE.** MODE scorre i modelli. CV IN controlla il pitch
(1 V/oct nominale). I/O e sempre un ingresso: un trigger risincronizza
l'oscillatore o pizzica Karplus. MANUAL fa lo stesso. I primi quattro modi
suonano in continuo, Karplus solo al trigger. Entrambe le uscite sono audio.

| MODE | LED1..4 | P1 | P2 | P3 | OUT1 | OUT2 |
| --- | --- | --- | --- | --- | --- | --- |
| Fold | 1000 | Asimmetria della triangolare | Profondita di fold | Miscela con il secondo stadio di fold | Triangolare ripiegata | Triangolare prima del fold |
| FM | 0100 | Rapporto modulante/portante: 0.5, 1, 1.5, 2, 2.5, 3, 4, 5 | Indice di modulazione di fase | Feedback della modulante | Portante modulata | Modulante |
| Chord | 0010 | Accordo: m6, maj6, 7sus2, 7sus4, m7, maj7, 7, m7add11 senza quinta | Quattro inversioni | Morph da seno a colore ricco | Mix di quattro note | Seno della fondamentale CV, indipendente dall'inversione |
| Wavetable | 0001 | Banca: Classic, Hollow, Glass | Scan continuo OUT1 | Scan continuo OUT2 | Scansione A | Scansione B della stessa banca |
| Karplus | 1111 | Dal corpo tonale al plettro rumoroso | Durata della risonanza | Posizione del pickup OUT2 | Corda pizzicata | Secondo pickup sulla stessa corda |

[Manuale](../../firmware/ALTERIA_MACRO_VOICE/README.md) ·
[Demo stereo WAV](../../firmware/ALTERIA_MACRO_VOICE/ALTERIA_MACRO_VOICE_1_0_0.wav) ·
[Report dei test](../../firmware/ALTERIA_MACRO_VOICE/TEST_REPORT.md). Demo da DSP, non da hardware:
0 s Fold, 4 s FM, 8 s Chord, 12 s Wavetable, 16 s Karplus.
