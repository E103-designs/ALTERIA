# ALTERIA ACPM 1.0.0

**Addressable or Clocked Preset Manager.** Firmware dedicato per ALTERIA REV5-2,
Arduino Nano ATmega328P 16 MHz. **Non testato sul modulo fisico.**

Ogni step memorizza OUT1, OUT2 e lo stato del gate I/O. Sedici step memorizzati,
lunghezza attiva da 1 a 16. Accorciare la lunghezza non cancella gli altri step.

## Play, edit e setup

| Azione | Funzione |
| --- | --- |
| MODE breve | PLAY ↔ EDIT; da SETUP torna alla pagina precedente |
| MANUAL | Avanza di uno step in PLAY e EDIT |
| MODE poi MANUAL, entrambi tenuti 400 ms | Entra/esce da SETUP |
| MODE lungo, circa 1 s | Salva il banco e le impostazioni, senza cambiare pagina |

In **EDIT**, P1 regola OUT1, P2 OUT2, P3 gate OFF/ON. Il LED CV IN e fisso
in EDIT. I pot usano pickup: raggiungere o attraversare il valore corrente
prima di poterlo modificare. Il clock/address esterno non cambia step in
EDIT e SETUP. I/O LED segue il gate, che e mantenuto fra step; due step ON
consecutivi non creano un nuovo fronte.

In **SETUP**, P1 regola **1..16 step**, P2 il range comune di uscita,
P3 la funzione di CV IN. Anche questi pot usano pickup: portare il pot nella
fascia del valore corrente (o attraversarla) prima di cambiarlo. La modifica
del range riscalibra immediatamente l'escursione dei due CV: i valori degli
step sono normalizzati, quindi vengono riscalati tutti insieme.

| LED da sinistra | In SETUP |
| --- | --- |
| LED1 | Range ±1 V |
| LED2 | Range ±2 V |
| LED3 | Range ±5 V |
| LED4 | Spento = CLOCK, acceso = ADDRESS |

Un solo LED fra 1, 2 e 3 e acceso. Muovendo **P1** i quattro LED mostrano
la lunghezza per un secondo dall'ultimo movimento, poi tornano alla mappa
SETUP. Muovere P2 o P3 torna subito alla mappa. Non c'e un sensore tattile:
il firmware rileva una variazione del pot.

## Numerazione

Da sinistra a destra, LED1..4 hanno pesi **8, 4, 2, 1**.
Si visualizza il **numero meno 1**, sia per lo step attivo sia per la
lunghezza: 1 = `0000`, 2 = `0001`, 3 = `0010`, 8 = `0111`,
16 = `1111`. `0000` e un'indicazione valida, non un modulo spento.

## CV IN e I/O

- **CLOCK:** un fronte crescente avanza e l'ultimo step torna al primo.
  Isteresi sul livello d'ingresso. Un livello gia alto al boot non conta
  come nuovo fronte.
- **ADDRESS:** ingresso nominale 0..+5 V, diviso in tante fasce quanti sono
  gli step attivi. Ogni cambio fascia seleziona il relativo step, con
  isteresi attorno ai confini. MANUAL puo avanzare e non viene annullato
  finche il CV resta nella stessa fascia. Se un evento CV coincide con
  MANUAL nel loop, prevale il CV. Al boot e al ritorno in PLAY si prende
  nota della fascia presente senza saltare subito allo step corrispondente.
- **I/O e sempre un'uscita 0/+5 V**. Non collegarvi clock o altre uscite.
  Il DAC riceve e aggancia i CV prima del nuovo gate.

I range ±1/±2/±5 V sono nominali e usano i punti calibrati 0/+5/-5 V
se presenti. Il range e comune a OUT1/OUT2; non e un quantizzatore 1 V/oct.
I limiti elettrici del jack CV IN vanno verificati sulla scheda. Non
applicare CV negative all'ingresso o collegare OUT bipolare a una
destinazione che non le accetta.

## Salvataggio e caricamento

MODE lungo salva tutti i 16 step, lunghezza, range, CLOCK/ADDRESS e step
selezionato. I quattro LED lampeggiano durante la scrittura e si accendono
fissi per circa 650 ms a conferma. Non spegnere prima della conferma.
Al boot riparte in PLAY. Default senza record valido: 16 step a 0 V e
gate spento, range ±1 V, CLOCK, step 1.

Due banchi ACPM EEPROM separati, **190..248 / 940..998**, CRC e commit
finale. Nessuna scrittura continua, nessuna sovrascrittura di preset/
sessione ALLROUNDER, FULL VOICE, DRUM LAB o ACAL. Taratura ACAL in sola
lettura; per crearla usare un firmware ALTERIA con taratura. Nessuna
importazione automatica degli altri preset.

Aprire `ALTERIA_ACPM.ino` nell'IDE, scheda Arduino Nano, ATmega328P; Old
Bootloader solo se necessario. Prima dell'upload scollegare driver
esterni da I/O. Vedere [TEST_REPORT.md](TEST_REPORT.md).
