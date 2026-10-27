# ALTERIA_ALLROUNDER 2.0.2

2026-10-27. Firmware completo per **Arduino Nano ATmega328P / PCB REV5-2**, derivato dal demo con taratura nascosta. Variante separata: i vecchi firmware non sono stati sovrascritti. Si usa **un modo alla volta**, non piu engine simultanei.

## Installazione

Aprire `ALTERIA_ALLROUNDER.ino` con Arduino IDE, scheda Arduino Nano, processore ATmega328P (Old Bootloader soltanto se necessario al caricamento). Solo SPI/EEPROM del core AVR; nessuna libreria audio esterna. Usare il file completo o importare lo ZIP, non soltanto un'anteprima parziale.

Taratura ACAL EEPROM 256..301 e preset 128..189 mantengono formato e posizioni della variante CAL: i valori gia salvati vengono riutilizzati. Nessuna scrittura o cancellazione automatica dei valori validi al boot. Non e stato eseguito upload sul modulo dall'assistente.

## Navigazione e tensioni

MODE breve al rilascio: 1..16 poi 1. LED in ordine **1,2,3,4**, pesi **1,2,4,8**; `1` acceso. Nel modo 16 indicano il preset one-hot, fisso in play e lampeggiante in edit. MODE lungo 650 ms, solo nel modo 16, alterna edit/play. Taratura nascosta solo MODE+MANUAL al boot; istruzioni separate sotto.

| Modi | OUT1 / OUT2 | Jack I/O |
| --- | --- | --- |
| 1..9 e 11..15 | CV/gate positivi 0..+5 V | Digitale 0/+5 V, tranne modo 3 ingresso B |
| 10 Mutant Drums | **Audio bipolare circa +/-2.5 V**, centrato allo zero calibrato | **Ingresso trigger snare** |
| 16 Preset | CV -5..+5 V | Gate memorizzato 0/+5 V |
| Taratura nascosta | Punti zero, +5 e -5 V | Ingresso, generatori sospesi |

Scollegare sorgenti esterne dall'I/O prima di uscire dai modi **3 o 10**, o prima di riavviare, per evitare contesa quando D2 torna OUTPUT. Il codice non puo proteggere elettricamente due driver collegati insieme. Per Drums collegare le uscite a ingressi audio/VCA/mixer adeguati, non direttamente a cuffie o altoparlanti. Partire con livello attenuato. La banda/filtraggio reale dello stadio analogico e il jitter vanno verificati al banco.

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
| 10 | 0101 | Mutant Drums | Kick pitch/body | Snare tone/noise | Drive kick + code graduali | **Trigger kick** | Trigger entrambi | **Audio kick / audio snare** | **Ingresso trigger snare** |
| 11 | 1101 | Logic XOR | Periodo A | Periodo B | Sfasamento B | Risincronizza | Risincronizza | Clock A / clock B | XOR A/B |
| 12 | 0011 | Turing Machine | Mutazione/lock | Lunghezza 1..16 | Quantizzazione livelli | Avanza registro | Avanza registro | CV / inversa | Bit registro |
| 13 | 1011 | Humanizer | Jitter 0..25% | Swing 0..33% | Variabilita gate | Evento clock | Evento clock | Trigger ritardato A / B | Gate variabile |
| 14 | 0111 | Bernoulli 3 | Bilanciamento A/B | Probabilita I/O | Memoria percorso | Evento da smistare | Evento da smistare | Route A / route B | Route C |
| 15 | 1111 | Trance Gate | 24 pattern | Gate length | Accent depth | Clock: 1 step | Avanza 1 step | Gate / gate accentato | Trigger accento |
| 16 | Preset 1..4 | Preset Manager | Tensione A in edit | Tensione B in edit | Gate in edit | Prossimo preset, solo in play | Prossimo preset | CV A / CV B salvate | Gate salvato |

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

## 10 - Mutant Drums

Sintesi digitale interna, due voci indipendenti, senza campioni WAV:

- **CV IN -> kick -> OUT1**: seno con pitch-drop rapido, decadimento e distorsione/FM di fase crescente.
- **I/O ingresso -> snare -> OUT2**: due risonanze con corpo corto e cordiera sintetica filtrata, con coda separata piu lunga. Nessuna distorsione/decimazione aggiunta da P3 allo snare.
- MANUAL suona entrambi per provare i suoni; un trigger non azzera l'altra voce. Un nuovo trigger della stessa voce la riavvia.
- P1: kick da basso/rotondo ad alto/pungente; frequenza finale circa 45..135 Hz.
- P2: snare da piu corposa/scura a piu rumorosa/brillante. Le risonanze salgono da circa 185/330 Hz a 255/440 Hz; anche tutto aperto resta una componente di corpo.
- P3: **Mutation progressiva**. Allunga le code di entrambe le voci e aumenta gradualmente FM di fase/saturazione del kick. Sullo snare cambia soltanto la durata della cordiera, non l'intonazione o la distorsione. Eliminati sample-hold e cambi di drive/durata a soglia. A P3 zero il kick mantiene la sintesi della 2.0.1.

**Punto di partenza:** P1 a un quarto, P2 al centro e P3 chiuso. Ascoltare separatamente CV IN/kick e I/O/snare, poi aumentare P3 lentamente su tutta la corsa. LED modo 10: LED2 e LED4 accesi, LED1 e LED3 spenti. La progressione resta quantizzata a 256 valori interni: non e un morph analogico continuo.

Motore Timer1 a **6.4 kHz**, sine table 8 bit su DAC 12 bit: carattere lo-fi deliberato, non audio hi-fi. Uscite nominalmente limitate a circa +/-2.5 V, con silenzio sullo zero calibrato; parametri generali aggiornati a circa 12 ms. Timer1 e SPI/DAC sono usati esclusivamente dall'ISR audio in questo modo, fermata prima di tornare a qualsiasi modo CV. Timer0/millis non viene riconfigurato. Non sono supportate librerie aggiunte che usino Timer1 o condividano SPI.

**Precauzione:** I/O cambia direzione come nel modo 3. Scollegare il trigger esterno prima di passare ad altri modi o riavviare. Non applicare gate oltre il range elettrico verificato degli ingressi. Per il primo collaudo usare impulsi positivi di almeno 5 ms; impulsi molto brevi possono sfuggire al polling. La riuscita della compilazione e la simulazione della sintesi non dimostrano banda, filtraggio, latenza e assenza di overrun sul circuito reale: collaudare con oscilloscopio e ascolto a basso volume.

## 13 - Humanizer

P1 aggiunge ritardi casuali positivi al canale A, fino al 25% del periodo stimato; non puo anticipare un evento gia ricevuto. P2 introduce swing sui colpi alterni di B, fino al 33%, con un piccolo contributo correlato. P3 varia la lunghezza del gate I/O; OUT1/OUT2 restano trigger. CV e MANUAL alimentano lo stesso flusso di eventi. Per stimare il periodo servono due eventi, prima si usa un fallback. Non continua da solo dopo lo stop del clock.

## Diagnostica silenzio Drums - revisione 2.0.1

L'utente ha riferito trigger e LED funzionanti, MODE ancora utilizzabile, ma OUT1/OUT2 piatti all'oscilloscopio. Questo **non identifica ancora la causa fisica**. Il firmware 2.0.0 compilato, eseguito con periferiche ATmega328P simulate, produce correttamente campioni SPI; quindi non si dichiara un guasto Timer/SPI riprodotto sulla scheda reale.

La 2.0.1 sostituisce le scritture dirette ai registri GPIO/SPI con lo stesso percorso digitalWrite/SPI.transfer del driver CV funzionante, amplia il LOW di LDAC ad almeno 1 us piu overhead GPIO e riduce il sample rate a 6.4 kHz per lasciare margine CPU al percorso conservativo. Il timbro rimane lo-fi; i periodi d'inviluppo si allungano leggermente rispetto a 8 kHz. Taratura, preset e trigger non cambiano.

**Aggiornamento 2026-10-27:** l'utente conferma che la 2.0.1 ora produce suono sul modulo, con kick soddisfacente. Non e stata isolata retroattivamente la causa del precedente silenzio. La 2.0.2 conserva esattamente driver DAC, sample rate, trigger e diagnostica di quella versione; interviene solo sulla sintesi Drums. L'ascolto del nuovo snare resta da confermare sul modulo. Copia della versione funzionante: `output/firmware/ALTERIA_ALLROUNDER_2.0.1.zip` nella radice progetto.

**Prova richiesta:** caricare il file completo, aprire Monitor Seriale 115200 (l'apertura puo resettare il Nano), riselezionare modo 10 con LED 2 e 4 accesi, P3 tutto chiuso, premere MANUAL. Inviare una riga come:

```text
DRUM Hz=6400 K=1 S=1 peakA=900 peakB=700
```

- Hz deve essere vicino a 6400: e la frequenza dei frame completati dal motore, non una misura dello strumento.
- K/S contano i colpi effettivamente consumati dall'ISR. MANUAL aumenta entrambi; CV IN solo K; I/O solo S.
- peakA/peakB sono il massimo scostamento del **codice DAC inviato** dallo zero dalla selezione del modo: devono diventare maggiori di zero. Non sono volt misurati al jack e non provano che il DAC li abbia ricevuti.
- Hz=0: motore audio/frame non in esecuzione. Hz normale e K/S=0: eventi non arrivati all'ISR. Hz, K/S e peak validi ma uscite piatte: serve verificare SPI/CS/LDAC e catena analogica sul PCB. Non e corretto attribuirlo automaticamente al trigger.

Il log e attivo soltanto nel modo Drums, disattivato all'uscita. Per escluderlo si puo porre ALTERIA_DRUM_DIAGNOSTICS a 0 e ricompilare. La revisione precedente e conservata come ZIP 2.0.0, ma non e stata dichiarata corretta elettricamente da questa diagnosi.

## Preset e taratura

Modo 16: MANUAL seleziona, MODE lungo 650 ms alterna edit/play, P1/P2 con pickup impostano CV e P3 il gate. Confermare l'uscita dall'editing salva i quattro preset. MODE breve dal 16 torna al modo 1 e salva se necessario. Preset e taratura hanno copie EEPROM distinte con CRC/commit.

Per la procedura nascosta completa leggere [GUIDA_CALIBRAZIONE.md](GUIDA_CALIBRAZIONE.md). MODE+MANUAL al boot, LED1 zero / LED2 +5 / LED3 -5, P1/P2 regolazione a passi con centro fermo; MANUAL conferma, MODE lungo annulla, ultimo LED4 lampeggiante richiede MANUAL 1 s per salvare. Il formato e compatibile con la variante CAL 1.2.0. Non sono state applicate automaticamente le stime delle vecchie misure.

## Verifica

Vedere `TEST_REPORT.md`. Compilazione Arduino AVR e test host ASan/UBSan, incluse le regressioni della taratura e dei preset. Non eseguito caricamento fisico, misura del tempo ISR sul Nano o collaudo audio/elettrico. I test non garantiscono precisione 1 V/oct ne caratteristiche elettriche del PCB.
