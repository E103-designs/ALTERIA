# ORBIT

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Frasi melodiche di 16 step con motivi e ritorno alla tonica; quattro scale.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Tonica C..B | C..B (12 toniche) · 12 settori, isteresi 5 ADC · Live, ogni 12 ms |
| P2 | Escursione fino a tre ottave | 2 gradi..3 ottave, dipende dalla scala · Lineare · Live, ogni 12 ms |
| P3 | Evoluzione; CCW blocco | 0 = lock; 1..4 tentativi di evoluzione per frase · Lineare · Al confine della frase |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Clock esterno; una salita = uno step, niente clock autonomo. |
| MANUAL (A3) | Breve: accoda tema; 650 ms: prossima scala. Applicazione al confine di frase. |
| OUT1 (MCP4822 A) | Pitch positivo nominale 1 V/oct |
| OUT2 (MCP4822 B) | Accento 0/+5 V |
| I/O (D2, uscita) | Gate ritardato 1 ms, 60% periodo, max 250 ms |

Livelli nominali: 0..+5 V nominali. Slot; per 1 s dopo MANUAL lungo: scala one-hot.



## Standalone

Aprire [ALTERIA_ar_orbit.ino](Standalone/ALTERIA_ar_orbit/ALTERIA_ar_orbit.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **ORBIT** (`ar_orbit`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: nessun adapter aggiuntivo. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_2_1/ALTERIA_ALLROUNDER_2_2_1.ino`.
