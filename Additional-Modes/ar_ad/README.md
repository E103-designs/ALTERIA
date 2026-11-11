# AD Shaper

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Inviluppo AD finito, retrigger dal livello corrente, eventi di fine salita e ciclo.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Attack 2..5000 ms | 2..5000 ms · Quadratica · Al fronte di trigger/clock |
| P2 | Decay 2..5000 ms | 2..5000 ms · Quadratica · Al fronte di trigger/clock |
| P3 | 12 coppie curva Attack/Decay | 12 coppie LOG/LIN/EXP × LOG/LIN/EXP/S · 12 settori con isteresi 7 ADC · Al fronte di trigger/clock |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Trigger AD; retrigger dal livello attuale. |
| MANUAL (A3) | Come CV IN |
| OUT1 (MCP4822 A) | Inviluppo |
| OUT2 (MCP4822 B) | End of Rise 10 ms |
| I/O (D2, uscita) | End of Fall 10 ms |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_ar_ad.ino](Standalone/ALTERIA_ar_ad/ALTERIA_ar_ad.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **AD Shaper** (`ar_ad`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: nessun adapter aggiuntivo. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_2_1/ALTERIA_ALLROUNDER_2_2_1.ino`.
