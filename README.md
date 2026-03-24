
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
├── Computation/
│   ├── TEXT
│   └── README.md
├── Controller/
│   ├── TEXT
│   └── README.md
├── Documentation/
│   ├── TEXT
│   └── README.md
├── Images/
│   ├── TEXT
│   └── README.md
├── Logs/
│   ├── TEXT
│   └── README.md
└── Simulator/
│   ├── TEXT
│   └── README.md
└── README.md
```



### File Descriptions
#### Data/
- `live_serial_monitoring.py`: Real-time serial monitor for incoming sensor data.
- `post_data_processing.py`: Reads CSV logs, computes statistics, and generates graphs.
- `static_data_test.csv`: Sample test data for post-processing validation.
- `dynamic_data_test.csv`: Sample test data for post-processing validation.
- `README.md`: Explains the purpose and usage of the `Data/` folder.

#### Documentation/
- `LaTeX/docs.tex`: Primary LaTeX source file for full project report.
- `LaTeX/docs.pdf`: Compiled report document.
- `LaTeX/*.aux, *.fdb_latexmk, *.fls, *.log, *.synctex.gz`: Auxiliary LaTeX files.
- `changelog.txt`: Chronological list of project changes and additions.
- `README.md`: Overview and compilation instructions for documentation.

#### Images/
- `image.png`: Output graph generated from data processing.
- `README.md`: Explains the origin and purpose of included images.

#### src/
- `main.cpp`: Core embedded software written for the Clue board.
- `testing.exe`: Executable simulation for data testing without hardware.
- `README.md`: Overview and usage for the source folder.

#### Utilities/
- `adafruit-circuitpython-clue_nrf52840_express-en_US-9.2.7.uf2`: CircuitPython firmware for Clue board (used for formatting internal flash).
- `README.md`: Documentation for firmware and board configuration.
