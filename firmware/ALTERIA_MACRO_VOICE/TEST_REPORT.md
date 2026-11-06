# ALTERIA MACRO VOICE 1.0.0 - test software

2026-11-06. **Non testato su hardware**: nessun upload, ascolto o misura
del modulo fisico. Arduino Nano ATmega328P 16 MHz / REV5-2.

## Build

- SHA256 `10366c3b79c652279fd018d0d444647c4ef84c3ada2ccef3dfb616c9b9b7a657`.
- Arduino AVR 1.8.6, avr-gcc 7.3.0, `arduino:avr:nano:cpu=atmega328`.
- Flash **22750/30720 byte (74.1%)**, SRAM statica **673/2048 byte (32.9%)**.
  Lo stack e separato; la linea Karplus usa 512 byte della SRAM statica.
- Nessun warning dal firmware; solo `unused parameter tag` in `new.cpp` del core.

## Host e binario AVR

ASan/UBSan, PASS: 2400 combinazioni di modello, CV e pot; limite codici
audio; pitch DDS monotono con errore matematico sotto 1 cent; fondamentale
separata e inversioni Chord; fallback ACAL; rollover debounce. Per Karplus:
silenzio prima del pluck, acquisizione di pitch e tap, retrigger, coda
fino a zero. Misure software con eccitazione sinusoidale sulle sei C
da C1 a C6: deviazione del pitch da -3.7 a +2.2 cent in questo test,
non una misura del tracking analogico.

AVR8js 0.21.1, PASS: 99 414 ISR, 99 414 latch, due parole DAC prima di
ogni latch, nessun periodo Timer1 perso nello scenario. Massimi ISR
osservati rispettivamente **1293 / 1366 / 1725 / 1774 / 1574 cicli** su
2000 per Fold, FM, Chord, Wavetable, Karplus. Margine minimo osservato
226 cicli; intervallo massimo fra latch 2560 cicli durante transizioni
e differenze di costo DSP. Stack massimo osservato 175 byte, distanza
minima dai dati statici 1199 byte. Pulsanti, passaggio fra cinque modi,
200 trigger da 5 ms nel carico DSP, I/O sempre input, gate alto all'avvio
e al cambio modo senza falso fronte, EEPROM intatta. Non e una prova
esaustiva di worst case e non emula il circuito analogico.

## Riproduzione

```sh
arduino-cli compile --warnings all --fqbn arduino:avr:nano:cpu=atmega328 \
  --build-path /tmp/alteria-macro-v1 .
clang++ -std=c++11 -O1 -fsanitize=address,undefined -Itests/stubs \
  tests/macro_voice_test.cpp -o /tmp/alteria-macro-test
/tmp/alteria-macro-test
npm --prefix tests install
AVR_NM=/path/to/avr-nm node tests/avr_firmware_test.cjs /tmp/alteria-macro-v1
clang++ -std=c++11 -O2 -Itests/stubs tests/render_macro.cpp -o /tmp/alteria-macro-render
/tmp/alteria-macro-render /tmp/macro-voice.wav
```

WAV stereo, OUT1 a sinistra e OUT2 a destra: 0 s Fold, 4 s FM, 8 s Chord,
12 s Wavetable, 16 s Karplus. Render dello stesso DSP, non una registrazione
del modulo. Livelli, jitter, tracking e timbro effettivo da verificare al banco.
