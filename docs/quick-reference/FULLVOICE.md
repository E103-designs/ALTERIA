# FULL VOICE 1.0.0 - Quick Reference

> **NON TESTATO SU HARDWARE / UNTESTED ON HARDWARE.** Build e test software
> passati; suono, livelli, jitter e tracking V/oct sul modulo da verificare.

Una sola voce: oscillatore -> drive -> filtro LP risonante -> VCA.
Quattro pagine dello stesso suono, non quattro engine separati.
LED in ordine fisico **LED1,2,3,4**; `1` acceso.

| Pagina | LED | POT1 | POT2 | POT3 |
| --- | --- | --- | --- | --- |
| OSC | 1000 | Sine / Triangle / Saw / Pulse | Pulse width circa 10..90%, solo Pulse | Fine tune circa -100..+100 cent; centro neutro |
| FILTER | 0100 | Cutoff LP circa 40 Hz..1.5 kHz | Risonanza | Apertura filtro via AD |
| VCA | 0010 | Livello oscillatore pre-filtro | AD amount: CCW drone, CW inviluppato | Drive pre-filtro |
| ENV | 0001 | Attack circa 2 ms..4 s | Decay circa 2 ms..4 s | Curva LOG -> LIN al centro -> EXP |

**AD, non AR/ADSR: nessun sustain o release. Attack e Decay sono entrambi
nella pagina ENV.** Tempi e curva acquisiti al trigger; altri controlli live.
Il gate breve avvia il ciclo completo, tenuto non lo prolunga e il rilascio
non lo tronca. Retrigger dal livello corrente.

## Jack

| CV IN | I/O | OUT1 | OUT2 |
| --- | --- | --- | --- |
| Pitch continuo nominale 1 V/oct, circa C1..C6 su 0..+5 V | **Sempre ingresso** gate/trigger positivo | Audio filtrato/VCA circa +/-2.5 V | AD positivo 0..+5 V |

CV IN non eccita note. Nessun audio-in, sequencer o terzo DAC. Per le prime
prove usare gate positivi di almeno 5 ms entro i limiti verificati della scheda.
ACAL corregge le uscite, **non** il tracking dell'ingresso pitch.

## Tasti

| Gesto | Azione |
| --- | --- |
| MODE breve, al rilascio | Pagina successiva, ENV -> OSC |
| MANUAL breve, al rilascio | Pagina precedente, OSC -> ENV; non suona |
| MANUAL almeno 500 ms | Una nota di prova al pitch corrente, senza navigare |
| MODE almeno 1.2 s | Salva la patch senza cambiare pagina o fermare l'audio |
| MODE+MANUAL al boot | Taratura nascosta con audio fermo |

Pot con **pickup**: dopo cambio pagina/boot raggiungere o attraversare il valore
salvato. Breve lampeggio pagina = pot non ancora acquisito. Salvataggio: tutti
i LED lampeggiano, poi conferma; lampeggio rapido persistente = errore.
LED CV IN = AD attivo; LED I/O = gate presente / MANUAL lungo tenuto.

Prima prova: OUT1 a mixer attenuato, pitch positivo a CV IN, gate a I/O; oppure
tenere MANUAL 500 ms. Default: saw con AD corto. VCA POT1 tutto CCW e silenzio;
VCA POT2 tutto CCW e invece **drone intenzionale**, anche senza gate.
Un jack pitch flottante puo dare intonazione instabile.

Taratura con tester: LED1 zero, LED2 +5 V, LED3 -5 V; P1/P2 correggono OUT1/2,
centrare prima di MANUAL per avanzare. LED4 lampeggiante: MANUAL 1 s salva;
MODE 1.2 s annulla. I/O rimane ingresso. Suono lo-fi a 6.4 kHz.

[Manuale completo](../../firmware/ALTERIA_FULL_VOICE/README.md) ·
[Stato non testato](../../firmware/ALTERIA_FULL_VOICE/STATUS.md) ·
[Indice quick reference](README.md).
