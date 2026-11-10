# Analog iWand

iWand with no microcontroller and no audio chip: photodiodes, op-amps, comparators, diodes and resistors. The result is shown on an LED wheel instead of spoken.

Status: the front end and max/min stage are sized. Next is a breadboard test of one channel to measure the real photocurrent.

## Output: LED wheel

Speech is the one part that needs digital hardware (the ESP32-S3, and the DFPlayer's decoder chip), so it is replaced with one LED per colour name.

- The 7 colours with a hue sit around the rim at their hue angle: Red, Orange, Yellow, Green, Cyan, Blue, Purple.
- Black, White and Gray have no hue, so they sit as a small stack in the middle.
- Brown shares a hue with Orange/Red and is just darker, so it gets an LED just inside that part of the rim.
- **Users are colour-blind, so each LED is identified by its position and a printed or embossed name, never by the LED's colour.** All LEDs can be the same colour.

## Signal chain

1. **Front end:** 3 photodiodes behind red, green and blue filters, lit by a white LED. Each feeds an op-amp that turns its current into a voltage (Vr, Vg, Vb).
2. **Max/min:** precision max and min of the three channels. Vmax is the brightness. Everything downstream is a fraction of Vmax or of the white level.
3. **Brightness comparators (3):** Black, the dark band where Brown lives, and White vs Gray.
4. **Saturation comparator:** compares Vmin against a resistor-divided copy of Vmax, so no divider chip is needed.
5. **Hue sector comparators (3):** R>G, G>B, B>R. These place the hue in one of six 60° sectors exactly.
6. **Hue boundary comparators:** one comparator and one trimmer per boundary inside a sector (10°, 15°, 40°, 50°, 70°, 150°, 200°, 230°, 330°). Each compares a weighted mix of channels against a fraction of Vmax−Vmin. They are approximate in linear light (see [the conversion notes](../README.md#converting-to-linear-light)) and need bench tuning.
7. **Priority network:** diodes, not logic chips. Black's output pulls every other line down, so Black always wins. Brown is enabled only when both "hue in the Orange/Red band" and "brightness in the dark band" are true.
8. **Outputs:** LM339-style open-collector comparators drive the LEDs directly. Each gets hysteresis (a positive-feedback resistor) so a reading on a boundary holds one LED instead of flickering between two.

## Sized so far: power, front end, max/min

### Virtual ground

Everything runs from a 3.3 V regulator, and every signal sits on a **virtual ground (VG) of 0.5 V**. Black reads VG; white reads VG + 2.0 V = 2.50 V.

- The min stage drives its op-amp output one diode drop below the lowest channel. With black at 0 V it would need a negative supply; the 0.5 V offset gives it room.
- Comparator dividers return to VG instead of GND. A ratio test like "Vmin is within 7% of Vmax" is then a single divider between Vmax and VG.

### Power and illumination

| Function | Part / value |
|---|---|
| Regulator | MCP1700-3302 (TO-92), 1 µF in and out |
| VG | 56 k / 10 k divider off 3.3 V → 0.50 V, op-amp follower, 47 Ω series + 1 µF |
| LED | 5 mm white LED, ~10 mA constant current from the battery: op-amp + 2N3904, 10 Ω sense resistor, ~0.1 V reference (33 k / 1 k off 3.3 V) |

The brightness thresholds are fixed voltages, so the LED must not dim as the battery runs down. With a 0.1 V sense voltage it holds its current down to about 3.35 V, close to where the regulator drops out.

### Front end (×3: R, G, B)

```
            Rf = 220k + 1M trimmer
          ┌────/\/\/\────┐
          ├──── 3.3nF ───┤
BPW34 K ──┤−             │
      A ─ VG    MCP6004 ─┴── V_R  (VG + I·Rf)
     VG ──┤+
          │
 4.7MΩ ───┘  ← wiper of 100k trimmer between VG and 3.3 V (black offset)
```

- **Sensor:** BPW34 (through-hole, 7.5 mm²) behind a primary red, green or blue gel, with one diffuser over all three so they see the same patch. LED at about 45° to the surface, photodiodes facing it straight on, so glossy reflections stay out.
  - Gels pass infrared and the BPW34 is most sensitive in the infrared. That's acceptable because a white LED gives off almost no IR and the nose blocks room light. If light leaks in, add an IR-cut filter.
- **Zero bias:** cathode to the inverting input, anode to VG, so the output goes positive.
- **Gain:** estimated 2–10 µA on white paper (unmeasured). To land 2.0 V above VG, Rf needs 0.2–1 MΩ: 220 k + a 1 MΩ multi-turn trimmer.
- **Filter:** 3.3 nF across Rf, a 40–220 Hz roll-off. It also keeps the amplifier stable with the photodiode's ~70 pF.
- **Black offset:** current fed into the summing node subtracts from the photocurrent: Vout = VG + (I_pd − I_off)·Rf. Because it subtracts current, the black and white trims don't interact. Range up to 0.6 µA.

**Calibration by hand** (replaces the firmware's `runCalibration()`):

1. Aim at black. Set each offset trimmer so its output equals VG within 2 mV (measure output minus VG).
2. Aim at white. Set each gain trimmer so its output reads 2.00 V above VG. This is also the white balance.

The black zero matters most: at a dim gray 200 mV above VG, a 20 mV mismatch between channels is a 10% error, enough to read gray as coloured.

### Max/min

One op-amp per channel per function, 6 in total. A one-op-amp version with matched diodes has 10–20 mV of error, a large share of the 94 mV Black threshold.

| | Max | Min |
|---|---|---|
| Per channel | follower, BAT85 **anode** at op-amp output, cathode to node | follower, BAT85 **cathode** at op-amp output, anode to node |
| Node pull | 10 k to GND | 10 k to 3.3 V |
| Worst-case op-amp output | 2.50 + ~0.3 = 2.8 V | 0.50 − ~0.3 = 0.2 V |

- Schottky diodes are required: a 1N4148 (0.6 V) would push one stage or the other outside the 3.3 V rails.
- The diode is inside the feedback loop, so the only error is op-amp offset (at most 4.5 mV, 0.2% of span).
- Keep downstream dividers at 100 k or more so the 10 k pulls stay stiff.

**Op-amps:** 3 TIA + 6 max/min + VG buffer + LED driver = 11, so 3× MCP6004 (DIP-14) with one spare. Rail-to-rail in and out, pA input bias, 4.5 mV maximum offset (cancelled by the black trim).

### Thresholds for the comparator stage

| Decision | Condition |
|---|---|
| Black | Vmax < 0.594 V (VG + 0.047 × 2 V) |
| Dark band (Brown) | Vmax < 1.41 V |
| White vs Gray | Vmax ≥ 1.72 V |
| No hue (White/Gray) | Vmin − VG > 0.93 × (Vmax − VG) |

## Next steps

1. Breadboard one channel (green): BPW34 + gel + TIA with a 1 MΩ trimmer. Measure the output on white and black paper at the intended nose distance. This fixes Rf and the offset range.
2. Size the comparator stage: threshold dividers, hue boundary weightings, hysteresis resistors, priority diodes.
3. Draw the schematic in KiCad (in this folder, separate from `electrical/`).

## Parts to source for the breadboard

- 3× BPW34 photodiode
- Red, green and blue filter gel, plus diffuser film
- 5 mm white LED, 2N3904
- 3× MCP6004 (DIP-14)
- 6× BAT85 Schottky diode
- MCP1700-3302 (TO-92)
- 3× 1 MΩ and 3× 100 k multi-turn trimmers
- Resistors: 220 k, 4.7 MΩ, 56 k, 10 k, 33 k, 1 k, 47 Ω, 10 Ω; capacitors: 3.3 nF, 1 µF
