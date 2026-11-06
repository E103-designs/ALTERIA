# ALTERIA_ALLROUNDER 2.2.1

> **NON TESTATO SU HARDWARE / UNTESTED ON HARDWARE.** Variante sperimentale:
> compilazione e test software passati; livelli, intonazione e tempi reali da verificare.
> [Stato dettagliato](STATUS.md) · [Quick reference 2.2.1](QUICK_REFERENCE.md).

2026-11-03. Firmware completo per **Arduino Nano ATmega328P / PCB REV5-2**, derivato dal demo con taratura nascosta. Variante separata: i vecchi firmware non sono stati sovrascritti. Si usa **un modo alla volta**, non piu engine simultanei.

## Installazione

Aprire `ALTERIA_ALLROUNDER_2_2_1.ino` con Arduino IDE, scheda Arduino Nano, processore ATmega328P (Old Bootloader soltanto se necessario al caricamento). Solo SPI/EEPROM del core AVR; nessuna libreria audio esterna. Usare il file completo o importare lo ZIP, non soltanto un'anteprima parziale.

Taratura ACAL 256..301 invariata. Nuovi otto preset in EEPROM 0..49 / 56..105; recupero dei quattro vecchi da 128..189, senza sovrascriverli. Patch FULLVOICE e kit DRUMLAB intatti. Non eseguito upload fisico.

## Navigazione e tensioni

MODE breve al rilascio: 1..16 poi 1. LED in ordine **1,2,3,4**, pesi **1,2,4,8**; `1` acceso. Nel modo 16 LED1..3 mostrano l'indice 0..7 del preset, LED4 fisso in play / lampeggiante in edit. MODE lungo 650 ms, solo nel 16, alterna edit/play. Taratura nascosta MODE+MANUAL al boot.

| Modi | OUT1 / OUT2 | Jack I/O |
| --- | --- | --- |
| 1..15 | CV/gate positivi 0..+5 V, nessuna generazione audio | Digitale 0/+5 V, tranne modo 3 ingresso B |
| 16 Preset | CV -5..+5 V | Gate memorizzato 0/+5 V |
| Taratura nascosta | Punti zero, +5 e -5 V | Ingresso, generatori sospesi |

**I/O ora e uscita anche nel modo 10 ORBIT. Scollegare il vecchio gate da I/O prima di caricare questa versione.** Ingresso soltanto in modo 3 e in taratura: scollegare i driver prima di lasciare modo 3/riavviare. Clock ORBIT su CV IN, non I/O. Nessun audio interno.

## Mappa completa

| N. | LED | Modo | P1 | P2 | P3 | CV IN | MANUAL | OUT1 / OUT2 | I/O |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 1000 | AD Shaper | Attack | Decay | 12 curve A/D | Trigger/retrigger | Trigger | AD / fine attack | Fine inviluppo |
| 2 | 0100 | AR Gate | Attack | Release | LOG/LIN/EXP | Gate sustain | Tenuto: sustain | AR / inverso | Fine release |
| 3 | 1100 | Dual Decay | Decay A | Decay B | Carattere vactrol | Trigger A | Trigger entrambi | Decay A / decay B | Ingresso trigger B |
| 4 | 0010 | Ducking | Slew entrata | Slew uscita | Depth | Livello CV continuo | Tenuto: piena scala | Duck / follower | Activity gate |
| 5 | 1010 | Burst | Spacing 8..600 ms | Numero 1..16 | Curva ramp | Avvia/retrigger burst | Avvia/retrigger burst | Ramp down / inversa | Impulsi burst |
| 6 | 0110 | Acid Generator | Densita | Escursione melodica | Slide/accenti | Avanza 1 step | **Rigenera pattern** | Pitch 1 V/oct nominale / accento | Gate note |
| 7 | 1110 | Tremolo/AutoPan | Velocita | Ampiezza | Randomizzazione | Risincronizza | Risincronizza | CV A / complementare B | Crossing/cambio target |
| 8 | 0001 | Grids XY | Pattern kick | Pattern snare | Density/fill | Clock: 1 step | Avanza 1 step | Kick / snare trigger | Hats trigger |
| 9 | 1001 | Triple Clock | Rapporto OUT1 | Rapporto OUT2 | Rapporto I/O | Clock prioritario | **Tap tempo: almeno 3 tap** | Clock A / clock B | Clock C |
| 10 | 0101 | ORBIT | Tonica C..B | Escursione | Evoluzione | Clock esterno | Breve: nuovo tema; lungo: scala | Pitch CV / accento | Gate note, uscita |
| 11 | 1101 | Logic XOR | Periodo A | Periodo B | Sfasamento B | Risincronizza | Risincronizza | Clock A / clock B | XOR A/B |
| 12 | 0011 | Turing Machine | Mutazione/lock | Lunghezza 1..16 | Quantizzazione livelli | Avanza registro | Avanza registro | CV registro / bit 0..+5 V | Copia logica CV IN oppure MANUAL tenuto |
| 13 | 1011 | Humanizer | Jitter 0..25% | Swing 0..33% | Variabilita gate | Evento clock | Evento clock | Trigger ritardato A / B | Gate variabile |
| 14 | 0111 | Bernoulli 3 | Bilanciamento A/B | Probabilita I/O | Memoria percorso | Evento da smistare | Evento da smistare | Route A / route B | Route C |
| 15 | 1111 | Trance Gate | 24 pattern | Gate length | Accent depth | Clock: 1 step | Avanza 1 step | Gate / gate accentato | Trigger accento |
| 16 | Preset 1..4 | Preset Manager | Tensione A in edit | Tensione B in edit | Gate in edit | Prossimo preset, solo in play | Prossimo preset, 1..8 | CV A / CV B salvate | Gate salvato |

AD/AR conservano tempi 2..5000 ms. Dual Decay: tau 7..1400 ms piu code/memoria. Ducking legge CV, non audio RMS. I marker Envelope durano 10 ms. Grids e un generatore a pattern ispirato alla funzione, non il firmware Mutable Instruments originale. Turing quantizza a livelli, non a una scala 1 V/oct.

## 5 - Burst

P1 regola la distanza tra gli impulsi, P2 quanti impulsi (settori uniformi 1..16), P3 il profilo della rampa log/lineare/exp. Ogni fronte CV o pressione MANUAL riavvia il burst da capo. I/O e la sequenza di trigger; OUT1 scende da +5 V a zero, OUT2 sale verso +5 V. A fine sequenza i due livelli restano 0/+5 V. Non e un generatore audio.

## 6 - Acid Generator

Adattamento nativo AVR **ispirato** a [drakh/acid-generator](https://github.com/drakh/acid-generator), non port completo dell'applicazione web e non emulazione sonora TB-303. Si riprendono la selezione di step sparsi, un pool di note, ottave, accenti e slide. Implementazione originale per AVR, senza Tone.js o lodash; vedi `ATTRIBUTION.md`.

- Pattern di 16 step, scala **frigia fissa**: intervalli 0,1,3,5,7,8,10. Root CV 0 V: accordare l'oscillatore esterno alla nota desiderata.
- P1 determina la densita massima; il numero effettivo e casuale fra circa meta e massimo. P1 a zero genera un pattern vuoto.
- P2 tutto chiuso limita le note al root; aumentando allarga il pool e il numero di ottave fino a tre registri (0..2 ottave sopra il root). Pitch massimo 34 semitoni, circa 2.83 V. Quantizzazione nominale: 12 semitoni per volt, nei limiti di risoluzione e taratura analogica.
- P3 aumenta probabilita di accento e slide. **I tre pot sono acquisiti alla generazione**, non riscrivono continuamente il pattern.
- MANUAL genera un nuovo pattern e riporta la posizione allo step 1; non lo fa avanzare. CV IN avanza di uno step a ogni fronte. Senza clock non parte una sequenza automatica.
- OUT1 mantiene il pitch anche nelle pause; I/O chiude il gate per le pause, OUT2 e l'accento 0/+5 V. Uno slide collega note adiacenti senza un nuovo fronte gate e con glide breve, fino a 120 ms. Il gate viene comunque chiuso se il clock si ferma.

Per ascoltarlo servono VCO e percorso audio esterni, eventualmente con inviluppo/filtro/VCA: OUT1 e CV, non audio. Il pitch dipende dalla taratura della scheda e dell'oscillatore esterno.

## 7 - Tremolo / AutoPan

Riprende il generatore precedente: modulazione triangolare complementare a bassa randomizzazione, deriva di velocita aumentando P3, movimento casuale smussato nell'ultimo tratto di P3. P1 e la velocita, P2 la profondita attorno al centro. I due output restano **CV positive 0..+5 V**: collegarle ai VCA/panner esterni; non processa un segnale audio in ingresso. CV/MANUAL risincronizzano fase/centro; I/O marca passaggi di semiciclo o cambi del target casuale.

## 9 - Triple Clock con tap tempo

Il clock genera impulsi autonomamente: al primo ingresso nel modo parte da **120 BPM**, poi riusa l'ultimo periodo in RAM. Non salva il tempo in EEPROM.

P1/P2/P3 controllano rispettivamente OUT1/OUT2/I/O, con gli stessi 11 rapporti: **/8, /6, /4, /3, /2, x1, x2, x3, x4, x6, x8**. I tre rapporti sono indipendenti ma condividono un unico tempo master. Le uscite sono 0/+5 V, **duty 50%**, non trigger fissi da 10 ms. Ai divisori lenti possono restare alte per secondi.

MANUAL: primo tap avvia la misura, secondo registra un intervallo, **terzo applica la media dei due intervalli**. Ulteriori tap aggiornano la media degli ultimi due. Intervalli tap accettati 80 ms..4 s; una pausa piu lunga ricomincia dal primo tap.

CV IN ha priorita. Il primo fronte aggancia la fase; dal secondo si misura il periodo esterno, 20 ms..4 s. Fronti troppo vicini vengono scartati. I divisori mantengono il conteggio dei beat, non vengono azzerati a ogni impulso esterno. Se il clock si interrompe le uscite **continuano senza pausa all'ultimo tempo**; dopo tre periodi mancanti si libera la priorita esterna e MANUAL puo impostare un nuovo tempo. Un periodo superiore a 4 s viene trattato come nuovo aggancio, non come tempo valido.

Cambiare rapporto riallinea il relativo canale al master. CV e MANUAL qui sono clock/tap, non un reset indipendente. Il clock di questo modo non gira in background mentre si usa Acid, Grids o Trance: questi modi continuano a richiedere i loro clock esterni. Tempi/jitter effettivi, soprattutto a x8 molto rapido, da misurare sul target.

## 10 - ORBIT: solo sequencer CV/gate

Nessun oscillatore, filtro o generazione audio. OUT1 pitch positivo nominale
1 V/oct, OUT2 accento 0/+5 V, I/O gate positivo. CV IN riceve il clock; senza
clock resta fermo. Per ascoltare servono VCO/inviluppo/VCA esterni, oppure un
secondo ALTERIA con FULLVOICE. Due firmware non girano insieme sullo stesso Nano.

- P1: tonica cromatica C..B, dodici settori con isteresi. Root 0 V: accordare il VCO esterno sulla nota C desiderata.
- P2: da poche note fino a tre ottave di escursione, senza rigenerare il tema.
- P3: zero blocca la frase; crescendo aumenta probabilita e numero di mutazioni, fino a quattro tentativi per frase.
- Frase 16 step A/A'/B/ritorno: piccoli intervalli favoriti, salti occasionali, pause/accenti. Primo/ultimo step sulla tonica, sempre attivi.
- Le mutazioni e le richieste di nuovo tema/scala sono applicate al prossimo step 1. Tonica/escursione sono live. P3 non garantisce una modifica a ogni frase.
- MANUAL breve al rilascio accoda un nuovo tema, non avanza il clock. MANUAL 650 ms scorre pentatonica maggiore / pentatonica minore / dorica / minore naturale, senza accodare un tema al rilascio.
- I quattro LED mostrano per un secondo la scala richiesta, uno acceso; poi tornano al modo 10 (0101). La scala entra al prossimo step 1. Iniziale: pentatonica maggiore; solo RAM.
- Pitch e aggiornato prima del gate, ritardato circa 1 ms; durata circa 60% del periodo, massimo 250 ms. Primo evento usa periodo 125 ms di fallback. Non e una copia del clock: le pause sono parte del tema. Il gate si chiude anche se il clock si ferma.
- Pitch mantiene la nota nelle pause; accento ritorna a zero su step vuoti, altrimenti e mantenuto fino allo step successivo.

Usare inizialmente clock con impulsi positivi di almeno 5 ms, entro limiti
elettrici verificati. Le tensioni sono target software: taratura DAC e VCO
necessarie per gli intervalli, nessuna precisione V/oct certificata.

## 13 - Humanizer

P1 aggiunge ritardi casuali positivi al canale A, fino al 25% del periodo stimato; non puo anticipare un evento gia ricevuto. P2 introduce swing sui colpi alterni di B, fino al 33%, con un piccolo contributo correlato. P3 varia la lunghezza del gate I/O; OUT1/OUT2 restano trigger. CV e MANUAL alimentano lo stesso flusso di eventi. Per stimare il periodo servono due eventi, prima si usa un fallback. Non continua da solo dopo lo stop del clock.

## 16 - Otto preset e taratura

Otto coppie CV -5..+5 V con gate digitale 0/+5 V, non otto patch di tutte le
modalita. MANUAL seleziona 1..8 e torna 1; CV IN avanza solo in play. MODE
650 ms alterna edit/play. P1/P2 con pickup impostano A/B, P3 gate. Uscita da
edit o dal modo salva la fotografia degli otto slot; nessun salvataggio continuo.

LED1..3 = indice del preset (0..7, pesi 1/2/4), LED4 acceso PLAY, lampeggiante
EDIT. Ordine fisico LED1,2,3,4:

| Preset | PLAY |
| --- | --- |
| 1 | 0001 |
| 2 | 1001 |
| 3 | 0101 |
| 4 | 1101 |
| 5 | 0011 |
| 6 | 1011 |
| 7 | 0111 |
| 8 | 1111 |

Nuovo formato v2, due record CRC/commit/rilettura in EEPROM 0..49 e 56..105.
Se non ci sono record v2 validi, carica i quattro vecchi da 128..189 nei primi
slot, gli altri quattro dai default. Nessuna scrittura automatica al boot: per
persistire la migrazione entrare in edit, modificare un valore e uscire.
I vecchi quattro record restano intatti per i firmware precedenti, ma le nuove
modifiche v2 non tornano automaticamente al vecchio formato.
Se manca anche il vecchio formato, otto default distribuiti nel range.
Errore di rilettura: quattro LED lampeggiano rapidamente, non considerare il
salvataggio riuscito. Rientrare in edit e uscire per riprovare. Non spegnere
durante la scrittura; doppia copia non sostituisce alimentazione corretta/brown-out.

ACAL 256..301, patch FULLVOICE 320..393 e kit DRUMLAB 512..939 non vengono
modificati. Procedura nascosta in [GUIDA_CALIBRAZIONE.md](GUIDA_CALIBRAZIONE.md).

## Verifica

Vedere `TEST_REPORT.md`. Compilazione Arduino AVR e test host ASan/UBSan, incluse le regressioni della taratura e dei preset. Non eseguito caricamento fisico, misura del tempo ISR sul Nano o collaudo audio/elettrico. I test non garantiscono precisione 1 V/oct ne caratteristiche elettriche del PCB.


## 12 - Turing Machine, nuovo routing

| Controllo / jack | Funzione |
| --- | --- |
| P1 | Mutazione / lock, invariato |
| P2 | Lunghezza registro 1..16, invariata |
| P3 | Quantizzazione a livelli, invariata: non una scala musicale |
| CV IN | Ogni fronte crescente avanza il registro |
| MANUAL | Ogni pressione avanza una volta; tenuto mantiene il gate I/O |
| OUT1 | CV positiva del registro, mantenuta fra gli step |
| OUT2 | Bit del registro a 0/+5 V, mantenuto fra gli step, non CV inversa |
| I/O | Copia logica del livello CV IN OR MANUAL tenuto, a 0/+5 V |

La CV viene caricata e latcheata sul DAC prima di alzare il gate del nuovo
step. Non e un buffer elettrico o a latenza zero: lettura ADC con isteresi e
debounce di MANUAL introducono ritardo. Se una sorgente resta alta, I/O resta
alto anche quando l'altra cambia. Una salita di ciascuna sorgente puo avanzare
il registro anche mentre l'altra e alta, senza creare un nuovo fronte I/O.

OUT2 puo restare alto per piu clock consecutivi: e utile come pattern binario
per switch, accenti o attivazioni, non garantisce un trigger per nota. I/O invece
segue ogni impulso del clock quando MANUAL e rilasciato. OUT2 parte basso
all'ingresso nel modo, poi segue il bit dal primo step. I/O e sempre uscita in
questo modo: clock da collegare a CV IN, non a I/O.
