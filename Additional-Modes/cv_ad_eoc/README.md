# AD + EOC

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

AD da zero a tempi quadratici e tre curve, con CV inversa e impulso di fine ciclo.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Attack | Attack 5..2500 ms · Quadratica · Al fronte di trigger/clock |
| P2 | Decay | Decay 5..2500 ms · Quadratica · Al fronte di trigger/clock |
| P3 | Curva | LOG / LIN / EXP · Tre regioni · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Fronte crescente: evento/reset. |
| MANUAL (A3) | Come CV IN; una pressione = un evento. |
| OUT1 (MCP4822 A) | AD positivo |
| OUT2 (MCP4822 B) | Inversa positiva |
| I/O (D2, uscita) | EOC pulse 10 ms |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_cv_ad_eoc.ino](Standalone/ALTERIA_cv_ad_eoc/ALTERIA_cv_ad_eoc.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **AD + EOC** (`cv_ad_eoc`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: scheduler. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_REV2_16MODES_POT3_OPTIMIZED.ino`.
