
/*
    File Name
*/



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
// GPIO-07 | U8 | S2

// Rack Connector 3 -------------------------------------------------------------------------------
// GPIO-08 | U9 | A1
// GPIO-09 | U10| T2
// GPIO-10 | U11| B2
// GPIO-11 | U12| F2

// Translation and Rotation Mapping ===============================================================
// Translation ------------------------------------------------------------------------------------
// Forward  (+X) : A1, A2 | 0, 8
// Backward (-X) : F1, F2 | 5, 11
// Left     (+Y) : S1, S2 | 1, 7
// Right    (-Y) : P1, P2 | 2, 6
// Up       (+Z) : B1, B2 | 4, 10
// Down     (-Z) : T1, T2 | 3, 9

// Rotation ---------------------------------------------------------------------------------------
// CW Pitch  : T2, B1 | 9, 4
// CCW Pitch : T1, B2 | 3, 10
// CC Roll   : P2, S1 | 6, 1
// CCW Roll  : P1, S2 | 2, 7
// CW Yaw    : A1, F2 | 0, 5
// CCW Yaw   : A2, F1 | 8, 11



#include <iostream>  // For console input/output
#include <conio.h>   // For _kbhit() and _getch() to detect key presses without blocking



using namespace std;



void keybindLogs(string movement, char key, string thruster1, string thruster2) {
    if (movement == "Translation") {
        cout << "\nTranslation : " << (char)toupper(key) << " | Thruster: " << thruster1 << " " << thruster2 << endl;
    } else {
        cout << "\nRotation    : " << (char)toupper(key) << " | Thruster: " << thruster1 << " " << endl;
    }
}



void keybindActivation(char keybind) {
    switch (toupper(keybind)) {
        // Translation ----------------------------------------------------------------------------
        case 'A': keybindLogs("Translation +X", keybind, "A1", "A2");  break;  // A1, A2 | 0, 8
        case 'D': keybindLogs("Translation -X", keybind, "F1", "F2");  break;  // F1, F2 | 5, 11
        case 'E': keybindLogs("Translation +Y", keybind, "S1", "S2");  break;  // S1, S2 | 1, 7
        case 'Q': keybindLogs("Translation -Y", keybind, "P1", "P2");  break;  // P1, P2 | 2, 6
        case 'W': keybindLogs("Translation +Z", keybind, "B1", "B2");  break;  // B1, B2 | 4, 10
        case 'S': keybindLogs("Translation -Z", keybind, "T1", "T2");  break;  // T1, T2 | 3, 9

        // Roation --------------------------------------------------------------------------------
        case 'O': keybindLogs("Rotation +R", keybind, "P2", "S1");  break;  // P1, S2 | 6, 1
        case 'U': keybindLogs("Rotation -R", keybind, "P1", "S2");  break;  // P2, S1 | 2, 7
        case 'K': keybindLogs("Rotation +P", keybind, "T1", "B2");  break;  // T1, B2 | 3, 10
        case 'I': keybindLogs("Rotation -P", keybind, "T2", "B1");  break;  // T2, B1 | 9, 4
        case 'J': keybindLogs("Rotation +Y", keybind, "A2", "F1");  break;  // A2, F1 | 8, 5
        case 'L': keybindLogs("Rotation -Y", keybind, "A1", "F2");  break;  // A1, F2 | 0, 11

        // Error Handling -------------------------------------------------------------------------
        default: 
            cout << "\n[ERROR-00: Invalid Keybind] The key '" << keybind 
                 << "' is not mapped to any thrusters. Please use: WASDQE, IJKLUO" << endl;
            break;
    }
}



int main() {
    cout << "Active Controller. Press Keys to see GPIO Pins. (ESC to exit)" << endl;

    while (true) {
        if (_kbhit()) {
            char input = _getch();
            if (input == 27) break;  // Exit on ESC key
            keybindActivation(input);
        }
    }

    return 0;
}

// cd Controller
// g++ controller_in_prog.cpp -o controller_in_prog
// ./controller_in_prog
// cd Controller g++ controller_in_prog.cpp -o controller_in_prog ./controller_in_prog