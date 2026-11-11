# Drum Lab · 8 motori

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Due voci audio, otto motori assegnabili per voce, pagine SOUND e FX con pickup.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | SOUND: pitch/colore; FX: filtro | Dipende dal motore; FX LP / bypass / HP · Macro specifica del motore · Live con pickup dopo cambio motore/voce/pagina |
| P2 | SOUND: decay; FX: drive | Decay del motore; FX saturazione dry/wet 0..100% · Macro specifica del motore · Live con pickup |
| P3 | SOUND: carattere; FX: crush | Carattere del motore; FX crush dry/wet 0..100%, 16 livelli / hold 8 campioni · Macro specifica del motore · Live con pickup |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Trigger voce A |
| MANUAL (A3) | Colpo sulla voce selezionata; 2 s salva kit. |
| OUT1 (MCP4822 A) | Audio voce A circa +/-2,5 V |
| OUT2 (MCP4822 B) | Audio voce B circa +/-2,5 V |
| I/O (D2, ingresso) | Ingresso trigger B |

Livelli nominali: Audio circa +/-2,5 V. 800 ms codice slot globale; poi pagina one-hot (Full Voice) o motore/voce/pagina (Drum Lab).

- MODE breve prossimo motore; 650 ms voce A/B; MODE poi MANUAL: SOUND/FX. Pickup dei pot.
- MODE+MANUAL tenuti 2 s: prossimo slot globale. I gesti interni sono mantenuti. LED globali per 800 ms all’ingresso, poi pagina/motore.
- SOUND P1/P2: Kick 45..135 Hz / 60..1800 ms; Tom 70..500 Hz / 50..1400 ms; Synth Perc 100..1400 Hz / 20..1200 ms; Snare 185..255 Hz / 60..1000 ms; Clap spread 32..160 campioni / 70..700 ms; FM Metal e Open Hat 250..1750 Hz / 100..2200 ms; Closed Hat 250..1750 Hz / 15..300 ms. Decay quadratico. P3 carattere 0..255; sul Clap regola i battiti secondari.

## Standalone

Aprire [ALTERIA_au_drum_lab.ino](Standalone/ALTERIA_au_drum_lab/ALTERIA_au_drum_lab.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE conserva le funzioni del motore. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Drum Lab · 8 motori** (`au_drum_lab`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: timer1-audio, SPI-exclusive, kit-512. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_DRUM_LAB/ALTERIA_DRUM_LAB.ino`.
