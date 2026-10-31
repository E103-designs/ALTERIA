# DRUM LAB 1.0.0 - Quick Reference

Due voci simultanee A/B; ciascuna puo usare uno degli otto motori. La voce in
modifica non cambia il routing dei jack. Pagine **SOUND** e **FX**, pot con pickup.
All'avvio senza kit: A kick, B snare, modifica A / SOUND.

## SOUND

LED1,2,3 in ordine fisico; `1` acceso. LED4 indica voce/pagina, non il motore.

| Motore | LED1..3 | POT1 | POT2 | POT3 |
| --- | --- | --- | --- | --- |
| 1 Kick | 100 | Fondamentale 45..135 Hz | Decay | PM / incisivita attacco |
| 2 Tom | 010 | Pitch 70..500 Hz | Decay | Puro -> modulato |
| 3 Synth Perc | 110 | Pitch 100..1400 Hz | Decay | Morph sine/triangle/saw/pulse con PM |
| 4 Snare | 001 | Pitch delle risonanze | Decay cordiera | Corpo -> rumore |
| 5 Clap | 101 | Distanza dei quattro battiti | Coda finale | Intensita battiti secondari |
| 6 FM Metal | 011 | Pitch | Decay | Indice PM / secondo modulatore verso noise |
| 7 Closed Hat | 111 | Colore / pitch metallico | Durata breve | Metallo -> rumore |
| 8 Open Hat | 000 | Colore / pitch metallico | Durata lunga | Metallo -> rumore |

## FX, per ogni motore di ogni voce

| POT1 | POT2 | POT3 |
| --- | --- | --- |
| Tone: LP a sinistra, bypass al centro, HP a destra | Drive: mix progressivo di saturazione | Crush: mix di 16 livelli e hold fisso 8 campioni |

Snare/clap: Tone filtra il rumore; il corpo tonale snare resta separato.
Drive/crush a zero = effetto chiuso. HP sul kick puo attenuarlo molto.

## Tasti e LED4

| Gesto | Azione |
| --- | --- |
| MODE breve, al rilascio | Prossimo motore della voce selezionata; torna SOUND |
| MODE 650 ms | Seleziona A/B; torna SOUND |
| Tenere MODE, aggiungere MANUAL entro 500 ms, rilasciare entrambi | Alterna SOUND / FX, senza suonare |
| MANUAL breve | Suona solo la voce in modifica |
| MANUAL 2 s | Suona il colpo iniziale e salva tutto il kit, senza roll |

LED4 SOUND: spento = A, acceso = B. In FX: breve lampo acceso per A, breve
interruzione per B ogni mezzo secondo. I due LED dei jack indicano i rispettivi
trigger, incluso MANUAL. Salvataggio: tutti lampeggianti, poi conferma breve.

## Jack

| CV IN | I/O | OUT1 | OUT2 |
| --- | --- | --- | --- |
| Trigger voce A | **Sempre ingresso** trigger voce B | Audio A circa +/-2.5 V | Audio B circa +/-2.5 V |

Non sono CV pitch, velocity o ingressi audio. Nessun sequencer interno.
Closed hat interrompe la coda open hat; su trigger simultanei vince closed,
anche con assegnazioni A/B invertite. Cambio motore tronca solo la voce modificata.
Se un pot non risponde, raggiungere/attraversare il valore memorizzato.

Taratura: MODE+MANUAL al boot, poi rilascio; tre punti con tester, P1 OUT1 / P2
OUT2, centratura prima di MANUAL. LED4 finale lampeggiante: MANUAL 1 s salva;
MODE 1.2 s annulla. I/O resta ingresso. Scollegarlo prima di cambiare firmware.

Audio lo-fi 6.4 kHz; non replica LXR-02. Prove software non equivalgono a
collaudo fisico del nuovo motore.
[Manuale completo](../../firmware/ALTERIA_DRUM_LAB/README.md) ·
[Indice quick reference](README.md).
