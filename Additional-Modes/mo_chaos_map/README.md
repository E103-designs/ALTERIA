# Chaos Map · Motion

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Mappa logistica iterata: due CV correlate e gate di soglia.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Logistic r 2.8..4 | Logistic r 2,8..4 · Lineare · Live, ogni 12 ms |
| P2 | Tick 5..350 ms | Tick 5..350 ms · Lineare · Live, ogni 12 ms |
| P3 | Soglia/curva | Soglia 0,35..0,92 e curva continua · Lineare · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Nuovo x |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | x sagomato |
| OUT2 (MCP4822 B) | x precedente sagomato |
| I/O (D2, uscita) | Threshold crossing |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_mo_chaos_map.ino](Standalone/ALTERIA_mo_chaos_map/ALTERIA_mo_chaos_map.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Chaos Map · Motion** (`mo_chaos_map`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: motion-math. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_MOTION_ENGINE_POT3/ALTERIA_MOTION_ENGINE_POT3.ino`.
