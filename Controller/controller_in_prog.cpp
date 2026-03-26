/*
    DTO Controller
    Features:
    - Logs keybinds for translation and rotation in a CSV file with timestamps.
    - Uses a simple console interface for real-time feedback.
*/

#include <windows.h> // Required for GetKeyState and GetAsyncKeyState


// Pins ===========================================================================================
// Rack Connector 1 -------------------------------------------------------------------------------
// GPIO-00 | U1 | A1
// GPIO-01 | U2 | S1
// GPIO-02 | U3 | P1
// GPIO-03 | U4 | T1

// Rack Connector 2 -------------------------------------------------------------------------------
// GPIO-04 | U5 | B1
// GPIO-05 | U6 | F1
// GPIO-06 | U7 | P2
// GPIO-07 | S2 | S2

// Rack Connector 3 -------------------------------------------------------------------------------
// GPIO-08 | U9 | A1
// GPIO-09 | U10| T2
// GPIO-10 | U11| B2
// GPIO-11 | U12| F2

// Translation and Rotation Mapping ===============================================================
// Translation ------------------------------------------------------------------------------------
// Forward  (+X) : W
// Backward (-X) : S
// Left     (+Y) : D
// Right    (-Y) : A
// Up       (+Z) : Space
// Down     (-Z) : Shift

// Rotation ---------------------------------------------------------------------------------------
// CW Pitch  : Ctrl + Right
// CCW Pitch : Ctrl + Left
// CC Roll   : Up Arrow
// CCW Roll  : Down Arrow
// CW Yaw    : Right Arrow
// CCW Yaw   : Left Arrow



#include <iostream>  // For console output
#include <fstream>   // For file handling
#include <conio.h>   // For _kbhit() and _getch() to handle keyboard input without blocking
#include <chrono>    // For high-resolution timing
#include <thread>    // For sleep functionality to prevent high CPU usage
#include <string>    // For string handling
#include <iomanip>   // For output formatting (e.g., fixed and setprecision)



using namespace std;



double time_counter = 0.0;
string last_key_fired = ""; // Tracks the key name to prevent repeat firing in Pulse mode



/*
    getVirtualKeyName() - Returns a string name for a given Virtual-Key code.
*/
string getVirtualKeyName(int vkCode) {
    if (vkCode == VK_SPACE) return "Space";
    if (vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT) return "Shift";
    if (vkCode == VK_UP) return "UpArrow";
    if (vkCode == VK_DOWN) return "DownArrow";
    if (vkCode == VK_LEFT) return "LeftArrow";
    if (vkCode == VK_RIGHT) return "RightArrow";
    if (vkCode == VK_CONTROL) return "Control";
    if (vkCode == VK_MENU) return "Alt";
    if (vkCode == VK_RETURN) return "Enter";

    if ((vkCode >= '0' && vkCode <= '9') || (vkCode >= 'A' && vkCode <= 'Z')) {
        return string(1, (char)vkCode);
    }
    return "Key_" + to_string(vkCode);
}



/*
    logData() - Logs the provided key event data to both the console and a CSV file.

    Parameters:
    - type: 'T' for translation, 'R' for rotation, 'F' for error/invalid input
    - direction: A string indicating the direction of movement or rotation (e.g., "+X", "-Y", etc.)
    - keyname: The string representing the key (e.g., "Space", "W", "Ctrl+RightArrow")
    - status: 'N' for normal event, 'E' for error event
    - mode: 'C' for continuous, 'P' for pulse
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
    }
}



/*
    processAction() - Map Virtual Keys/Combinations to Directions and Names
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

        // Intentional Error Keys (Set to E status) -----------------------------------------------
        case 'Q':            logData('F', "--", "Q", 'E', mode); break;
        case 'E':            logData('F', "--", "E", 'E', mode); break;
        case 'G':            logData('F', "--", "G", 'E', mode); break; 
        case 'F':            logData('F', "--", "F", 'E', mode); break;

        // Error Handling -------------------------------------------------------------------------
        default:             logData('F', "--", getVirtualKeyName(vkCode), 'E', mode); break;
    }
}



/*
    main() - The entry point of the program.
*/
int main() {
    // Reset the log file and add CSV header ------------------------------------------------------
    ofstream resetFile("Keybind-Log-Test.csv", ios::trunc);
    resetFile << "Time(s),Constant,Type,Direction,Key,Status" << endl;
    resetFile.close();

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

        // Check for ANY key down to ensure instant response across the whole keyboard ------------
        for (int i = 0x08; i <= 0xFE; i++) {
            if (GetAsyncKeyState(i) & 0x8000) {
                // Ignore stand-alone Control/Alt to allow combo detection (Shift is allowed here)
                if (i == VK_CONTROL || i == VK_MENU || i == VK_LCONTROL || i == VK_RCONTROL) continue;

                active_vk = i;
                
                // Generate unique ID for Pulse tracking
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
            // Aggressively flush buffer while key is down
            while (_kbhit()) { _getch(); }
        } 
        else if (_kbhit()) {
            int key_raw = _getch();
            char error_char = (char)toupper(key_raw);
            
            // Ignore ghosting/residue characters (WASDQEGFT R L C H P K M)
            string mapped = "WASDQEGFT R L C";
            if (mapped.find(error_char) != string::npos || key_raw == 'H' || key_raw == 'P' || key_raw == 'K' || key_raw == 'M') {
                logData('F', "--", "-", 'N', currentMode);
            } 
            else if (key_raw >= 32 && key_raw <= 126) {
                logData('F', "--", string(1, (char)key_raw), 'E', currentMode);
            } 
            else {
                logData('F', "--", "-", 'N', currentMode);
            }
            last_key_fired = "";
        }
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


// Compilation Instructions:
// cd Controller; g++ controller_in_prog.cpp -o controller_in_prog; ./controller_in_prog