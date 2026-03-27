
# Changelogs

## Version 0.3.1

### New Features
**LaTeX (`Application Codes`)**
- Added a new document called `Application Codes` with up to date status and fault codes. 

### Changes
**Logs (`Logs/`)**
- Simplified the logging files to `Activity_log.csv` and `Keybind_Log.csv` which collect all the application codes and keybind triggers.

**Controller (`Controller_in_prog.cpp`)**
- Modified the file writers to update to the new Application Codes. 



## Version 0.3.0 - Physical Controls and Data Logging Update
This is the Physical Controls and Data Logging Update where all of the keyboard keybinds have now been linked to each thruster. Logs are now estabilished for Error and Keybinds. Lastly, custom keyboard commands are now a feature to help manuever the drone easier. 

### New Features
**Controller (`controller.cpp`)**
- Added keybinds for translation and rotational controls.
- Added the CapsLk key being a Continuous and Pulsating custom command. 
- Added Keybind and Error Logs that are stored in `Logs/` folder. 
- All keybinds are linked to specified thrusters. 

**Documentation (`README.md`)**
- Added `Activity Codes`

### Changes
**Examples (`Controller/Examples`)**
- Moved all the Professors code examples to `Examples/`.


### Removed
**Prompts (`Prompts/`)**
- Removed the `changelogs_prompt.txt` as making the changelogs automated proved to be unfeasible. 



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