# ALLROUNDER 2.3.0

Sedici modalità CV, inviluppo e ritmo per ALTERIA REV5-2 / Nano ATmega328P a 16 MHz. [Sketch Arduino](Firmware/ALTERIA_ALLROUNDER/ALTERIA_ALLROUNDER.ino) · [Taratura](Firmware/ALTERIA_ALLROUNDER/GUIDA_CALIBRAZIONE.md) · [Test](Firmware/ALTERIA_ALLROUNDER/TEST_REPORT.md).

Aprire lo sketch dalla propria cartella, con entrambi i file `*.h`. Arduino AVR Boards 1.8.6, scheda Nano, processore ATmega328P; scegliere Old Bootloader solo se richiesto dalla scheda. Dipendenze: SPI ed EEPROM del core. Questa versione è stata compilata e verificata in software. Collaudo fisico in sospeso.

MODE breve al rilascio avanza e torna al modo 1 dopo il 16. I LED, da sinistra a destra, mostrano l’indice 0–15 con pesi **8/4/2/1**: modo 1 `0000`, modo 16 `1111`.

OUT1/OUT2 sono positivi (0…+5 V nominali); in modo 7, MANUAL alterna anche la gamma bipolare. I/O è digitale: è ingresso solo nel modo 3 e durante la taratura. Scollegare il driver esterno da I/O prima di lasciare il modo 3 o riavviare.

## Mappa completa

| N. | LED | Modo | P1 | P2 | P3 | CV IN | MANUAL | OUT1 / OUT2 | I/O |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 0000 | AD Shaper | Attack | Decay | 12 curve A/D | Trigger/retrigger | Trigger | AD / fine attack | Fine inviluppo |
| 2 | 0001 | AR Gate | Attack | Release | LOG/LIN/EXP | Gate sustain | Tenuto: sustain | AR / inverso | Fine release |
| 3 | 0010 | Dual Decay | Decay A | Decay B | Carattere vactrol | Trigger A | Trigger entrambi | Decay A / decay B | Ingresso trigger B |
| 4 | 0011 | Ducking | Slew entrata | Slew uscita | Depth | Livello CV continuo | Tenuto: piena scala | Duck / follower | Activity gate |
| 5 | 0100 | Burst | Velocita: lento -> rapido (600..8 ms) | Numero 1..16 | Curva ramp | Avvia/retrigger burst | Avvia/retrigger burst | Ramp down / inversa | Impulsi burst |
| 6 | 0101 | Acid Generator | Densita | Escursione melodica | Slide/accenti | Avanza 1 step | **Rigenera pattern** | Pitch 1 V/oct nominale / accento | Gate note |
| 7 | 0110 | Tremolo/AutoPan | Velocita lenta -> rapida | Ampiezza | Randomizzazione | Risincronizza | **Alterna unipolare/bipolare** | CV A / complementare B | Crossing/cambio target |
| 8 | 0111 | Grids XY | Pattern kick | Pattern snare | Density/fill | Clock: 1 step | Avanza 1 step | Kick / snare trigger | Hats trigger |
| 9 | 1000 | Triple Clock | Rapporto OUT1 | Rapporto OUT2 | Rapporto I/O | Clock prioritario | **Tap tempo: almeno 3 tap** | Clock A / clock B | Clock C |
| 10 | 1001 | Euclidean Trigs | Densità OUT1 | Densità OUT2 | Densità I/O | Clock esterno | Reset allo step 1 | Trigger P1 / trigger P2 | Trigger P3 |
| 11 | 1010 | Logic XOR | Velocita A lenta -> rapida | Velocita B lenta -> rapida | Sfasamento B | Risincronizza | Risincronizza | Clock A / clock B | XOR A/B |
| 12 | 1011 | Turing Machine | Double-lock / random / lock | Lunghezza 1..16 | Quantizzazione livelli | Avanza registro | Avanza registro | CV registro / bit 0..+5 V | Copia logica CV IN oppure MANUAL tenuto |
| 13 | 1100 | Humanizer | Jitter 0..25% | Swing 0..33% | Variabilita gate | Evento clock | Evento clock | Trigger ritardato A / B | Gate variabile |
| 14 | 1101 | Bernoulli 3 | Bilanciamento A/B | Probabilita I/O | Memoria percorso | Evento da smistare | Evento da smistare | Route A / route B | Route C |
| 15 | 1110 | Trance Gate | 24 pattern | Gate length | Accent depth | Clock: 1 step | Avanza 1 step | Gate / gate accentato | Trigger accento |
| 16 | 1111 | S&H Glide | Escursione casuale | Glide OUT2 | Quantizzazione | Nuovo campione | Nuovo campione | Sample / sample con glide | Trigger di campionamento |

AD/AR conservano tempi 2..5000 ms. Dual Decay: tau 7..1400 ms piu code/memoria. Ducking legge CV, non audio RMS. I marker Envelope durano 10 ms. Grids e un generatore a pattern ispirato alla funzione, non il firmware Mutable Instruments originale. Turing quantizza a livelli, non a una scala 1 V/oct.

## Verso dei potenziometri

**CCW = valore basso o velocita lenta; CW = valore alto o velocita rapida.**
La regola riguarda il parametro indicato, non inverte indiscriminatamente
tutti gli ADC. In questa versione:

| Controllo | CCW | CW |
| --- | --- | --- |
| Burst P1 | Lento, 600 ms tra impulsi | Rapido, 8 ms |
| Tremolo P1 | LFO lento | LFO rapido |
| Logic XOR P1/P2 | Lento, periodo 2000 ms | Rapido, periodo 10 ms |
| Triple Clock P1/P2/P3 | /8 | x8 |
| Turing P1 (eccezione) | Double-lock | Lock (random al centro) |
| Attack/Decay/Release e tempi slew | Durata breve | Durata lunga |
| Ampiezza, profondita, probabilita, jitter e swing | Quantita minore | Quantita maggiore |
| Turing P3 | Nessuna quantizzazione | Quantizzazione piu marcata, meno livelli |
| Pattern, curve, tonica e bilanciamenti A/B | Prima scelta / lato A | Ultima scelta / lato B |

Le durate degli inviluppi e dello slew restano in millisecondi: un tempo
maggiore e numericamente piu alto ma una risposta piu lenta. Non sono
manopole di frequenza. Curve/pattern sono selettori, non grandezze di velocita.
Burst P1 e XOR P1/P2 mantengono il verso introdotto nella 2.2.2. Turing P1
e un'eccezione intenzionale: ripristina i due lock agli estremi, random al
centro. Gli altri pot, il routing Turing, la taratura sono conservati.

## 5 - Burst

P1 regola la velocita degli impulsi: CCW distanza 600 ms, CW 8 ms; P2 quanti impulsi (settori uniformi 1..16), P3 il profilo della rampa log/lineare/exp. Ogni fronte CV o pressione MANUAL riavvia il burst da capo. I/O e la sequenza di trigger; OUT1 scende da +5 V a zero, OUT2 sale verso +5 V. A fine sequenza i due livelli restano 0/+5 V. Non e un generatore audio.

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

Modulazione triangolare complementare a bassa randomizzazione, deriva di
velocita aumentando P3, movimento casuale smussato nell'ultimo tratto di P3.
P1 aumenta la velocita in senso orario; P2 aumenta la profondita attorno
al centro; P3 aumenta il carattere casuale.

- All'ingresso nel modo: **unipolare 0..+5 V**. Un avvio da sessione
  salvata in Tremolo ripristina invece il range che avevi salvato.
- Una pressione MANUAL: **bipolare -5..+5 V** su OUT1 e OUT2.
- Pressione successiva: unipolare; tenere premuto non ripete.
- MANUAL non azzera la fase, non genera un impulso aggiuntivo su I/O e non
  scrive EEPROM da solo. La scelta si salva soltanto con la memoria di avvio.
- CV IN continua a risincronizzare la fase, senza cambiare la polarita.
- Con P2 a zero: entrambe le uscite circa +2.5 V in unipolare, 0 V in
  bipolare. Con P2 aperto: uscite complementari, in controfase.
- I/O marca i passaggi di semiciclo o cambi del target casuale ed e sempre
  digitale 0/+5 V. I quattro LED superiori rimangono 0110.

Collegare le CV a VCA/panner esterni, verificando che accettino tensioni
negative prima di attivare il range bipolare. Non processa audio in ingresso.
La commutazione cambia offset/escursione e puo essere udibile nel circuito
controllato: non e un crossfade. Range nominali, da misurare sul modulo.

## 9 - Triple Clock con tap tempo

Il clock genera impulsi autonomamente: al primo ingresso nel modo parte da **120 BPM**, poi riusa l'ultimo periodo in RAM. Non salva il tempo in EEPROM.

P1/P2/P3 controllano rispettivamente OUT1/OUT2/I/O, con gli stessi 11 rapporti: **/8, /6, /4, /3, /2, x1, x2, x3, x4, x6, x8**. I tre rapporti sono indipendenti ma condividono un unico tempo master. Le uscite sono 0/+5 V, **duty 50%**, non trigger fissi da 10 ms. Ai divisori lenti possono restare alte per secondi.

MANUAL: primo tap avvia la misura, secondo registra un intervallo, **terzo applica la media dei due intervalli**. Ulteriori tap aggiornano la media degli ultimi due. Intervalli tap accettati 80 ms..4 s; una pausa piu lunga ricomincia dal primo tap.

CV IN ha priorita. Il primo fronte aggancia la fase; dal secondo si misura il periodo esterno, 20 ms..4 s. Fronti troppo vicini vengono scartati. I divisori mantengono il conteggio dei beat, non vengono azzerati a ogni impulso esterno. Se il clock si interrompe le uscite **continuano senza pausa all'ultimo tempo**; dopo tre periodi mancanti si libera la priorita esterna e MANUAL puo impostare un nuovo tempo. Un periodo superiore a 4 s viene trattato come nuovo aggancio, non come tempo valido.

Cambiare rapporto riallinea il relativo canale al master. CV e MANUAL qui sono clock/tap, non un reset indipendente. Il clock di questo modo non gira in background mentre si usa Acid, Grids o Trance: questi modi continuano a richiedere i loro clock esterni. Tempi/jitter effettivi, soprattutto a x8 molto rapido, da misurare sul target.

## 10 · Euclidean Trigs

Tre sequenze indipendenti di **16 step**. P1 imposta **0–16 trigger su OUT1**, P2 su **OUT2**, P3 su **I/O**. Il fronte crescente del clock in **CV IN** avanza tutte e tre. **MANUAL** spegne gli impulsi attivi e riallinea le sequenze allo step 1: il prossimo fronte CV IN esegue lo step 1. In caso di coincidenza, prevale il reset. Non ci sono clock interno, morph o rotazione regolabile.

OUT1 e OUT2 emettono trigger positivi nominali 0/+5 V tramite DAC calibrato; I/O emette il terzo trigger digitale su D2. I tre impulsi durano circa un quarto del periodo, con limite di 2–12 ms. [Standalone e sorgente](../Additional-Modes/rh_euclidean_trigs/).

## 16 · S&H Glide

P1 imposta l’escursione casuale attorno a metà scala, P2 il glide di OUT2, P3 la quantizzazione. Ogni fronte CV IN o pressione MANUAL sceglie un nuovo campione. OUT1 salta al valore, OUT2 lo raggiunge con il glide, I/O emette un impulso. OUT1 e OUT2 partono a metà scala. [Standalone e sorgente](../Additional-Modes/cv_sh_glide/).

[ORBIT](../Additional-Modes/ar_orbit/) e [Preset Manager a 8 step](../Additional-Modes/ar_presets8/) restano nella libreria e nel Builder. I banchi dei preset non vengono toccati da ALLROUNDER 2.3.0. [ACPM](../ACPM/) è il gestore dedicato fino a 16 step.

## Memoria di avvio e taratura

Per salvare la sessione, premere MODE, aggiungere MANUAL e tenere entrambi per circa un secondo. Il firmware salva il modo, la posizione dei pot con pickup, il tempo master e lo stato musicale di Acid, Turing o Tremolo. Euclidean Trigs riparte allo step 1; S&H Glide riparte da metà scala. Le sessioni precedenti restano leggibili per le modalità invariate; una vecchia sessione ORBIT o Preset Manager avvia AD Shaper.

Tenere MODE all’avvio per ignorare la sessione. Tenere MODE + MANUAL all’avvio per entrare nella [taratura ACAL](Firmware/ALTERIA_ALLROUNDER/GUIDA_CALIBRAZIONE.md). Aree EEPROM: ACAL 256–301, sessione 400–437 / 448–485.

## 12 - Turing Machine

P1 riprende il comportamento del [Turing Machine originale](https://www.musicthing.co.uk/Turing-Machine/):

| P1 | Comportamento |
| --- | --- |
| Ore 7, tutto CCW | **Double-lock**: feedback sempre invertito, ciclo nominale 2 x lunghezza P2 |
| Ore 9 | Double-loop con qualche slip |
| Ore 12 | Feedback casuale: circa 50% di probabilita di invertire il bit a ogni clock |
| Ore 3 | Loop normale con qualche slip |
| Ore 5, tutto CW | **Lock**: feedback invariato, ciclo nominale uguale a P2 |

La curva e continua fra le posizioni, con una zona di aggancio vicino a ciascun
fine corsa. Nel double-lock dopo N step il registro e il complemento di quello
iniziale; dopo 2N torna uguale. Pattern simmetrici o quantizzazione grossolana
possono far sentire una ripetizione piu breve. P2 resta 1..16, P3 quantizza a
livelli, non a una scala musicale. Il routing di ALTERIA resta invariato.

| Controllo / jack | Funzione |
| --- | --- |
| P1 | Double-lock / slip / random / slip / lock |
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
