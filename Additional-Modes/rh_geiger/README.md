# Geiger · Rhythm

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

CV di attività e spike pseudocasuali; A7 non è un sensore di radiazioni.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Density | Densità 1..22 · Lineare · Live, ogni 12 ms |
| P2 | Persistence | Persistenza 1..60 · Lineare · Live, ogni 12 ms |
| P3 | Sensibilita/chaos | Soglie differenza 22..3, deviazione 26..4, boost 8..32 · Lineare · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Ricarica attivita |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | Energia A |
| OUT2 (MCP4822 B) | Energia B |
| I/O (D2, uscita) | Spike trigger |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.

- Port REV5-2: DAC calibrato e LDAC D12; D3/D5 sono LED. A7 è solo rumore ADC, nessun sensore di radiazione.

## Standalone

Aprire [ALTERIA_rh_geiger.ino](Standalone/ALTERIA_rh_geiger/ALTERIA_rh_geiger.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Geiger · Rhythm** (`rh_geiger`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: scheduler. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_RHYTHM_ENGINE_328P/ALTERIA_RHYTHM_ENGINE_328P.ino`.
