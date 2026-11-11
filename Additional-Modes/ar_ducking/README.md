# Ducking

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Follower di CV e curva di attenuazione, senza percorso audio in ingresso.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Tempo discesa | Salita follower 1..300 ms · Quadratica · Live, campionamento dei pot ogni 12 ms |
| P2 | Tempo risalita | Discesa follower 4..2500 ms · Quadratica · Live, campionamento dei pot ogni 12 ms |
| P3 | Profondità ducking | Profondità 0..100% · Lineare · Live, campionamento dei pot ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | CV continuo: follower |
| MANUAL (A3) | Simula CV piena scala (+5 V nominali) finché tenuto. |
| OUT1 (MCP4822 A) | Gain ducking |
| OUT2 (MCP4822 B) | Envelope follower |
| I/O (D2, uscita) | Stato duck |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_ar_ducking.ino](Standalone/ALTERIA_ar_ducking/ALTERIA_ar_ducking.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Ducking** (`ar_ducking`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: nessun adapter aggiuntivo. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_2_1/ALTERIA_ALLROUNDER_2_2_1.ino`.
