# Origine e attribuzione - ALLROUNDER 2.2.1

2026-11-03. Port REV5-2 dei firmware ALTERIA precedenti. Gli altri modi,
backend MCP4822 e procedura ACAL derivano da Allrounder 2.1.0.

ORBIT e un algoritmo originale a motivi, passeggiata vincolata, scale tabulate
e mutazioni al confine della frase. Nessun modello ML, libreria audio, Mozzi,
oscillatore o filtro digitale audio viene incorporato nel modo 10.

Il modo 6 Acid Generator conserva l'ispirazione funzionale a
[drakh/acid-generator](https://github.com/drakh/acid-generator): selezione sparsa
di step, pool di note, accenti e slide. Implementazione AVR originale, non port
completo del web synth, senza Tone.js o lodash. Non e un'emulazione TB-303.

Librerie firmware: SPI/EEPROM del core Arduino AVR e avr-libc. I test del binario
usano facoltativamente avr8js 0.21.1, non incluso nel firmware da caricare.
