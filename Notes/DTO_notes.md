# DTO (Design Test Objective)

This document outlines the requirements, equipment, and critical considerations for the thruster control system test.



## Core Objectives
The following control parameters are required for a successful test:
- **Translational Control**: Movement along the X, Y, and Z axes.
- **Attitude Control**: Management of the craft's orientation.
- **Rotational Control**: Precise movement around the center of mass (Roll, Pitch, Yaw).



## System Specifications

### General Information
- **Pneumatics**: Single-source compressed air.
- **Operating Pressure**: Approximately 100 psi.

### Critical Considerations
- **Drag**: High-pressure air systems can be sensitive to aerodynamic or fluid resistance.
- **Buoyancy and Balance**: Ensure the center of mass is aligned with thrust vectors to prevent unintended rotation.



## Hardware Manifest

### Primary Electronics & Control
- **Raspberry Pi 5**: Main flight computer.
- **Micro-SD Card**: Operating system and data logging.
- **Mini Pushbutton Power Switch**: Manual power cycle control.
- **Docking Camera**: Visual navigation and alignment.

### Propulsion System
- **Brass Solenoid Valves**: Used as thrusters for air release.
- **Air Tubing & Splitters**: Pneumatic routing from the single source to individual valves.

### Connectors & Wiring
- **2-Pin Waterproof Connectors**: For valve power and signal.
- **8-Pin Male Connector**: Main interface (Souriau UTS Series).
- **Male Pins**: Compatible crimp pins for the 8-pin housing.



## Equipment & Data Sheet

| Item | Resource Link |
| :--- | :--- |
| **Raspberry Pi 5** | [Adafruit Product Page](https://www.adafruit.com/product/5812) |
| **Power Switch** | [Pololu Specs](https://www.pololu.com/product/2809/specs) |
| **Solenoid Valves** | [Electric Solenoid Valves](https://www.electricsolenoidvalves.com/1-4-12v-dc-electric-brass-solenoid-valve/) |
| **2-Pin Connectors** | [Amazon Listing](https://www.amazon.com/dp/B01LCV8DXQ) |
| **8-Pin Connector** | [Mouser Catalog](https://www.mouser.com/ProductDetail/souriau/uts1gjc128p/?qs=gjHRuiJX3UYavAmbgytN1A==) |
| **Male Pins** | [Mouser Catalog](https://www.mouser.com/ProductDetail/souriau/rm20m12k/?qs=TAV84hLEf50oZ3JfjrCd0g==) |


## Thruster Tracking Table
| Thruster ID | Function (e.g., Pitch Up) | Valve Port | Status/Notes |
| :--- | :--- | :--- | :--- |
| 01 | | | |
| 02 | | | |
| 03 | | | |
| 04 | | | |
