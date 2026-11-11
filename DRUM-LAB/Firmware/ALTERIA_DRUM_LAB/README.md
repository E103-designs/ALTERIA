# ALTERIA DRUM LAB 1.0.0

2026-10-27. Firmware separato per **ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz**. Non sostituisce i sorgenti Allrounder.

**Due sintetizzatori di percussioni indipendenti, otto motori assegnabili a ciascuno.** Nessun VCO o VCA esterno necessario per produrre audio. Implementazione originale a punto fisso ispirata alle famiglie di sintesi LXR, non emulazione fedele, porting del firmware Erica Synths o prodotto ufficiale. Leggere [ATTRIBUTION.md](ATTRIBUTION.md) per fonti e differenze.

## Partenza in un minuto

1. Aprire `ALTERIA_DRUM_LAB.ino` nell'IDE. Scheda **Arduino Nano / ATmega328P**; Old Bootloader solo se necessario per il caricamento. Dipendenze: SPI/EEPROM del core Arduino AVR. Non ATmega168P.
2. OUT1 e OUT2 a due ingressi audio di un mixer/VCA, con livello inizialmente attenuato. **Non collegare direttamente cuffie o altoparlanti.** Il target software e circa +/-2.5 V, non 0..+5 V di trigger.
3. Al primo avvio senza kit salvato: **A = kick, B = snare**, pagina SOUND, modifica A. MANUAL suona il kick.
4. MODE tenuto circa 0.65 s seleziona B: LED4 acceso. Ora MANUAL suona lo snare.
5. Clock/trigger positivo su **CV IN -> A -> OUT1**. Altro trigger su **I/O -> B -> OUT2**. I due percorsi non cambiano quando si seleziona la voce da modificare.
6. Provare P1 pitch, P2 durata, P3 carattere. I pot hanno **pickup**: dopo un richiamo non modificano il suono finche non raggiungono o attraversano il valore memorizzato. Se un pot sembra fermo, percorrere lentamente tutta la corsa per agganciarlo.

L'utente ha confermato funzionante il percorso DAC dell'Allrounder 2.0.1. Qui si conserva SPI.transfer/digitalWrite e l'impulso LDAC esteso; il nuovo firmware richiede comunque caricamento e ascolto sul modulo. Non sono dichiarati collaudi fisici di DRUM LAB da parte dell'assistente.

## Pulsanti

| Gesto | Azione |
| --- | --- |
| MODE breve, azione al rilascio | Motore successivo **solo sulla voce selezionata**, poi ritorna alla pagina SOUND |
| MODE circa 650 ms | Seleziona A/B, senza cambiare motore; ritorna alla pagina SOUND |
| MODE premuto, poi MANUAL **entro mezzo secondo**, rilascio di entrambi | Alterna pagina SOUND / FX, senza colpo o avanzamento del motore |
| MANUAL breve | Suona **solo la voce selezionata**, diversamente dal modo Drums dell'Allrounder |
| MANUAL tenuto 2 s | Dopo il colpo iniziale salva il kit; non genera un roll |
| MODE + MANUAL gia tenuti all'accensione | Taratura nascosta, con audio fermo |

Il cambio motore interrompe la coda della voce modificata; l'altra continua. Ritornando a un motore si ritrovano i suoi sei parametri in RAM. Il salvataggio include entrambe le assegnazioni e tutti gli otto suoni di ciascuna voce, non soltanto la pagina visibile. Nessun salvataggio automatico ad ogni movimento dei pot.

Durante il salvataggio tutti i LED superiori lampeggiano; successo: tutti accesi per circa 0.7 s. Errore di rilettura: lampeggio piu rapido, per 0.7 s. Aspettare la conferma prima di spegnere. Le modifiche successive all'inizio del salvataggio richiedono un nuovo salvataggio. All'avvio si torna sempre alla modifica A / pagina SOUND.

## LED

**LED1, LED2, LED3**, in questo ordine fisico, rappresentano i pesi **1, 2, 4** del motore 1..8, modulo 8. `1` = acceso, `0` = spento. Il motore 8 mostra `000`, non e un guasto.

| Motore | LED1 LED2 LED3 | SOUND P1 | SOUND P2 | SOUND P3 |
| --- | --- | --- | --- | --- |
| 1 Kick | 1 0 0 | Fondamentale 45..135 Hz | Coda corta -> lunga | PM e attacco, dal rotondo al piu incisivo |
| 2 Tom | 0 1 0 | Intonazione 70..500 Hz | Decadimento | Da puro a modulato |
| 3 Synth Perc | 1 1 0 | Intonazione 100..1400 Hz | Decadimento | Morph seno/triangolo/saw/pulse con PM |
| 4 Snare | 0 0 1 | Intonazione delle due risonanze | Coda della cordiera | Corpo -> rumore, senza distorsione automatica |
| 5 Clap | 1 0 1 | Distanza dei quattro battiti, circa 5..25 ms | Coda finale | Intensita dei tre battiti secondari |
| 6 FM Metal | 0 1 1 | Intonazione | Coda, anche lunga | Indice PM e secondo modulatore seno -> noise |
| 7 Closed Hat | 1 1 1 | Colore/intonazione metallica | Durata breve | Metallo -> piu rumore |
| 8 Open Hat | 0 0 0 | Colore/intonazione metallica | Durata lunga | Metallo -> piu rumore |

**LED4 = voce in modifica**, non quarto bit del motore:

| Pagina | A / OUT1 | B / OUT2 |
| --- | --- | --- |
| SOUND | LED4 spento | LED4 acceso |
| FX | Breve lampo acceso ogni mezzo secondo | Acceso con breve interruzione ogni mezzo secondo |

I LED CV IN e I/O mostrano un lampo per il trigger della rispettiva voce, compreso MANUAL; non indicano la voce in modifica. Per capire se un gate e riconosciuto, guardare questi LED. Un closed hat puo sopprimere intenzionalmente un open hat anche se il LED del trigger open si accende.

## Pagina FX

Ogni motore di ogni voce ha tre effetti indipendenti, memorizzati e richiamati con pickup.

| Pot | Funzione |
| --- | --- |
| P1 | **Tone**: a sinistra low-pass, al centro bypass, a destra high-pass. Sugli snare/clap filtra il rumore, non il corpo tonale dello snare. Agli estremi puo attenuare molto il suono, soprattutto il kick con high-pass |
| P2 | **Drive**: crossfade progressivo verso la saturazione; tutto chiuso = nessun drive |
| P3 | **Crush wet**: crossfade progressivo verso 16 livelli e sample-hold fisso di 8 campioni; tutto chiuso = nessun crush |

Non c'e un cambio improvviso di frequenza del sample-hold a meta corsa: aumenta il contributo dell'effetto. I parametri hanno comunque risoluzione finita e il morph delle forme d'onda non e band-limited. Drive/crush sono volutamente distruttivi se aperti; prima ascoltare con entrambi chiusi. Cambiare pagina non attiva effetti da solo.

## Coppie da provare

- **Kick / snare**, default: kick P3 basso; snare P3 intorno a meta, drive e crush chiusi. Due ritmi esterni, due suoni indipendenti.
- **Kick / closed hat**: selezionare B con MODE lungo; dal suo snare premere MODE tre volte. I/O diventa il trigger del charleston, OUT2 il suo audio.
- **Closed / open hat**: impostare i due motori sulle due voci. Il closed interrompe rapidamente la coda dell'open; se arrivano insieme vince il closed. Funziona con i ruoli A/B invertiti. Non c'e choke tra motori non hat.
- **Tom / FM Metal**: accordare P1, allungare P2, aprire gradualmente P3 della voce FM. Provare il filtro prima del crush per conservare il corpo.
- **Synth Perc / clap**: sul perc muovere lentamente P3; sul clap P1 regola il flam e P3 la forza degli attacchi secondari. Ottimo per due sequenze irregolari.

Il firmware non contiene un sequencer autonomo o il sequencer dell'LXR: per pattern ripetuti servono trigger esterni. MANUAL permette di ascoltare e programmare ogni suono senza patch di clock.

## Anteprima

`output/audio/ALTERIA_DRUM_LAB_preview.wav` nella radice ALTERIA: **render software, non registrazione del modulo**. Primi 12 secondi: gli otto motori in ordine, due colpi ciascuno. Poi quattro groove: kick/snare, kick/hat, tom/FM, perc/clap. Nei groove A a sinistra e B a destra. Serve a valutare il carattere lo-fi prima del caricamento; stadio analogico e livelli fisici non sono simulati.

## Taratura e memoria

Legge i record **ACAL 1** gia usati da Allrounder/demo_CAL a EEPROM 256..301. Se entrambi sono invalidi usa i default teorici, non misure specifiche della scheda. Non converte le vecchie letture in coefficienti inventati.

Per tarare da questo firmware: tenere MODE+MANUAL al boot per almeno 100 ms dall'inizio dello sketch, poi rilasciare. Audio fermo, I/O sempre INPUT. LED1 = zero, LED2 = +5 V, LED3 = -5 V. P1 corregge OUT1, P2 OUT2; partire centrati, orario alza la tensione. Tornare al centro prima di MANUAL breve per avanzare. MODE breve torna indietro; MODE 1.2 s annulla. LED4 lampeggiante finale: MANUAL 1 s salva. Due tasti per 3 s durante la procedura ripristinano i default nella sessione, non li salvano subito. Dopo il salvataggio riparte l'audio. Servono multimetro e riferimenti esterni; non e autotaratura.

Kit DRUM LAB a due record con CRC, sequenza e commit finale: **512..715 e 736..939**. Non scrive i preset Allrounder a 128..189. Formati kit non intercambiabili. Il ritorno a un altro firmware non implica che questo mantenga I/O come ingresso: scollegare driver esterni prima di caricare firmware differenti.

## Limiti da conoscere

- Due voci simultanee, non sei. Tutte le famiglie base rappresentate, non tutti i parametri, algoritmi di filtro o effetti originali.
- Audio a 6.4 kHz, oscillatori tabulati e livello audio a risoluzione ridotta nel DAC 12 bit. Banda teorica sotto 3.2 kHz e aliasing delle forme ricche/FM: **lo-fi**, non qualita o timbro identico LXR-02.
- Filter semplici non risonanti, nessun SVF completo LXR. Niente import sample/kit, MIDI, mod matrix, LFO per voce, master delay/compressor/ringmod o uscita stereo per singola voce.
- CV IN e I/O sono trigger, non ingressi audio, non controlli continui di pitch e non velocity campionata. Soglie CV software circa ADC 388/225, pari a circa 1.9/1.1 V al pin con reference 5 V; non certificano il range sicuro del jack.
- I/O D2 resta INPUT per tutta questa variante, anche in taratura. Non c'e un terzo output audio o gate.
- Per le prime prove usare trigger positivi di almeno 5 ms. Impulsi piu brevi, carichi e stabilita dell'alimentazione vanno verificati sul PCB. Non applicare tensioni negative o oltre rail al pin Nano.
- Build e simulazioni non misurano banda analogica, rumore, livelli, jitter o margine elettrico reale. Vedere [TEST_REPORT.md](TEST_REPORT.md).
