
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



#include <iostream>
#include <fstream>   
#include <conio.h>   
#include <chrono>    
#include <thread>    
#include <string>
#include <iomanip> 



using namespace std;



double time_counter = 0.0;



void logData(char type, string direction, char keybind, char status) {
    // We keep the spaces for the terminal output so it stays readable for you
    cout << fixed << setprecision(2) 
         << time_counter << ", C, " << type << ", " << direction << ", " << keybind << ", " << status << endl;

    // We remove extra spaces for the CSV file to ensure clean data parsing
    ofstream outFile("Keybind-Log-Test.csv", ios_base::app);
    if (outFile.is_open()) {
        outFile << fixed << setprecision(2) 
                << time_counter << ",C," << type << "," << direction << "," << keybind << "," << status << endl;
        outFile.close();
    }
}



void processInput(char key) {
    char upperKey = toupper(key);
    switch (upperKey) {
        // Translation ----------------------------------------------------------------------------
        case 'A': logData('T', "+X", upperKey, 'N'); break;  // A1, A2 | 0, 8
        case 'D': logData('T', "-X", upperKey, 'N'); break;  // F1, F2 | 5, 11
        case 'E': logData('T', "+Y", upperKey, 'N'); break;  // S1, S2 | 1, 7
        case 'Q': logData('T', "-Y", upperKey, 'N'); break;  // P1, P2 | 2, 6
        case 'W': logData('T', "+Z", upperKey, 'N'); break;  // B1, B2 | 4, 10
        case 'S': logData('T', "-Z", upperKey, 'N'); break;  // T1, T2 | 3, 9

        // Rotation -------------------------------------------------------------------------------
        case 'O': logData('R', "+R", upperKey, 'N'); break;  // T2, B1 | 9, 4
        case 'U': logData('R', "-R", upperKey, 'N'); break;  // T1, B2 | 3, 10
        case 'K': logData('R', "+P", upperKey, 'N'); break;  // P2, S1 | 6, 1
        case 'I': logData('R', "-P", upperKey, 'N'); break;  // P1, S2 | 2, 7
        case 'J': logData('R', "+Y", upperKey, 'N'); break;  // A1, F2 | 0, 5
        case 'L': logData('R', "-Y", upperKey, 'N'); break;  // A2, F1 | 8, 11

        // Error Handling -------------------------------------------------------------------------
        default:  logData('F', "--", upperKey, 'E'); break;

        // Add error handling and Error Log CSV
    }
}



int main() {
    // Reset the log file and add CSV header
    ofstream resetFile("Keybind-Log-Test.csv", ios::trunc);
    resetFile << "Time(s),Constant,Type,Direction,Key,Status" << endl;
    resetFile.close();

    cout << "Logging Active (0.25s intervals). Saving to Keybind-Log-Test.csv" << endl;
    cout << "Press Keys (ESC to exit)..." << endl;
    cout << "------------------------------------------------------------" << endl;

    while (true) {
        if (_kbhit()) {
            char input = _getch();
            if (input == 27) break; // ESC to exit

            // Buffer Flush: Eats extra key repeats to prevent time lag
            while (_kbhit()) { _getch(); }

            processInput(input);
        } else {
            logData('F', "--", '-', 'N');
        }

        time_counter += 0.10;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    cout << "\nLogging complete. File saved as Keybind-Log-Test.csv" << endl;
    return 0;
}



// cd Controller; g++ controller_in_prog.cpp -o controller_in_prog; ./controller_in_prog