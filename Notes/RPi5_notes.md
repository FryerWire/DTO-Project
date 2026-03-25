# Project Control & Hardware Documentation

This manual covers the Design Test Objective (DTO) and the technical implementation for GPIO control on the Raspberry Pi 5.



# I. DTO: Design Test Objective

## Core Objectives
The system must demonstrate stable command over the following:
- **Translational Control**: Linear movement across axes.
- **Attitude Control**: Maintaining or changing orientation.
- **Rotational Control**: Precise movement around the center of mass.

## Hardware Manifest
### Primary Electronics
| Item | Description | Resource |
| :--- | :--- | :--- |
| **Microcontroller** | Raspberry Pi 5 | [Adafruit](https://www.adafruit.com/product/5812) |
| **Power Management** | Mini Pushbutton Power Switch | [Pololu](https://www.pololu.com/product/2809/specs) |
| **Imaging** | Docking Camera | — |

### Propulsion & Pneumatics
- **Thrusters**: [Brass Solenoid Valves (12V DC)](https://www.electricsolenoidvalves.com/1-4-12v-dc-electric-brass-solenoid-valve/)
- **Plumbing**: Air Tubing & Tubing Splitters
- **Operating Pressure**: ~100 psi (Single Source)

### Connectors
- **Waterproof 2-Pin**: [Amazon](https://www.amazon.com/dp/B01LCV8DXQ)
- **8-Pin Male (Main)**: [Mouser (UTS)](https://www.mouser.com/ProductDetail/souriau/uts1gjc128p)
- **Male Pins**: [Mouser (RM)](https://www.mouser.com/ProductDetail/souriau/rm20m12k)

## Critical Design Factors
> [!CAUTION]
> **Drag & Balance**: High-pressure air systems are sensitive to fluid resistance. Ensure the center of gravity aligns with thruster vectors to prevent unintended tumbling.



# II. Raspberry Pi 5 GPIO Control

## Architecture & Firmware
- **Controller**: RP1 I/O Controller.
- **Device Path**: `/dev/gpiochip0`.
- **Library**: `libgpiod` (Standard for RPi 5).

## Hardware Specifications
- **Logic Level**: 3.3V (Inputs/Outputs). **Not 5V tolerant.**
- **Power Pins**: 5V (x2) and 3.3V (x2) available.
- **Note**: BCM GPIO names **do not** match physical pin numbers.

### Pin Modes
| Interface | Pins |
| :--- | :--- |
| **Hardware PWM** | GPIO 12, 13, 18, 19 |
| **I2C (Standard)** | Data (GPIO 2), Clock (GPIO 3) |
| **SPI0** | MOSI (10), MISO (9), SCLK (11), CE0 (8) |
| **UART (Serial)** | TX (GPIO 14), RX (GPIO 15) |



## ## C++ Implementation (`libgpiod`)

### 1. Initialization & Pin Definition
```cpp
#include <gpiod.h>
#include <iostream>
#include <unistd.h>

const char *chip_path = "/dev/gpiochip0";
const unsigned int sw1 = 0; // GPIO0

// Configure settings
struct gpiod_line_settings *settings = gpiod_line_settings_new();
gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

if (gpiod_line_config_add_line_settings(line_cfg, &sw1, 1, settings) < 0) {
    perror("gpiod_line_config_add_line_settings");
}


// Cycle through 24 pins
for (unsigned int i = 0; i < 24; i++) {
    gpiod_line_request_set_value(req, i, GPIOD_LINE_VALUE_ACTIVE);
    sleep(1);
    gpiod_line_request_set_value(req, i, GPIOD_LINE_VALUE_INACTIVE);
}


// Logic for WASD control mapping
switch (ch) {
    case 'w': idx = 0; break;
    case 'a': idx = 1; break;
    case 's': idx = 2; break;
    case 'd': idx = 3; break;
}

state[idx] = !state[idx]; // Toggle boolean state
gpiod_line_request_set_value(req, idx, state[idx] ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);

g++ relay_wasd_toggle.cpp -lgpiod -o relay_wasd_toggle
sudo ./relay_wasd_toggle
```
