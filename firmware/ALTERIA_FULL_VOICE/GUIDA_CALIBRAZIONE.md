# Taratura uscite - ALTERIA FULL VOICE 1.0.0

2026-10-31, Nano ATmega328P / REV5-2. Procedura ACAL compatibile con Allrounder
2.1.0 e Drum Lab. Non e autotaratura: serve un multimetro esterno. Corregge i
DAC e gli stadi di uscita, **non il tracking dell'ingresso V/oct**.

## Preparazione

Scollegare le patch e collegare il tester DC ad alta impedenza fra tip e sleeve
(GND) di OUT1 o OUT2. Attendere la stabilizzazione termica. Durante questa
procedura entrambe le uscite raggiungono anche -5 V: scollegare ingressi audio,
loopback e altri moduli. I/O rimane ingresso durante e dopo la taratura.
L'audio e fermo, quindi non si tara misurando una waveform.

Tenere MODE e MANUAL premuti prima dell'accensione o del RESET del Nano,
attraverso il bootloader, finche si accende LED1. La combinazione e qualificata
per 100 ms dall'inizio dello sketch. Rilasciare entrambi. Il gesto durante l'uso
normale non apre la taratura.

## Regolazione

P1 corregge OUT1; P2 corregge OUT2; P3 non e usato.
A ogni nuovo punto centrare P1/P2, circa ore 12: i LED CV IN e I/O indicano
l'acquisizione rispettiva dei due pot, non la precisione della misura.

Nel centro (ADC 442..582) la correzione si ferma. Ruotando verso ADC maggiori,
normalmente CW, la tensione sale; nell'altro senso scende. Verificare il verso
fisico sul proprio pannello. Non sono valori assoluti: la posizione determina
la velocita, un codice DAC ogni circa 40..300 ms. Ogni codice vale circa 2.5 mV
con i componenti nominali. Rientrare al centro per fermare il valore.

La finestra e +/-256 codici dal valore iniziale, entro il range DAC. Se non basta
raggiungere il target verificare hardware/carico/alimentazione, non forzare.

| LED1..4 | Target OUT1 e OUT2 | Comando |
| --- | --- | --- |
| 1000 | 0 V | Regolare entrambi, centrare P1/P2, MANUAL breve |
| 0100 | +5 V | Regolare entrambi, centrare P1/P2, MANUAL breve |
| 0010 | -5 V | Regolare entrambi, centrare P1/P2, MANUAL breve |
| 0001 lampeggiante | Punto zero, conferma finale | MANUAL tenuto 1 s salva |
| Tutti lampeggianti rapidi | Scrittura | Attendere senza togliere alimentazione |
| 1111 fisso per circa 0.8 s | Completato | Riparte FULL VOICE, pagina OSC |

MODE breve torna al punto precedente. MODE tenuto 1.2 s annulla la sessione e
riparte con i vecchi coefficienti. Entrambi i tasti tenuti 3 s, dopo il primo
rilascio e fuori scrittura, ripristinano i sei default teorici **nella sessione**:
per salvarli occorre confermare tutti i punti. Non cancella immediatamente EEPROM.

La conferma di ciascun punto richiede entrambi i pot acquisiti e centrati.
Lampeggio rapido persistente = dati non validi o errore di scrittura/rilettura:
tornare indietro, riprovare o annullare. Non considerarli salvati.

## Persistenza e verifica

Sei codici 0/+5/-5 V separati per canale, due record CRC16/commit/versione/sequenza
nell'area EEPROM **256..301**. Il firmware legge automaticamente quelli gia
validi. Non sovrascrive la patch FULL VOICE 320..393, i vecchi preset 128..189 o
i kit Drum Lab 512..939. Se entrambi i record sono invalidi usa i punti teorici
2048 / 87 / 4008, che non sono misure del tuo modulo. La sessione non salvata
non altera i coefficienti attivi. Salvare valori invariati gia presenti evita
ulteriori scritture. La doppia copia non sostituisce una corretta alimentazione
ne garantisce comportamento elettrico durante brown-out.

Monitor Seriale facoltativo a 115200 baud durante la taratura: mostra codici DAC,
non volt misurati. Aprirlo puo resettare il Nano; tenere i due pulsanti per
rientrare. Il seriale viene disattivato al ritorno alla voce.

Dopo il salvataggio riavviare ed entrare di nuovo nella taratura per verificare
0/+5/-5 V con tester e carico previsto. Uscire annullando se non si modifica nulla.
Controllare separatamente offset, scala, rumore e deriva. Per il pitch misurare
la tensione CV applicata contro l'ADC e configurare i due endpoint del sorgente;
ACAL non li modifica. Nessuna promessa di intonazione perfetta o misura fisica
eseguita dall'assistente. Report software: [TEST_REPORT.md](TEST_REPORT.md).
