# Dual Decay

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Due decay vactrol indipendenti, con memoria dei colpi precedenti.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Decay A | 7..1400 ms · Quadratica · Al fronte di trigger/clock |
| P2 | Decay B | 7..1400 ms · Quadratica · Al fronte di trigger/clock |
| P3 | Carattere vactrol | ADC 0..1023: strike 3..18 ms, miscela lenta 0..56% · Lineare · Al fronte di trigger/clock |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Trigger A |
| MANUAL (A3) | Eccita entrambi i decay |
| OUT1 (MCP4822 A) | Decay A |
| OUT2 (MCP4822 B) | Decay B |
| I/O (D2, ingresso) | Ingresso trigger B |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_ar_dual_decay.ino](Standalone/ALTERIA_ar_dual_decay/ALTERIA_ar_dual_decay.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Dual Decay** (`ar_dual_decay`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: nessun adapter aggiuntivo. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_2_1/ALTERIA_ALLROUNDER_2_2_1.ino`.
