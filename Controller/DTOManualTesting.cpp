
/*
    DTO Manual Testing Program

    Features:
    - Logs key events with timestamps, types, directions, and modes to Keybind_Log.csv.
    - Supports Continuous (Caps Lock OFF) and Pulse (Caps Lock ON) modes.
    - Maps specific keys to translation and rotation movements.
    - Activity_Log.csv tracks high-level Application Codes (STATUS and ERROR).
    - Toggle Modes: ~ + S (Startup Sequence) | ~ + O (Operational Mode).
      Note: GPIO control is simulated with console output for testing purposes. Actual GPIO implementation will be done in the RPi 5 environment using libgpiod as per architecture requirements[cite: 758, 918].

    Functions:
    - logActivity(string code, string description): Logs STATUS and ERROR codes to Activity_Log.csv.
    - logError(string errorCode, string title): Helper function to log errors with specific codes and descriptions.
    - getVirtualKeyName(int vkCode): Converts virtual key codes to human-readable names for logging.
    - logData(char type, string direction, string keyname, char statusChar, char mode): Logs key events to Keybind_Log.csv and the console, with status codes for successful registrations and errors.
    - processAction(int vkCode, char mode): Maps virtual key codes to specific translation and rotation actions based on the current mode (Continuous or Pulse).
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
bool isStartupMode = false;      // Tracks if the system is in Startup Sequence Mode (~S)
bool isOperationalMode = false;  // Tracks if the system is in Operational Mode (~O)
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
    if (vkCode == VK_OEM_3) return "~";

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
         << time_counter << ", " << mode << ", " << type << ", " << direction << ", " << keyname << ", " << statusChar << endl;

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
    bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000);

    switch (vkCode) {  
        // Translation ----------------------------------------------------------------------------
        case 'W':            logData('T', "+X", "W", 'N', mode); break;
        case 'S':            logData('T', "-X", "S", 'N', mode); break;
        case 'D':            logData('T', "+Y", "D", 'N', mode); break;
        case 'A':            logData('T', "-Y", "A", 'N', mode); break;
        case VK_SPACE:       logData('T', "+Z", "Space", 'N', mode); break;
        case VK_SHIFT:       logData('T', "-Z", "Shift", 'N', mode); break;

        // Rotation -------------------------------------------------------------------------------
        case VK_RIGHT: 
            if (ctrl)        logData('R', "+R", "Ctrl+RightArrow", 'N', mode); 
            else             logData('R', "+Y", "RightArrow", 'N', mode); 
            break;
        case VK_LEFT:  
            if (ctrl)        logData('R', "-R", "Ctrl+LeftArrow", 'N', mode); 
            else             logData('R', "-Y", "LeftArrow", 'N', mode); 
            break;
        case VK_UP:          logData('R', "+P", "UpArrow", 'N', mode); break;
        case VK_DOWN:        logData('R', "-P", "DownArrow", 'N', mode); break;

        // Intentional Fault Trigger Keys ---------------------------------------------------------
        case 'Q':            logData('F', "--", "Q", 'E', mode); logError("ERROR-03", "Incorrect Keybind"); break;
        case 'E':            logData('F', "--", "E", 'E', mode); logError("ERROR-03", "Incorrect Keybind"); break;
        case 'G':            logData('F', "--", "G", 'E', mode); logError("ERROR-03", "Incorrect Keybind"); break; 
        case 'F':            logData('F', "--", "F", 'E', mode); logError("ERROR-03", "Incorrect Keybind"); break;

        // General Error Handling -----------------------------------------------------------------
        default:             logData('F', "--", getVirtualKeyName(vkCode), 'E', mode); logError("ERROR-03", "Incorrect Keybind"); break;
    }
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

    // Welcome Message ----------------------------------------------------------------------------
    cout << "DTO Program:" << endl;
    cout << "- Enter '~S' Startup Sequence Mode" << endl;
    cout << "- Enter '~O' Operational Mode" << endl;
    cout << "- Enter 'Esc' Exit Mode/Program." << endl;
    cout << "------------------------------------------------------------" << endl;

    logActivity("STATUS-02", "Session Started");

    // Main Loop ----------------------------------------------------------------------------------
    while (true) {
        // Exit check -----------------------------------------------------------------------------
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break; 

        // Check for Mode Changes (~ + S or ~ + O) ------------------------------------------------
        bool tildePressed = (GetAsyncKeyState(VK_OEM_3) & 0x8000);
        if (tildePressed) {
            if (GetAsyncKeyState('S') & 0x8000) {
                // Constraint: Cannot enter ~S if already in ~O. Must Esc and restart.
                if (isOperationalMode) {
                    logError("ERROR-06", "Mode Switch Denied: Exit ~O first");
                    cout << ">> ERROR: Cannot enter Startup Mode while Operational Mode is active." << endl;
                }
                else if (!isStartupMode) {
                    isStartupMode = true;
                    isOperationalMode = false;
                    cout << ">> MODE CHANGE: STARTUP SEQUENCE MODE ACTIVATED" << endl;
                    logActivity("STATUS-05", "Mode Changed: Startup Sequence Mode");
                    logActivity("STATUS-10", "Sequence Initiated");

                    // Startup Sequence Mode (~S) Variables & Testing -----------------------------
                    int rackConnector0[4] = {0, 1, 2, 3};
                    int rackConnector1[4] = {4, 5, 6, 7};
                    int rackConnector2[4] = {8, 11, 12, 13}; 
                    
                    int* connectors[3] = {rackConnector0, rackConnector1, rackConnector2};

                    // Label for jumping out of nested loops on Esc -------------------------------
                    bool force_exit = false;

                    for (int r = 0; r < 3; r++) {
                        if (force_exit) break;
                        double sequence_time = 0.00;
                        cout << "Rack Connector " << r + 1 << " Test:" << endl;
                        logActivity("STATUS-11", "Testing Rack Connector " + to_string(r + 1));

                        // Phase 1: 1.0s Pulses with Real-Time Delay ------------------------------
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

                        // Phase 2: Double 0.5s Pulses with Real-Time Delay -----------------------
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
                    cout << "------------------------------------------------------------" << endl;
                    isStartupMode = false; // Reset so ~S can be ran again
                }
            }

            else if (GetAsyncKeyState('O') & 0x8000) {
                if (!isOperationalMode) {
                    isOperationalMode = true;
                    isStartupMode = false;
                    cout << ">> MODE CHANGE: OPERATIONAL MODE ACTIVATED" << endl;
                    logActivity("STATUS-05", "Mode Changed: Operational Mode");
                }
            }
        }

        // Logic branching based on Mode ----------------------------------------------------------
        if (isStartupMode) {
            // Sequence completed above; waiting for mode toggle or exit.
        } 
        else if (isOperationalMode) {
            // Operational Mode (~O) Logic 
            bool isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
            char currentMode = isCapsOn ? 'P' : 'C';

            int active_vk = 0;
            string current_key_id = "";

            // Check all virtual keys for activity ------------------------------------------------
            for (int i = 0x08; i <= 0xFE; i++) {
                if (GetAsyncKeyState(i) & 0x8000) {
                    // Skip modifier keys to prevent them from being logged as primary keys -----------
                    if (i == VK_CONTROL || i == VK_MENU || i == VK_CAPITAL || 
                        i == VK_LCONTROL || i == VK_RCONTROL || i == VK_OEM_3) continue;

                    active_vk = i;
                    
                    // Handle Ctrl + Arrow combinations for rotation ------------------------------
                    bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000);
                    if (i == VK_RIGHT && ctrl) current_key_id = "Ctrl+Right";
                    else if (i == VK_LEFT && ctrl) current_key_id = "Ctrl+Left";
                    else current_key_id = to_string(i);
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
            // Handle unmapped keys and system ghosting -------------------------------------------
            else if (_kbhit()) {
                int key_raw = _getch();
                char error_char = (char)toupper(key_raw);
                
                string mapped = "WASDQEGFT R L C";
                if (mapped.find(error_char) != string::npos || key_raw == 'H' || key_raw == 'P' || key_raw == 'K' || key_raw == 'M') {
                    logData('F', "--", "-", 'N', currentMode);
                } 
                else if (key_raw >= 32 && key_raw <= 126) {
                    logData('F', "--", string(1, (char)key_raw), 'E', currentMode);
                    logError("ERROR-03", "Incorrect Keybind");
                } 
                else {
                    logData('F', "--", "-", 'N', currentMode);
                    logError("ERROR-04", "System Ghosting");
                }
                last_key_fired = "";
            }
            // No key activity detected, reset last_key_fired for Pulse mode ----------------------
            else {
                last_key_fired = "";
                logData('F', "--", "-", 'N', currentMode);
            }

            // Only increment time in Operational Mode --------------------------------------------
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
