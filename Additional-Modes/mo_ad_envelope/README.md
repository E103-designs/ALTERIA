# Ad Envelope · Motion

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

AD da zero con curva continua e uscita inversa, diverso da AD Shaper.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Attack log 5..4000 ms | Attack 5..4000 ms · Logaritmica · Al fronte di trigger/clock |
| P2 | Decay log 5..4000 ms | Decay 5..4000 ms · Logaritmica · Al fronte di trigger/clock |
| P3 | Curva continua | LOG..LIN..EXP, esponente 1..5 · Continua, centro piatto 0,48..0,52 · Live, ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Trigger da zero |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | Envelope |
| OUT2 (MCP4822 B) | Inversa |
| I/O (D2, uscita) | EOC pulse |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_mo_ad_envelope.ino](Standalone/ALTERIA_mo_ad_envelope/ALTERIA_mo_ad_envelope.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Ad Envelope · Motion** (`mo_ad_envelope`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: motion-math. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_MOTION_ENGINE_POT3/ALTERIA_MOTION_ENGINE_POT3.ino`.
