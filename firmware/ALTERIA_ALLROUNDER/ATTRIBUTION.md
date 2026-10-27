# Fonti e adattamenti

- Base locale: ALTERIA_demo_CAL 1.2.0, derivato da ALTERIA_demo 1.1.0; sorgenti precedenti conservati.
- Burst e Humanizer recuperati da ALTERIA_RHYTHM_ENGINE_328P; Tremolo/AutoPan da ALTERIA_REV2_16MODES_POT3_OPTIMIZED, adattati al backend REV5-2 senza mirror PWM. Tremolo regolare ora segue subito la velocita quando P3 e zero.
- Acid: adattamento nativo AVR ispirato a https://github.com/drakh/acid-generator, branch dev consultato il 2026-10-26. Riferimenti: `src/audio-engine/generator.ts`, `src/audio-engine/scales.ts`, `src/constants.ts`, `LICENSE` (CC0-1.0). Il progetto upstream genera pattern sparsi con pool di gradi, ottave, accenti e slide; scala predefinita frigia, 16 step. Questa variante reimplementa il comportamento generativo a tre macro in C++ per MCU, non incorpora Tone.js/lodash, non riproduce l'app o il suo synth, non e un port bit-per-bit. Si mantengono attribuzione e link all'upstream senza dichiarare affiliazione.
- Triple Clock e Mutant Drums: nuovi motori per questa variante. Nessun codice/sample di Instruo ochd, 808 o 909 incorporato; la proposta LFO e stata sostituita dalla modalita Drums.
