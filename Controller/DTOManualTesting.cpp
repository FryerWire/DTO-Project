/*
DTOManualTesting.cpp
DTO Manual Testing Program - Windows Manual Test Environment

Features:
- Windows-compatible manual testing version of DTOController.cpp. Replicates all DTOController logic exactly, substituting libgpiod GPIO control with [SIM] console output and replacing POSIX terminal input with Windows _kbhit()/_getch() equivalents.
- Logs all key events and high-level application activities to separate CSV files (Keybind_Log.csv and Activity_Log.csv) with hardware-based elapsed timestamps and standardized STATUS/ERROR codes.
- Implements a three-mode menu system: Menu Mode (0) for mode selection, Startup Sequence Mode (1) for simulated hardware validation across three rack connectors, and Operational Mode (2) for real-time thruster control simulation.
- Startup Sequence Mode simulates a two-phase test per rack connector: Phase 1 performs four individual 1-second ON/OFF pulses, and Phase 2 performs four double 0.5-second pulse pairs. Always reports 12/12 GPIO activated (no real hardware to fail).
- Operational Mode fires simulated thrusters continuously for the duration of a key hold and deactivates all thrusters when the key is released.
- Maps 12 keybinds (W, S, A, D, E, Q for translation; I, K, U, O, J, L for rotation) to specific simulated GPIO pin pairs matching DTOController hardware mappings exactly.
- Uses Windows _kbhit()/_getch() for non-blocking terminal input, mirroring DTOController's POSIX checkKeyboardInput() behavior.
- Uses a configurable no-input cycle counter (RELEASE_CYCLES) to bridge terminal key repeat delay gaps, matching DTOController release detection logic.
- Uses chrono::high_resolution_clock for hardware-based timing, matching DTOController timestamp precision.

Functions:
- main(): Initializes the log directory and log files; runs the primary input polling loop for mode management and thruster control simulation; performs cleanup on ESC exit.
- getElapsedTime(): Returns the number of seconds elapsed since programStartTime using chrono::high_resolution_clock.
- formatTimestamp(double t): Formats a floating-point elapsed time value into a "seconds:centiseconds" string (e.g., "12:07") for use in log entries.
- logActivity(string code, string description): Appends a timestamped STATUS or ERROR code entry to Activity_Log.csv and echoes it to stdout.
- logKeyData(char type, string direction, string keyName, char statusChar): Appends a timestamped key event row to Keybind_Log.csv; triggers STATUS-011 for valid new key presses.
- processMovementAction(string keyId): Maps the key to a translation or rotation action, calls logKeyData and outputs [SIM] GPIO console output for the two associated thruster pins; logs ERROR-004 for unmapped keys.
- displayMenu(): Logs a STATUS-019 UI redraw event and prints the mode-appropriate menu to stdout based on the current value of programMode.
- checkKeyboardInput(): Returns non-zero if keyboard input is available, using Windows _kbhit(). Mirrors DTOController's POSIX checkKeyboardInput() interface.

Codes:
- STATUS-001: Session Started
- STATUS-002: Session Ended
- STATUS-003: Shutdown Successful
- STATUS-005: Log Directory Created Successfully
- STATUS-008: All Log Files Initialized with CSV Headers
- STATUS-009: Startup Successful: All Log Files Ready
- STATUS-010: Mode Changed
- STATUS-011: Key Registered
- STATUS-012: Startup Sequence Initiated
- STATUS-013: Startup Sequence Completed: All GPIO Activated
- STATUS-014: Rack Connector Test Started
- STATUS-015: Rack Connector Test Passed
- STATUS-016: GPIO Pin Activated (ON)
- STATUS-017: GPIO Pin Deactivated (OFF)
- STATUS-019: UI Menu Refreshed
- STATUS-020: Operational Mode Activated
- STATUS-106: All 12 Thrusters Deactivated on Key Release
- STATUS-107: All 12 Thrusters Deactivated on Program Exit
- STATUS-110: Translation Action Processed: GPIO Pair Activated
- STATUS-111: Rotation Action Processed: GPIO Pair Activated
- ERROR-000: Startup Failure: Log Path Inaccessible or Cannot Be Created
- ERROR-002: Keybind Log Write Failure: File Inaccessible
- ERROR-003: Log Directory Creation Failed: Check Permissions
- ERROR-004: Incorrect Keybind: No Mapping Found for Key
- ERROR-005: Startup Sequence Aborted by User via ESC
*/



// Libraries ========================================================================================================================================
// Standard Library Headers -------------------------------------------------------------------------------------------------------------------------
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <string>
#include <iomanip>
#include <vector>
#include <algorithm>

// Windows Specific Headers (replaces Linux fcntl/termios/ioctl/gpiod) ------------------------------------------------------------------------------
#include <windows.h>  // GetAsyncKeyState for ESC detection during startup sequence
#include <conio.h>    // _kbhit(), _getch() for non-blocking terminal input



using namespace std;



// Global Variables =================================================================================================================================
// Program State Variables --------------------------------------------------------------------------------------------------------------------------
int programMode = 0;                          // 0 = Menu, 1 = Startup Sequence, 2 = Operational Mode

// Logging and Input Tracking Variables -------------------------------------------------------------------------------------------------------------
chrono::high_resolution_clock::time_point programStartTime;  // Track actual program start time
string currentKeyPressed = "";                // Tracks currently pressed key
int noInputCount = 0;                         // Counts consecutive no-input poll cycles for release confirmation
const int RELEASE_CYCLES = 8;                 // Number of consecutive no-input cycles before confirming key release
const string logDirectoryPath = "./Logs/";    // Data logging path



// Function Prototypes ==============================================================================================================================
void displayMenu();
void logActivity(string code, string description);
void processMovementAction(string keyId);
void logKeyData(char type, string direction, string keyName, char statusChar);
double getElapsedTime();



/*
    getElapsedTime - Returns elapsed time in seconds since program start

    Returns:
    - double : The number of seconds elapsed since program initialization
*/
double getElapsedTime() {
    auto currentTime = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = currentTime - programStartTime;
    return elapsed.count();
}



/*
    formatTimestamp - Converts a double time value into a string formatted as "seconds:centiseconds"

    Parameters:
    - t (double) : The time value to format
*/
string formatTimestamp(double t) {
    int wholeSeconds = (int)t;
    int decimalSeconds = (int)((t - wholeSeconds) * 100 + 0.5);
    string decimalString = (decimalSeconds < 10) ? "0" + to_string(decimalSeconds) : to_string(decimalSeconds);
    return to_string(wholeSeconds) + ":" + decimalString;
}



/*
    logActivity - Logs a high-level activity or event with a standardized code and description, along with a timestamp

    Parameters:
    - code (string)        : A standardized code representing the type of activity or event (e.g., "STATUS-001", "ERROR-004")
    - description (string) : A human-readable description providing additional context about the activity or event
*/
void logActivity(string code, string description) {
    double timeCounter = getElapsedTime();
    string timeString = formatTimestamp(timeCounter);

    ofstream activityFile(logDirectoryPath + "Activity_Log.csv", ios_base::app);
    if (activityFile.is_open()) {
        activityFile << timeString << "," << code << "," << description << endl;
        activityFile.close();
    }

    cout << timeString << "," << code << "," << description << endl;
}



/*
    logKeyData - Logs detailed information about key events, including type, direction, and key name

    Parameters:
    - type (char)          : 'T' for Translation, 'R' for Rotation, 'N' for release, 'F' for error
    - direction (string)   : Direction of movement (e.g., "+X", "-P", "--" for release/error)
    - keyName (string)     : The name of the key that triggered the event ("-" for release)
    - statusChar (char)    : 'N' for new press, 'R' for release, 'E' for error
*/
void logKeyData(char type, string direction, string keyName, char statusChar) {
    double timeCounter = getElapsedTime();
    string timeString = formatTimestamp(timeCounter);
    ofstream keyLogFile(logDirectoryPath + "Keybind_Log.csv", ios_base::app);
    if (keyLogFile.is_open()) {
        keyLogFile << timeString << "," << type << "," << direction << "," << keyName << endl;
        keyLogFile.close();
        if (statusChar == 'N' && keyName != "-") {
            logActivity("STATUS-011", "Key Registered: " + keyName);
        }
    } else {
        logActivity("ERROR-002", "Write Failure: Keybind_Log is inaccessible");
    }
}



/*
    processMovementAction - Processes a key event for movement or rotation, logs the action, and outputs simulated GPIO activation
                            GPIO pin pairs match DTOController hardware mappings exactly.
                            setGPIOPin calls are replaced with [SIM] console output for Windows manual testing.

    Parameters:
    - keyId (string) : The identifier of the key that triggered the event (e.g., "W", "A", "K")
*/
void processMovementAction(string keyId) {
    // Translation Mappings -------------------------------------------------------------------------------------------------------------------------
    if (keyId == "W")      { logKeyData('T', "+X", "W", 'N'); cout << "[SIM] GPIO 0 ON | GPIO 8 ON"   << endl; logActivity("STATUS-110", "Translation +X: GPIO 0, 8 Activated");  }  // A1, A2
    else if (keyId == "S") { logKeyData('T', "-X", "S", 'N'); cout << "[SIM] GPIO 5 ON | GPIO 11 ON"  << endl; logActivity("STATUS-110", "Translation -X: GPIO 5, 11 Activated"); }  // F1, F2
    else if (keyId == "A") { logKeyData('T', "+Y", "A", 'N'); cout << "[SIM] GPIO 1 ON | GPIO 7 ON"   << endl; logActivity("STATUS-110", "Translation +Y: GPIO 1, 7 Activated");  }  // S1, S2
    else if (keyId == "D") { logKeyData('T', "-Y", "D", 'N'); cout << "[SIM] GPIO 2 ON | GPIO 6 ON"   << endl; logActivity("STATUS-110", "Translation -Y: GPIO 2, 6 Activated");  }  // P1, P2
    else if (keyId == "E") { logKeyData('T', "+Z", "E", 'N'); cout << "[SIM] GPIO 4 ON | GPIO 10 ON"  << endl; logActivity("STATUS-110", "Translation +Z: GPIO 4, 10 Activated"); }  // B1, B2
    else if (keyId == "Q") { logKeyData('T', "-Z", "Q", 'N'); cout << "[SIM] GPIO 3 ON | GPIO 9 ON"   << endl; logActivity("STATUS-110", "Translation -Z: GPIO 3, 9 Activated");  }  // T1, T2

    // Rotation Mappings ----------------------------------------------------------------------------------------------------------------------------
    else if (keyId == "K") { logKeyData('R', "-P", "K", 'N'); cout << "[SIM] GPIO 9 ON | GPIO 4 ON"   << endl; logActivity("STATUS-111", "Rotation -P: GPIO 9, 4 Activated");   }  // T2, B1
    else if (keyId == "I") { logKeyData('R', "+P", "I", 'N'); cout << "[SIM] GPIO 3 ON | GPIO 10 ON"  << endl; logActivity("STATUS-111", "Rotation +P: GPIO 3, 10 Activated");  }  // T1, B2
    else if (keyId == "U") { logKeyData('R', "-R", "U", 'N'); cout << "[SIM] GPIO 6 ON | GPIO 1 ON"   << endl; logActivity("STATUS-111", "Rotation -R: GPIO 6, 1 Activated");   }  // P2, S1
    else if (keyId == "O") { logKeyData('R', "+R", "O", 'N'); cout << "[SIM] GPIO 2 ON | GPIO 7 ON"   << endl; logActivity("STATUS-111", "Rotation +R: GPIO 2, 7 Activated");   }  // P1, S2
    else if (keyId == "J") { logKeyData('R', "-Y", "J", 'N'); cout << "[SIM] GPIO 0 ON | GPIO 11 ON"  << endl; logActivity("STATUS-111", "Rotation -Y: GPIO 0, 11 Activated");  }  // A1, F2
    else if (keyId == "L") { logKeyData('R', "+Y", "L", 'N'); cout << "[SIM] GPIO 8 ON | GPIO 5 ON"   << endl; logActivity("STATUS-111", "Rotation +Y: GPIO 8, 5 Activated");   }  // A2, F1

    else {
        logKeyData('F', "--", keyId, 'E');
        logActivity("ERROR-004", "Incorrect Keybind: No Mapping Found for Key");
    }
}



/*
    displayMenu - Displays the appropriate menu options based on the current program mode, with logging for UI redraws
                  Matches DTOController displayMenu() output exactly.
*/
void displayMenu() {
    logActivity("STATUS-019", "UI Redraw: Menu refreshed");
    if (programMode == 0) {
        cout << "\n=================================================\nDTO Program\n-------------------------------------------------\nProgram Modes:\n- 0   : Menu Mode\n- 1   : Startup Sequence Mode\n- 2   : Operational Mode\n- Esc : Quit Mode\n=================================================\n>> " << flush;
    } else if (programMode == 1) {
        cout << "\n=================================================\nStartup Sequence Mode\n-------------------------------------------------\nProgram Modes:\n- Esc : Quit Mode\n=================================================\n>> " << flush;
    } else if (programMode == 2) {
        cout << "\n=================================================\nOperational Mode (2)\n-------------------------------------------------\nProgram Modes:\n- 0   : Menu Mode\n- Esc : Quit Program\n=================================================\n>> " << flush;
    }
}



/*
    checkKeyboardInput - Returns non-zero if keyboard input is available without blocking
                         Windows equivalent of DTOController's POSIX checkKeyboardInput().
                         Uses _kbhit() in place of FIONREAD ioctl on stdin.

    Returns:
    - int : Non-zero if at least one byte of keyboard input is available, 0 otherwise
*/
int checkKeyboardInput() {
    return _kbhit();
}



/*
    main - The main entry point of the DTO Manual Testing software
           Mirrors DTOController main() exactly: same initialization sequence, same mode logic, same loop structure.
           GPIO calls replaced with [SIM] console output. Startup sequence always reports 12/12 (no real hardware).

    Returns:
    - int : 0 on successful execution, 1 on initialization failure
*/
int main() {
    // Initialize high-resolution timer
    programStartTime = chrono::high_resolution_clock::now();

    // Initialization and Setup ---------------------------------------------------------------------------------------------------------------------
    if (system(("mkdir " + logDirectoryPath + " 2>nul").c_str()) == 0) {
        logActivity("STATUS-005", "Directory Created: Log storage ready");
    } else {
        logActivity("ERROR-003", "Dir Creation Fail: Check permissions");
    }

    // Log File Initialization ----------------------------------------------------------------------------------------------------------------------
    ofstream keybindLogFile(logDirectoryPath + "Keybind_Log.csv", ios::trunc);
    ofstream activityLogFile(logDirectoryPath + "Activity_Log.csv", ios::trunc);
    if (keybindLogFile.is_open() && activityLogFile.is_open()) {
        logActivity("STATUS-008", "Path Validated: File Open Successful");
        keybindLogFile << "Time(s),Type,Direction,Key" << endl;
        activityLogFile << "Time(s),Code,Description" << endl;
        keybindLogFile.close(); activityLogFile.close();
        logActivity("STATUS-009", "Startup Successful: Files Ready");
    } else {
        logActivity("ERROR-000", "Startup Failure: Path Inaccessible");
        return 1;
    }

    // Main Program Loop ----------------------------------------------------------------------------------------------------------------------------
    logActivity("STATUS-001", "Session Started");
    displayMenu();

    while (true) {
        // Keyboard Input Handling ------------------------------------------------------------------------------------------------------------------
        if (checkKeyboardInput()) {
            unsigned char charInput = (unsigned char)_getch();
            // Reset no-input counter on any input received
            noInputCount = 0;
            // Handle ESC key for quitting the program ----------------------------------------------------------------------------------------------
            if (charInput == 27) break; // ESC

            // Handle input based on current program mode -------------------------------------------------------------------------------------------
            if (programMode == 0) {
                // Menu Mode: Only accepts mode selection inputs ------------------------------------------------------------------------------------
                if (charInput == '1') {
                    programMode = 1;
                    logActivity("STATUS-012", "Sequence Initiated");
                    displayMenu();

                    // Rack Connector Definitions (organized by rack)
                    int rackConnector1[] = {0, 1, 2, 3};
                    int rackConnector2[] = {4, 5, 6, 7};
                    int rackConnector3[] = {8, 9, 10, 11};
                    int* connectors[3] = {rackConnector1, rackConnector2, rackConnector3};

                    bool force_exit = false;

                    // Test each rack connector
                    for (int rackNum = 0; rackNum < 3; rackNum++) {
                        if (force_exit) break;

                        cout << "\nRack Connector " << (rackNum + 1) << " Test:" << endl;
                        logActivity("STATUS-014", "Testing Rack Connector " + to_string(rackNum + 1));

                        // Phase 1: Individual 1-second ON/OFF pulses for each GPIO
                        for (int g = 0; g < 4; g++) {
                            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { force_exit = true; break; }

                            int gpioPin = connectors[rackNum][g];
                            double phaseTime = g * 2.0;

                            cout << fixed << setprecision(2) << phaseTime << " GPIO " << gpioPin << " On" << endl;
                            logActivity("STATUS-016", "GPIO " + to_string(gpioPin) + " ON");
                            this_thread::sleep_for(chrono::milliseconds(1000));

                            phaseTime += 1.0;
                            cout << fixed << setprecision(2) << phaseTime << " GPIO " << gpioPin << " Off" << endl;
                            logActivity("STATUS-017", "GPIO " + to_string(gpioPin) + " OFF");
                            this_thread::sleep_for(chrono::milliseconds(1000));
                        }

                        // Phase 2: Double 0.5-second pulses for each GPIO
                        for (int g = 0; g < 4; g++) {
                            if (force_exit) break;

                            int gpioPin = connectors[rackNum][g];
                            double phaseTime = 8.0 + (g * 2.0);

                            for (int pulse = 0; pulse < 2; pulse++) {
                                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { force_exit = true; break; }

                                cout << fixed << setprecision(2) << phaseTime << " GPIO " << gpioPin << " On" << endl;
                                logActivity("STATUS-016", "GPIO " + to_string(gpioPin) + " ON (PULSE)");
                                this_thread::sleep_for(chrono::milliseconds(500));

                                phaseTime += 0.5;
                                cout << fixed << setprecision(2) << phaseTime << " GPIO " << gpioPin << " Off" << endl;
                                logActivity("STATUS-017", "GPIO " + to_string(gpioPin) + " OFF (PULSE)");
                                this_thread::sleep_for(chrono::milliseconds(500));
                                phaseTime += 0.5;
                            }
                        }

                        if (!force_exit) {
                            cout << "Rack Connector " << (rackNum + 1) << " Test Successfully Completed.\n" << endl;
                            logActivity("STATUS-015", "Rack Connector " + to_string(rackNum + 1) + " Test Successfully Completed");
                        }
                    }

                    if (force_exit) {
                        logActivity("ERROR-005", "Startup Sequence Aborted by User via ESC");
                    } else {
                        // Hardcoded 12/12 success — no real GPIO hardware to fail
                        cout << "All 12/12 GPIO Successfully Activated." << endl;
                        logActivity("STATUS-013", "Sequence Complete: All 12/12 GPIO Successfully Activated");
                    }

                    programMode = 0;
                    displayMenu();

                // Operational Mode: Transitions to Operational Mode --------------------------------------------------------------------------------
                } else if (charInput == '2') {
                    programMode = 2;
                    logActivity("STATUS-020", "Mode Changed: Operational Mode Active");
                    displayMenu();
                }
            // Operational Mode: Accepts movement/rotation inputs -----------------------------------------------------------------------------------
            } else if (programMode == 2) {
                if (charInput == '0') {
                    // Return to menu — deactivate all thrusters first
                    if (!currentKeyPressed.empty()) {
                        cout << "[SIM] All GPIO OFF" << endl;
                        logKeyData('N', "--", "-", 'R');
                        logActivity("STATUS-106", "All Thrusters Deactivated: Key Released (" + currentKeyPressed + ")");
                        currentKeyPressed = "";
                    }
                    programMode = 0;
                    displayMenu();
                } else {
                    string keyIdString = string(1, toupper(charInput));
                    // Only process if it's a different key than currently tracked
                    if (keyIdString != currentKeyPressed) {
                        processMovementAction(keyIdString);
                        currentKeyPressed = keyIdString;
                    }
                }
            }
        // Handle thruster deactivation when no keys are pressed in Operational Mode ----------------------------------------------------------------
        } else {
            if (programMode == 2 && !currentKeyPressed.empty()) {
                noInputCount++;
                if (noInputCount >= RELEASE_CYCLES) {
                    logKeyData('N', "--", "-", 'R');
                    cout << "[SIM] All GPIO OFF" << endl;
                    logActivity("STATUS-106", "All Thrusters Deactivated: Key Released (" + currentKeyPressed + ")");
                    currentKeyPressed = "";
                    noInputCount = 0;
                }
            }
        }

        // Small delay to prevent excessive CPU usage
        this_thread::sleep_for(chrono::milliseconds(50));
    }

    // Cleanup and Exit -----------------------------------------------------------------------------------------------------------------------------
    cout << "[SIM] All GPIO OFF" << endl;
    logActivity("STATUS-107", "All Thrusters Deactivated on Program Exit");
    logActivity("STATUS-002", "Session Ended");
    logActivity("STATUS-003", "Shutdown Successful");
    return 0;
}



// Compilation Instructions =========================================================================================================================
// cd Controller
// g++ DTOManualTesting.cpp -o DTOManualTesting -luser32
// DTOManualTesting.exe
