# Arduino Sensorless BLDC Motor Controller (ESC)

This project implements a **sensorless BLDC (Brushless DC) motor controller (ESC)** using an Arduino UNO. It drives a three-phase motor with MOSFET half-bridges (using IR2101 drivers), enabling adjustable speed control without Hall sensors—using back-EMF zero-crossing detection for commutation.

## Features

- Six-step (trapezoidal) commutation algorithm in C for Arduino.
- Sensorless operation using the built-in analog comparator for zero-crossing detection.
- Manual speed adjustment via push buttons (increase/decrease).
- PWM duty cycles mapped to high/low side MOSFET gates.
- EEPROM support for storing calibrated min/max throttle values.
- Proteus schematic included for simulation and hardware reference.

## Schematic

See `Screenshot-2025-08-21-193358.jpg` or load the provided Proteus files to examine the full ESC hardware:  
- Arduino UNO generates logic signals for all three phases.
- IR2101 MOSFET drivers for high/low-side switching.
- Three-phase BLDC motor, motor voltage supply, integrated pushbuttons for user speed control.
- Analog voltage dividers to feed back-EMF from unpowered phase into the Arduino for comparator-based commutation.

## Getting Started

**Hardware required:**
- Arduino UNO (or compatible ATmega328P board)
- IR2101 gate drivers (x3)
- N-channel MOSFETs (x6, e.g., IRFZ44N)
- Sensorless BLDC motor (3-phase)
- Proteus simulator for schematic simulation (or real breadboard/circuit)
- Push buttons for SPEED UP / SPEED DOWN

**Software:**
- Copy the provided Arduino sketches (`main.ino`) to your Arduino IDE.
- Upload to your Uno.
- Adjust the pin mapping or PWM values as needed for your particular hardware setup.

## Usage

- Power up the ESC circuit with the Arduino and external motor voltage.
- Use the SPEED UP / SPEED DOWN buttons to change motor speed.
- On startup, the controller uses forced commutation then switches to sensorless (back-EMF) operation.
- PWM and phase timing are handled fully by the microcontroller according to the schematic diagram.

## Files

- `main.ino`        — Arduino firmware with detailed comments on BLDC PWM phasing, comparator use, and hardware register control.
- `Screenshot-2025-08-21-193358.jpg` — Full schematic (Proteus screenshot).
- (Add any

