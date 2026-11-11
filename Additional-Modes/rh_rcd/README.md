# RCD Rotation

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Tre divisori di clock ruotabili con scelta di set semplice o complesso.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Set semplice/completo | ADC <=512 semplice, >512 complesso · Selettore discreto · Al fronte di trigger/clock |
| P2 | Rotazione | Rotazione 0..len-1 · Lineare · Al fronte di trigger/clock |
| P3 | Spread assegnazioni | Spread 1..len-1 · Lineare · Al fronte di trigger/clock |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Clock master |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | Divisione A |
| OUT2 (MCP4822 B) | Divisione B |
| I/O (D2, uscita) | Divisione C |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.

- Port REV5-2, acquisizione gate ADC con isteresi; debounce MANUAL.

## Standalone

Aprire [ALTERIA_rh_rcd.ino](Standalone/ALTERIA_rh_rcd/ALTERIA_rh_rcd.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **RCD Rotation** (`rh_rcd`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: scheduler. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_RHYTHM_ENGINE_328P/ALTERIA_RHYTHM_ENGINE_328P.ino`.
