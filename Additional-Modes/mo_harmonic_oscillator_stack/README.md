# Harmonic Oscillator Stack · Motion

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Somma di armoniche CV con mix/spread, drive e fold.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Fondamentale 0.03..15 Hz | Fondamentale 0,03..15 Hz · Lineare · Live, ogni 12 ms |
| P2 | Mix/spread | Mix e spread, ADC 0..1023 · Lineare · Live, ogni 12 ms |
| P3 | Drive/fold | Drive 1..4 e fold · Lineare · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Azzera fase |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | Mix armonico |
| OUT2 (MCP4822 B) | Armonica correlata |
| I/O (D2, uscita) | Phase/cross marker |

Livelli nominali: -5..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_mo_harmonic_oscillator_stack.ino](Standalone/ALTERIA_mo_harmonic_oscillator_stack/ALTERIA_mo_harmonic_oscillator_stack.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Harmonic Oscillator Stack · Motion** (`mo_harmonic_oscillator_stack`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: motion-math. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_MOTION_ENGINE_POT3/ALTERIA_MOTION_ENGINE_POT3.ino`.
