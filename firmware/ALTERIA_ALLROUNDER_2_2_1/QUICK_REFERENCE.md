# ALLROUNDER 2.2.1 - Modifiche rapide

**NON TESTATO SU HARDWARE.** Altri modi: [tabella completa](README.md#mappa-completa).

## Modo 10 ORBIT, LED 0101

| Controllo | Funzione |
| --- | --- |
| P1 | Tonica C..B |
| P2 | Escursione da poche note a tre ottave |
| P3 | Evoluzione della frase: CCW blocca, CW mutazioni piu frequenti |
| CV IN | Clock esterno; senza clock non avanza |
| MANUAL breve al rilascio | Nuovo tema dal prossimo step 1 |
| MANUAL 650 ms | Prossima scala dal prossimo step 1, senza nuovo tema al rilascio |
| OUT1 | Pitch CV quantizzato nominale 1 V/oct, positivo |
| OUT2 | Accento 0/+5 V, mantenuto fino allo step successivo |
| I/O | Gate note, **uscita**, con pause della frase |

Scale: pentatonica maggiore / pentatonica minore / dorica / minore naturale.
Un LED acceso indica per 1 s la scala richiesta. Frase di 16 step, primo e ultimo
sulla tonica. P3 agisce solo al confine della frase; tonica/escursione sono live.
Nessuna generazione audio: collegare VCO e inviluppo/VCA esterni.

## Modo 16, otto preset

P1/P2 tensioni A/B in edit con pickup; P3 gate. MANUAL scorre 1..8; CV IN
avanza solo in play. MODE 650 ms alterna edit/play e salva all'uscita da edit.
LED1..3 codificano indice 0..7, LED4 fisso PLAY / lampeggiante EDIT:

| Preset | LED1..4 PLAY |
| --- | --- |
| 1 | 0001 |
| 2 | 1001 |
| 3 | 0101 |
| 4 | 1101 |
| 5 | 0011 |
| 6 | 1011 |
| 7 | 0111 |
| 8 | 1111 |

I vecchi quattro preset vengono recuperati; per salvare il nuovo formato
modificare un valore in edit e uscire. Vecchia EEPROM e taratura restano intatte.
Errore di rilettura: tutti i LED lampeggiano rapidamente, rientrare/uscire da edit
per riprovare. Otto slot CV/gate, non otto patch complete degli engine.

**Prima di caricare: scollegare qualsiasi gate esterno da I/O. Nel modo 10 ora
quel jack e un'uscita. Il clock ORBIT va su CV IN.**

## Modo 12 Turing, LED 0011

OUT1 = CV registro; OUT2 = bit mantenuto 0/+5 V; I/O = gate che segue
CV IN oppure MANUAL tenuto. CV/manopole mantengono il comportamento precedente:
P1 mutazione/lock, P2 lunghezza, P3 livelli. Ogni salita CV o pressione MANUAL
avanza una volta; tenere alto non ripete. Con ingressi sovrapposti I/O rimane
alto fino a quando entrambi scendono. Il bit puo restare alto per piu step.
CV e latcheata prima del gate, ma la copia ha latenza firmware e non e analogica.
