# Triple XOR LFO

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Tre LFO quadri confrontati a coppie in XOR, con rephase comune.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Rate LFO1 | Semiperiodo 4000..5 ms · Quadratica inversa · Live, ogni 12 ms |
| P2 | Rate LFO2 | Semiperiodo 4000..5 ms · Quadratica inversa · Live, ogni 12 ms |
| P3 | Rate LFO3 | Semiperiodo 4000..5 ms · Quadratica inversa · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Risincronizza |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | LFO1 XOR LFO2 |
| OUT2 (MCP4822 B) | LFO2 XOR LFO3 |
| I/O (D2, uscita) | LFO3 XOR LFO1 |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.

- Port REV5-2, acquisizione gate ADC con isteresi; debounce MANUAL.

## Standalone

Aprire [ALTERIA_rh_triple_xor.ino](Standalone/ALTERIA_rh_triple_xor/ALTERIA_rh_triple_xor.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Triple XOR LFO** (`rh_triple_xor`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: scheduler. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_RHYTHM_ENGINE_328P/ALTERIA_RHYTHM_ENGINE_328P.ino`.
