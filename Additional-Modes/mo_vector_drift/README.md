# Vector Drift · Motion

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Movimento XY caotico con attrazione al centro e gate sui confini.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Speed 0.2..8 | Speed 0,2..8 · Lineare · Live, ogni 12 ms |
| P2 | Chaos 0..1.2 | Chaos 0..1,2 · Lineare · Live, ogni 12 ms |
| P3 | Gravita verso centro | Gravità 0,02..0,85; drag 0,05..0,35 · Lineare · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Randomizza XY |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | X |
| OUT2 (MCP4822 B) | Y |
| I/O (D2, uscita) | Boundary/turn |

Livelli nominali: -5..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_mo_vector_drift.ino](Standalone/ALTERIA_mo_vector_drift/ALTERIA_mo_vector_drift.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Vector Drift · Motion** (`mo_vector_drift`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: motion-math. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_MOTION_ENGINE_POT3/ALTERIA_MOTION_ENGINE_POT3.ino`.
