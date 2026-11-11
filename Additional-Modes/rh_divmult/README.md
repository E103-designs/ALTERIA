# Clock Div/Mult · esterno

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Divider/moltiplicatore duale da clock esterno; distinto dal Triple Clock con tap.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Rapporto A | /8 /6 /4 /3 /2 x1 x2 x3 x4 x6 x8 · map intero, x8 solo al massimo · Al fronte di trigger/clock |
| P2 | Rapporto B | Stessi 11 rapporti A · map intero, x8 solo al massimo · Al fronte di trigger/clock |
| P3 | Pulse width | 2 ms..min(periodo-1,80 ms) · Lineare · Al fronte di trigger/clock |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Clock riferimento |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | Clock derivato A |
| OUT2 (MCP4822 B) | Clock derivato B |
| I/O (D2, uscita) | Clock x1 |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.

- Port REV5-2, acquisizione gate ADC con isteresi; debounce MANUAL.

## Standalone

Aprire [ALTERIA_rh_divmult.ino](Standalone/ALTERIA_rh_divmult/ALTERIA_rh_divmult.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Clock Div/Mult · esterno** (`rh_divmult`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: scheduler. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_RHYTHM_ENGINE_328P/ALTERIA_RHYTHM_ENGINE_328P.ino`.
