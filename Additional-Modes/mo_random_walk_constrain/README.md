# Random Walk Constrain · Motion

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Cammino casuale con vincolo centrale, velocità e impulso al confine.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Tick 15..300 ms | Tick 15..300 ms · Lineare · Live, ogni 12 ms |
| P2 | Vincolo al centro | Vincolo 0..1,4 · Lineare · Live, ogni 12 ms |
| P3 | Ampiezza passo/boundary | Passo 0,035..0,28; bounce 0,35..1; gain velocità 2..6 · Lineare · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Centra posizione/velocita |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | Posizione |
| OUT2 (MCP4822 B) | Velocita |
| I/O (D2, uscita) | Boundary pulse |

Livelli nominali: -5..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_mo_random_walk_constrain.ino](Standalone/ALTERIA_mo_random_walk_constrain/ALTERIA_mo_random_walk_constrain.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Random Walk Constrain · Motion** (`mo_random_walk_constrain`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: motion-math. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_MOTION_ENGINE_POT3/ALTERIA_MOTION_ENGINE_POT3.ino`.
