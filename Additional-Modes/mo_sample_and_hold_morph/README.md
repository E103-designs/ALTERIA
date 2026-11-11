# Sample & Hold Morph · Motion

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Campioni random con glide e griglia, clock esterno o fallback interno.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Intervallo interno 30..1800 ms | Intervallo 30..1800 ms · Lineare · Live, ogni 12 ms |
| P2 | Glide | Glide: coefficiente 40..1, tau nominale 333..8,3 ms · Lineare · Live, ogni 12 ms |
| P3 | Griglia tensione | Quantizzazione continua / 24 / 12 / 8 / 5 / 3 livelli · Selettore discreto · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Campiona immediatamente; il clock interno riparte dopo l’intervallo P1 dall’ultimo campione. |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | Sample raw |
| OUT2 (MCP4822 B) | Sample smoothed |
| I/O (D2, uscita) | Sample pulse |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_mo_sample_and_hold_morph.ino](Standalone/ALTERIA_mo_sample_and_hold_morph/ALTERIA_mo_sample_and_hold_morph.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Sample & Hold Morph · Motion** (`mo_sample_and_hold_morph`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: motion-math. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_MOTION_ENGINE_POT3/ALTERIA_MOTION_ENGINE_POT3.ino`.
