# Humanizer

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Variazioni temporali correlate di tre uscite da uno stesso clock.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Jitter A | Jitter 0..25% periodo · Lineare · Al fronte di trigger/clock |
| P2 | Swing B | Swing 0..33% periodo · Lineare · Al fronte di trigger/clock |
| P3 | Variazione gate | Variazione ampiezza temporale gate 0..100% · Lineare · Al fronte di trigger/clock |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Clock: ogni salita genera un gruppo di eventi variati. |
| MANUAL (A3) | Un evento clock per pressione; tenuto non ripete. |
| OUT1 (MCP4822 A) | Trigger A con jitter |
| OUT2 (MCP4822 B) | Trigger B con swing |
| I/O (D2, uscita) | Gate con durata variabile |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_ar_humanizer.ino](Standalone/ALTERIA_ar_humanizer/ALTERIA_ar_humanizer.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Humanizer** (`ar_humanizer`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: nessun adapter aggiuntivo. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_2_1/ALTERIA_ALLROUNDER_2_2_1.ino`.
