# Full Voice · 4 pagine

**Versione 1.0** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

Voce monofonica con quattro pagine di tre pot: OSC, FILTER, VCA, ENV.

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
| P1 | OSC: forma; FILTER: cutoff; VCA: livello; ENV: Attack | OSC 4 forme; FILTER circa 40 Hz..1,5 kHz; VCA 0..100%; ENV 2 ms..4 s · Selettore OSC; cutoff logaritmico; VCA lineare; tempi quadratici · Live con pickup; Attack acquisito al trigger |
| P2 | OSC: PWM; FILTER: risonanza; VCA: AD amount; ENV: Decay | OSC PWM 10..90% solo pulse (inutilizzato sulle altre forme); FILTER risonanza; VCA AD amount 0..100%; ENV 2 ms..4 s · Lineare; Decay quadratico · Live con pickup; Decay acquisito al trigger |
| P3 | OSC: fine tune; FILTER: Env Mod; VCA: drive; ENV: curva | OSC circa ±100 cent; FILTER AD amount 0..100%; VCA drive; ENV LOG/LIN/EXP continua · Lineare / curva continua · Live con pickup; curva ENV acquisita al trigger |

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | Pitch continuo nominale 1 V/oct, ADC 0..+5 V; non certificato. |
| MANUAL (A3) | Breve: pagina indietro; 500 ms nota di prova. |
| OUT1 (MCP4822 A) | Audio voce circa +/-2,5 V |
| OUT2 (MCP4822 B) | Inviluppo AD 0..+5 V |
| I/O (D2, ingresso) | Ingresso gate/trigger |

Livelli nominali: Audio circa +/-2,5 V; ENV 0..+5 V. 800 ms codice slot globale; poi pagina one-hot (Full Voice) o motore/voce/pagina (Drum Lab).

- MODE breve pagina successiva; MODE 1,2 s salva patch. Pickup a ogni pagina.
- MODE+MANUAL tenuti 2 s: prossimo slot globale. I gesti interni sono mantenuti. LED globali per 800 ms all’ingresso, poi pagina/motore.

## Standalone

Aprire [ALTERIA_au_full_voice.ino](Standalone/ALTERIA_au_full_voice/ALTERIA_au_full_voice.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere `firmware.hpp` nella stessa cartella. Il modo si avvia da solo. MODE conserva le funzioni del motore. La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **Full Voice · 4 pagine** (`au_full_voice`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: timer1-audio, SPI-exclusive, patch-320. Stato re-inizializzato entrando; code/eventi del modo precedente cancellati, nessun trigger sintetico.

Origine: `firmwares/ALTERIA_FULL_VOICE/ALTERIA_FULL_VOICE.ino`.
