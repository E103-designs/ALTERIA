# ALTERIA_ALLROUNDER 2.0.2 - report software

2026-10-27. Nano ATmega328P, 16 MHz, PCB REV5-2. Sorgenti precedenti mantenuti separati. Versione 2.0.1 confermata funzionante dall'utente e archiviata prima della revisione timbrica.

## Build finale

Arduino CLI / core Arduino AVR 1.8.6, FQBN `arduino:avr:nano:cpu=atmega328`, `--warnings all`.

- Flash: **29448 / 30720 byte (95%)**.
- SRAM statica: **1094 / 2048 byte (53%)**; 954 byte prima dello stack.
- Compilazione riuscita; nessun warning nello sketch. Rimangono i noti unused-parameter nel `new.cpp` del core Arduino.
- Stack massimo e tempi ISR effettivi non misurati. Non sono stati aggiunti buffer WAV o allocazioni dinamiche al firmware.

## Test host con ASan e UBSan

Tre eseguibili riproducibili compilando con clang++ C++11 e `-fsanitize=address,undefined -Itests/stubs`:

1. `tests/allrounder_test.cpp`: tre clock con /8, x1 e x8, duty 50%, cambio tempo al terzo tap, media intervalli, priorita esterna, scarto fronti troppo vicini, continuita dopo lo stop, divisione per beat e sottrazioni a rollover micros; Acid vuoto/non vuoto, scala, escursione, pattern stabile, note/slide/accento e chiusura gate; Burst e rampe complementari; Drums due trigger indipendenti, campioni non nulli su un solo canale, ritorno a zero, mutation/retrigger e limiti DAC, uso dei coefficienti calibrati, stop audio e cambio direzione GPIO all'uscita.
2. `tests/demo_regression.cpp`: regressione dei modi mantenuti, intervalli DAC, GPIO e LED adattati alla nuova mappa, Envelope, UI MODE/MANUAL, preset, pickup, EEPROM preset e 24 pattern Trance x 16 step.
3. `tests/calibration_test.cpp`: ingresso solo con boot chord, protezione I/O, acquisizione e correzione pot, tre punti completi, MODE back/cancel, ripristino teorico in RAM, hold finale, sei coefficienti, CRC e fallback, ogni interruzione simulata prima del commit, rilettura fallita, isolamento preset, conferma invariata senza nuove scritture, ricaricamento e ritorno al modo 1.

Tutti passati. Gli stub non simulano il chip DAC, il filtro analogico, le temporizzazioni di SPI/EEPROM, la priorita interrupt AVR o una caduta di alimentazione reale. Il corpo audio e eseguito dai test come funzione per verificarne aritmetica/uscite, non per misurarne il costo sul Nano. Timer1 e configurato a 6.4 kHz sul target; vanno verificati al banco periodo, jitter e margine prima di un uso affidabile in concerto.

Estensioni 2.0.2, passate: tutti i 256 valori P3 con decadimento monotono e variazione iniziale <=1 unita di inviluppo per passo; render deterministici di 2048 campioni su entrambe le voci intorno alle vecchie soglie 64/128/192, differenza RMS relativa <10%; corpo snare che termina prima della cordiera a tre posizioni P2, risonanze indipendenti da P3 e ritorno al codice zero anche con coda massima. Confronto aggiuntivo tra sorgente archiviato 2.0.1 e sorgente corrente: **36000 codici DAC kick identici** con P3=0 e P1=0/512/1023. La continuita e quantizzata, non una garanzia psicoacustica di resa musicale.

## Collaudo fisico richiesto

Non effettuati upload, ascolto sul modulo o misure elettriche dall'assistente. Prima della patch completa: ricaricare/verificare taratura, misurare OUT a riposo, poi provare Drums con due trigger positivi indipendenti e oscilloscopio, controllare livello e ritorno a zero. Verificare ingresso/uscita I/O nelle transizioni 3/10 -> altri modi senza driver esterni collegati. Ascolto iniziale attenuato. Testare tap con 3+ pressioni, x8 e divisori lenti, perdita/ripresa del clock, e precisione melodica Acid con VCO esterno calibrato. Nessuna certificazione elettrica o 1 V/oct implicita.

## Diagnostica AVR aggiunta nella 2.0.1

Storico: l'utente riferiva nessun suono/uscite piatte all'oscilloscopio, anche con MANUAL, mentre i LED ingresso rispondevano, MODE cambiava e le altre modalita funzionavano su ATmega328P. Ora conferma suono funzionante con la 2.0.1. Causa fisica originale **non riprodotta o identificata**. Anche il binario 2.0.0 emulato produceva campioni SPI corretti; non dichiarare quindi dimostrato un guasto del timer o un impulso LDAC insufficiente sul PCB.

Revisione 2.0.1: SPI.transfer/digitalWrite come nel percorso CV, LDAC LOW esteso ad almeno 1 us piu overhead GPIO, sample rate 6.4 kHz per margine CPU, contatori diagnostici seriali attivi solo in modo 10. La 2.0.2 conserva questo driver senza modifiche e cambia soltanto la sintesi Drums; EEPROM e altri modi restano invariati.

`tests/avr_firmware_test.cjs` esegue HEX/ELF compilati con avr8js 0.21.1: CPU ATmega328P con 2048 byte SRAM, Timer0/1/2, ADC, SPI, UART, EEPROM cancellata e pulsanti/gate simulati. Passati boot, selezione con 9 pressioni MODE, MANUAL entrambi, CV solo kick, I/O solo snare, dati DAC variabili, diagnostica seriale e arresto Timer1 uscendo dal modo. Sweep con entrambe le voci, P1/P2 ai due estremi e P3 ai limiti e attorno alle vecchie soglie. Build 2.0.2: **24756 interrupt, 49731 transazioni canale, ISR massimo osservato 1966 cicli su 2500 disponibili**, report Hz 6390..6408. Il massimo osservato non e una prova esaustiva del worst case. Il simulatore non include il DAC analogico, carichi, cablaggi o fronti elettrici.

Per riprodurre: installare la dipendenza opzionale in tests/package.json (non e una dipendenza Arduino), compilare il firmware, poi eseguire `AVR_NM=/percorso/avr-nm node tests/avr_firmware_test.cjs CARTELLA_BUILD`. In alternativa AVR8JS_PATH puo indicare una copia temporanea di avr8js. Nessun upload o ascolto fisico della revisione 2.0.2 dall'assistente: nuovo snare e progressione P3 da confermare sul modulo. SHA-256 sketch: `6243c5deb29e7082eb011cea992f7d42888b365861dce2ebfaccb5a795f985e9`.
