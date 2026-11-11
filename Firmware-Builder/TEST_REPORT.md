# Firmware Builder 3.1.0 · Verifica

- `npm test`: 81 controlli più verifica di corrispondenza tra i 47 snippet, gli sketch standalone, le guide, il pacchetto browser e i due header di ALLROUNDER.
- `npm run verify:avr`: 47 standalone compilati e collegati con il core Arduino AVR 1.8.6 e avr-gcc 7.3.0; sei composizioni entro budget. Altri due casi superano intenzionalmente il budget di flash o SRAM e non producono HEX.
- Arduino CLI ha compilato anche gli sketch standalone di Euclidean Trigs (10.646/640 byte flash/SRAM), S&H Glide (10.970/651) e Preset Manager a 8 step (11.896/737) aperti dalle cartelle della libreria.
- Il preset da 16 modi genera un firmware da 25.898/992 byte, distinto dal firmware dedicato ALLROUNDER (ha una propria codifica LED e non include la memoria della sessione).
- `npm run verify:host`: regressione della taratura e isolamento EEPROM con il preset di test che contiene ORBIT e Preset Manager.
- Simulazione avr8js 0.21.1: cambio modo e Timer1, rollover, direzione I/O, ACAL, Acid Generator, ORBIT, salvataggio dei preset e routing Turing. Euclidean Trigs verificato sia in composizione sia in standalone: tre uscite, pattern su 16 step, clock esterno e reset MANUAL.
- Browser: catalogo, ricerca, visualizzazione dello snippet e impaginazione verificati su desktop e a 390 px.

Dopo `npm run verify:avr` e `npm install`, dalla cartella Builder:

```sh
V=../output/builder/verification
node tests/avr-runtime.cjs "$V/orbit-presets"
node tests/avr-transitions.cjs "$V"
node tests/avr-acid.cjs "$V/allrounder-16"
node tests/avr-euclidean.cjs "$V/rh_euclidean_trigs"
node tests/avr-euclidean.cjs "$V/allrounder-16" --composed
```

Se necessario, impostare `AVR_NM` e `AVR_TOOLCHAIN`. Nessun upload o collaudo fisico eseguito.
