# Cv Cross Mod · Motion

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Due oscillatori che modulano reciprocamente frequenza e forma; marker near-sync.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Base 0.05..18 Hz | Base 0,05..18 Hz · Lineare · Live, ogni 12 ms |
| P2 | Depth 0..1.2 | Depth 0..1,2 · Lineare · Live, ogni 12 ms |
| P3 | Shape/sync width | Sine..fold triangle, near-sync 0,008..0,055 cicli · Lineare · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Rephase |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | Oscillatore 1 |
| OUT2 (MCP4822 B) | Oscillatore 2 |
| I/O (D2, uscita) | Near-sync pulse |

Livelli nominali: -5..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_mo_cv_cross_mod.ino](Standalone/ALTERIA_mo_cv_cross_mod/ALTERIA_mo_cv_cross_mod.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Cv Cross Mod · Motion** (`mo_cv_cross_mod`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: motion-math. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_MOTION_ENGINE_POT3/ALTERIA_MOTION_ENGINE_POT3.ino`.
