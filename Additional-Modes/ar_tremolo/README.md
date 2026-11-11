# Tremolo / AutoPan

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Due CV complementari con transizione da oscillazione regolare a movimento casuale.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Velocità | Incremento fase 2..2200 / ms · Quadratica · Live, campionamento dei pot ogni 12 ms |
| P2 | Ampiezza | Ampiezza 0..2047 livelli Q12 · Quadratica · Live, campionamento dei pot ogni 12 ms |
| P3 | Randomizzazione | Randomizzazione ADC 0..1023; regione chaos oltre 650 · Lineare · Live, campionamento dei pot ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Fronte crescente: risincronizza senza cambiare polarità. |
| MANUAL (A3) | Commuta OUT1 e OUT2 tra 0…+5 V e −5…+5 V senza risincronizzare. |
| OUT1 (MCP4822 A) | Modulazione unipolare o bipolare |
| OUT2 (MCP4822 B) | Complementare unipolare o bipolare |
| I/O (D2, uscita) | Crossing semiciclo / cambio target random |

Livelli nominali: 0..+5 V default; −5..+5 V su MANUAL. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_ar_tremolo.ino](Standalone/ALTERIA_ar_tremolo/ALTERIA_ar_tremolo.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Tremolo / AutoPan** (`ar_tremolo`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: nessun adapter aggiuntivo. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_2_4/ALTERIA_ALLROUNDER_2_2_4.ino`.
