# ALLROUNDER 2.2.1 / 2.1.0 / 2.0.2 - Quick Reference

La tabella descrive **2.2.1, NON TESTATA SU HARDWARE / UNTESTED ON HARDWARE**.
Le versioni 2.0.2 (Mutant Drums) e 2.1.0 (Acid Voice, anch'essa non testata
su hardware) restano disponibili. Le differenze dei modi 10, 12 e 16 sono
riassunte sotto.
Un solo modo alla volta.
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
| 10 | 0101 | ORBIT | Tonica C..B | Escursione fino a tre ottave | Evoluzione della frase (CCW blocco) |
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
| 10 | Clock ORBIT | Breve: nuovo tema; 650 ms: prossima scala | Pitch 1 V/oct nominale | Accento 0/+5 V | **Uscita gate note** |
| 11 | Risincronizza | Risincronizza | Clock A | Clock B | XOR A/B |
| 12 | Fronte: avanza registro | Pressione: avanza registro | CV registro | Bit mantenuto 0/+5 V | Gate segue CV IN o MANUAL tenuto |
| 13 | Evento clock | Evento clock | Trigger jitter A | Trigger swing B | Gate variabile |
| 14 | Evento da smistare | Evento da smistare | Route A | Route B | Route C |
| 15 | Clock: avanza uno step | Avanza uno step | Gate | Gate accentato | Trigger accento |
| 16 | Prossimo preset, solo play | Prossimo preset | CV A salvata | CV B salvata | Gate salvato |

## Differenze tra versioni

| Versione | Modo 10 | Modo 12, OUT2 / I/O | Modo 16 |
| --- | --- | --- | --- |
| 2.2.1 | ORBIT: pitch, accento, I/O gate **in uscita** | Bit / gate di clock | 8 preset |
| 2.1.0 | Acid Voice: audio OUT1, AD OUT2; I/O ingresso trigger | CV inversa / bit | 4 preset |
| 2.0.2 | Mutant Drums: audio kick/snare; CV IN trigger kick, I/O trigger snare, MANUAL entrambi; P1 pitch/body kick, P2 corpo/rumore snare, P3 carattere kick e code | CV inversa / bit | 4 preset |

[Dettagli ORBIT, Turing e otto preset](../../firmware/ALTERIA_ALLROUNDER_2_2_1/QUICK_REFERENCE.md).
ORBIT richiede clock esterno a CV IN e un oscillatore esterno: non genera audio.

## Modo 10 nella 2.1.0: Acid Voice - NON TESTATO SU HARDWARE

LED superiori sempre **0101**. Sostituisce solo il modo 10 Drums; non e una
replica TB-303. Non confondere Acid Voice 10 con il sequencer CV Acid 6.

| Controllo | Funzione nella 2.1.0 |
| --- | --- |
| POT1 | Decay volume e apertura filtro |
| POT2 | Risonanza |
| POT3 | Env Mod sul filtro, cutoff di base circa 110 Hz |
| CV IN | **Pitch continuo nominale 1 V/oct**, non trigger |
| I/O | **Ingresso gate/trigger nota** |
| MANUAL | Alterna saw/square; **non suona una nota** |
| OUT1 | **Audio mono circa +/-2.5 V** |
| OUT2 | **Inviluppo positivo 0..+5 V**, non audio snare |

Il gate avvia il decay; tenerlo alto non sostiene/ripete. Cutoff aperto dall'AD
fino a circa 1.5 kHz. LED CV IN spento = saw, acceso = square; LED I/O segue
il gate. Selezionare 10 prima di collegare I/O e scollegarlo prima di uscire o
riavviare. Per ascoltare servono pitch e gate esterni: modo 6 non gira insieme
al 10. ACAL corregge uscite DAC, non tracking dell'ingresso pitch.

## Da ricordare

- Nella 2.2.1 OUT1/OUT2: modi 1..15 **0..+5 V**, modo 16 **-5..+5 V**.
  I/O e digitale, non un terzo DAC analogico.
- **Nella 2.2.1 I/O e ingresso solo in modo 3 e in taratura.** Prima di caricarla
  scollegare eventuali gate esterni da I/O. In modo 10 il clock va a CV IN.
  Nelle versioni precedenti I/O e ingresso anche nel modo 10.
- Clock 9: rapporti /8, /6, /4, /3, /2, x1, x2, x3, x4, x6, x8. Parte a
  120 BPM; dopo almeno tre tap usa il nuovo tempo. CV ha priorita; se manca
  continua all'ultimo tempo. Duty 50%. Non gira dietro gli altri modi.
- Acid 6 e Tremolo 7 generano **CV, non audio**. Acid: cambiare i pot e poi
  MANUAL per rigenerare; serve clock esterno per avanzare e un VCO esterno per suonare.
- Preset 16: MODE 650 ms alterna edit/play; MANUAL seleziona 1..8; P1/P2 con
  pickup. Uscire dall'edit salva. LED1..3 indicano l'indice 0..7, LED4 acceso in
  PLAY e lampeggiante in EDIT. Le versioni precedenti hanno 4 preset, LED one-hot.
- Taratura nascosta: MODE+MANUAL al boot, rilasciare; LED1 zero, LED2 +5 V,
  LED3 -5 V; P1/P2 correggono le due uscite con tester. Centrare prima di
  MANUAL per avanzare; LED4 lampeggiante: MANUAL 1 s salva. MODE 1.2 s annulla.

Nuovo timbro 2.0.2 non dichiarato collaudato sul modulo da questa guida.
[Manuale 2.2.1 NON TESTATO](../../firmware/ALTERIA_ALLROUNDER_2_2_1/README.md) ·
[Manuale 2.0.2](../../firmware/ALTERIA_ALLROUNDER/README.md) ·
[Manuale 2.1.0 NON TESTATO](../../firmware/ALTERIA_ALLROUNDER_2_1_0/README.md) ·
[Indice quick reference](README.md).
