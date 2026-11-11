# Swarm Trigger

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Tre agenti autonomi generano trigger con spread, wobble e probabilità di salto.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Speed/density | Incremento 8..1800 · Quadratica · Live, ogni 12 ms |
| P2 | Spread tre agenti | Spread incremento 0..1200 · Lineare · Live, ogni 12 ms |
| P3 | Chaos/wobble | Chaos 0..900, drop 0..45%, pulse 3..14 ms · Lineare · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Randomizza le tre fasi e genera pulse I/O |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | Trigger agente A |
| OUT2 (MCP4822 B) | Trigger agente B |
| I/O (D2, uscita) | Trigger agente C |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.

- Port REV5-2, acquisizione gate ADC con isteresi; debounce MANUAL.

## Standalone

Aprire [ALTERIA_rh_swarm.ino](Standalone/ALTERIA_rh_swarm/ALTERIA_rh_swarm.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Swarm Trigger** (`rh_swarm`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: scheduler. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_RHYTHM_ENGINE_328P/ALTERIA_RHYTHM_ENGINE_328P.ino`.
