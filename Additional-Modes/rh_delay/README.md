# Delay Trigger

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Due copie ritardate di ogni evento, con riferimento immediato su I/O.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Delay A | 0..1800 ms · Quadratica · Al fronte di trigger/clock |
| P2 | Delay B | 0..1800 ms · Quadratica · Al fronte di trigger/clock |
| P3 | Pulse width | 2 ms..min(periodo-1,300 ms) · Lineare · Al fronte di trigger/clock |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Trigger |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | Trigger ritardato A |
| OUT2 (MCP4822 B) | Trigger ritardato B |
| I/O (D2, uscita) | Riferimento |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.

- Port REV5-2, acquisizione gate ADC con isteresi; debounce MANUAL.

## Standalone

Aprire [ALTERIA_rh_delay.ino](Standalone/ALTERIA_rh_delay/ALTERIA_rh_delay.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Delay Trigger** (`rh_delay`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: scheduler. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_RHYTHM_ENGINE_328P/ALTERIA_RHYTHM_ENGINE_328P.ino`.
