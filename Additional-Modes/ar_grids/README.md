# Grids XY

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Tre corsie ritmiche deterministiche; il clock esterno percorre pattern di 16 step.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | X: 4 settori | X 0..3 · 4 settori uniformi · Al fronte di trigger/clock |
| P2 | Y: 4 settori | Y 0..3 · 4 settori uniformi · Al fronte di trigger/clock |
| P3 | Densità | Densità 0..100% · Lineare · Al fronte di trigger/clock |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Ogni fronte crescente avanza di uno step il pattern. |
| MANUAL (A3) | Ogni pressione avanza di uno step; tenuto non ripete. |
| OUT1 (MCP4822 A) | Kick trigger |
| OUT2 (MCP4822 B) | Snare trigger |
| I/O (D2, uscita) | Hat trigger |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_ar_grids.ino](Standalone/ALTERIA_ar_grids/ALTERIA_ar_grids.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Grids XY** (`ar_grids`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: nessun adapter aggiuntivo. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_2_1/ALTERIA_ALLROUNDER_2_2_1.ino`.
