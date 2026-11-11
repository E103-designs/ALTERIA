# Acid Generator

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Sequencer CV frigio a 16 step con pause, accenti e slide; nessuna sintesi audio.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Densità | 0..16 note attive · Lineare · Alla generazione della frase |
| P2 | Spread note/ottave | Pool 0..6, massimo 0..2 ottave · Lineare · Alla generazione della frase |
| P3 | Slide / accenti | Probabilità slide/accento 0..circa 50% · Lineare · Alla generazione della frase |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Clock: avanza frase frigia di 16 step. |
| MANUAL (A3) | Rigenera frase e torna al primo step. |
| OUT1 (MCP4822 A) | Pitch CV |
| OUT2 (MCP4822 B) | Accento |
| I/O (D2, uscita) | Gate |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.



## Standalone

Aprire [ALTERIA_ar_acid.ino](Standalone/ALTERIA_ar_acid/ALTERIA_ar_acid.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Acid Generator** (`ar_acid`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: nessun adapter aggiuntivo. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_2_1/ALTERIA_ALLROUNDER_2_2_1.ino`.
