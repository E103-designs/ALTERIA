# Turing Machine

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Registro mutante: CV e bit mantenuti; I/O segue il gate CV IN OR MANUAL.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Double-lock / slip / random / slip / lock | 100% inversione CCW, 50% al centro, 0% CW · Zone di lock agli estremi e curva quadratica verso il centro · Al fronte di trigger/clock |
| P2 | Lunghezza 1..16 | Lunghezza 1..16 bit · map intero: 16 solo a fondo corsa · Al fronte di trigger/clock |
| P3 | Quantizzazione continua / 24 / 12 / 8 / 5 / 3 livelli | Continua con ADC <150; 24/12/8/5/3 livelli alle soglie 150/340/520/700/880 · Selettore discreto · Al fronte di trigger/clock |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Ogni fronte crescente avanza il registro; livello tenuto contribuisce al gate I/O. |
| MANUAL (A3) | Ogni pressione avanza una volta; tenuto mantiene I/O alto, anche con CV basso. |
| OUT1 (MCP4822 A) | CV registro 0..+5 V |
| OUT2 (MCP4822 B) | Bit mantenuto 0/+5 V |
| I/O (D2, uscita) | CV IN OR MANUAL tenuti |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.

- DAC latcheato prima della salita I/O. P1 CCW: ciclo invertito 2N; CW: ciclo diretto N. Nessuna scala musicale.

## Standalone

Aprire [ALTERIA_ar_turing.ino](Standalone/ALTERIA_ar_turing/ALTERIA_ar_turing.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Turing Machine** (`ar_turing`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: nessun adapter aggiuntivo. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_2_4/ALTERIA_ALLROUNDER_2_2_4.ino`.
