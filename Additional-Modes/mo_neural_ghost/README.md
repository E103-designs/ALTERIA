# Neural Ghost · Motion

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Membrana con memoria, eccitazione interna e spike alla soglia.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Memoria/leak | Leak 3,5..0,12 / s · Lineare · Live, ogni 12 ms |
| P2 | Soglia | Soglia 0,12..0,95 · Lineare · Live, ogni 12 ms |
| P3 | Ghost/noise excitation | Iniezione 0,18..0,70; refrattario 60..12 ms e rumore · Lineare · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Iniezione |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | Membrana |
| OUT2 (MCP4822 B) | Soglia |
| I/O (D2, uscita) | Spike |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_mo_neural_ghost.ino](Standalone/ALTERIA_mo_neural_ghost/ALTERIA_mo_neural_ghost.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Neural Ghost · Motion** (`mo_neural_ghost`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: motion-math. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_MOTION_ENGINE_POT3/ALTERIA_MOTION_ENGINE_POT3.ino`.
