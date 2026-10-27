# ALTERIA output calibration

Applies to **ALLROUNDER 2.0.2 / REV5-2 / Nano ATmega328P**. DRUM LAB 1.0.0 uses the same ACAL format and a matching boot procedure; consult its manual for the return to audio operation.

This is **manual three-point output calibration**. The Nano does not independently measure its outputs. Use a DC multimeter between jack tip and sleeve/GND.

1. Allow the module to warm up. Unpatch all inputs/outputs and I/O; remove loopbacks. Attach only the meter. The outputs will visit positive and negative 5 V and may overshoot slightly while adjusted.
2. Hold **MODE + MANUAL before power-on or Nano reset**. Keep them held through the bootloader until LED1 lights, then release both. The chord must qualify for 100 ms after sketch startup. Pressing it during normal play does not open calibration.
3. **LED1: 0 V.** Center P1 and P2 to acquire them. P1 adjusts OUT1; P2 adjusts OUT2. They are rate controls: center stops adjustment, clockwise normally raises voltage, counterclockwise lowers it. Verify direction on the meter if the installed pots are reversed. Adjust both outputs, return both knobs to center, then short MANUAL.
4. **LED2: +5 V.** Repeat for both outputs, center both, short MANUAL.
5. **LED3: −5 V.** Repeat, center both, short MANUAL.
6. **LED4 blinking: final confirmation.** Hold MANUAL **1 second** to save. Wait for flashing during writes and the steady success indication. Allrounder then starts mode 1; DRUM LAB restarts audio.
7. Power-cycle and check zero and intermediate positive/negative voltages with the intended load. A save confirms valid stored codes, not measured accuracy.

MODE short returns to the previous point. MODE held **1.2 seconds** cancels without replacing the previous stored calibration. Holding both buttons **3 seconds during calibration**, after releasing the entry chord, restores theoretical defaults in RAM only: save explicitly to make them permanent, or cancel to retain the old EEPROM values.

The center dead band is ADC 442–582. Each adjustment changes one DAC code, about every 40–300 ms depending on distance from center, bounded to ±256 codes from the session's initial point and to 0–4095. P3 is unused. Input/I/O LEDs indicate knob acquisition, not electrical accuracy. If the target cannot be reached within the window, investigate the hardware rather than forcing coefficients.

Calibration uses EEPROM **256–301** (ACAL 1), separate from Allrounder presets **128–189** and DRUM LAB kits **512–715 / 736–939**. Valid records are reused by both current firmware families. An older firmware, a chip erase or reused EEPROM addresses can invalidate this persistence. The old demo and HW_TEST do not automatically load ACAL.

Optional serial logging: **115200 baud**; opening the monitor may reset the Nano. Logged values are DAC codes, not measured volts. The [detailed Italian guide](../firmware/ALTERIA_ALLROUNDER/GUIDA_CALIBRAZIONE.md) documents errors, CRC/fallback and all gestures.

Calibration corrects offset and positive/negative output scale. It does not remove noise, drift, nonlinearity or load-dependent drops and does not certify pitch accuracy. Keep I/O unpatched throughout: Allrounder returns it to output mode after exit.
