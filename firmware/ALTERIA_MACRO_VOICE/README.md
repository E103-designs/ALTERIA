# ALTERIA MACRO VOICE 1.0.0

Cinque oscillatori per **ALTERIA REV5-2, Arduino Nano ATmega328P a 16 MHz**.
**Non testato sul modulo.** Sketch autonomo: non sostituisce FULL VOICE nel
progetto e non richiede Mozzi o librerie esterne al core Arduino AVR.

## Patch rapida

1. Collegare **OUT1** a un mixer Eurorack attenuato. OUT2 porta una seconda
   uscita audio, non un inviluppo. Non collegare direttamente cuffie o speaker.
2. Collegare il pitch del sequencer a **CV IN** (1 V/oct nominale, C1 a 0 V,
   C6 a +5 V). Non applicare tensioni negative o oltre il range verificato del
   proprio modulo. L'intonazione fisica dipende da ADC, riferimento e clock.
3. **MODE** breve scorre i cinque modelli. I LED si leggono da LED1 a LED4.
4. **I/O** e sempre **ingresso** trigger. I primi quattro modelli suonano
   anche senza cavo: un fronte risincronizza la fase, non accende una nota.
   In Karplus il fronte pizzica la corda. **MANUAL** fa la stessa cosa, una
   volta per pressione. Un gate tenuto non ripete.

Per articolare i primi quattro modelli con un gate usare un VCA esterno;
non c'e un inviluppo, un LPG, un VCA o un filtro di uscita nel firmware.
La risincronizzazione e il cambio modello sono diretti e possono fare click.
P1/P2/P3 agiscono subito sul modello corrente, senza pagine o pickup.
Il LED CV IN lampeggia al movimento del pitch; il LED I/O mostra il gate
o un lampo alla pressione MANUAL. Karplus a riposo e muto.

## Modelli

| MODE | LED1..4 | P1 | P2 | P3 | OUT1 | OUT2 |
| --- | --- | --- | --- | --- | --- | --- |
| Fold | 1000 | Asimmetria della triangolare | Profondita di fold | Miscela con il secondo stadio di fold | Triangolare ripiegata | Triangolare prima del fold |
| FM | 0100 | Rapporto modulante/portante: 0.5, 1, 1.5, 2, 2.5, 3, 4, 5 | Indice di modulazione di fase | Feedback della modulante | Portante modulata | Modulante |
| Chord | 0010 | Accordo: m6, maj6, 7sus2, 7sus4, m7, maj7, 7, m7add11 senza quinta | Quattro inversioni | Morph da seno a colore ricco | Mix di quattro note | Seno della fondamentale CV, indipendente dall'inversione |
| Wavetable | 0001 | Banca: Classic, Hollow, Glass | Scan continuo OUT1 | Scan continuo OUT2 | Scansione A | Scansione B della stessa banca |
| Karplus | 1111 | Dal corpo tonale al plettro rumoroso | Durata della risonanza | Posizione del pickup OUT2 | Corda pizzicata | Secondo pickup sulla stessa corda |

In Wavetable ogni banca ha quattro forme originali: P2 e P3 interpolano
fra le forme; le uscite hanno lo stesso pitch. In FM il rapporto scende di
un'ottava alla volta se il modulante supererebbe 3.2 kHz. In Chord e
Wavetable le tabelle riducono il contenuto armonico quando il pitch sale;
in Chord il morph diventa neutro nella banda piu alta. Fold e FM possono
produrre aliasing udibile sulle note alte.

Karplus usa una linea di ritardo frazionaria. Pitch e posizione pickup
vengono acquisiti al pizzico, la durata risponde anche durante la coda.
Lo smorzamento e interno alla corda, non un filtro o inviluppo applicato
all'uscita. L'effetto del pickup cambia con posizione e contenuto armonico.

## Caricamento e taratura

Aprire `ALTERIA_MACRO_VOICE.ino` in Arduino IDE, selezionare **Arduino Nano /
ATmega328P** e la porta del modulo. Se il proprio Nano richiede **ATmega328P
(Old Bootloader)**, usare quella voce per l'upload. Non adatto ad ATmega168P.

Il DAC eroga audio bipolare con zero al punto calibrato e fondo scala
software circa +/-2.5 V per canale. Il firmware legge soltanto ACAL
(EEPROM 256..301) e altrimenti usa i codici nominali. Non offre un menu di
taratura e non scrive EEPROM. Tarare con un firmware ALTERIA che supporta
ACAL, se necessario. La taratura delle uscite non corregge il tracking CV.
Scollegare la sorgente da I/O prima di caricare un altro firmware: alcune
varianti ALLROUNDER usano I/O come uscita.

Sample rate 8 kHz, banda teorica sotto 4 kHz, carattere lo-fi. Il [WAV di anteprima](ALTERIA_MACRO_VOICE_1_0_0.wav) contiene il DSP digitale, non riproduce elettronica, livelli
fisici, rumore o risposta del PCB. Prima dell'uso dal vivo verificare
tensione di riposo, livelli, accordatura, trigger e comportamento dei jack
sul proprio modulo.
