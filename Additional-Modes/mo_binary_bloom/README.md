# Binary Bloom · Motion

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Popolazione binaria generativa con nascite, decadimento e mutazione.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Birth bias | Birth bias 0..255 · Lineare · Live, ogni 12 ms |
| P2 | Decay bias | Decay bias 0..255 · Lineare · Live, ogni 12 ms |
| P3 | Tick/mutazione | Tick 220..22 ms; mutazione 0..55 · Lineare · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Nuovo pattern |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | Popolazione bit |
| OUT2 (MCP4822 B) | Byte basso a CV |
| I/O (D2, uscita) | Bloom change |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_mo_binary_bloom.ino](Standalone/ALTERIA_mo_binary_bloom/ALTERIA_mo_binary_bloom.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Binary Bloom · Motion** (`mo_binary_bloom`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: motion-math. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_MOTION_ENGINE_POT3/ALTERIA_MOTION_ENGINE_POT3.ino`.
