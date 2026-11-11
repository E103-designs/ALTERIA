# Preset Manager a 8 step

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Otto preset persistenti di due tensioni bipolari e un gate. EDIT con pickup; EEPROM compatibile con ALLROUNDER 2.2.4. Funzione separata da ACPM.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | CV A con pickup ±8 ADC | -5..+5 V nominali · Lineare, pickup ±8 ADC · Solo EDIT e dopo pickup |
| P2 | CV B con pickup ±8 ADC | -5..+5 V nominali · Lineare, pickup ±8 ADC · Solo EDIT e dopo pickup |
| P3 | Gate, soglie 420/604 ADC | LOW <420 ADC, HIGH >604 ADC, centro mantiene · Isteresi · Solo EDIT e dopo pickup |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Prossimo preset solo in PLAY. |
| MANUAL (A3) | Prossimo degli otto preset anche in EDIT. |
| OUT1 (MCP4822 A) | CV A salvata |
| OUT2 (MCP4822 B) | CV B salvata |
| I/O (D2, uscita) | Gate salvato |

Livelli nominali: -5..+5 V nominali. LED1–3 mostrano l’indice 0–7. LED4 acceso in PLAY, lampeggiante in EDIT; tutti lampeggiano su errore.

- MODE 650 ms alterna EDIT/PLAY. Uscita da EDIT o dalla modalità salva. EEPROM 0..105, migra in lettura 128..189, nessuna scrittura al boot.

## Standalone

Aprire [ALTERIA_ar_presets8.ino](Standalone/ALTERIA_ar_presets8/ALTERIA_ar_presets8.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE lungo alterna PLAY ed EDIT. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Preset Manager a 8 step** (`ar_presets8`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: preset-v2. Torna in PLAY; mantiene valori e preset selezionato in RAM. Uscendo da EDIT avvia il salvataggio, code precedenti svuotate.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_2_4/ALTERIA_ALLROUNDER_2_2_4.ino`.
