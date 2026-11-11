# Gate Length Sculptor

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Gate correlati al periodo con pattern di accento, rotazione e variazione di durata.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Lunghezza base | Durata 2 ms..min(periodo-1,900 ms) · Lineare · Al fronte di trigger/clock |
| P2 | Lunghezza/accento secondario | Pattern 0..3, rotazione 0..15; livello debole 900..3000 Q12 · Selettore discreto · Al fronte di trigger/clock |
| P3 | Variazione width | Variazione 0..65% · Lineare · Al fronte di trigger/clock |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Clock |
| MANUAL (A3) | Come CV |
| OUT1 (MCP4822 A) | Gate scolpito |
| OUT2 (MCP4822 B) | Gate correlato/accento |
| I/O (D2, uscita) | Riferimento |

Livelli nominali: 0..+5 V nominali. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.

- Port REV5-2, acquisizione gate ADC con isteresi; debounce MANUAL.

## Standalone

Aprire [ALTERIA_rh_sculptor.ino](Standalone/ALTERIA_rh_sculptor/ALTERIA_rh_sculptor.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Gate Length Sculptor** (`rh_sculptor`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: scheduler. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_RHYTHM_ENGINE_328P/ALTERIA_RHYTHM_ENGINE_328P.ino`.
