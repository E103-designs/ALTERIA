# ALLROUNDER 2.3.0 · Verifica

Target Nano ATmega328P / REV5-2, 16 MHz. Arduino AVR Boards 1.8.6, avr-gcc 7.3.0, Arduino CLI. **27.572 / 30.720 byte flash**, **1.042 / 2.048 byte SRAM statica**. Nessun warning dal firmware; solo warning del core Arduino.

- Test host con ASan/UBSan: 16 slot distinti, codici LED, direzione I/O, inviluppi, Trance Gate, clock/tap, Burst e XOR, range Tremolo, Turing, taratura e sessione. La regressione dei nuovi modi copre densità indipendenti, pattern di 16 step, reset, trigger solo da clock in Euclidean Trigs, estremi delle manopole e impulso/zero-glide di S&H Glide.
- Simulazione del binario con avr8js: DAC A/B e D2, due cicli euclidei, reset, S&H Glide, Turing e latch DAC, snapshot/nuovo avvio e pickup, Burst e Tremolo. Stack massimo osservato di 125 byte nello scenario principale; il valore non è un limite garantito.
- Gli altri 14 algoritmi di ALLROUNDER sono quelli della 2.2.4. EEPROM ACAL e sessione sono mantenute. Le aree dei preset non vengono scritte.

```sh
arduino-cli compile --warnings all --fqbn arduino:avr:nano:cpu=atmega328 --build-path /tmp/alteria-build .
npm install --prefix tests --ignore-scripts --no-audit --no-fund
node tests/avr_firmware_test.cjs /tmp/alteria-build
node tests/avr_firmware_test.cjs /tmp/alteria-build --session
node tests/avr_firmware_test.cjs /tmp/alteria-build --controls
```

Usare `AVR_NM` se avr-nm non è nel PATH. Non eseguiti upload o misure fisiche. Restano da verificare i livelli, la durata dei trigger, il comportamento con clock molto veloci e il cablaggio sul modulo reale.
