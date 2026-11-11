# ALTERIA ACPM 1.0.0 - test software

2026-11-11. **Non testato su hardware.** Nessun upload o misura fisica.
Arduino Nano ATmega328P 16 MHz, core Arduino AVR 1.8.6, avr-gcc 7.3.0.

- SHA-256: `143d769c49bebbccabae6073dd9fd36bdc5eebbf3b5e8e0ec0c99572c9ce1452`.
- Build PASS: **5970/30720 byte flash**, **257/2048 byte SRAM statica**.
  Solo warning noti del core Arduino in `new.cpp`.
- Host ASan/UBSan PASS: tutti i codici LED 0..15, SETUP e preview/timeout,
  indirizzamento per ogni lunghezza e valore ADC, isteresi, CLOCK a
  fronte, MANUAL senza ritorno alla stessa fascia, pickup, range DAC
  monotoni sui due canali con ACAL differenziata, gate, EEPROM e fallback
  dopo ogni interruzione di scrittura, nessuna modifica alle altre aree.
- avr8js 0.21.1 PASS sul binario compilato: 16 step via clock ADC,
  gesto SETUP, codici range/address, preview P1 e ritorno automatico,
  ADDRESS con MANUAL, EDIT, latcheggio prima del gate, scrittura e nuova
  CPU che recupera configurazione/step/gate. I/O sempre OUTPUT.
  Stack massimo osservato nella prima esecuzione: 168 byte, gap 1622.

```sh
arduino-cli compile --warnings all --fqbn arduino:avr:nano:cpu=atmega328 \
  --build-path /tmp/alteria-acpm-build .
clang++ -std=c++11 -O1 -fsanitize=address,undefined -Itests/stubs \
  tests/acpm_test.cpp -o /tmp/alteria-acpm-test
/tmp/alteria-acpm-test
npm install --prefix tests --ignore-scripts --no-audit --no-fund
AVR_NM=/path/to/avr-nm node tests/avr_firmware_test.cjs /tmp/alteria-acpm-build
```

Restano da verificare sul modulo: ordine fisico LED, livelli e calibrazione,
soglie e limiti del jack CV IN, comportamento in presenza di rumore e
temporizzazione gate/DAC sotto carico. La simulazione non modella il PCB.
