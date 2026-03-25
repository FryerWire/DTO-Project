
# Changelogs

## Version 0.2.0
This update introduces new lecture materials, hardware specifications, and code examples, alongside the initial framework for the controller.

### New Features & Additions
**Simulator (`Simulator/`)**
- Added `controller.cpp` as the starting foundation for the overall controller.
- Added the professor's example code for reference:
    - `GPIO_Example.cpp` (GPIO toggles)
    - `Relay_Cycle_Example.cpp` (Relay Cycle toggles)
    - `Relay_WASD_Example.cpp` (Relay WASD toggles)

**Lecture Materials (`Lecture/`)**
- Added `DTO_Lecture.pdf` and `GPIO_Lecture.pdf` lecture notes.
- Added the `Harness_Build_Procedure.pdf` instruction guide.

**Documentation & Notes (`Notes/` & Root)**
- Added `DTO_notes.md` and `RPi5_notes.md` containing personal study notes.
- Added `GitHub_links.md` containing reference links to similar open-source projects.



## Version 0.1.1
This update focuses on project documentation and AI prompt configurations, introducing new templates to automate and standardize READMEs and changelogs.

### New Features & Changes
**Prompts (`DTO-Project/Prompts/`)**
- Added `changelogs_prompt.txt` to automate changelog generation.
- Added `READMEs_prompt.txt` for standardized README generation.
- Added `GEMINI.md` containing instructions for AI-assisted updates.

### Changes
**Prompts (`DTO-Project/Prompts/`)**
- Updated `README.md` to include the new prompt files in the file tree and description.

**Documentation (`DTO-Project/Documentation/`)**
- Updated `README.md` to include external project links for Error Codes, Controller, and Simulator documentation.

**Logs (`DTO-Project/Logs/`)**
- Updated `README.md` to include `changelogs.md` in the file tree.



## Version 0.1.0
Initial project setup. 

### New Features
**Computation (`DTO-Project/Computation/`)**
- Added the `computation.py` file.

**Controller (`DTO-Project/Controller/`)**
- Added the `controller.cpp` file.

**Simulator (`DTO-Project/Simulator/`)**
- Added the `simulator.py` file.