# Radioactive Geiger · Motion

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Carica e collassi pseudocasuali con CV correlata, diverso dal Geiger ritmico.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Rate 0.1..80 Hz, modulato | Base 0,1..80 Hz, moltiplicata da P3 per 0,35..2,4 · Lineare · Live, ogni 12 ms |
| P2 | Recupero carica 0.05..4 | Recupero 0,05..4 / s · Lineare · Live, ogni 12 ms |
| P3 | Instabilita/collapse | Instabilità 0..1; curva carica e soglia collapse · Lineare · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Ricarica |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | Carica sagomata |
| OUT2 (MCP4822 B) | S&H correlato |
| I/O (D2, uscita) | Evento |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_mo_radioactive_geiger.ino](Standalone/ALTERIA_mo_radioactive_geiger/ALTERIA_mo_radioactive_geiger.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Radioactive Geiger · Motion** (`mo_radioactive_geiger`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: motion-math. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_MOTION_ENGINE_POT3/ALTERIA_MOTION_ENGINE_POT3.ino`.
