# Burst

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Un trigger avvia una raffica finita e una CV a rampa sagomata.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Spacing | 600 ms CCW → 8 ms CW · Quadratica · Al fronte di trigger/clock |
| P2 | Numero eventi | 1..16 eventi · 16 settori uniformi · Al fronte di trigger/clock |
| P3 | Curva ramp | LOG / LIN / EXP · Tre regioni · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Avvia/retriggera una raffica finita; cancella la precedente. |
| MANUAL (A3) | Avvia/retriggera una raffica; tenuto non ripete. |
| OUT1 (MCP4822 A) | Ramp down 0..+5 V |
| OUT2 (MCP4822 B) | Complementare positivo |
| I/O (D2, uscita) | Trigger di ogni evento |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_ar_burst.ino](Standalone/ALTERIA_ar_burst/ALTERIA_ar_burst.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Burst** (`ar_burst`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: nessun adapter aggiuntivo. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_2_4/ALTERIA_ALLROUNDER_2_2_4.ino`.
