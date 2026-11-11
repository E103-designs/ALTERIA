# Logic XOR

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Due clock autonomi e combinazione XOR; il trigger risincronizza.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Periodo A | 2000 ms CCW → 10 ms CW · Quadratica · Live, campionamento dei pot ogni 12 ms |
| P2 | Periodo B | 2000 ms CCW → 10 ms CW · Quadratica · Live, campionamento dei pot ogni 12 ms |
| P3 | Skew B | Skew 0..100% del periodo B · Lineare · Live, campionamento dei pot ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Risincronizza entrambi i clock con lo skew B. |
| MANUAL (A3) | Come CV IN; i clock poi continuano autonomamente. |
| OUT1 (MCP4822 A) | Clock A 0/+5 V |
| OUT2 (MCP4822 B) | Clock B 0/+5 V |
| I/O (D2, uscita) | A XOR B |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_ar_xor.ino](Standalone/ALTERIA_ar_xor/ALTERIA_ar_xor.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Logic XOR** (`ar_xor`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: nessun adapter aggiuntivo. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_2_4/ALTERIA_ALLROUNDER_2_2_4.ino`.
