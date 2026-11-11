# ALTERIA FULL VOICE 1.0.0 - test software

2026-10-31. Target ATmega328P 16 MHz / Nano / REV5-2.
Nessun upload, ascolto o misura del PCB eseguito dall'assistente.

## Identita e build

- Sketch: `ALTERIA_FULL_VOICE.ino`.
- SHA256: `248a7a62e211e88f59981d5b991aafaff8f4942266af6ba4845ab83b3c8bfc63`.
- Core Arduino AVR 1.8.6, avr-gcc 7.3.0, FQBN `arduino:avr:nano:cpu=atmega328`.
- **14536/30720 byte flash (47.3%)**, 16184 liberi.
- **513/2048 byte SRAM statica (25.0%)**, 1535 prima dello stack.
- Build `--warnings all` riuscita. Soli warning noti `unused parameter tag`
  in `new.cpp` del core, nessun warning del firmware nella build AVR.
- Nuova cartella: Allrounder 2.0.2 e 2.1.0 rimasti invariati.

## Test host, AddressSanitizer e UndefinedBehaviorSanitizer

`full_voice_test.cpp`: PASS.

- Calcolo pitch monotono su tutti i 4096 valori Q12, cinque ottave nominali,
  errore numerico inferiore a un cent rispetto al riferimento ideale.
  **Non equivale a misurare il tracking del CV reale.**
- Quattro pagine, pickup per canale, cambio sui rilasci, hold che non navigano,
  MANUAL lungo una sola nota, MODE lungo salvataggio snapshot.
- AD completo e finito, curva monotona su 32769 punti per cinque profili,
  retrigger dal livello corrente, rollover del timestamp millisecondi.
- CV non eccita note; I/O gia alto all'avvio non crea falsi eventi; gate tenuto
  non ripete; I/O sempre INPUT in tutte le pagine.
- **1536 combinazioni DSP**: quattro forme, tre pulse width, 32 combinazioni
  filtro/VCA/drive e quattro pitch. Stati filtro entro i limiti, inviluppo <=32768,
  DAC entro range calibrato, silenzio esatto con VCA/oscillatore chiusi.
- Persistenza patch, snapshot, errore di rilettura, CRC fallback, ogni confine
  pre-commit di una scrittura interrotta, isolamento delle altre aree EEPROM.

`calibration_test.cpp`: PASS.

- Boot a due pulsanti, obbligo di rilascio/centratura, tre punti, direzione,
  velocita e limiti dei pot, annulla/indietro/default di sessione.
- Persistenza sei coefficienti, errore di scrittura, interruzioni prima del
  commit, CRC fallback, nessuna scrittura sui vecchi preset, no rewrite se uguale.
- Uscita verso OSC, aggiornamento dei range audio/envelope con taratura indipendente.

Comandi dalla directory del firmware:

```sh
for t in full_voice_test calibration_test; do
  clang++ -std=c++11 -fsanitize=address,undefined -Itests/stubs \
    tests/$t.cpp -o /tmp/alteria-$t && /tmp/alteria-$t || exit 1
done
arduino-cli compile --warnings all --fqbn arduino:avr:nano:cpu=atmega328 \
  --build-path /tmp/alteria-full-voice/build .
```

## Binario AVR e periferiche, avr8js 0.21.1

Eseguiti HEX/ELF effettivi, CPU con SRAM 2 KB, Timer0/1/2, ADC, SPI, UART,
EEPROM e ingressi digitali. Non emula DAC analogico, TL074, alimentazione o jack.

Scenario audio/UI/EEPROM: PASS, **92277 interrupt audio** osservati.

- Navigazione reale con pulsanti, MANUAL corto/lungo, avvio con gate gia alto.
- 256 combinazioni: quattro forme, quattro pitch e 16 corner di PWM, cutoff,
  risonanza e drive, con ADC e parametri controllati. Pitch da ADC entro la
  tolleranza numerica del modello, gate 5 ms catturati.
- Crossfade delle forme, anche pulse/saw con tre letture interpolate.
- Rate 6400 Hz, massimo ISR osservato **2008/2500 cicli**: margine 492 cicli
  (30.75 us) rispetto al periodo 156.25 us. Non e una prova esaustiva del worst case
  ne una misura di jitter sulla scheda.
- Hold MODE/scrittura EEPROM con audio continuo e 14 gate di 5 ms durante il gesto;
  nessuna nota persa nello scenario. Pagina conservata, record committed e nessuna
  scrittura esterna all'area patch.
- Massimo stack osservato tramite SP: **151 byte**; gap minimo osservato fra
  fine dati statici e SP **1383 byte**. Non e un limite superiore garantito per
  ogni cammino, in particolare la sessione di taratura e separata.

Scenario `--cal`: PASS. Boot chord, audio spento, tre conferme via pulsante,
commit ACAL verificato, altre aree EEPROM intatte, riavvio audio con I/O INPUT
e nessuna nota spuria dal rilascio del pulsante.

Per riprodurre dopo la build, installazione del solo simulatore nei test:

```sh
npm install --prefix tests --ignore-scripts --no-audit --no-fund
export AVR_NM="$HOME/Library/Arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7/bin/avr-nm"
node tests/avr_firmware_test.cjs /tmp/alteria-full-voice/build
node tests/avr_firmware_test.cjs /tmp/alteria-full-voice/build --cal
```

Un prototipo iniziale superava il budget ISR (2771/2500 cicli) ed e stato
scartato. La versione consegnata usa indirizzi delle tabelle precalcolati,
controlli interlacciati e aritmetica semplificata, senza cambiare il driver
GPIO/SPI.transfer/LDAC della famiglia funzionante.

## Preview e verifiche fisiche mancanti

`tests/render_voice.cpp` produce `output/audio/ALTERIA_FULL_VOICE_1_0_0.wav`:
6.4 s mono PCM16 a 6400 Hz, sine/triangle/saw/pulse in quattro sezioni.
Picco 12112/32767, RMS circa 3703. Solo DSP software, non prova del PCB.

Restano da verificare: ascolto, rumore/aliasing, banda e immagini di campionamento,
livelli reali audio/CV sotto carico, ADC/Vref/clock e tracking V/oct, jitter/overrun
reali, minima durata trigger, comportamento elettrico durante brown-out e
sicurezza/range degli ingressi. ACAL corregge solo le uscite e non certifica il pitch.
