# Sviluppo e verifica

I firmware correnti dichiarano come target ALTERIA REV5-2, Arduino Nano ATmega328P 16 MHz. I singoli manuali specificano direzione dei jack, modalità e limiti. Non trasferire i pin delle vecchie revisioni ai firmware correnti.

## Arduino AVR

Installare Arduino AVR Boards 1.8.6, che usa avr-gcc 7.3.0. Esempio dalla radice:

```sh
arduino-cli compile --warnings all --fqbn arduino:avr:nano:cpu=atmega328 ALLROUNDER/Firmware/ALTERIA_ALLROUNDER
sh tools/test_host.sh
```

I test host usano clang++, AddressSanitizer e UndefinedBehaviorSanitizer. Per la simulazione binaria, installare le dipendenze della cartella `tests/`, compilare con `--build-path` e passare quel percorso a `tests/avr_firmware_test.cjs`. Impostare `AVR_NM` sul binario avr-nm della toolchain se non è sul PATH. La simulazione non rappresenta livelli, jitter elettrico o limiti del PCB fisico.

## Libreria e Builder

Il codice modificabile risiede in `Additional-Modes/<id>/Snippet/mode.inc`. Descrizione, adapter e metadati sono in `Firmware-Builder/data/catalog.json`. I componenti del runtime sono in `Firmware-Builder/runtime/`. Quando si modifica uno snippet o il runtime:

```sh
cd Firmware-Builder
npm run catalog
npm test
npm run verify:avr
```

La generazione aggiorna i README, gli sketch standalone, il pacchetto del browser e i due header condivisi da ALLROUNDER. La matrice AVR compila tutti gli standalone, alcune composizioni e due casi intenzionalmente troppo grandi, che devono restituire il report senza HEX.

Vedere il [README del Builder](../Firmware-Builder/) per il server locale, gli export e le opzioni della toolchain.
