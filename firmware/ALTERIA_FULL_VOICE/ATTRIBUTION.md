# Origine e dipendenze

ALTERIA FULL VOICE 1.0.0, 2026-10-31.

- Codice dedicato al progetto ALTERIA. Driver GPIO/SPI.transfer/LDAC, punti
  DAC e procedura ACAL derivati dai firmware ALTERIA precedenti, in particolare
  Allrounder 2.1.0. Non modifica gli originali.
- Sintesi DDS, tabelle generate da somme armoniche di seno, triangolo e saw,
  pulse da differenza di saw, filtro SVF fixed-point e AD implementati per AVR.
  Non incorpora il firmware di un sintetizzatore commerciale.
- Dipendenze firmware: core Arduino AVR, SPI, EEPROM, avr-libc. Nessuna
  dipendenza Mozzi; nessuna nuova libreria da installare nell'IDE.
- Mozzi consultato per la valutazione architetturale:
  [repository ufficiale](https://github.com/sensorium/Mozzi),
  [uscite esterne e configurazione](https://sensorium.github.io/Mozzi/doc/html/group__config.html).
  La presenza di questi riferimenti non implica uso/distribuzione di codice Mozzi.
- Solo per i test su computer: avr8js 0.21.1, MIT,
  [repository](https://github.com/wokwi/avr8js). Non incluso nel firmware.

La preview WAV e generata da questo DSP: non e una registrazione o una
misura della catena analogica del modulo.
