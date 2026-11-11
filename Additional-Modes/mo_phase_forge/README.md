# Phase Forge · Motion

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Due LFO di forma asimmetrica, offset, curvature e fold.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Rate 0.03..20 Hz | 0,03..20 Hz · Lineare · Live, ogni 12 ms |
| P2 | Offset/asimmetria | Warp A 0,15..0,85; offset 0..0,95 cicli · Lineare · Live, ogni 12 ms |
| P3 | Curvature/fold | Curva continua e fold drive 1..4 · Lineare · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Azzera fase |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | LFO A |
| OUT2 (MCP4822 B) | LFO B |
| I/O (D2, uscita) | Crossing marker |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_mo_phase_forge.ino](Standalone/ALTERIA_mo_phase_forge/ALTERIA_mo_phase_forge.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Phase Forge · Motion** (`mo_phase_forge`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: motion-math. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_MOTION_ENGINE_POT3/ALTERIA_MOTION_ENGINE_POT3.ino`.
