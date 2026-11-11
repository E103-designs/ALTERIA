# ALTERIA DRUM LAB 1.0.0 - verifica software

2026-10-27. Nano ATmega328P 16 MHz / REV5-2. Variante sperimentale completa, da ascoltare e misurare sul modulo. Nessun upload o collaudo fisico effettuato dall'assistente.

## Build AVR

Arduino AVR core 1.8.6, FQBN `arduino:avr:nano:cpu=atmega328`, warnings all.

- **18458 / 30720 byte flash (60%)**.
- **966 / 2048 byte SRAM statica (47%)**, 1082 prima dello stack.
- Nessun warning dello sketch; rimangono gli unused-parameter del `new.cpp` del core.
- Guard di compilazione: target AVR diverso da ATmega328P o clock diverso da 16 MHz rifiutato.
- Routine audio compilate O2, resto con le opzioni normali del core. Nessun float, heap o buffer WAV nel firmware.
- Audio 6400 Hz; quattro inviluppi interlacciati, ciascuno a 1600 Hz. Prodotti bounded a 16 bit e guadagno audio quantizzato per contenere il costo CPU. Non sono gli algoritmi numerici originali LXR.
- SHA-256 sketch: `fd3f506c6b6d2cbcda7af0ccba06d537417051523a8e54d3c3fb6dfeeb932826`.

## Test Host

Compilati C++11 con clang++, AddressSanitizer e UndefinedBehaviorSanitizer. Passati:

1. `tests/drum_lab_test.cpp`: 8 motori x 64 combinazioni estreme dei sei parametri x 2 canali, indipendenza, limiti DAC e ritorno allo zero. Gli estremi di high-pass possono rimuovere un kick: non si impone un livello minimo dove un filtro deve poter attenuare. Conversione DAC scomposta verificata contro la formula intera di riferimento per tutti i 255 campioni firmati ammessi. Quattro burst clap anche con pause lunghe, retrigger, choke in entrambi i versi, closed prioritario sui simultanei. Fronti CV/I/O senza falsi eventi a startup, UI, chord senza colpo/cambio motore, selezione lunga, MANUAL selettivo, pickup e rollover lampeggio ingressi. Kit: CRC, isolamento EEPROM sotto 512, ogni interruzione prima/dopo commit, rilettura fallita e fallback su corruzione della copia piu recente.
2. `tests/calibration_test.cpp`: qualificazione del boot chord, I/O ingresso/audio fermo, rilascio e centratura obbligatori, segno e limite di correzione, tutti i tre punti e conferma finale, back/cancel, ripristino default nella sola sessione, sei coefficienti persistenti, tutti i confini di scrittura interrotta, CRC/fallback, nessuna scrittura con conferma invariata, conversione audio dopo ricaricamento e ritorno al motore senza falsi colpi.

Gli stub non modellano elettronica, tempi AVR o conversione ADC reale. Non sostituiscono il test del binario compilato.

## Esecuzione del binario AVR

`tests/avr_firmware_test.cjs`, avr8js 0.21.1: eseguiti HEX/ELF compilati con 2048 byte SRAM e periferiche Timer0/1/2, ADC, GPIO, SPI, UART ed EEPROM. Nessun modello del chip DAC o dello stadio analogico.

Passati boot, silenzio iniziale, CV solo A, I/O solo B, MANUAL voce selezionata, MODE breve/lungo, chord SOUND/FX senza colpo, scrittura EEPROM asincrona con audio attivo e regioni preesistenti intatte. Stress DSP: 64 coppie di motori x 4 profili (estremi, intermedio, bypass), 25 ms per caso; parametri iniettati in SRAM a confine ISR per coprire i rami DSP senza migliaia di gesti UI. Non e un test esaustivo di ogni fase, valore e combinazione runtime.

- 71123 interrupt audio, 142248 transazioni canale DAC.
- **ISR massimo osservato 2368 / 2500 cicli disponibili**, caso closed hat / synth perc con effetti, inclusi i casi di attacco/reset.
- Frequenza interrupt coerente con il timer a 6400 Hz; nessun superamento del periodo nei casi osservati.
- Due trigger fisici simulati da **5 ms** riconosciuti separatamente anche con due motori pesanti.
- Stack massimo osservato dopo l'avvio audio: **156 byte**, distanza minima dalla SRAM statica 925 byte nel carico eseguito. Non e una prova globale dello stack massimo.
- I/O rimasto INPUT in tutti i casi. Seconda esecuzione dal boot con entrambi i pulsanti: calibrazione/audio spento e annullamento/riavvio senza falsi trigger passati.

Il margine del caso piu pesante e ridotto: 132 cicli, circa 8.25 us, rispetto al solo periodo audio. Questo massimo osservato non certifica il worst case ne include possibili modifiche future o librerie aggiunte. Nessun supporto a librerie che occupino Timer1, SPI o aggiungano carichi interrupt significativi. Misurare su PCB periodo/jitter, reattivita e uso intensivo FX prima di considerare la variante pronta per uso live.

## Riproduzione

Dalla cartella del firmware:

```sh
clang++ -std=c++11 -fsanitize=address,undefined -Itests/stubs tests/drum_lab_test.cpp -o /tmp/drum_lab_test
/tmp/drum_lab_test
clang++ -std=c++11 -fsanitize=address,undefined -Itests/stubs tests/calibration_test.cpp -o /tmp/drum_lab_cal_test
/tmp/drum_lab_cal_test
arduino-cli compile --warnings all --fqbn arduino:avr:nano:cpu=atmega328 --build-path /tmp/drum_lab_build .
AVR_NM=/percorso/avr-nm node tests/avr_firmware_test.cjs /tmp/drum_lab_build
AVR_NM=/percorso/avr-nm node tests/avr_firmware_test.cjs /tmp/drum_lab_build calibration
```

La dipendenza avr8js e opzionale, solo per i test (`tests/package.json`); non va installata nell'IDE Arduino. `AVR8JS_PATH` puo indicare una copia temporanea gia installata.

## Anteprima software

`tests/render_demo.cpp` usa lo stesso DSP con gli stub host: WAV stereo 6400 Hz / 16 bit / 28 s, senza simulare filtro analogico o DAC. 0..12 s: due colpi di ciascuno degli otto motori nell'ordine del selettore. 12..16 s kick/snare; 16..20 s kick/closed hat; 20..24 s tom/FM; 24..28 s synth perc/clap. Nei groove A a sinistra, B a destra, non un panner della singola voce. Picco PCM 15424/32767, RMS circa 1883; il livello del file non misura i volt al jack.

## Banco richiesto

Partire con ingressi audio attenuati, kit default ed FX chiusi. Verificare zero a riposo, livelli e forme d'onda OUT1/2, trigger separati e simultanei, otto motori, clap, choke hat, tutti i gesti UI e pickup. Misurare Timer/SPI/LDAC nel caso piu pesante, filtro reale, banda, rumore e stabilita con FX intermedi/aperti. Provare salvataggio e riaccensione su hardware, poi annullamento/conferma della taratura con tester. Prima di caricare altri firmware scollegare sorgenti esterne da I/O. Nessuna certificazione elettrica o somiglianza sonora LXR dedotta dai test software.
