# Triple Clock / Tap

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Tre clock a rapporti indipendenti con master autonomo, clock esterno prioritario e tap tempo.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Rapporto OUT1 | /8 /6 /4 /3 /2 x1 x2 x3 x4 x6 x8 · 11 settori uniformi · Live, campionamento dei pot ogni 12 ms |
| P2 | Rapporto OUT2 | /8 /6 /4 /3 /2 x1 x2 x3 x4 x6 x8 · 11 settori uniformi · Live, campionamento dei pot ogni 12 ms |
| P3 | Rapporto I/O | /8 /6 /4 /3 /2 x1 x2 x3 x4 x6 x8 · 11 settori uniformi · Live, campionamento dei pot ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Clock esterno prioritario; al distacco continua all’ultimo tempo. |
| MANUAL (A3) | Almeno tre tap; media degli ultimi due intervalli. |
| OUT1 (MCP4822 A) | Clock 50% duty |
| OUT2 (MCP4822 B) | Clock 50% duty |
| I/O (D2, uscita) | Clock 50% duty |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_ar_clock3.ino](Standalone/ALTERIA_ar_clock3/ALTERIA_ar_clock3.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Triple Clock / Tap** (`ar_clock3`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: nessun adapter aggiuntivo. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_2_1/ALTERIA_ALLROUNDER_2_2_1.ino`.
