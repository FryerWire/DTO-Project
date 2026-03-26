
/*
    DTO Controller

    Features:
    - Logs key events with timestamps, types, directions, and statuses to both console and CSV.
    - Supports Continuous (Caps Lock OFF) and Pulse (Caps Lock ON) modes.
    - Maps specific keys to translation and rotation movements, with error handling for unmapped keys.

    Key Mappings:
    Translation:
    - Forward  (+X) : W
    - Backward (-X) : S
    - Left     (+Y) : D
    - Right    (-Y) : A
    - Up       (+Z) : Space
    - Down     (-Z) : Shift

    Rotation:
    - CW Pitch  : Ctrl + Right
    - CCW Pitch : Ctrl + Left
    - CC Roll   : Up Arrow
    - CCW Roll  : Down Arrow
    - CW Yaw    : Right Arrow
    - CCW Yaw   : Left Arrow

    Error Codes:
    - Error-01: Failed Startup - Occurs if the program cannot initialize the log files.
    - Error-02: Write Failure - Occurs if the log file is locked or unavailable during a log attempt.
    - Error-03: Incorrect Keybind - Occurs when a key is pressed that has no mapped movement function.
    - Error-04: System Ghosting - Occurs when non-printable or system scan codes leak into the buffer.
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



/*
    logError() - Logs error messages with timestamps to a separate error log file.

    Parameters:
    - errorCode (string) : A string representing the specific error code (e.g., "Error-01").
    - title (string)     : A brief description of the error for context.
*/
void logError(string errorCode, string title) {
    ofstream errFile("Error_Log_Test.txt", ios_base::app);
    if (errFile.is_open()) {
        errFile << fixed << setprecision(2) << time_counter << " " << errorCode << ": " << title << endl;
        errFile.close();
    }
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

    // Alphanumeric keys (A-Z, 0-9) ---------------------------------------------------------------
    if ((vkCode >= '0' && vkCode <= '9') || (vkCode >= 'A' && vkCode <= 'Z')) {
        return string(1, (char)vkCode);
    }
    return "Key_" + to_string(vkCode);
}



/*
    logData() - Logs key event data to both the console and a CSV file with a consistent format.

    Parameters:
    - type (char)        : 'T' for Translation, 'R' for Rotation, 'F' for Failed/Other
    - direction (string) : A string representing the direction or action (e.g., "+X", "-Y", "CW Pitch")
    - keyname (string)   : The name of the key that triggered the event (e.g., "W", "Ctrl+RightArrow")
    - status (char)      : 'N' for Normal, 'E' for Error
    - mode (char)        : 'C' for Continuous, 'P' for Pulse

*/
void logData(char type, string direction, string keyname, char status, char mode = 'C') {
    // Log to console -----------------------------------------------------------------------------
    cout << fixed << setprecision(2) 
         << time_counter << ", " << mode << ", " << type << ", " << direction << ", " << keyname << ", " << status << endl;

    // Log to CSV file ----------------------------------------------------------------------------
    ofstream outFile("Keybind-Log-Test.csv", ios_base::app);
    if (outFile.is_open()) {  
        outFile << fixed << setprecision(2)
                << time_counter << "," << mode << "," << type << "," << direction << "," << keyname << "," << status << endl;
        outFile.close();
    } else {
        logError("Error-02", "Failed to log to document");
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

        // Intentional Error Keys -----------------------------------------------------------------
        case 'Q':            logData('F', "--", "Q", 'E', mode); logError("Error-03", "Incorrect Keybind"); break;
        case 'E':            logData('F', "--", "E", 'E', mode); logError("Error-03", "Incorrect Keybind"); break;
        case 'G':            logData('F', "--", "G", 'E', mode); logError("Error-03", "Incorrect Keybind"); break; 
        case 'F':            logData('F', "--", "F", 'E', mode); logError("Error-03", "Incorrect Keybind"); break;

        // Error Handling -------------------------------------------------------------------------
        default:             logData('F', "--", getVirtualKeyName(vkCode), 'E', mode); logError("Error-03", "Incorrect Keybind"); break;
    }
}



/*
    main() - The entry point of the program.
*/
int main() {
    // Reset the log files and add CSV header -----------------------------------------------------
    ofstream resetFile("Keybind-Log-Test.csv", ios::trunc);
    ofstream resetErr("Error_Log_Test.txt", ios::trunc);

    if (!resetFile.is_open() || !resetErr.is_open()) {
        cerr << "Error-01: Failed Startup. Check file permissions." << endl;
        return 1;
    }

    resetFile << "Time(s),Constant,Type,Direction,Key,Status" << endl;
    resetFile.close();
    resetErr.close();

    // User Instructions --------------------------------------------------------------------------
    cout << "Logging Active (0.10s intervals). Saving to Keybind-Log-Test.csv" << endl;
    cout << "CapsLK OFF: Continuous ('C') | CapsLK ON: Pulse ('P')" << endl;
    cout << "Press Keys (ESC to exit)..." << endl;
    cout << "------------------------------------------------------------" << endl;

    // Main Loop ----------------------------------------------------------------------------------
    while (true) {
        bool isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
        char currentMode = isCapsOn ? 'P' : 'C';

        if (GetAsyncKeyState(VK_ESCAPE)) break; // Exit on ESC

        int active_vk = 0;
        string current_key_id = "";

        // Check all virtual keys for activity ----------------------------------------------------
        for (int i = 0x08; i <= 0xFE; i++) {
            if (GetAsyncKeyState(i) & 0x8000) {
                // Skip modifier keys to prevent them from being logged as primary keys -----------
                if (i == VK_CONTROL || i == VK_MENU || i == VK_CAPITAL || 
                    i == VK_LCONTROL || i == VK_RCONTROL) continue;

                active_vk = i;
                
                // Handle Ctrl + Arrow combinations for rotation ----------------------------------
                bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000);
                if (i == VK_RIGHT && ctrl) current_key_id = "Ctrl+Right";
                else if (i == VK_LEFT && ctrl) current_key_id = "Ctrl+Left";
                else current_key_id = to_string(i);
                break; 
            }
        }

        // Process the active key if detected -----------------------------------------------------
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
        // Handle unmapped keys and system ghosting -----------------------------------------------
        else if (_kbhit()) {
            int key_raw = _getch();
            char error_char = (char)toupper(key_raw);
            
            string mapped = "WASDQEGFT R L C";
            if (mapped.find(error_char) != string::npos || key_raw == 'H' || key_raw == 'P' || key_raw == 'K' || key_raw == 'M') {
                logData('F', "--", "-", 'N', currentMode);
            } 
            else if (key_raw >= 32 && key_raw <= 126) {
                logData('F', "--", string(1, (char)key_raw), 'E', currentMode);
                logError("Error-03", "Incorrect Keybind");
            } 
            else {
                logData('F', "--", "-", 'N', currentMode);
                logError("Error-04", "System Ghosting");
            }
            last_key_fired = "";
        }
        // No key activity detected, reset last_key_fired for Pulse mode --------------------------
        else {
            last_key_fired = "";
            logData('F', "--", "-", 'N', currentMode);
        }

        time_counter += 0.10;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    cout << "\nLogging complete. File saved as Keybind-Log-Test.csv" << endl;

    return 0;
}


// Compilation Instructions =======================================================================
// cd Controller; g++ controller_in_prog.cpp -o controller_in_prog; ./controller_in_prog
