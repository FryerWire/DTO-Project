/*
    DTO Controller

    Features:
    - Logs key events with timestamps, types, directions, and modes to Keybind_Log.csv.
    - Supports Continuous (Caps Lock OFF) and Pulse (Caps Lock ON) modes.
    - Maps specific keys to translation and rotation movements.
    - Activity_Log.csv tracks high-level Application Codes (STATUS and FAULT).
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
    - STATUS-03: Key Registered          - A valid movement key was processed.
    - STATUS-04: Shutdown Successful    - Program exited and cleaned up without error.
    - STATUS-05: Mode Changed           - System switched between Startup and Operational modes.
*/



#include <windows.h> // Required for GetKeyState and GetAsyncKeyState
#include <iostream>  // For console output
#include <fstream>   // For file handling
#include <conio.h>   // For _kbhit() and _getch() to handle keyboard input without blocking
#include <chrono>    // For high-resolution timing
#include <thread>    // For sleep functionality to prevent high CPU usage
#include <string>    // For string handling
#include <iomanip>   // For output formatting (e.g., fixed and setprecision)



using namespace std;



// Global Variables ===============================================================================
double time_counter = 0.0;
string last_key_fired = ""; // Tracks the key name to prevent repeat firing in Pulse mode
const string LOG_PATH = "C:\\Users\\maxwe\\OneDrive\\Desktop\\GitHub Repos\\DTO-Project\\Logs\\";
bool isStartupMode = false;    // Tracks if the system is in Startup Sequence Mode (~S)
bool isOperationalMode = false; // Tracks if the system is in Operational Mode (~O)



/*
    logActivity() - Writes Application Codes (STATUS/FAULT) to the Activity Log CSV.
    Format: Timestamp, Code, Description
*/
void logActivity(string code, string description) {
    ofstream activityFile(LOG_PATH + "Activity_Log.csv", ios_base::app);
    if (activityFile.is_open()) {
        activityFile << fixed << setprecision(2) << time_counter << "," << code << "," << description << endl;
        activityFile.close();
    }
}



/*
    logFault() - Redirects Fault messages to the unified Activity Log CSV.

    Parameters:
    - faultCode (string) : A string representing the specific fault code (e.g., "FAULT-03").
    - title (string)     : A brief description of the fault for context.
*/
void logFault(string faultCode, string title) {
    logActivity(faultCode, title);
}



/*
    getVirtualKeyName() - Converts a virtual key code to a human-readable string for logging purposes.

    Parameters:
    - vkCode (int) : The virtual key code to be converted.
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
    logData() - Logs key event data to both the console and a CSV file.
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
        
        // Log Status Code for successful key registration
        if (statusChar == 'N' && keyname != "-") {
            logActivity("STATUS-03", "Key Registered: " + keyname);
        }
    } else {
        logFault("FAULT-02", "Write Failure: Keybind CSV file locked");
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
        case 'Q':            logData('F', "--", "Q", 'E', mode); logFault("FAULT-03", "Incorrect Keybind"); break;
        case 'E':            logData('F', "--", "E", 'E', mode); logFault("FAULT-03", "Incorrect Keybind"); break;
        case 'G':            logData('F', "--", "G", 'E', mode); logFault("FAULT-03", "Incorrect Keybind"); break; 
        case 'F':            logData('F', "--", "F", 'E', mode); logFault("FAULT-03", "Incorrect Keybind"); break;

        // General Fault Handling -----------------------------------------------------------------
        default:             logData('F', "--", getVirtualKeyName(vkCode), 'E', mode); logFault("FAULT-03", "Incorrect Keybind"); break;
    }
}



/*
    main() - The entry point of the program.
*/
int main() {
    // Reset the log files and add CSV headers ----------------------------------------------------
    ofstream resetFile(LOG_PATH + "Keybind_Log.csv", ios::trunc);
    ofstream resetActivity(LOG_PATH + "Activity_Log.csv", ios::trunc);

    if (!resetFile.is_open() || !resetActivity.is_open()) {
        cerr << "FAULT-00: Startup Failure. Check file path: " << LOG_PATH << endl;
        return 1;
    }

    // Initialize CSV Headers
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
        // Exit check
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break; 

        // Check for Mode Changes (~ + S or ~ + O) ------------------------------------------------
        bool tildePressed = (GetAsyncKeyState(VK_OEM_3) & 0x8000);
        if (tildePressed) {
            if (GetAsyncKeyState('S') & 0x8000) {
                if (!isStartupMode && !isOperationalMode) {
                    isStartupMode = true;
                    isOperationalMode = false;
                    cout << ">> MODE CHANGE: STARTUP SEQUENCE MODE ACTIVATED" << endl;
                    logActivity("STATUS-05", "Mode Changed: Startup Sequence Mode");

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

                        // Phase 1: 1.0s Pulses ---------------------------------------------------
                        for (int g = 0; g < 4; g++) {
                            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { force_exit = true; break; }
                            cout << fixed << setprecision(2) << sequence_time << " GPIO " << connectors[r][g] << " On" << endl;
                            this_thread::sleep_for(chrono::milliseconds(1000));
                            sequence_time += 1.00;

                            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { force_exit = true; break; }
                            cout << fixed << setprecision(2) << sequence_time << " GPIO " << connectors[r][g] << " Off" << endl;
                            this_thread::sleep_for(chrono::milliseconds(1000));
                            sequence_time += 1.00;
                        }

                        // Phase 2: Double 0.5s Pulses --------------------------------------------
                        for (int g = 0; g < 4; g++) {
                            if (force_exit) break;
                            for (int i = 0; i < 2; i++) {
                                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { force_exit = true; break; }
                                cout << fixed << setprecision(2) << sequence_time << " GPIO " << connectors[r][g] << " On" << endl;
                                this_thread::sleep_for(chrono::milliseconds(500));
                                sequence_time += 0.50;

                                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { force_exit = true; break; }
                                cout << fixed << setprecision(2) << sequence_time << " GPIO " << connectors[r][g] << " Off" << endl;
                                this_thread::sleep_for(chrono::milliseconds(500));
                                sequence_time += 0.50;
                            }
                        }
                        if (!force_exit) cout << "Rack Connector " << r + 1 << " Test Successfully Completed." << endl << endl;
                    }

                    if (force_exit) break; // Break main while loop if Esc was hit

                    cout << "All GPIO Successfully Activated." << endl;
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

        if (isOperationalMode) {
            // Operational Mode (~O) Logic 
            bool isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
            char currentMode = isCapsOn ? 'P' : 'C';

            int active_vk = 0;
            string current_key_id = "";

            for (int i = 0x08; i <= 0xFE; i++) {
                if (GetAsyncKeyState(i) & 0x8000) {
                    if (i == VK_CONTROL || i == VK_MENU || i == VK_CAPITAL || 
                        i == VK_LCONTROL || i == VK_RCONTROL || i == VK_OEM_3) continue;

                    active_vk = i;
                    bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000);
                    if (i == VK_RIGHT && ctrl) current_key_id = "Ctrl+Right";
                    else if (i == VK_LEFT && ctrl) current_key_id = "Ctrl+Left";
                    else current_key_id = to_string(i);
                    break; 
                }
            }

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
            else if (_kbhit()) {
                int key_raw = _getch();
                char error_char = (char)toupper(key_raw);
                string mapped = "WASDQEGFT R L C";
                if (mapped.find(error_char) != string::npos || key_raw == 'H' || key_raw == 'P' || key_raw == 'K' || key_raw == 'M') {
                    logData('F', "--", "-", 'N', currentMode);
                } 
                else if (key_raw >= 32 && key_raw <= 126) {
                    logData('F', "--", string(1, (char)key_raw), 'E', currentMode);
                    logFault("FAULT-03", "Incorrect Keybind");
                } 
                else {
                    logData('F', "--", "-", 'N', currentMode);
                    logFault("FAULT-04", "System Ghosting");
                }
                last_key_fired = "";
            }
            else {
                last_key_fired = "";
                logData('F', "--", "-", 'N', currentMode);
            }

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
        cerr << "FAULT-05: Shutdown Failed. Activity log could not be finalized." << endl;
    }

    return 0;
}


// Compilation Instructions =======================================================================
// cd Controller; g++ controller_in_prog.cpp -o controller_in_prog; ./controller_in_prog