# AR Gate

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Gate con attack, sustain finché tenuto e release; uscita CV complementare.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Attack 2..5000 ms | 2..5000 ms · Quadratica · Al fronte di trigger/clock |
| P2 | Release 2..5000 ms | 2..5000 ms · Quadratica · Al fronte di trigger/clock |
| P3 | Curva LOG/LIN/EXP | LOG / LIN / EXP · Tre regioni alle soglie ADC 341 / 683 (nessuna isteresi) · Al fronte di trigger/clock |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Gate e trigger |
| MANUAL (A3) | Gate OR CV |
| OUT1 (MCP4822 A) | Inviluppo |
| OUT2 (MCP4822 B) | Inviluppo inverso |
| I/O (D2, uscita) | End of Release 10 ms |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_ar_ar.ino](Standalone/ALTERIA_ar_ar/ALTERIA_ar_ar.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **AR Gate** (`ar_ar`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: nessun adapter aggiuntivo. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_2_1/ALTERIA_ALLROUNDER_2_2_1.ino`.
