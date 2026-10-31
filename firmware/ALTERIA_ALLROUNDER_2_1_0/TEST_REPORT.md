# ALTERIA ALLROUNDER 2.1.0 - verifica software

2026-10-31. Target Arduino Nano ATmega328P, 16 MHz, PCB REV5-2. Variante separata dalla 2.0.2, che resta invariata. Nessun upload, ascolto sul modulo o misura elettrica effettuati dall'assistente.

## Build finale

Core Arduino AVR 1.8.6, FQBN `arduino:avr:nano:cpu=atmega328`, warnings all:

- **30012 / 30720 byte flash**, 708 liberi; Arduino CLI riporta 97% (circa 97.7%).
- **1081 / 2048 byte SRAM statica (52%)**, 967 prima dello stack. Stack massimo non misurato.
- Nessun warning nello sketch; warning unused-parameter noti nel `new.cpp` del core.
- Guard di compilazione per ATmega328P a 16 MHz. Nessun heap, float, libreria DSP o buffer WAV nel firmware.
- SHA-256 sketch: `8ddf107946f6a1cc4ac3afaeb559f6f514e96a6905538352006b93c4884da9f2`.
- SHA-256 baseline 2.0.2 rimasta intatta: `6243c5deb29e7082eb011cea992f7d42888b365861dce2ebfaccb5a795f985e9`.

## Sintesi e limiti aritmetici

Motore a 6400 Hz con fase DDS a 32 bit. Saw/square da somme armoniche precalcolate, quattro bande di tabelle da 128 campioni, interpolazione lineare; passaggio forma in 32 campioni, senza trigger. Non completamente alias-free: restano quantizzazione e cambi di banda. Il filtro e un SVF a due substep per campione, non un ladder 303. Coefficienti Q8, stati low/band limitati a +/-8192, high a +/-16000: con ingresso massimo +/-3040 e damping massimo 469/256, le somme intermedie restano entro int16_t. I prodotti usano int32_t dove necessario. Risonanza alta puo saturare; non e dichiarata auto-oscillazione.

Inviluppo a picco 32768 Q15, attacco 16 campioni dal livello corrente e decadimento aggiornato ogni quattro campioni. Retrigger prossimo al picco verificato senza superamento del target 0..+5 V. OUT1 e audio limitato a circa +/-2.5 V nominali; OUT2 e inviluppo unipolare, conversioni indipendenti dai coefficienti di taratura di ciascun DAC. Nessun segnale residuo a riposo nei test.

## Quattro suite host

clang++ C++11 con AddressSanitizer e UndefinedBehaviorSanitizer, tutte passate:

1. `allrounder_test.cpp`: tre clock, divisioni/moltiplicazioni, tap, priorita esterna, free-run e rollover; Acid Generator modo 6, note/slide/accenti e chiusura gate; Burst.
2. `demo_regression.cpp`: altri modi conservati, range e GPIO, Envelope, MODE/MANUAL, preset, pickup, EEPROM e scheduler; 24 pattern Trance x 16 step. Il flag generico bipolare vale soltanto per il preset: il range misto del modo 10 usa il driver dedicato.
3. `calibration_test.cpp`: boot chord, I/O sicuro, centro dei pot, direzione e limiti, tre punti e conferma finale, cancel/back/default in RAM, sei coefficienti, rilettura fallita, tutti i confini di scrittura interrotta, CRC/fallback, isolamento preset e mancata riscrittura di taratura identica.
4. `acid_voice_test.cpp`: pitch monotono su tutti i 4096 valori Q12, cinque ottave e errore numerico ideale inferiore a 1 cent; MANUAL cambia solo forma. **750 casi** di forma/decay/risonanza/Env Mod/pitch, limiti degli stati, range DAC misti, picco OUT2 esatto e code finite. Ulteriori 256 casi di retrigger vicino al picco, startup con gate gia alto, nessun trigger dal pitch, gate tenuto senza ripetizione, calibrazione diversa sui due canali e uscita sicura dal modo audio.

L'errore ideale della funzione pitch non e la precisione dello strumento: l'ingresso reale usa ADC a 10 bit, reference e circuito non misurati. Su cinque ottave la risoluzione ideale e circa 6 cent per passo ADC, prima di rumore/deriva. L'ACAL corregge solo i DAC, non il tracking ADC.

## Binario AVR e periferiche simulate

`tests/avr_firmware_test.cjs`, avr8js 0.21.1: HEX/ELF compilati, ATmega328P con 2048 byte SRAM, Timer0/1/2, ADC, GPIO, SPI, UART e EEPROM cancellata.

Passati boot, nove pressioni MODE, avvio modo 10, silenzio senza gate, CV pitch senza trigger, MANUAL senza nota, trigger I/O da 5 ms. Verificati i parametri ricevuti dal vero percorso ADC: entrambe le forme, quattro pitch e otto angoli dei pot (64 casi), oltre a cambio forma e attacco simultanei. Pitch assestato coerente con CV entro la tolleranza numerica di test di 12 cent, non misura reale. Uscita dal modo ferma Timer1 prima del ritorno al driver CV e della commutazione I/O in OUTPUT.

- **29789 interrupt**, 59811 transazioni canale DAC.
- **Massimo ISR osservato 1894 / 2500 cicli**, margine osservato 606 cicli, circa 37.9 us a 16 MHz.
- Frequenza interrupt coerente con 6400 Hz e nessun superamento del periodo nei casi eseguiti.
- Valori DAC variabili su audio e inviluppo, corretta separazione pitch/gate/wave.

Le prime implementazioni del filtro superavano il budget ISR: non sono quelle consegnate. Il kernel Q8 e l'estrazione di fase allineata ai byte riducono i costi senza modificare il sample rate. Il massimo osservato finale non dimostra il worst case esaustivo o i tempi sul PCB; il simulatore non include DAC analogico, carichi, rumore, fronti elettrici o filtro d'uscita reale. Non aggiungere librerie che usino Timer1 o SPI senza una nuova verifica.

## Riproduzione

Dalla cartella della versione:

```sh
for t in allrounder_test demo_regression calibration_test acid_voice_test; do
  clang++ -std=c++11 -fsanitize=address,undefined -Itests/stubs tests/$t.cpp -o /tmp/$t && /tmp/$t || exit 1
done
arduino-cli compile --warnings all --fqbn arduino:avr:nano:cpu=atmega328 --build-path /tmp/allrounder-210-build .
AVR_NM=/percorso/avr-nm node tests/avr_firmware_test.cjs /tmp/allrounder-210-build
```

avr8js e una dipendenza opzionale solo dei test, non una libreria Arduino; il file tests/package.json ne fissa la versione. AVR8JS_PATH puo indicare una copia temporanea installata. Tutti i test host e il simulatore sono presenti nel progetto; lo ZIP per Arduino contiene sketch e documentazione, non questi strumenti.

## Anteprima

`tests/render_voice.cpp` genera `output/audio/ALTERIA_ALLROUNDER_2_1_0_Acid_Voice.wav`: mono, PCM16 a 6400 Hz, 7.5 s, prima saw poi square. Picco PCM 15552/32767, RMS circa 2658. E un render dello stesso DSP con gli stub, non una registrazione o simulazione analogica del modulo. Il livello del file non misura i volt delle uscite.

## Banco ancora necessario

Selezionare modo 10 prima di collegare un gate esterno a I/O. Verificare zero, livelli OUT1/OUT2, forma e fine coda; misurare frequency tracking con CV note e reference note, latenza gate, risonanza, bande armoniche e resa all'ascolto. Provare MODE/MANUAL, riavvio, taratura e preset su hardware. Scollegare sorgenti I/O prima di lasciare 3/10 o riavviare; partire con audio attenuato, mai cuffie/altoparlanti direttamente al jack. Nessuna prova software certifica limiti elettrici o precisione 1 V/oct.
