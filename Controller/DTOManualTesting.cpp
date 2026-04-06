/*
    DTO Manual Testing Program

    Features:
    - Logs key events with timestamps, types, directions, and modes to Keybind_Log.csv.
    - Supports Continuous (Caps Lock OFF) and Pulse (Caps Lock ON) modes.
    - Maps specific keys to translation and rotation movements.
    - Activity_Log.csv tracks high-level Application Codes (STATUS and ERROR).
    - Toggle Modes: Input '1' (Startup Sequence) | Input '2' (Operational Mode) | Input '0' (Return to Menu).
      Note: GPIO control is simulated with console output for testing purposes. Actual GPIO implementation will be done in the RPi 5 environment using libgpiod as per architecture requirements[cite: 758, 918].

    Functions:
    - logActivity(string code, string description): Logs STATUS and ERROR codes to Activity_Log.csv.
    - logError(string errorCode, string title): Helper function to log errors with specific codes and descriptions.
    - getVirtualKeyName(int vkCode): Converts virtual key codes to human-readable names for logging.
    - logData(char type, string direction, string keyname, char statusChar, char mode): Logs key events to Keybind_Log.csv and the console, with status codes for successful registrations and errors.
    - processAction(int vkCode, char mode): Maps virtual key codes to specific translation and rotation actions based on the current mode (Continuous or Pulse).
    - printMenu(): Prints the main menu to the console.
    - printOperationalHeader(): Prints the operational mode header to the console.
    - main(): Initializes log files, handles mode toggling, processes key events, and manages the overall program flow.

    Codes:
    - STATUS-00: Startup Successful: Files Ready
    - STATUS-01: Session Ended
    - STATUS-02: Session Started
    - STATUS-03: Key Registered: [KeyName]
    - STATUS-04: Shutdown Successful
    - STATUS-05: Mode Changed: [ModeName]
    - STATUS-10: Sequence Initiated
    - STATUS-11: Testing Rack Connector [Number]
    - STATUS-12: Rack Connector [Number] PASS
    - STATUS-13: GPIO [Number] ON/OFF (PULSE)
    - STATUS-14: All GPIO Successfully Activated
    - ERROR-00: Startup Failure. Check file path: [Path]
    - ERROR-02: Write Failure: Keybind CSV file locked
    - ERROR-03: Incorrect Keybind
    - ERROR-04: System Ghosting
    - ERROR-06: Mode Switch Denied: Exit [CurrentMode] first
    - ERROR-10: Sequence Aborted by User
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
            logActivity("STATUS-03", "Key Registered: " + keyname);
        }

    } else {
        logError("ERROR-02", "Write Failure: Keybind CSV file locked");
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
        default:       logData('F', "--", getVirtualKeyName(vkCode), 'E', mode); logError("ERROR-03", "Incorrect Keybind"); break;
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
        cerr << "ERROR-00: Startup Failure. Check file path: " << LOG_PATH << endl;
        return 1;
    }

    // Initialize CSV Headers ---------------------------------------------------------------------
    resetFile << "Time(s),Mode,Type,Direction,Key" << endl;
    resetActivity << "Time(s),Code,Description" << endl;

    resetFile.close();
    resetActivity.close();

    logActivity("STATUS-00", "Startup Successful: Files Ready");
    logActivity("STATUS-02", "Session Started");

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
                    logActivity("STATUS-05", "Mode Changed: Startup Sequence Mode");
                    logActivity("STATUS-10", "Sequence Initiated");

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
                        logActivity("STATUS-11", "Testing Rack Connector " + to_string(r + 1));

                        // Phase 1: 1.0s Pulses ---------------------------------------------------
                        for (int g = 0; g < 4; g++) {
                            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { force_exit = true; break; }
                            cout << fixed << setprecision(2) << sequence_time << " GPIO " << connectors[r][g] << " On" << endl;
                            logActivity("STATUS-13", "GPIO " + to_string(connectors[r][g]) + " ON");
                            this_thread::sleep_for(chrono::milliseconds(1000));
                            sequence_time += 1.00;

                            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { force_exit = true; break; }
                            cout << fixed << setprecision(2) << sequence_time << " GPIO " << connectors[r][g] << " Off" << endl;
                            logActivity("STATUS-13", "GPIO " + to_string(connectors[r][g]) + " OFF");
                            this_thread::sleep_for(chrono::milliseconds(1000));
                            sequence_time += 1.00;
                        }

                        // Phase 2: Double 0.5s Pulses --------------------------------------------
                        for (int g = 0; g < 4; g++) {
                            if (force_exit) break;
                            for (int i = 0; i < 2; i++) {
                                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { force_exit = true; break; }
                                cout << fixed << setprecision(2) << sequence_time << " GPIO " << connectors[r][g] << " On" << endl;
                                logActivity("STATUS-13", "GPIO " + to_string(connectors[r][g]) + " ON (PULSE)");
                                this_thread::sleep_for(chrono::milliseconds(500));
                                sequence_time += 0.50;

                                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { force_exit = true; break; }
                                cout << fixed << setprecision(2) << sequence_time << " GPIO " << connectors[r][g] << " Off" << endl;
                                logActivity("STATUS-13", "GPIO " + to_string(connectors[r][g]) + " OFF (PULSE)");
                                this_thread::sleep_for(chrono::milliseconds(500));
                                sequence_time += 0.50;
                            }
                        }
                        if (!force_exit) {
                            cout << "Rack Connector " << r + 1 << " Test Successfully Completed." << endl << endl;
                            logActivity("STATUS-12", "Rack Connector " + to_string(r + 1) + " PASS");
                        }
                    }

                    if (force_exit) {
                        logError("ERROR-10", "Sequence Aborted by User");
                        break;
                    }

                    cout << "All GPIO Successfully Activated." << endl;
                    logActivity("STATUS-14", "All GPIO Successfully Activated");

                    isStartupMode = false;

                    // Return to menu -------------------------------------------------------------
                    printMenu();
                }

                else if (input == '2') {
                    // Enter Operational Mode -----------------------------------------------------
                    isOperationalMode = true;
                    cout << "2" << endl;
                    logActivity("STATUS-05", "Mode Changed: Operational Mode");
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
                    logActivity("STATUS-05", "Mode Changed: Menu Mode");
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
    logActivity("STATUS-01", "Session Ended");

    ofstream finalize(LOG_PATH + "Activity_Log.csv", ios_base::app);
    if (finalize.is_open()) {
        finalize << fixed << setprecision(2) << time_counter << ",STATUS-04,Shutdown Successful" << endl;
        finalize.close();
        cout << "\nLogging complete. Files saved in: " << LOG_PATH << endl;
    } else {
        cerr << "ERROR-05: Shutdown Failed. Activity log could not be finalized." << endl;
    }

    return 0;
}



// Compilation Instructions =======================================================================
// cd Controller; g++ DTOManualTesting.cpp -o DTOManualTesting; ./DTOManualTesting