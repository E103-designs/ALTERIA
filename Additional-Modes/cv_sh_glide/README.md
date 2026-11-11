# S&H Glide

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Un trigger sceglie una tensione casuale; seconda uscita con glide e quantizzazione.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Escursione random | Range ±0..2047 Q12 attorno al centro · Lineare · Al fronte di trigger/clock |
| P2 | Glide | Glide 0..450 ms · Lineare · Live, ogni 12 ms |
| P3 | Quantizzazione | Continua con ADC <150; 24/12/8/5/3 livelli alle soglie 150/340/520/700/880 · Selettore discreto · Al fronte di trigger/clock |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Fronte crescente: sceglie una nuova CV casuale. |
| MANUAL (A3) | Sceglie una nuova CV casuale come CV IN. |
| OUT1 (MCP4822 A) | Sample stepped positivo |
| OUT2 (MCP4822 B) | Sample slewed positivo |
| I/O (D2, uscita) | Sample trigger |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.

- OUT1 salta al nuovo valore quantizzato; OUT2 raggiunge lo stesso valore con glide; I/O segnala il nuovo campione.

## Standalone

Aprire [ALTERIA_cv_sh_glide.ino](Standalone/ALTERIA_cv_sh_glide/ALTERIA_cv_sh_glide.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **S&H Glide** (`cv_sh_glide`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: scheduler. All’ingresso OUT1 e OUT2 partono a metà scala; nessun trigger sintetico.

Origine: `firmwares/ALTERIA_REV2_16MODES_POT3_OPTIMIZED.ino`.
