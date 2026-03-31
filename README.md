# DTO Project
Spring Semester Senior Design GOAT Drone Design Test Objective Code Repository

**Code Written By:** Maxwell Seery and Stren



## Overview  
- Text



## Features
- Text



## Hardware Info
- Text



## Software Info
- Text

/*
    DTO Controller

    Features:
    - Logs key events with timestamps, types, directions, and modes to Keybind_Log.csv.
    - Supports Continuous (Caps Lock OFF) and Pulse (Caps Lock ON) modes.
    - Maps specific keys to translation and rotation movements.
    - Activity_Log.csv tracks high-level Application Codes (STATUS and ERROR).
    - Toggle Modes: ~ + S (Startup Sequence) | ~ + O (Operational Mode).

    Key Mappings:
    Translation:
    - Forward  (+X) : W
    - Backward (-X) : S
    - Left      (+Y) : D
    - Right      (-Y) : A
    - Up        (+Z) : Space
    - Down      (-Z) : Shift

    Rotation:
    - CW Pitch  : Ctrl + Right
    - CCW Pitch : Ctrl + Left
    - CC Roll   : Up Arrow
    - CCW Roll  : Down Arrow
    - CW Yaw    : Right Arrow
    - CCW Yaw   : Left Arrow

    Application Codes (Logged to Activity_Log.csv):
    Status Codes:
    - STATUS-00: Startup Successful     - System initialized and files opened.
    - STATUS-01: Session Ended          - Main loop exited.
    - STATUS-02: Session Started        - Main loop entered.
    - STATUS-03: Key Registered         - A valid movement key was processed.
    - STATUS-04: Shutdown Successful    - Program exited and cleaned up without error.
    - STATUS-05: Mode Changed           - System switched between Startup and Operational modes.
    - STATUS-10: Sequence Initiated     - Startup Sequence (~S) has begun execution.
    - STATUS-11: Connector Test Start   - A specific Rack Connector (1-3) has started testing.
    - STATUS-12: Connector Test Pass    - A specific Rack Connector (1-3) passed all pulse tests.
    - STATUS-13: GPIO State Change      - A GPIO pin was successfully toggled (ON/OFF).
    - STATUS-14: Sequence Complete      - All Startup tests finished without interruption.

    Error Codes:
    - ERROR-00: Startup Failure         - Occurs if the program cannot initialize logs or paths.
    - ERROR-01: File Failed to Close    - Occurs if a file handle remains locked at exit.
    - ERROR-02: Write Failure           - Occurs if the log file is locked during a log attempt.
    - ERROR-03: Incorrect Keybind       - Occurs when a key is pressed that has no mapped function.
    - ERROR-04: System Ghosting         - Occurs when non-printable scan codes leak into the buffer.
    - ERROR-05: Shutdown Failed         - Occurs if resources fail to release during exit.
    - ERROR-06: Mode Switch Denied      - User tried to switch to ~S while in ~O (Illegal Move).
    - ERROR-10: Sequence Aborted        - User pressed ESC during a Startup Sequence test.
*/



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
