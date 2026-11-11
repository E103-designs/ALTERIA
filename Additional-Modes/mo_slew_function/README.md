# Slew / Function · Motion

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Generatore di funzione dal livello gate, con velocità separate e curva di uscita.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Velocita salita 1..350 | Velocità 1..350 livelli Q12 / ms · Lineare · Live, ogni 12 ms |
| P2 | Velocita discesa 1..350 | Velocità 1..350 livelli Q12 / ms · Lineare · Live, ogni 12 ms |
| P3 | Curva uscita | LOG..LIN..EXP · Continua · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Gate target |
| MANUAL (A3) | Gate OR con CV |
| OUT1 (MCP4822 A) | Livello slewed |
| OUT2 (MCP4822 B) | Inverso |
| I/O (D2, uscita) | Copia gate |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_mo_slew_function.ino](Standalone/ALTERIA_mo_slew_function/ALTERIA_mo_slew_function.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Slew / Function · Motion** (`mo_slew_function`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: motion-math. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_MOTION_ENGINE_POT3/ALTERIA_MOTION_ENGINE_POT3.ino`.
