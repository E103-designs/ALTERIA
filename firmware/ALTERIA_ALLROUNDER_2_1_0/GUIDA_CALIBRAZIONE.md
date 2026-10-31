# Taratura nascosta - ALTERIA_ALLROUNDER 2.1.0

2026-10-26. Procedura ereditata dalla variante CAL 1.2.0, con formato EEPROM compatibile. Nano ATmega328P / PCB REV5-2. Per la nuova mappa dei 16 modi leggere README.md.

## Caricamento e funzionamento normale

Aprire `ALTERIA_ALLROUNDER_2_1_0.ino` nell'IDE, Arduino Nano / ATmega328P. Se il Nano lo richiede usare Old Bootloader per l'upload. Dipendenze solo SPI/EEPROM del core AVR. Le normali funzioni sono quelle del [manuale Allrounder](README.md): OUT1/OUT2 unipolari nei modi 1..9 e 11..15; OUT1 audio bipolare circa +/-2.5 V e OUT2 inviluppo 0..+5 V nel modo 10; CV bipolari -5..+5 V nel modo 16. I/O ingresso nei modi 3 e 10, uscita digitale negli altri.

La taratura caricata dalla EEPROM viene applicata alla conversione di entrambi i DAC in tutte le modalita. Non modifica ingressi ADC, gate digitali o oscillatori esterni. I preset continuano a salvare valori logici, non codici DAC grezzi: mantengono il significato di tensione anche dopo una nuova taratura.

## Prima di tarare

- Lasciare stabilizzare termicamente il modulo, con alimentazione/cablaggio previsti. Verificare che l'eventuale connessione USB contemporanea sia compatibile con il proprio hardware.
- Scollegare le patch, soprattutto I/O e ogni loopback fra uscite e ingressi. Collegare solo multimetro DC ad alta impedenza a OUT1 o OUT2, tip rispetto a sleeve/GND. **Le uscite raggiungeranno +5 e -5 V e possono oltrepassare lievemente il target durante la regolazione.**
- Non confondere tensioni richieste con misure: il Nano non misura indipendentemente OUT1/OUT2. Serve il tester; non e autotaratura.
- Non inserisco automaticamente i codici stimati dalle cinque misure precedenti: la prima taratura parte dai punti teorici o dagli ultimi punti realmente salvati.

## Entrare nella modalita nascosta

Tenere premuti **MODE e MANUAL prima dell'accensione**, mantenerli durante il boot fino a vedere LED1 acceso, quindi rilasciarli entrambi. La combinazione viene verificata per 100 ms dall'inizio dello sketch (oltre al tempo del bootloader). Funziona anche tenendo i due pulsanti mentre si preme RESET del Nano.

Solo un pulsante all'avvio, oppure la combinazione abbandonata troppo presto, avvia il demo normale. Durante il normale funzionamento la combinazione non apre la taratura. Nei preset del modo 16 MODE lungo conserva il comportamento di editing precedente.

Durante la taratura tutti i generatori sono sospesi; I/O D2 e A5 restano ingressi senza pull-up. All'uscita riparte il modo 1 e D2 torna uscita come nel demo: lasciare I/O scollegato per tutta la procedura.

## Regolare con POT1 e POT2

POT1 corregge OUT1 e POT2 corregge OUT2. **Non sono controlli assoluti del codice:** funzionano come manopole di avanzamento lento.

1. A ogni nuovo punto portare entrambi i pot al centro, circa ore 12. Il LED ingresso D3 indica che POT1 e stato acquisito; il LED I/O D5 fa lo stesso per POT2. Non indicano che il valore sia elettricamente corretto.
2. Nel centro (ADC 442..582) la regolazione e ferma. Ruotare verso valori ADC maggiori, normalmente in senso orario, per alzare i volt; verso valori minori per abbassarli. Se il montaggio del pot inverte il verso fisico, verificare sul tester. Il firmware compensa la polarita invertente del backend DAC.
3. Piu ci si allontana dal centro, piu velocemente il codice cambia: **un solo passo DAC per aggiornamento**, circa ogni 40..300 ms. Avvicinarsi lentamente al target e riportare il pot al centro per fermare la regolazione. Ogni codice vale circa 2.5 mV con il guadagno nominale; non ogni lettura ADC del pot.
4. Misurare alternativamente OUT1 e OUT2 e sistemarle entrambe. P3 non ha funzione in taratura. Tornare al centro con entrambi prima di confermare: il firmware non avanza se un pot sta ancora impartendo una correzione.

La finestra e limitata a +/-256 codici rispetto al punto iniziale di ciascuna sessione, e comunque a 0..4095. Se il target non e raggiungibile, non forzare la taratura: verificare componenti, alimentazione, carico e stadio analogico. Raggiunto il limite, la manopola non modifica ulteriormente il codice.

## Sequenza LED e tasti

LED in ordine 1,2,3,4, dove 1 significa acceso:

| Passaggio | LED | Uscite | Cosa fare |
| --- | --- | --- | --- |
| ZERO | 1000 | Punto 0 V per entrambi | Regolare P1/P2 fino a 0 V; centrare entrambi; MANUAL breve |
| POSITIVO | 0100 | Punto +5 V per entrambi | Regolare fino a +5 V; centrare; MANUAL breve |
| NEGATIVO | 0010 | Punto -5 V per entrambi | Regolare fino a -5 V; centrare; MANUAL breve |
| CONFERMA | 0001 lampeggiante | Ritorno al nuovo punto zero, pot disattivati | Tenere MANUAL 1 s per salvare |
| SCRITTURA | 1111 lampeggiante rapido | Zero | Attendere; non togliere alimentazione |
| COMPLETATO | 1111 fisso per circa 0.8 s | Zero | Ripartenza automatica nel modo 1 |

- **MODE breve:** torna al punto precedente. Dal punto zero resta al punto zero. Occorre ricentrare i pot per acquisirli nuovamente.
- **MODE lungo 1.2 s:** annulla l'intera sessione e torna al demo con la taratura precedente. Prima della conferma finale non e stata scritta EEPROM.
- **Entrambi i tasti 3 s durante la taratura**, dopo aver rilasciato la combinazione di ingresso: ripristina i sei punti teorici **solo nella sessione in RAM**, torna a ZERO e attende il rilascio. Per renderli permanenti occorre confermare tutti i punti e salvare. MODE lungo annulla anche questo ripristino e mantiene la precedente EEPROM.
- Durante scrittura e breve conferma finale i comandi sono ignorati. Un lampeggio rapido che persiste indica salvataggio non accettato/errore di rilettura: i coefficienti precedenti restano attivi. Tornare indietro con MODE breve per riprovare o annullare con MODE lungo.

Non basta correggere solo lo zero per avere intervalli melodici corretti: serve anche il guadagno positivo e, per gli output bipolari, il punto negativo.

## Monitor Seriale facoltativo

La calibrazione funziona senza computer. Per annotare i codici usare Monitor Seriale a **115200 baud**. Aprirlo puo resettare il Nano: tenere entrambi i tasti mentre si apre/resetta per rientrare nella modalita nascosta.

Durante i punti compare `CAL step=0 A=... B=...` (step 0/1/2 = zero/+5/-5). Al termine della nuova scrittura compare `SAVED ZERO=... PLUS5=... MINUS5=...`. Il messaggio di salvataggio invariato puo essere omesso quando non serve riscrivere la stessa calibrazione. Numeri = codici DAC, **non volt misurati**. Il seriale viene disattivato all'uscita dalla taratura se il debug normale e disabilitato.

## Persistenza e verifica dopo la taratura

Sei codici a 12 bit, due copie EEPROM con versione, sequenza, CRC16, commit finale e verifica di rilettura. Area riservata **256..301**, record con stride 24. Non tocca i preset a 128..189. Salva solo alla conferma finale; se i coefficienti sono gia salvati e invariati evita ulteriori scritture. Se l'ultimo record non e valido carica la copia precedente; se entrambe le copie sono invalide usa i valori teorici, senza scriverli automaticamente.

La doppia copia protegge dall'interruzione di una scrittura nel modello software; non sostituisce alimentazione corretta e adeguata gestione brown-out del MCU. Non e stata simulata elettricamente una caduta di tensione sul PCB.

Spegnere e riaccendere, verificare il caricamento dei valori e controllare anche +1/+2/+3 V e il carico previsto. Per misure fisse si puo usare Preset Manager impostando le tensioni, ma la verifica del caricamento dei codici e indipendente dal raggiungimento dei volt fisici. Per conservare la taratura usando un altro firmware, quest'ultimo deve implementare lo stesso formato/caricamento: **il vecchio demo 1.1.0 e HW_TEST non la leggono automaticamente**. Normali spegnimenti non cancellano EEPROM; programmazione con cancellazione EEPROM o altri firmware che riutilizzano l'area possono perderla.

La taratura corregge offset e scale positivo/negativo della scheda completa. Non corregge deriva, rumore, non linearita o cadute di tensione variabili con il carico. Per uso 1 V/oct rimane il limite di risoluzione/linearita del MCP4822 e dello stadio analogico. Nessuna promessa di intonazione perfetta.

## Test riproducibili

Build core Arduino AVR 1.8.6, FQBN `arduino:avr:nano:cpu=atmega328` con `--warnings all`. Vedere `TEST_REPORT.md` per memoria e risultati finali. Nessun upload o misura della nuova variante eseguiti dall'assistente.

```sh
clang++ -std=c++11 -fsanitize=address,undefined -Itests/stubs tests/calibration_test.cpp -o /tmp/alteria-cal-test
/tmp/alteria-cal-test
clang++ -std=c++11 -fsanitize=address,undefined -Itests/stubs tests/demo_regression.cpp -o /tmp/alteria-cal-demo-test
/tmp/alteria-cal-demo-test
```
