# Euclidean Trigs

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Tre sequencer di trigger euclidei indipendenti da 16 step. Ogni pot controlla il numero di hit della propria uscita.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Densità OUT1 | 0–16 hit su 16 step · 17 settori ADC · A ogni fronte CV IN |
| P2 | Densità OUT2 | 0–16 hit su 16 step · 17 settori ADC · A ogni fronte CV IN |
| P3 | Densità I/O | 0–16 hit su 16 step · 17 settori ADC · A ogni fronte CV IN |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Solo il fronte crescente del clock esterno avanza tutte e tre le sequenze. Nessun clock interno. |
| MANUAL (A3) | Ferma gli impulsi attivi e riporta tutti allo step 1. Lo step 1 suona al prossimo fronte CV IN; se coincidono, prevale il reset. |
| OUT1 (MCP4822 A) | Trigger della corsia P1 tramite DAC A |
| OUT2 (MCP4822 B) | Trigger della corsia P2 tramite DAC B |
| I/O (D2, uscita) | Trigger della corsia P3 su D2 |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.

- Ogni sequenza distribuisce 0–16 hit su 16 step, con rotazione fissa a zero. Stessa densità = stesso pattern.
- Impulsi di un quarto del periodo, limitati a 2–12 ms. Il primo fronte dopo reset usa 12 ms.

## Standalone

Aprire [ALTERIA_rh_euclidean_trigs.ino](Standalone/ALTERIA_rh_euclidean_trigs/ALTERIA_rh_euclidean_trigs.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Euclidean Trigs** (`rh_euclidean_trigs`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: scheduler. Ricomincia dallo step 1 senza emettere impulsi; MANUAL ripete questo reset.

Origine: `firmwares/ALTERIA_RHYTHM_ENGINE_328P/ALTERIA_RHYTHM_ENGINE_328P.ino`. Derivata da Euclidean Morph; modalità rivista per tre densità indipendenti e reset comune.
