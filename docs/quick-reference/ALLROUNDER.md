# ALLROUNDER 2.0.2 - Quick Reference

**Questa guida corrisponde al firmware 2.0.2 pubblicato: modo 10 = Mutant Drums,
non Acid Voice della variante 2.1.0.** Un solo modo alla volta.
MODE breve al rilascio avanza 1..16. LED in ordine **1,2,3,4**, pesi 1,2,4,8;
`1` acceso. Nel modo 16 mostrano il preset, non il numero del modo.

## Potenziometri

| # | LED | Modo | POT1 | POT2 | POT3 |
| --- | --- | --- | --- | --- | --- |
| 1 | 1000 | AD Shaper | Attack | Decay | 12 combinazioni curve A/D |
| 2 | 0100 | AR Gate | Attack | Release | LOG / LIN / EXP |
| 3 | 1100 | Dual Decay | Decay A | Decay B | Carattere vactrol |
| 4 | 0010 | Ducking | Slew entrata | Slew uscita | Profondita |
| 5 | 1010 | Burst | Spacing 8..600 ms | Numero 1..16 | Curva delle rampe |
| 6 | 0110 | Acid Generator | Densita | Escursione melodica | Slide / accenti |
| 7 | 1110 | Tremolo/AutoPan | Velocita | Ampiezza | Randomizzazione |
| 8 | 0001 | Grids XY | Pattern kick | Pattern snare | Density / fill |
| 9 | 1001 | Triple Clock | Rapporto OUT1 | Rapporto OUT2 | Rapporto I/O |
| 10 | 0101 | Mutant Drums | Pitch/body kick | Corpo/rumore snare | Carattere kick e durata delle code |
| 11 | 1101 | Logic XOR | Periodo A | Periodo B | Sfasamento B |
| 12 | 0011 | Turing Machine | Mutazione / lock | Lunghezza 1..16 | Quantizzazione a livelli |
| 13 | 1011 | Humanizer | Jitter 0..25% | Swing 0..33% | Variabilita del gate |
| 14 | 0111 | Bernoulli 3 | Bilanciamento A/B | Probabilita I/O | Memoria del percorso |
| 15 | 1111 | Trance Gate | 24 pattern | Lunghezza gate | Profondita accento |
| 16 | Preset | Preset Manager | Tensione A in edit | Tensione B in edit | Gate in edit |

## Ingressi e uscite

| # | CV IN | MANUAL | OUT1 | OUT2 | I/O |
| --- | --- | --- | --- | --- | --- |
| 1 | Trigger / retrigger | Trigger | AD | Fine attack | Fine inviluppo |
| 2 | Gate, mantiene sustain | Tenuto: sustain | AR | Inverso AR | Fine release |
| 3 | Trigger A | Suona entrambi | Decay A | Decay B | **Ingresso trigger B** |
| 4 | Livello CV continuo | Tenuto: piena scala | Duck CV | Follower CV | Activity gate |
| 5 | Avvia / riavvia burst | Avvia / riavvia burst | Rampa down | Rampa inversa | Impulsi burst |
| 6 | Clock: avanza uno step | **Rigenera pattern** | Pitch CV | Accento | Gate note |
| 7 | Risincronizza | Risincronizza | Modulazione CV A | Complementare B | Crossing / cambio target |
| 8 | Clock: avanza uno step | Avanza uno step | Trigger kick | Trigger snare | Trigger hats |
| 9 | Clock prioritario | **Tap tempo: almeno 3 tap** | Clock A | Clock B | Clock C |
| 10 | Trigger kick | Suona kick + snare | **Audio kick** | **Audio snare** | **Ingresso trigger snare** |
| 11 | Risincronizza | Risincronizza | Clock A | Clock B | XOR A/B |
| 12 | Avanza registro | Avanza registro | CV registro | CV inversa | Bit registro |
| 13 | Evento clock | Evento clock | Trigger jitter A | Trigger swing B | Gate variabile |
| 14 | Evento da smistare | Evento da smistare | Route A | Route B | Route C |
| 15 | Clock: avanza uno step | Avanza uno step | Gate | Gate accentato | Trigger accento |
| 16 | Prossimo preset, solo play | Prossimo preset | CV A salvata | CV B salvata | Gate salvato |

## Da ricordare

- OUT1/OUT2: modi 1..9 e 11..15 **0..+5 V**; modo 10 audio circa **+/-2.5 V**;
  modo 16 CV **-5..+5 V**. I/O e digitale, non un terzo DAC analogico.
- **I/O ingresso solo nei modi 3 e 10 e in taratura**. Scollegare la sorgente
  esterna prima di lasciare 3/10 o riavviare: gli altri modi lo pilotano in uscita.
- Clock 9: rapporti /8, /6, /4, /3, /2, x1, x2, x3, x4, x6, x8. Parte a
  120 BPM; dopo almeno tre tap usa il nuovo tempo. CV ha priorita; se manca
  continua all'ultimo tempo. Duty 50%. Non gira dietro gli altri modi.
- Acid 6 e Tremolo 7 generano **CV, non audio**. Acid: cambiare i pot e poi
  MANUAL per rigenerare; serve clock esterno per avanzare e un VCO esterno per suonare.
- Preset 16: MODE 650 ms alterna edit/play; MANUAL seleziona uno dei quattro
  preset; P1/P2 con pickup. Uscire dall'edit salva. LED one-hot del preset,
  lampeggianti in edit. MODE breve torna al modo 1.
- Taratura nascosta: MODE+MANUAL al boot, rilasciare; LED1 zero, LED2 +5 V,
  LED3 -5 V; P1/P2 correggono le due uscite con tester. Centrare prima di
  MANUAL per avanzare; LED4 lampeggiante: MANUAL 1 s salva. MODE 1.2 s annulla.

Nuovo timbro 2.0.2 non dichiarato collaudato sul modulo da questa guida.
[Manuale completo](../../firmware/ALTERIA_ALLROUNDER/README.md) ·
[Indice quick reference](README.md).
