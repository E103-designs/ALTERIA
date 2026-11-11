# Bernoulli 3

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Ogni evento sceglie una delle tre destinazioni, con memoria della rotta.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Bilanciamento A/B | A/B 100/0..0/100% della quota residua · Lineare · Al fronte di trigger/clock |
| P2 | Probabilità C | Probabilità I/O 0..95% · Lineare · Al fronte di trigger/clock |
| P3 | Stickiness | Stickiness 0..95% · Lineare · Al fronte di trigger/clock |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Una salita instrada un evento su una sola destinazione. |
| MANUAL (A3) | Una pressione instrada un evento; tenuto non ripete. |
| OUT1 (MCP4822 A) | Route A trigger |
| OUT2 (MCP4822 B) | Route B trigger |
| I/O (D2, uscita) | Route C trigger |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_ar_bernoulli.ino](Standalone/ALTERIA_ar_bernoulli/ALTERIA_ar_bernoulli.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Bernoulli 3** (`ar_bernoulli`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: nessun adapter aggiuntivo. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_2_1/ALTERIA_ALLROUNDER_2_2_1.ino`.
