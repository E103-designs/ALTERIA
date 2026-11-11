# Acid Voice 2.1.0

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Voce monofonica acid con AD, filtro SVF risonante e inviluppo CV separato.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | Decay 20..2000 ms | Decay 20..2000 ms · Quadratica · Live, campionamento dei pot ogni 12 ms |
| P2 | Risonanza SVF | Damping SVF 469..55 / 256 · Lineare · Live, campionamento dei pot ogni 12 ms |
| P3 | Apertura filtro con AD | Env Mod 0..172 / 256 · Lineare · Live, campionamento dei pot ogni 12 ms |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Pitch continuo nominale 1 V/oct, ADC 0..+5 V; non certificato. |
| MANUAL (A3) | Alterna saw/square; non eccita la voce. |
| OUT1 (MCP4822 A) | Audio voce circa +/-2,5 V |
| OUT2 (MCP4822 B) | Inviluppo AD 0..+5 V |
| I/O (D2, ingresso) | Ingresso gate/trigger |

Livelli nominali: Audio circa +/-2,5 V; ENV 0..+5 V. LED1…4: pesi 1/2/4/8. Lo slot 16 ha codice 0000; in standalone si usa il codice del primo slot.

- Trigger I/O avvia AD anche a gate tenuto; niente sustain. Motore 6,4 kHz.
- Saw/square resta in RAM tra i cambi modalità, a boot riparte saw.

## Standalone

Aprire [ALTERIA_au_acid_voice.ino](Standalone/ALTERIA_au_acid_voice/ALTERIA_au_acid_voice.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE breve non scorre modalità. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Acid Voice 2.1.0** (`au_acid_voice`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: timer1-audio, SPI-exclusive. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_ALLROUNDER_2_1_0/ALTERIA_ALLROUNDER_2_1_0.ino`.
