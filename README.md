# DTO Project
Spring Semester Senior Design GOAT Drone Design Test Objective Code Repository

**Code Written By:** Maxwell Seery

---

# Overview
The DTO-Project repository is responsible for housing all software, documentation, and data assets for the Design Test Objective drone thruster control system. The project controls 12 thrusters across all six degrees of freedom via GPIO on a Raspberry Pi 5, supports Windows-based manual testing simulation, and provides post-session trajectory visualization and telemetry analysis.

# File Tree
```
DTO-Project/
├── Computations/
│   ├── compuation.py
│   └── README.md
├── Controller/
│   ├── Examples/
│   │   ├── GPIO_Example.cpp
│   │   ├── Relay_Cycle_Example.cpp
│   │   ├── Relay_WASD_Example.cpp
│   │   └── README.md
│   ├── Logs/
│   │   ├── Activity_Logs.csv
│   │   ├── Keybind_Logs.csv
│   │   └── README.md
│   ├── DTOController.cpp
│   ├── DTOManualTesting.cpp
│   ├── DTOManualTesting.exe
│   └── README.md
├── Documentation/
│   ├── ApplicationCodes.txt
│   ├── DTO Drone Propulsion Control Tables.pdf
│   ├── DTO_Code_Report.pdf
│   ├── Raspberry Pi 5 Pin Layout.jpg
│   └── README.md
├── Images/
│   ├── 3D_Chunk_01.png
│   ├── 3D_Chunk_02.png
│   ├── 3D_Chunk_03.png
│   ├── Acceleration.png
│   ├── Attitude.png
│   ├── Position.png
│   ├── Velocity.png
│   └── README.md
├── Lecture/
│   ├── DTO_Lecture.pdf
│   ├── GPIO_Lecture.pdf
│   ├── Harness_Build_Procedure.pdf
│   └── README.md
├── Logs/
│   ├── Activity_Log.csv
│   ├── Keybind_Log.csv
│   ├── changelogs.md
│   └── README.md
└── README.md
```

---

# Computations/

## Overview
The DTO-Project/Computations folder is responsible for providing all post-session data analysis, including 3D trajectory visualization and 2D telemetry plots derived from controller log output.

## File Tree
```
DTO-Project/
└── Computations/
    ├── compuation.py
    └── README.md
```

### File Tree Description
- **compuation.py** Reads Keybind_Log.csv output from DTOController or DTOManualTesting, reconstructs a 6-DOF trajectory via sequential rotation matrices and body-frame translations, generates segmented 3D trajectory chunk PNGs, and produces four 2D telemetry plots (position, attitude, velocity, acceleration) saved to the Images/ directory.
- **README.md** General overview of project folder.

---

# Controller/

## Overview
The DTO-Project/Controller folder is responsible for housing all code that interacts with the hardware, including GPIO thruster control, Windows-based manual testing simulation, reference example programs, and session log output.

## File Tree
```
DTO-Project/
└── Controller/
    ├── Examples/
    │   ├── GPIO_Example.cpp
    │   ├── Relay_Cycle_Example.cpp
    │   ├── Relay_WASD_Example.cpp
    │   └── README.md
    ├── Logs/
    │   ├── Activity_Logs.csv
    │   ├── Keybind_Logs.csv
    │   └── README.md
    ├── DTOController.cpp
    ├── DTOManualTesting.cpp
    ├── DTOManualTesting.exe
    └── README.md
```

### File Tree Description
- **Examples/** Subfolder containing standalone GPIO reference programs used for hardware validation and development reference.
- **Examples/GPIO_Example.cpp** Simple single-pin GPIO toggle using libgpiod on a Raspberry Pi 5; demonstrates the minimal setup required to activate and deactivate GPIO0.
- **Examples/Relay_Cycle_Example.cpp** Cycles through all 24 GPIOs on a relay board attached to a Raspberry Pi 5 one by one, with SIGINT (Ctrl+C) signal handling for clean shutdown.
- **Examples/Relay_WASD_Example.cpp** Non-blocking WASD keyboard input example that maps W/A/S/D to GPIO0–3 on a Raspberry Pi 5 using libgpiod, POSIX termios, and fcntl.
- **Examples/README.md** General overview of Examples subfolder.
- **Logs/** Subfolder holding the most recent session log CSVs written by DTOController.cpp or DTOManualTesting.cpp.
- **Logs/Activity_Logs.csv** Timestamped log of all STATUS and ERROR code entries emitted during a controller session; columns are Time(s), Code, and Description.
- **Logs/Keybind_Logs.csv** Timestamped log of all key events recorded during Operational Mode; columns are Time(s), Type, Direction, and Key.
- **Logs/README.md** General overview of Logs subfolder.
- **DTOController.cpp** Primary Raspberry Pi 5 GPIO thruster controller; implements a three-mode menu system (Menu, Startup Sequence, Operational), maps 12 keybinds to thruster GPIO pairs across all six degrees of freedom, and logs all activity to CSV files.
- **DTOManualTesting.cpp** Windows-compatible manual testing version of DTOController.cpp; replaces libgpiod GPIO calls with [SIM] console output and POSIX terminal input with Windows `_kbhit()`/`_getch()` equivalents; mirrors all controller logic exactly.
- **DTOManualTesting.exe** Compiled Windows executable of DTOManualTesting.cpp for testing without a Raspberry Pi.
- **README.md** General overview of project folder.

---

# Documentation/

## Overview
The DTO-Project/Documentation folder is responsible for holding all reference documentation for the project, including hardware pin layouts, application code references, and design reports.

## Links
- **Application Codes** https://www.overleaf.com/6877435425gbgbdpcynmrg#c6830e
- **Controller** https://www.overleaf.com/6356412253nhmztbkhykpq#b85af6
- **Simulator** https://www.overleaf.com/3725659625jyvnctkwhyfc#533a65

## File Tree
```
DTO-Project/
└── Documentation/
    ├── ApplicationCodes.txt
    ├── DTO Drone Propulsion Control Tables.pdf
    ├── DTO_Code_Report.pdf
    ├── Raspberry Pi 5 Pin Layout.jpg
    └── README.md
```

### File Tree Description
- **ApplicationCodes.txt** Master reference listing all STATUS and ERROR codes used across DTOController.cpp, DTOManualTesting.cpp, and compuation.py, with full titles and descriptions.
- **DTO Drone Propulsion Control Tables.pdf** Propulsion control tables for the DTO drone, including thruster mapping and force allocation data.
- **DTO_Code_Report.pdf** Formal code report documenting the DTOController software design, logic, and implementation details.
- **Raspberry Pi 5 Pin Layout.jpg** Physical GPIO pin layout diagram for the Raspberry Pi 5, used as reference for thruster wiring.
- **README.md** General overview of project folder.

---

# Images/

## Overview
The DTO-Project/Images folder is responsible for holding all image output generated by the Computations script, including segmented 3D trajectory chunks and 2D telemetry plots.

## File Tree
```
DTO-Project/
└── Images/
    ├── 3D_Chunk_01.png
    ├── 3D_Chunk_02.png
    ├── 3D_Chunk_03.png
    ├── Acceleration.png
    ├── Attitude.png
    ├── Position.png
    ├── Velocity.png
    └── README.md
```

### File Tree Description
- **3D_Chunk_01.png** 3D trajectory plot for the first 30-second time chunk, color-coded by degree of freedom with time-tick labels.
- **3D_Chunk_02.png** 3D trajectory plot for the second 30-second time chunk.
- **3D_Chunk_03.png** 3D trajectory plot for the third 30-second time chunk.
- **Acceleration.png** 2D time-series plot of linear acceleration (Ax, Ay, Az) derived from the global position trajectory.
- **Attitude.png** 2D time-series plot of Euler angles (Roll, Pitch, Yaw in degrees) computed from the cumulative rotation matrix.
- **Position.png** 2D time-series plot of absolute world-frame position (X, Y, Z) integrated from all movement events.
- **Velocity.png** 2D time-series plot of linear velocity (Vx, Vy, Vz) derived from the global position trajectory.
- **README.md** General overview of project folder.

---

# Lecture/

## Overview
The DTO-Project/Lecture folder is responsible for holding all lecture and procedural material related to the project, covering the DTO mission, GPIO hardware, and physical harness construction.

## File Tree
```
DTO-Project/
└── Lecture/
    ├── DTO_Lecture.pdf
    ├── GPIO_Lecture.pdf
    ├── Harness_Build_Procedure.pdf
    └── README.md
```

### File Tree Description
- **DTO_Lecture.pdf** Lecture material covering the Design Test Objective, mission goals, and drone system overview.
- **GPIO_Lecture.pdf** Lecture material covering GPIO fundamentals, libgpiod usage, and Raspberry Pi 5 hardware interfacing.
- **Harness_Build_Procedure.pdf** Step-by-step procedure for physically constructing the thruster wiring harness used in the DTO hardware setup.
- **README.md** General overview of project folder.

---

# Logs/

## Overview
The DTO-Project/Logs folder is responsible for holding project-level log files, including the running changelog that tracks all version updates and feature changes across the codebase.

## File Tree
```
DTO-Project/
└── Logs/
    ├── Activity_Log.csv
    ├── Keybind_Log.csv
    ├── changelogs.md
    └── README.md
```

### File Tree Description
- **Activity_Log.csv** Timestamped STATUS and ERROR code log from the most recent controller session; mirrors the format written by DTOController.cpp and DTOManualTesting.cpp.
- **Keybind_Log.csv** Timestamped key event log from the most recent controller session; records type, direction, and key name per event.
- **changelogs.md** Running version history for the project, documenting new features, bug fixes, and structural changes per release.
- **README.md** General overview of project folder.
