# ALTERIA Firmware Builder 3.1.0

Webapp locale in italiano per comporre **1..16 slot** da un catalogo di **47 modalità**, con sorgenti Arduino, configurazione versionata, cheatsheet HTML/PDF, compilazione AVR, HEX e misure flash/SRAM. Una sola modalità viene eseguita alla volta.

## Avvio

Richiede Node.js 18 o successivo. Nessuna dipendenza npm per utilizzare l'app.

```sh
cd Firmware-Builder
npm start
```

Aprire **http://127.0.0.1:4173**. In alternativa, doppio clic su `Avvia Builder.command`. Il servizio ascolta solo sul computer locale; non pubblica il sito e non esegue upload sulla scheda. `PORT=4174 npm start` cambia la porta. Dopo modifiche al generatore o al compilatore riavviare il servizio; dopo modifiche ai dati o all'interfaccia ricaricare la pagina.

La webapp può essere servita anche con un server statico: composizione, configurazioni, sorgenti ZIP e PDF funzionano nel browser, senza API di compilazione. Non aprire direttamente `index.html` con `file://`, perché vengono caricati moduli e JSON.

## Uso

Cercare per nome o controllo, filtrare per famiglia, aprire i dettagli e il codice sorgente prima di scegliere. La selezione iniziale è **ALLROUNDER 2.3.0** nel suo ordine, con Euclidean Trigs in 10 e S&H Glide in 16. Aggiungere/rimuovere modalità, trascinare gli slot o utilizzare le frecce. Il diciassettesimo inserimento viene impedito anche in importazione e nell'API.

Nome e versione sono inclusi nella configurazione esportata, nel pacchetto e nella guida. “Salva nel browser” conserva configurazioni nominate; “Esporta JSON” produce un file indipendente, importabile senza cambiare ordine. I salvataggi browser non sono sincronizzati tra dispositivi. Le configurazioni 3.0.0 restano importabili: l’ID di Euclidean Morph viene migrato a Euclidean Trigs, con il nuovo comportamento. ORBIT e Preset Manager restano selezionati se presenti nel file. Le altre versioni sconosciute vengono rifiutate.

“Scarica pacchetto sorgenti” funziona sempre, anche senza compilatore. “Compila e scarica” aggiunge HEX solo se la compilazione e i budget passano. In caso di errore scarica sorgenti, guida e report, mantenendo tutte le modalità scelte.

Il pacchetto contiene:

- cartella sketch con `.ino` e `firmware.hpp` (un solo setup/loop);
- `configuration.json`, `README.md`, `cheatsheet.html` stampabile A4 e `cheatsheet.pdf`;
- `build-report.json`, con stati `not-requested`, `unavailable`, `failed` o `compiled`;
- `firmware.hex` **solo con `compiled` e `ok: true`**.

## Compilatore riproducibile

Versioni verificate: **Arduino AVR Boards 1.8.6**, **avr-gcc 7.3.0-atmel3.6.1-arduino7**, Nano ATmega328P a 16 MHz. Installare Arduino AVR Boards 1.8.6 con Board Manager di Arduino IDE, oppure con Arduino CLI:

```sh
arduino-cli core update-index
arduino-cli core install arduino:avr@1.8.6
```

Il Builder verifica la versione prima della build. Su macOS cerca `~/Library/Arduino15/packages/arduino`; su Linux `~/.arduino15/packages/arduino`. Per installazioni alternative:

```sh
AVR_TOOLCHAIN=/percorso/avr-gcc/bin ARDUINO_AVR_CORE=/percorso/arduino/hardware/avr/1.8.6 npm start
```

`ARDUINO_PACKAGES` cambia la radice dei pacchetti Arduino. Su Windows impostare esplicitamente i percorsi e usare Node/strumenti AVR compatibili con il sistema; questa piattaforma non è stata verificata. La build usa il core, SPI ed EEPROM inclusi nel pacchetto Arduino, senza MCP48xx, Mozzi o librerie hardware aggiuntive. I comandi di compilazione sono in `src/compiler.mjs`; usa LTO, sezioni separate e garbage collection al link. La cache del core è in `.cache/`, gli intermedi in una cartella temporanea distinta per build.

Build da terminale:

```sh
node tools/build.mjs data/default.json --out ../output/builder/custom
node tools/build.mjs data/default.json --out ../output/builder/sources --sources-only
```

Upload **manuale**: aprire lo sketch nell'IDE, scegliere Nano / ATmega328P e il bootloader realmente montato (anche “Old Bootloader”). Il Builder non sceglie una porta, non effettua upload, non modifica fuse e non cancella EEPROM.

## Memoria e compatibilità

Budget conservativo per Nano con bootloader storico: **30.720 byte flash**, **2.048 byte SRAM**, con **384 byte riservati allo stack**. La misura statica viene dall'ELF collegato con `avr-size`, non da una somma di stime delle modalità. `.data + .bss + .noinit + 384` deve entrare nella SRAM. La riserva è un criterio minimo di progetto, non una dimostrazione del massimo stack di qualsiasi combinazione.

La build di riferimento del preset da 16 modalità misura **25.898 B flash / 992 B SRAM statica**. Tutte le 16 Motion insieme superano la flash disponibile: il numero di slot non basta a garantire compatibilità. Lo ZIP senza HEX conserva selezione e motivazione. Nessun algoritmo viene degradato o rimosso per farlo entrare.

Timer1 è assegnato al solo motore audio attivo; una sola ISR smista al proprietario. Prima del cambio modo viene fermato l'audio, poi vengono svuotate le code, azzerate le uscite e cambiata la direzione I/O. Non vengono usati mirror PWM su D3/D5 né interrupt PCINT del vecchio Rhythm. Il polling CV con isteresi conserva la baseline recente: impulsi più brevi del tempo di osservazione non sono garantiti.

## Navigazione, LED e persistenza

MODE breve avanza **al rilascio**, modulo il numero effettivo di slot. Con un solo slot non cambia modalità; i comandi interni del motore restano attivi. Nel Builder i LED1..4 hanno pesi 1/2/4/8 (il firmware dedicato ALLROUNDER usa una codifica diversa): si leggono da sinistra a destra; slot 1 `1000`, 15 `1111`, **16 `0000`**.

Eccezioni documentate nel catalogo e nella cheatsheet:

- Preset Manager a 8 step: LED1..3 indice preset 0..7, LED4 PLAY acceso / EDIT lampeggiante. Questa indicazione sostituisce il codice slot; MODE 650 ms alterna PLAY/EDIT. EEPROM v2 0..105; migrazione v1 128..189 in sola lettura.
- ORBIT mostra per un secondo la scala selezionata con MANUAL lungo.
- Full Voice e Drum Lab sono **una modalità ciascuna**, con le pagine interne originali. **MODE+MANUAL per 2 s** passa allo slot globale successivo. Per 800 ms all'ingresso mostrano lo slot globale, poi i LED interni. Conservano patch 320..393 e kit 512..939.

ACAL v1 a due record CRC/commit, **256..301**, e procedura nascosta MODE+MANUAL al boot sono conservati. I preset/kit non vengono salvati automaticamente al boot. Driver DAC e polarità invertente provengono dalla baseline recente. I/O resta un jack digitale D2+A5, non una terza uscita DAC: scollegare driver esterni prima di entrare in un modo che lo pilota, compresi riavvio e cambio firmware. Taratura e tensioni nominali non certificano protezioni o tracking V/oct.

## Catalogo e sorgenti

Le 47 modalità sono versionate **1.0**. Il [catalogo completo](../Additional-Modes/) offre ogni modalità come snippet e sketch standalone. La scheda di ogni motore mostra il codice usato nella composizione. ORBIT e il Preset Manager a 8 step sono opzioni distinte; ACPM e MACRO VOICE sono firmware dedicati, con una propria interfaccia.

Modificare lo snippet in `Additional-Modes/<id>/Snippet/mode.inc` e gli eventuali metadati/adapter in `data/catalog.json`. `npm run catalog` rigenera i sorgenti del browser, gli standalone, i README e gli header condivisi. Il generatore include i motori scelti in namespace separati e il runtime comune di REV5-2. `npm test` rileva divergenze tra i sorgenti canonici e i file generati.

## Verifiche

```sh
npm test
npm run verify:avr
npm run verify:host
```

La simulazione del binario usa `avr8js` 0.21.1, dipendenza solo di sviluppo (`npm install`), con Timer0/1/2, ADC, SPI, GPIO, UART ed EEPROM. In alternativa `AVR8JS_PATH` indica un'installazione esistente. I test `tests/avr-runtime.cjs` e `tests/avr-transitions.cjs` leggono gli ELF compilati; vedere [i risultati](TEST_REPORT.md) per comandi e risultati. I collaudi sul binario non simulano il circuito analogico, il DAC fisico o una patch reale.
