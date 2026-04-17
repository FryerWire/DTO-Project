# Overview
The DTO-Project/Controller/Examples folder is responsible for housing standalone GPIO reference programs used during early hardware validation and development on the Raspberry Pi 5.

# File Tree
```
DTO-Project/
└── Controller/
    └── Examples/
        ├── GPIO_Example.cpp
        ├── Relay_Cycle_Example.cpp
        ├── Relay_WASD_Example.cpp
        └── README.md
```

## File Tree Description
- **GPIO_Example.cpp** Simple single-pin GPIO toggle using libgpiod on a Raspberry Pi 5; demonstrates the minimal setup required to activate and deactivate GPIO0.
- **Relay_Cycle_Example.cpp** Cycles through all 24 GPIOs on a relay board attached to a Raspberry Pi 5 one by one, with SIGINT (Ctrl+C) signal handling for clean shutdown.
- **Relay_WASD_Example.cpp** Non-blocking WASD keyboard input example that maps W/A/S/D to GPIO0–3 on a Raspberry Pi 5 using libgpiod, POSIX termios, and fcntl.
- **README.md** General overview of project folder.
