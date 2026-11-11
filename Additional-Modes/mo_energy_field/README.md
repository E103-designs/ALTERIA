# Energy Field · Motion

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Particella con rumore, attrazione, drag e zona calda.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Energia/noise | Energia 0..1,5 · Lineare · Live, ogni 12 ms |
| P2 | Attrazione | Attrazione 0,2..5 · Lineare · Live, ogni 12 ms |
| P3 | Hot radius/drag | Drag 0,45..3,4; hot radius 0,035..0,16 · Lineare · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Randomizza particella |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | X |
| OUT2 (MCP4822 B) | Y |
| I/O (D2, uscita) | Hot zone gate |

Livelli nominali: -5..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_mo_energy_field.ino](Standalone/ALTERIA_mo_energy_field/ALTERIA_mo_energy_field.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Energy Field · Motion** (`mo_energy_field`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: motion-math. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_MOTION_ENGINE_POT3/ALTERIA_MOTION_ENGINE_POT3.ino`.
