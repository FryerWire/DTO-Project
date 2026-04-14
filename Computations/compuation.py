/*
DTOManualTesting.cpp
DTO Manual Testing Program - Windows Simulation of GPIO Thruster Control

Features:
- Simulates GPIO thruster control on Windows using console output in place of physical libgpiod calls, allowing full software-level validation of the controller logic before Raspberry Pi 5 deployment.
- Logs all key events with elapsed timestamps, event type (Translation/Rotation/Fault), direction codes, key names, and active firing mode to Keybind_Log.csv on every detected key action.
- Logs all high-level application lifecycle and status events with elapsed timestamps and standardized STATUS/ERROR codes to Activity_Log.csv.
- Supports Continuous Mode (Caps Lock OFF), where actions fire repeatedly for each detected key press event, and Pulse Mode (Caps Lock ON), where each distinct key identifier fires only once per new press.
- Maps 12 keybinds (W, S, A, D, E, Q for translation; I, K, U, O, J, L for rotation) to their corresponding six-degree-of-freedom direction codes and logs the mappings identically to the flight controller format.
- Implements a three-mode structure: Menu Mode for mode selection, Startup Sequence Mode (1) for simulated per-rack GPIO validation across three rack connectors, and Operational Mode (2) for real-time key-to-thruster mapping.
- Startup Sequence Mode simulates Phase 1 (four individual 1-second ON/OFF pulses per rack) and Phase 2 (four double 0.5-second pulse pairs per rack) with ESC abort support at each sleep boundary.
- Uses Windows GetAsyncKeyState for non-blocking global key state polling in Operational Mode and _kbhit()/_getch() for menu-mode character input without blocking the main loop.
- Uses Windows GetKeyState(VK_CAPITAL) to detect Caps Lock state at the time of each key event to dynamically determine the active firing mode without requiring manual mode-switch inputs.
- Resets both CSV log files to empty with fresh headers at each program launch to ensure clean session data.
- Uses a software time_counter variable incremented by 0.10 per loop tick in Operational Mode as a simulated elapsed timestamp for log entries.

Functions:
- main(): Resets and initializes both log files with CSV headers; manages the main loop for menu navigation, Startup Sequence execution, and Operational Mode key polling; performs final activity log finalization on exit.
- logActivity(string code, string description): Appends a timestamped STATUS or ERROR code entry to Activity_Log.csv using the current time_counter value; silently skips the write if the file cannot be opened.
- logError(string errorCode, string title): Convenience wrapper around logActivity for ERROR-prefixed codes, providing consistent error logging with a single call site.
- getVirtualKeyName(int vkCode): Resolves a Windows virtual key code integer to a human-readable string for use in Keybind_Log entries; handles Space, Shift, arrow keys, modifier keys, Enter, and all alphanumeric keys; returns "Key_N" for unmapped codes.
- logData(char type, string direction, string keyname, char statusChar, char mode): Writes a timestamped key event row to both stdout and Keybind_Log.csv; triggers a STATUS-011 logActivity entry for successful new key registrations (statusChar == 'N' and keyname != "-"); logs ERROR-002 if the file cannot be opened.
- processAction(int vkCode, char mode): Maps a Windows virtual key code to its translation or rotation direction and calls logData for the matched case; calls logData with type 'F' and direction "--" plus logError for ERROR-004 on any unmapped key code.
- printMenu(): Prints the main mode-selection menu to stdout listing Menu Mode, Startup Sequence Mode, Operational Mode, and ESC quit instructions.
- printOperationalHeader(): Prints the Operational Mode header to stdout including Caps Lock firing mode instructions and the return-to-menu keybind.

Codes:
- STATUS-000: Program Initialization Started
- STATUS-001: Session Started
- STATUS-002: Session Ended
- STATUS-003: Shutdown Successful
- STATUS-004: Log Directory Verified or Already Exists
- STATUS-005: Log Directory Created Successfully
- STATUS-006: Activity Log Opened and Header Written
- STATUS-007: Keybind Log Opened and Header Written
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
- STATUS-018: Partial GPIO Activation: Connection Issues Detected
- STATUS-019: UI Menu Refreshed
- STATUS-020: Operational Mode Activated
- STATUS-200: Keybind Log File Reset and Header Written at Session Start
- STATUS-201: Activity Log File Reset and Header Written at Session Start
- STATUS-202: Startup Sequence Mode Entered from Menu
- STATUS-203: Operational Mode Entered from Menu
- STATUS-204: Returned to Menu Mode from Operational Mode via Input '0'
- STATUS-205: Caps Lock State Sampled: Continuous Mode Detected
- STATUS-206: Caps Lock State Sampled: Pulse Mode Detected
- STATUS-207: Active Virtual Key Code Detected in Operational Mode Polling Loop
- STATUS-208: Pulse Mode New Key Accepted: Action Dispatched
- STATUS-209: Pulse Mode Repeat Detected: Idle Entry Logged Instead
- STATUS-210: No Active Keys Detected: Pulse Tracking Reset and Idle Logged
- STATUS-211: Virtual Key Code Successfully Resolved to Human-Readable Name
- STATUS-212: Translation Action Dispatched: Direction Code Written to Log
- STATUS-213: Rotation Action Dispatched: Direction Code Written to Log
- STATUS-214: Startup Sequence Phase 1 Tick: GPIO Simulated ON (1-Second Pulse)
- STATUS-215: Startup Sequence Phase 1 Tick: GPIO Simulated OFF (1-Second Pulse)
- STATUS-216: Startup Sequence Phase 2 Tick: GPIO Simulated ON (0.5-Second Pulse)
- STATUS-217: Startup Sequence Phase 2 Tick: GPIO Simulated OFF (0.5-Second Pulse)
- STATUS-218: Rack Connector All Phases Completed: Test Pass Confirmed
- STATUS-219: time_counter Reset to 0.0 on Return to Menu Mode
- STATUS-220: last_key_fired Reset to Empty String on Return to Menu Mode
- STATUS-221: Input Character Re-Routed from _getch to processAction After '0' Check
- STATUS-222: Modifier Key Skipped During Virtual Key Scan Loop
- STATUS-223: Keyboard Input Buffer Flushed After Active Key Processed
- ERROR-000: Startup Failure: Log Path Inaccessible or Cannot Be Created
- ERROR-001: Activity Log Write Failure: File Inaccessible
- ERROR-002: Keybind Log Write Failure: File Inaccessible or Locked
- ERROR-003: Log Directory Creation Failed: Check Permissions
- ERROR-004: Incorrect Keybind: No Virtual Key Mapping Found
- ERROR-005: Startup Sequence Aborted by User via ESC
- ERROR-006: Rack Connector Test Failed: GPIO Connection Issue on Pin
- ERROR-007: GPIO Pin Activation Failed: setGPIOPin Returned False
- ERROR-200: Startup Failure: Keybind_Log.csv Could Not Be Opened for Reset
- ERROR-201: Startup Failure: Activity_Log.csv Could Not Be Opened for Reset
- ERROR-202: Shutdown Failure: Activity Log Could Not Be Finalized on Exit
- ERROR-203: System Key Ghosting Detected: Multiple Simultaneous Keys Masked
- ERROR-204: Mode Switch Denied: Must Exit Current Sub-Mode Before Switching
- ERROR-205: _getch Character Consumed Before processAction Could Use It
- ERROR-206: Virtual Key Code Out of Expected Scan Range (0x08 to 0xFE)
*/



#include <windows.h>  // Required for GetKeyState and GetAsyncKeyState
#include <iostream>   // For console output
#include <fstream>    // For file handling
#include <conio.h>    // For _kbhit() and _getch() to handle keyboard input without blocking
#include <chrono>     // For high-resolution timing
#include <thread>     // For sleep functionality to prevent high CPU usage
#include <string>     // For string handling
#include <iomanip>    // For output formatting (e.g., fixed and setprecision)



using namespace std;



// Global Variables ===============================================================================
double time_counter = 0.0;
string last_key_fired = "";      // Tracks the key name to prevent repeat firing in Pulse mode
bool isStartupMode = false;      // Tracks if the system is in Startup Sequence Mode (1)
bool isOperationalMode = false;  // Tracks if the system is in Operational Mode (2)
const string LOG_PATH = "C:\\Users\\maxwe\\OneDrive\\Desktop\\GitHub Repos\\DTO-Project\\Logs\\";



/*
    logActivity() - Logs STATUS and ERROR codes to Activity_Log.csv.

    Parameters:
    - code (string)        : A string representing the specific status or error code (e.g., "STATUS-03" or "ERROR-02").
    - description (string) : A brief description of the activity or error for context.
*/
void logActivity(string code, string description) {
    ofstream activityFile(LOG_PATH + "Activity_Log.csv", ios_base::app);
    // Log format: Time(s), Code, Description -----------------------------------------------------
    if (activityFile.is_open()) {
        activityFile << fixed << setprecision(2) << time_counter << "," << code << "," << description << endl;
        activityFile.close();
    }
}



/*
    logError() - Helper function to log errors with specific codes and descriptions.

    Parameters:
    - errorCode (string) : A string representing the specific error code (e.g., "ERROR-03").
    - title (string)     : A brief title or description of the error for context.
*/
void logError(string errorCode, string title) {
    logActivity(errorCode, title);
}



/*
    getVirtualKeyName() - Converts virtual key codes to human-readable names for logging.

    Parameters:
    - vkCode (int) : The virtual key code of the pressed key.

    Returns:
    - A string representing the human-readable name of the key (e.g., "Space", "Enter", "A", etc.). For unmapped keys, it returns "Key_" followed by the vkCode.
*/
string getVirtualKeyName(int vkCode) {
    // Common keys with special names -------------------------------------------------------------
    if (vkCode == VK_SPACE) return "Space";
    if (vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT) return "Shift";
    if (vkCode == VK_UP) return "UpArrow";
    if (vkCode == VK_DOWN) return "DownArrow";
    if (vkCode == VK_LEFT) return "LeftArrow";
    if (vkCode == VK_RIGHT) return "RightArrow";
    if (vkCode == VK_CONTROL) return "Control";
    if (vkCode == VK_MENU) return "Alt";
    if (vkCode == VK_RETURN) return "Enter";

    // Alphanumeric keys (A-Z, 0-9) ---------------------------------------------------------------
    if ((vkCode >= '0' && vkCode <= '9') || (vkCode >= 'A' && vkCode <= 'Z')) {
        return string(1, (char)vkCode);
    }
    return "Key_" + to_string(vkCode);
}



/*
    logData() - Logs key events to Keybind_Log.csv and the console, with status codes for successful registrations and errors.

    Parameters:
    - type (char)        : 'T' for Translation, 'R' for Rotation, 'F' for Fault/Error.
    - direction (string) : A string representing the direction of movement (e.g., "+X", "-Y", "+P", etc.) or "--" for faults.
    - keyname (string)   : The human-readable name of the key that triggered the event.
    - statusChar (char)  : 'N' for Normal registration, 'E' for Error, used to determine if an activity log entry should be made for successful key registrations.
    - mode (char)        : 'C' for Continuous mode, 'P' for Pulse mode. Defaults to 'C' if not specified.
*/
void logData(char type, string direction, string keyname, char statusChar, char mode = 'C') {
    // Log to console -----------------------------------------------------------------------------
    cout << fixed << setprecision(2)
         << time_counter << "," << mode << "," << type << "," << direction << "," << keyname << endl;

    // Log to Keybind_Log.csv ---------------------------------------------------------------------
    ofstream outFile(LOG_PATH + "Keybind_Log.csv", ios_base::app);
    if (outFile.is_open()) {
        outFile << fixed << setprecision(2)
                << time_counter << "," << mode << "," << type << "," << direction << "," << keyname << endl;
        outFile.close();

        // Log Status Code for successful key registration ----------------------------------------
        if (statusChar == 'N' && keyname != "-") {
            logActivity("STATUS-011", "Key Registered: " + keyname);
        }

    } else {
        logError("ERROR-002", "Write Failure: Keybind CSV file locked");
    }
}



/*
    processAction() - Map Virtual Keys/Combinations to Directions and Names

    Parameters:
    - vkCode (int) : The virtual key code of the pressed key.
    - mode (char)  : 'C' for Continuous mode, 'P' for Pulse mode
*/
void processAction(int vkCode, char mode) {
    switch (vkCode) {
        // Translation ----------------------------------------------------------------------------
        case 'W':      logData('T', "+X", "W", 'N', mode); break;  // Forward
        case 'S':      logData('T', "-X", "S", 'N', mode); break;  // Backward
        case 'A':      logData('T', "+Y", "A", 'N', mode); break;  // Left
        case 'D':      logData('T', "-Y", "D", 'N', mode); break;  // Right
        case 'E':      logData('T', "+Z", "E", 'N', mode); break;  // Up
        case 'Q':      logData('T', "-Z", "Q", 'N', mode); break;  // Down

        // Rotation -------------------------------------------------------------------------------
        case 'I':      logData('R', "+P", "I", 'N', mode); break;  // Pitch CCW
        case 'K':      logData('R', "-P", "K", 'N', mode); break;  // Pitch CW
        case 'O':      logData('R', "+R", "O", 'N', mode); break;  // Roll CCW
        case 'U':      logData('R', "-R", "U", 'N', mode); break;  // Roll CW
        case 'L':      logData('R', "+Y", "L", 'N', mode); break;  // Yaw CCW
        case 'J':      logData('R', "-Y", "J", 'N', mode); break;  // Yaw CW

        // General Error Handling -----------------------------------------------------------------
        default:       logData('F', "--", getVirtualKeyName(vkCode), 'E', mode); logError("ERROR-004", "Incorrect Keybind"); break;
    }
}



/*
    printMenu() - Prints the main menu to the console.
*/
void printMenu() {
    cout << "=================================================" << endl;
    cout << "DTO Program"                                        << endl;
    cout << "-------------------------------------------------"  << endl;
    cout << "Program Modes:"                                     << endl;
    cout << "- 0   : Menu Mode"                                  << endl;
    cout << "- 1   : Startup Sequence Mode"                      << endl;
    cout << "- 2   : Operational Mode"                           << endl;
    cout << "- Esc : Quit Mode"                                  << endl;
    cout << "================================================="  << endl;
    cout << ">> ";
}



/*
    printOperationalHeader() - Prints the operational mode header to the console.
*/
void printOperationalHeader() {
    cout << "================================================="  << endl;
    cout << "Operational Mode (2)"                               << endl;
    cout << "-------------------------------------------------"  << endl;
    cout << "Firing Mode: Caps Lock OFF = Continuous | Caps Lock ON = Pulse" << endl;
    cout << "Program Modes:"                                     << endl;
    cout << "- 0   : Return to Menu"                             << endl;
    cout << "- Esc : Quit Program"                               << endl;
    cout << "================================================="  << endl;
    cout << ">> "                                                 << endl;
}



/*
    main() - The entry point of the program.
*/
int main() {
    // Reset the log files and add CSV headers ----------------------------------------------------
    ofstream resetFile(LOG_PATH + "Keybind_Log.csv", ios::trunc);
    ofstream resetActivity(LOG_PATH + "Activity_Log.csv", ios::trunc);

    // Check if files opened successfully before writing headers ----------------------------------
    if (!resetFile.is_open() || !resetActivity.is_open()) {
        cerr << "ERROR-000: Startup Failure. Check file path: " << LOG_PATH << endl;
        return 1;
    }

    // Initialize CSV Headers ---------------------------------------------------------------------
    resetFile << "Time(s),Mode,Type,Direction,Key" << endl;
    resetActivity << "Time(s),Code,Description" << endl;

    resetFile.close();
    resetActivity.close();

    logActivity("STATUS-009", "Startup Successful: Files Ready");
    logActivity("STATUS-001", "Session Started");

    // Display Menu -------------------------------------------------------------------------------
    printMenu();

    // Main Loop ----------------------------------------------------------------------------------
    while (true) {
        // Exit check -----------------------------------------------------------------------------
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;

        // Menu Mode (not in any sub-mode) --------------------------------------------------------
        if (!isStartupMode && !isOperationalMode) {
            if (_kbhit()) {
                int input = _getch();

                if (input == '1') {
                    // Enter Startup Sequence Mode ------------------------------------------------
                    isStartupMode = true;
                    cout << "1" << endl;
                    logActivity("STATUS-010", "Mode Changed: Startup Sequence Mode");
                    logActivity("STATUS-012", "Sequence Initiated");

                    // Startup Sequence Mode Variables & Testing ----------------------------------
                    int rackConnector0[4] = {0, 1, 2, 3};
                    int rackConnector1[4] = {4, 5, 6, 7};
                    int rackConnector2[4] = {8, 9, 10, 11};

                    int* connectors[3] = {rackConnector0, rackConnector1, rackConnector2};

                    bool force_exit = false;

                    for (int r = 0; r < 3; r++) {
                        if (force_exit) break;
                        double sequence_time = 0.00;
                        cout << "Rack Connector " << r + 1 << " Test:" << endl;
                        logActivity("STATUS-014", "Testing Rack Connector " + to_string(r + 1));

                        // Phase 1: 1.0s Pulses ---------------------------------------------------
                        for (int g = 0; g < 4; g++) {
                            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { force_exit = true; break; }
                            cout << fixed << setprecision(2) << sequence_time << " GPIO " << connectors[r][g] << " On" << endl;
                            logActivity("STATUS-016", "GPIO " + to_string(connectors[r][g]) + " ON");
                            this_thread::sleep_for(chrono::milliseconds(1000));
                            sequence_time += 1.00;

                            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { force_exit = true; break; }
                            cout << fixed << setprecision(2) << sequence_time << " GPIO " << connectors[r][g] << " Off" << endl;
                            logActivity("STATUS-017", "GPIO " + to_string(connectors[r][g]) + " OFF");
                            this_thread::sleep_for(chrono::milliseconds(1000));
                            sequence_time += 1.00;
                        }

                        // Phase 2: Double 0.5s Pulses --------------------------------------------
                        for (int g = 0; g < 4; g++) {
                            if (force_exit) break;
                            for (int i = 0; i < 2; i++) {
                                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { force_exit = true; break; }
                                cout << fixed << setprecision(2) << sequence_time << " GPIO " << connectors[r][g] << " On" << endl;
                                logActivity("STATUS-016", "GPIO " + to_string(connectors[r][g]) + " ON (PULSE)");
                                this_thread::sleep_for(chrono::milliseconds(500));
                                sequence_time += 0.50;

                                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { force_exit = true; break; }
                                cout << fixed << setprecision(2) << sequence_time << " GPIO " << connectors[r][g] << " Off" << endl;
                                logActivity("STATUS-017", "GPIO " + to_string(connectors[r][g]) + " OFF (PULSE)");
                                this_thread::sleep_for(chrono::milliseconds(500));
                                sequence_time += 0.50;
                            }
                        }
                        if (!force_exit) {
                            cout << "Rack Connector " << r + 1 << " Test Successfully Completed." << endl << endl;
                            logActivity("STATUS-015", "Rack Connector " + to_string(r + 1) + " PASS");
                        }
                    }

                    if (force_exit) {
                        logError("ERROR-005", "Sequence Aborted by User");
                        break;
                    }

                    cout << "All GPIO Successfully Activated." << endl;
                    logActivity("STATUS-013", "All GPIO Successfully Activated");

                    isStartupMode = false;

                    // Return to menu -------------------------------------------------------------
                    printMenu();
                }

                else if (input == '2') {
                    // Enter Operational Mode -----------------------------------------------------
                    isOperationalMode = true;
                    cout << "2" << endl;
                    logActivity("STATUS-010", "Mode Changed: Operational Mode");
                    printOperationalHeader();
                }

                else if (input == 27) { // Esc via _getch
                    break;
                }
            }
        }

        // Operational Mode (2) Logic -------------------------------------------------------------
        else if (isOperationalMode) {
            // Check for '0' to return to menu ----------------------------------------------------
            if (_kbhit()) {
                int peeked = _getch();
                if (peeked == '0') {
                    isOperationalMode = false;
                    time_counter = 0.0;
                    last_key_fired = "";
                    logActivity("STATUS-010", "Mode Changed: Menu Mode");
                    printMenu();
                    continue;
                }
                // Put the character back via an internal flag ------------------------------------
                // Since _getch() consumes the char, re-process it as a VK if possible
                // For letter keys, push back to processAction directly
                if (peeked >= 32 && peeked <= 126) {
                    int vk = toupper(peeked);
                    bool isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
                    char currentMode = isCapsOn ? 'P' : 'C';

                    if (currentMode == 'P') {
                        string key_id = to_string(vk);
                        if (key_id != last_key_fired) {
                            processAction(vk, 'P');
                            last_key_fired = key_id;
                        } else {
                            logData('F', "--", "-", 'N', 'P');
                        }
                    } else {
                        processAction(vk, 'C');
                    }
                }
                continue;
            }

            bool isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
            char currentMode = isCapsOn ? 'P' : 'C';

            int active_vk = 0;
            string current_key_id = "";

            // Check all virtual keys for activity ------------------------------------------------
            for (int i = 0x08; i <= 0xFE; i++) {
                if (GetAsyncKeyState(i) & 0x8000) {
                    // Skip modifier keys ---------------------------------------------------------
                    if (i == VK_CONTROL || i == VK_MENU || i == VK_CAPITAL ||
                        i == VK_LCONTROL || i == VK_RCONTROL) continue;

                    active_vk = i;
                    current_key_id = to_string(i);
                    break;
                }
            }

            // Process the active key if detected -------------------------------------------------
            if (active_vk != 0) {
                if (currentMode == 'C') {
                    processAction(active_vk, 'C');
                }
                else if (currentMode == 'P') {
                    if (current_key_id != last_key_fired) {
                        processAction(active_vk, 'P');
                        last_key_fired = current_key_id;
                    } else {
                        logData('F', "--", "-", 'N', 'P');
                    }
                }
                while (_kbhit()) { _getch(); }
            }
            // No key activity, reset pulse tracking and log idle --------------------------------
            else {
                last_key_fired = "";
                logData('F', "--", "-", 'N', currentMode);
            }

            // Increment time only in Operational Mode --------------------------------------------
            time_counter += 0.10;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Final exit sequence ------------------------------------------------------------------------
    logActivity("STATUS-002", "Session Ended");

    ofstream finalize(LOG_PATH + "Activity_Log.csv", ios_base::app);
    if (finalize.is_open()) {
        finalize << fixed << setprecision(2) << time_counter << ",STATUS-003,Shutdown Successful" << endl;
        finalize.close();
        cout << "\nLogging complete. Files saved in: " << LOG_PATH << endl;
    } else {
        cerr << "ERROR-202: Shutdown Failed. Activity log could not be finalized." << endl;
    }

    return 0;
}



// Compilation Instructions =======================================================================
// cd Controller; g++ DTOManualTesting.cpp -o DTOManualTesting; ./DTOManualTesting