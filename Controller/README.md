# Overview
The DTO-Project/Controller folder is responsible for housing all code that interacts with the hardware, including GPIO thruster control, Windows-based manual testing simulation, reference example programs, and session log output.

# File Tree
```
DTO-Project/
└── Controller/
    ├── Examples/
    │   ├── GPIO_Example.cpp
    │   ├── Relay_Cycle_Example.cpp
    │   └── Relay_WASD_Example.cpp
    ├── Logs/
    │   ├── Activity_Logs.csv
    │   └── Keybind_Logs.csv
    ├── DTOController.cpp
    ├── DTOManualTesting.cpp
    ├── DTOManualTesting.exe
    └── README.md
```

## File Tree Description
- **Examples/** Subfolder containing standalone GPIO reference programs used for hardware validation and development reference.
- **Examples/GPIO_Example.cpp** Simple single-pin GPIO toggle example using libgpiod on a Raspberry Pi 5; toggles GPIO0 on and off.
- **Examples/Relay_Cycle_Example.cpp** Cycles through all 24 GPIOs on a relay board attached to a Raspberry Pi 5, with clean SIGINT shutdown handling.
- **Examples/Relay_WASD_Example.cpp** Non-blocking WASD keyboard input example that toggles GPIO0–3 on a Raspberry Pi 5 using libgpiod and POSIX terminal control.
- **Logs/** Subfolder holding the most recent session log CSVs written by DTOController.cpp or DTOManualTesting.cpp.
- **Logs/Activity_Logs.csv** Timestamped STATUS and ERROR code entries logged by the controller across a session.
- **Logs/Keybind_Logs.csv** Timestamped key event records (type, direction, key name) logged per keypress during Operational Mode.
- **DTOController.cpp** Primary Raspberry Pi 5 GPIO thruster controller; implements a three-mode menu system (Menu, Startup Sequence, Operational), maps 12 keybinds to thruster GPIO pairs across all six degrees of freedom, and logs all activity to CSV files.
- **DTOManualTesting.cpp** Windows-compatible manual testing version of DTOController.cpp; replaces libgpiod GPIO calls with [SIM] console output and POSIX terminal input with Windows `_kbhit()`/`_getch()` equivalents; mirrors all controller logic exactly.
- **DTOManualTesting.exe** Compiled Windows executable of DTOManualTesting.cpp for testing without a Raspberry Pi.
- **README.md** General overview of project folder.
