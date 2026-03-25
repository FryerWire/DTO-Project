# DTO Project
Spring Semester Senior Design GOAT Drone Design Test Objective Code Repository

**Code Written By:** Maxwell Seery and Stren

## Note
Most of this info is copied from 361 so some files and info are wrong for this project. 



## Overview  
Text

The project is divided into two primary parts:
- **Data Acquisition**: Text
- **Post-Processing**: Text



## Features
- Text



## Hardware Info
- **Microcontroller**: Adafruit Clue nRF52840 (Cortex-M4, 64 MHz, 1MB code flash)
- **Sensors**:
  - BMP280 – Altitude, Pressure, and Temperature
  - LSM6DS33 – Accelerometer and Gyroscope (6-DoF)
- **Power Supply**: USB or 3x AAA batteries
- **Onboard Storage**: 2MB QSPI flash (formatted to FAT32)
- **Outputs**: NeoPixel RGB LED, white illumination LEDs



## Software Info
### Requirements
- **Text** 
- **Python 3.8+** 



## Libraries Used
### PlatformIO (C++)
```ini
lib_deps =
  adafruit/Adafruit Unified Sensor
  adafruit/Adafruit BMP280 Library
  adafruit/Adafruit LSM6DS
  adafruit/Adafruit NeoPixel@^1.12.5
  adafruit/SdFat - Adafruit Fork@^2.2.54
  adafruit/Adafruit SPIFlash@^5.1.1
```



## File Structure
### File Tree
```
DTO-Project/
├───Computations/
│   ├───compuation.py
│   └───README.md
├───Controller/
│   ├───controller.cpp
│   ├───GPIO_Example.cpp
│   ├───README.md
│   ├───Relay_Cyle_Example.cpp
│   └───Relay_WASD_Example.cpp
├───Documentation/
│   └───README.md
├───Images/
│   └───README.md
├───Lecture/
│   ├───DTO_Lecture.pdf
│   ├───GPIO_Lecture.pdf
│   ├───Harness_Build_Procedure.pdf
│   └───README.md
├───Logs/
│   ├───changelogs.md
│   └───README.md
├───Notes/
│   ├───code_examples.md
│   ├───DTO_notes.md
│   ├───README.md
│   └───RPi5_notes.md
├───Prompts/
│   ├───changelogs_prompt.txt
│   ├───GEMINI.md
│   ├───README.md
│   └───READMEs_prompt.txt
├───Simulator/
│   ├───README.md
│   └───simulator.py
└───README.md
```



### File Descriptions
#### Computations/
- **compuation.py**: EMPTY DOCUMENT.
- **README.md**: General overview of project folder.

#### Controller/
- **controller.cpp**: A C++ program that simulates thruster control and logs keypresses.
- **GPIO_Example.cpp**: Example C++ code for GPIO control on a Raspberry Pi.
- **README.md**: General overview of project folder.
- **Relay_Cyle_Example.cpp**: A C++ program to cycle through 24 GPIOs on a Raspberry Pi.
- **Relay_WASD_Example.cpp**: A C++ program to toggle GPIOs on a Raspberry Pi using WASD keyboard input.

#### Documentation/
- **README.md**: General overview of project folder.

#### Images/
- **README.md**: General overview of project folder.

#### Lecture/
- **DTO_Lecture.pdf**: Contains information about the DTO project.
- **GPIO_Lecture.pdf**: Contains information about GPIO.
- **Harness_Build_Procedure.pdf**: Contains information about the harness build procedure.
- **README.md**: General overview of project folder.

#### Logs/
- **changelogs.md**: Contains all changelogs for the project.
- **README.md**: General overview of project folder.

#### Notes/
- **code_examples.md**: Contains links to relevant GitHub repositories.
- **DTO_notes.md**: Detailed notes on the Design Test Objective (DTO).
- **README.md**: General overview of project folder.
- **RPi5_notes.md**: Notes on the Raspberry Pi 5 and GPIO control.

#### Prompts/
- **changelogs_prompt.txt**: Prompt for generating changelogs.
- **GEMINI.md**: Instructions for the Gemini model to follow.
- **README.md**: General overview of project folder.
- **READMEs_prompt.txt**: Prompt for generating READMEs.

#### Simulator/
- **README.md**: General overview of project folder.
- **simulator.py**: EMPTY DOCUMENT.

