# Pendulum · Motion

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Oscillatore smorzato: posizione, velocità e impulso al passaggio centrale.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Frequenza 0.05..10 Hz | 0,05..10 Hz · Lineare · Live, ogni 12 ms |
| P2 | Damping 0.03..1.8 | Damping 0,03..1,8 · Lineare · Live, ogni 12 ms |
| P3 | Fold/tensione | Drive 1..4 con fold · Lineare · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Riavvia/eccita |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | Posizione |
| OUT2 (MCP4822 B) | Velocita |
| I/O (D2, uscita) | Centro crossing |

Livelli nominali: -5..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_mo_pendulum.ino](Standalone/ALTERIA_mo_pendulum/ALTERIA_mo_pendulum.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Pendulum · Motion** (`mo_pendulum`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: motion-math. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_MOTION_ENGINE_POT3/ALTERIA_MOTION_ENGINE_POT3.ino`.
