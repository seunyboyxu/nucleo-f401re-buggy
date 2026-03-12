# Nucleo F401RE Buggy - PlatformIO Project

Motor-controlled buggy project for the STM32 Nucleo F401RE, built with PlatformIO and the Mbed framework.

## Features
- Dual motor PWM control with direction and bipolar mode
- QEI (Quadrature Encoder Interface) for wheel speed measurement
- C12832 LCD display for real-time telemetry (RPM, velocity, duty cycle)
- State machine with joystick fire button interrupt (Duty Cycle → Pulses → Bluetooth → Sensor → Idle)
- Bluetooth HM-10 serial communication
- Potentiometer-based speed control with ticker-based sampling

## Hardware
- **Board:** STM32 Nucleo F401RE
- **LCD:** C12832 128x32 pixel SPI display
- **Encoders:** Quadrature encoders (256 PPR, X2 encoding)
- **Motors:** Dual DC motors with H-bridge driver
- **Bluetooth:** HM-10 BLE module (UART)
- **Potentiometers:** 2x analog pots on A0, A1

## Pin Mapping

| Function | Pin |
|---|---|
| LCD MOSI | D11 |
| LCD SCK | D13 |
| LCD RESET | D12 |
| LCD A0 | D7 |
| LCD CS | D10 |
| Right Motor PWM | PC_6 |
| Right Motor Bipolar | PB_13 |
| Right Motor Direction | PB_14 |
| Right Encoder Ch A | PB_8 |
| Right Encoder Ch B | PB_12 |
| Left Motor PWM | PC_8 |
| Left Motor Bipolar | PB_15 |
| Left Motor Direction | PB_1 |
| Left Encoder Ch A | PC_10 |
| Left Encoder Ch B | PC_12 |
| Motor Enable | PC_4 |
| Left Pot | A0 |
| Right Pot | A1 |
| Bluetooth TX | PA_11 |
| Bluetooth RX | PA_12 |
| Fire Button | D4 |

## Getting Started

### Prerequisites
- [VS Code](https://code.visualstudio.com/)
- [PlatformIO IDE Extension](https://platformio.org/install/ide?install=vscode)

### Setup
1. Clone this repository
2. Open the folder in VS Code
3. PlatformIO will automatically detect the project and install dependencies
4. Connect your Nucleo F401RE via USB
5. Click **Build** (checkmark icon) to compile
6. Click **Upload** (arrow icon) to flash

### Project Structure
```
├── platformio.ini          # PlatformIO configuration
├── src/
│   └── main.cpp            # Main application code
├── lib/
│   ├── QEI/                # Quadrature Encoder Interface library
│   │   ├── QEI.h
│   │   └── QEI.cpp
│   └── C12832/             # LCD display library
│       ├── C12832.h
│       ├── C12832.cpp
│       ├── GraphicsDisplay.h
│       ├── GraphicsDisplay.cpp
│       ├── TextDisplay.h
│       ├── TextDisplay.cpp
│       └── Small_7.h
└── include/                # Project-level headers (if needed)
```

## Libraries
- **QEI** by Aaron Berk (ARM) — Quadrature encoder pulse counting
- **C12832** by Peter Drescher (DC2PD) — SPI LCD display driver

## Notes
- The QEI library callback syntax has been updated from Mbed 2 style to Mbed OS 5 compatible `callback()` syntax
- The project uses `framework = mbed` in PlatformIO, which uses Mbed OS 5 (backwards compatible with Mbed 2 API)
- Upload protocol is set to `stlink` (default for Nucleo boards)
