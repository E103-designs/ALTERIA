# Trance Gate · 24 pattern

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Pattern di gate e accenti: 24 frasi di 16 step, avanzamento esterno.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | 24 pattern | 24 pattern · 24 settori uniformi · Al fronte di trigger/clock |
| P2 | Durata gate | 5..95% periodo; minimo 2..4 ms · Lineare · Al fronte di trigger/clock |
| P3 | Profondità accento | Accent depth 0..4095 Q12 · Lineare · Al fronte di trigger/clock |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Fronte crescente: evento/reset. |
| MANUAL (A3) | Come CV IN; una pressione = un evento. |
| OUT1 (MCP4822 A) | Pattern principale |
| OUT2 (MCP4822 B) | Accento positivo |
| I/O (D2, uscita) | Trigger accento |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_ar_trance.ino](Standalone/ALTERIA_ar_trance/ALTERIA_ar_trance.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Trance Gate · 24 pattern** (`ar_trance`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: nessun adapter aggiuntivo. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_2_1/ALTERIA_ALLROUNDER_2_2_1.ino`.
