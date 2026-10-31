# ALTERIA FULL VOICE 1.0.0

> **NON TESTATO SU HARDWARE / UNTESTED ON HARDWARE.** Variante sperimentale:
> compilazione e test software passati, nessun collaudo fisico di questo firmware.
> [Stato dettagliato](STATUS.md) · [Quick reference](../../docs/quick-reference/FULLVOICE.md).

2026-10-31. Firmware **separato** per Nano ATmega328P 16 MHz / PCB REV5-2.
Non modifica Allrounder, Drum Lab o i loro preset. Monofonico, digitale e lo-fi:
oscillatore -> drive -> filtro LP risonante -> VCA, con un inviluppo AD condiviso.
Non e un synth analogico, un clone 303, o un ingresso audio per filtrare sorgenti esterne.

## Collegamenti

| Jack | Funzione |
| --- | --- |
| CV IN | Pitch continuo, nominalmente 1 V/oct; circa C1 a 0 V e C6 a +5 V, fine tune al centro |
| I/O | **Sempre ingresso** gate/trigger positivo, anche cambiando pagina e in taratura |
| OUT1 | Audio monofonico filtrato e controllato dal VCA, nominalmente fino a circa +/-2.5 V |
| OUT2 | Inviluppo AD positivo nominale 0..+5 V, indipendente dall'attenuatore audio |

Un fronte crescente su I/O avvia il ciclo AD completo. Il gate tenuto non prolunga
la nota; il fronte discendente non interrompe la coda. Un nuovo fronte riparte
dal livello AD corrente, senza azzerare la fase dell'oscillatore. Se I/O e gia alto
all'avvio non viene generata una falsa nota: occorre prima abbassarlo.
CV IN aggiorna anche una nota gia in corso, senza quantizzazione o portamento.

Usare inizialmente gate di almeno **5 ms**, verificati nel simulatore AVR ma non
misurati sul PCB. Non si garantisce cattura di impulsi arbitrariamente brevi.
Gli ingressi non sono certificati per CV negative o superiori al rail del Nano:
verificare il proprio esemplare e usare sorgenti positive nel range collaudato.
Le uscite vanno a mixer/VCA/interfaccia adatti ai livelli Eurorack, non direttamente
a cuffie o altoparlanti. Prima di caricare altri firmware scollegare I/O: alcuni lo pilotano in uscita.

## Quattro pagine

I LED sono elencati nell'ordine fisico **LED1, LED2, LED3, LED4**; 1 = acceso.
Non sono i numeri binari dell'Allrounder. La navigazione non interrompe la nota.

| Pagina | LED | POT1 | POT2 | POT3 |
| --- | --- | --- | --- | --- |
| 1 OSC | 1000 | Sine / Triangle / Saw / Pulse, quattro zone da CCW a CW | Pulse width circa 10..90%, solo con Pulse; al centro square | Fine tune circa -100..+100 cent, centro neutro |
| 2 FILTER | 0100 | Cutoff LP, risposta approssimativamente esponenziale, circa 40 Hz..1.5 kHz | Risonanza crescente | Apertura positiva del filtro tramite AD |
| 3 VCA | 0010 | Livello oscillatore prima di drive/filtro; tutto CCW = silenzio | Quantita AD sul VCA: CCW = drone, CW = nota completamente inviluppata | Drive prima del filtro, da pulito a saturato |
| 4 ENV | 0001 | Attack, circa 2 ms..4 s | Decay, circa 2 ms..4 s | Curvatura continua LOG -> LIN al centro -> EXP |

**L'inviluppo e AD, non AR, ASR o ADSR: non esistono sustain/release o Decay
su un'altra pagina.** Tempi e curva vengono acquisiti al trigger e valgono per quel
ciclo; gli altri controlli sono live. L'AD pilota insieme filtro, VCA e OUT2.
Il suo picco interno e sempre piena scala; la breve levigatura audio puo ridurre
il picco fisico dei cicli molto veloci. I tempi minimi includono una piccola coda ant-click.

Con VCA POT2 tutto CCW l'oscillatore suona anche senza gate: e intenzionalmente
una modalita drone. Il trigger puo comunque muovere il filtro e OUT2.
Con VCA POT1 tutto CCW non si sente nulla, anche con inviluppo aperto.
L'Env Amount filtro apre dal cutoff impostato fino al limite del motore; se
POT1 FILTER e gia al massimo, POT3 non ha ulteriore spazio per aprire.
Drive e risonanza elevati possono produrre saturazione digitale, non un ladder analogico.

### Tasti

| Gesto | Risultato |
| --- | --- |
| MODE breve, al rilascio | Pagina successiva, da ENV torna OSC |
| MANUAL breve, al rilascio | Pagina precedente, da OSC torna ENV; non suona |
| MANUAL tenuto almeno 500 ms | Una nota di prova al pitch corrente, senza cambiare pagina; nessuna ripetizione se resta premuto |
| MODE tenuto almeno 1.2 s | Salva il suono corrente, senza cambiare pagina o fermare l'audio |
| Entrambi tenuti durante l'accensione | Taratura nascosta delle uscite, audio fermo |

I tempi sono successivi al debounce del pulsante. Entrambi i tasti premuti durante
l'uso normale non aprono altri menu: le azioni sono annullate fino al rilascio.
Il LED CV IN indica l'AD attivo, non la tensione di pitch; il LED I/O indica il
gate presente o MANUAL lungo tenuto. Non sono indicatori di intonazione.

### Recupero dei pot

Ogni pagina conserva i propri tre valori. Tornando a una pagina bisogna portare
ciascun pot vicino al valore memorizzato o attraversarlo prima di modificarlo.
Questo evita salti fra cutoff, livello e tempi. Se muovi un pot non ancora
acquisito, il LED di pagina lampeggia brevemente: continua a ruotarlo verso il
valore precedente. Vale anche all'accensione con la patch caricata.

Durante il salvataggio lampeggiano tutti i LED; alla fine c'e una breve conferma
con tutti e quattro. Un lampeggio rapido persistente segnala errore: riprovare
MODE lungo, non considerare salvato il suono. Il suono puo continuare a essere
modificato durante la scrittura, ma viene salvata **la fotografia al momento del gesto**.
All'avvio riparte sempre OSC con l'ultimo suono salvato, non con l'ultimo AD in corso.

## Prima prova

1. Collegare OUT1 a un ingresso audio con attenuazione adeguata. All'avvio il suono
   predefinito e una saw filtrata con attacco corto, decay di circa 260 ms e VCA inviluppato.
2. Collegare CV positivo a CV IN e gate a I/O, oppure tenere MANUAL per mezzo secondo.
   Senza pitch esterno il riferimento nominale e C1; il pitch di un jack flottante non e garantito.
3. MODE va a FILTER. Recuperare i pot, poi aprire cutoff e Env Amount; provare la risonanza.
4. Andare a ENV per cambiare Attack, Decay e curva. Non cercare il Decay su VCA.
5. Tenere MODE 1.2 s per salvare. Per conservare la modifica attendere la conferma dei LED.

## Installazione e memoria persistente

Aprire `ALTERIA_FULL_VOICE.ino`, selezionare **Arduino Nano / ATmega328P**.
Se necessario usare **Old Bootloader** per il caricamento. Servono solo SPI ed
EEPROM del core Arduino AVR; non installare Mozzi o driver DAC esterni.
Non caricare su ATmega168P, LGT8F328P o Nano Every.

La taratura ACAL precedente viene letta da **256..301**. Per crearla o modificarla
seguire [GUIDA_CALIBRAZIONE.md](GUIDA_CALIBRAZIONE.md).
La singola patch usa due record CRC/commit da 34 byte a **320 e 360**, ultimo
byte 393; riserva 320..399. Non scrive preset Allrounder 128..189 o kit Drum Lab
512..939. Record interrotto/corrotto: ripiega sull'altro valido o sui default;
nessun salvataggio automatico dei default. Una EEPROM cancellata da programmazione
o riutilizzata da altri firmware puo perdere patch e taratura.

## Limiti e scelta del motore

Motore dedicato a **6.4 kHz**, DDS a tabelle multibanda, PWM da differenza di saw,
interpolazione, filtro SVF a due substep, levigatura dei controlli e crossfade breve
tra le forme. Non ci sono sequencer, polifonia, effetti, modulation matrix o audio-in.
Il limite teorico di Nyquist e 3.2 kHz; il filtro arriva a circa 1.5 kHz. Timbro
deliberatamente lo-fi: aliasing/immagini di campionamento, cambi banda armonica e
distorsione non sono eliminati. Nessun filtro analogico di ricostruzione e stato
aggiunto; l'uscita reale deve essere ascoltata/misurata.

[Mozzi supporta DAC esterni e ATmega328](https://github.com/sensorium/Mozzi), ma
non e necessario qui: si mantiene il percorso SPI/LDAC dei firmware gia usati
sul modulo, con temporizzazione verificata sul binario. Non e un giudizio di
superiorita generale rispetto a Mozzi; nessun suo codice e incorporato.

**1 V/oct e nominale, non certificato.** La taratura DAC delle uscite non calibra
ADC, reference, condizionamento o clock. La risoluzione ideale a 5 ottave su ADC
10 bit e circa 5.9 cent per passo, prima di rumore/errori. Il fine tune corregge
l'offset musicalmente, non l'errore di scala; per tracking accurato misurare CV
contro letture ADC e aggiornare `CV_IN_ADC_ZERO` / `CV_IN_ADC_PLUS5` nel sorgente.
Nessun valore di misura e stato inventato.

Build, prove riproducibili e test mancanti in [TEST_REPORT.md](TEST_REPORT.md).
