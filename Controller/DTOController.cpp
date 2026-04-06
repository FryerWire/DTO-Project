/*
    DTO Controller - Raspberry Pi 5 Optimized Port
    
    UI Flow:
    - 0 : Menu Mode
    - 1 : Startup Sequence Mode
    - 2 : Operational Mode (Starts in Continuous 'C')
    - Esc : Quit Program
    
    Mappings:
    - Translation: W/S (+/-X), A/D (-/+Y), Q/E (-/+Z)
    - Rotation:     I/K (+/-Pitch), J/L (-/+Yaw), U/O (-/+Roll)
*/

#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <string>
#include <iomanip>
#include <vector>

// Linux Specific Headers
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <gpiod.h> 

using namespace std;

// Global Variables ===============================================================================
double timeCounter = 0.0;
string lastKeyFired = ""; 
char firingMode = 'C'; 
int programMode = 0; 
const string logDirectoryPath = "./Logs/"; 
struct gpiod_chip* gpioChip;
const char* gpioChipPath = "/dev/gpiochip4"; 

// Forward Declarations
void logActivity(string code, string description);
void logKeyData(char type, string direction, string keyName, char statusChar, char mode);
void displayMenu();
void processMovementAction(string keyId, char mode);

// Helper for 0:10 Timestamp Format
string formatTimestamp(double t) {
    int wholeSeconds = (int)t;
    int decimalSeconds = (int)((t - wholeSeconds) * 100 + 0.5);
    string decimalString = (decimalSeconds < 10) ? "0" + to_string(decimalSeconds) : to_string(decimalSeconds);
    return to_string(wholeSeconds) + ":" + decimalString;
}

// GPIO Logic =====================================================================================

void initGPIO() {
    gpioChip = gpiod_chip_open(gpioChipPath);
    if (!gpioChip) {
        gpioChip = gpiod_chip_open("/dev/gpiochip0"); 
        if (!gpioChip) {
            logActivity("ERROR-303", "GPIO Chip Failure: Cannot find chip4 or chip0");
            return;
        }
    }
    logActivity("STATUS-306", "GPIO Init Success: Controller linked to hardware");
}

void setGPIOPin(int pinOffset, int pinValue) {
    if (!gpioChip) return;
    struct gpiod_line_settings* lineSettings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(lineSettings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(lineSettings, pinValue ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
    struct gpiod_line_config* lineConfig = gpiod_line_config_new();
    unsigned int offset = (unsigned int)pinOffset;
    gpiod_line_config_add_line_settings(lineConfig, &offset, 1, lineSettings);
    struct gpiod_request_config* requestConfig = gpiod_request_config_new();
    gpiod_request_config_set_consumer(requestConfig, "DTO_Controller");
    struct gpiod_line_request* lineRequest = gpiod_chip_request_lines(gpioChip, requestConfig, lineConfig);
    if (lineRequest) gpiod_line_request_release(lineRequest);
    gpiod_request_config_free(requestConfig);
    gpiod_line_config_free(lineConfig);
    gpiod_line_settings_free(lineSettings);
}

// Input Handling =================================================================================

int checkKeyboardInput() {
    static const int STDIN_FILENO_ID = 0;
    static bool isInputInitialized = false;
    if (!isInputInitialized) {
        termios terminalSettings;
        tcgetattr(STDIN_FILENO_ID, &terminalSettings);
        terminalSettings.c_lflag &= ~ICANON;
        terminalSettings.c_lflag &= ~ECHO; 
        tcsetattr(STDIN_FILENO_ID, TCSANOW, &terminalSettings);
        setbuf(stdin, NULL);
        isInputInitialized = true;
    }
    int bytesAvailable;
    ioctl(STDIN_FILENO_ID, FIONREAD, &bytesAvailable);
    return bytesAvailable;
}

// Logging ========================================================================================

void logActivity(string code, string description) {
    string timeString = formatTimestamp(timeCounter);
    ofstream activityFile(logDirectoryPath + "Activity_Log.csv", ios_base::app);
    if (activityFile.is_open()) {
        activityFile << timeString << "," << code << "," << description << endl;
        activityFile.close();
    }
    cout << timeString << "," << code << "," << description << endl;
}

void logKeyData(char type, string direction, string keyName, char statusChar, char mode) {
    string timeString = formatTimestamp(timeCounter);
    ofstream keyLogFile(logDirectoryPath + "Keybind_Log.csv", ios_base::app);
    if (keyLogFile.is_open()) {  
        keyLogFile << timeString << "," << mode << "," << type << "," << direction << "," << keyName << endl;
        keyLogFile.close();
        if (statusChar == 'N' && keyName != "-") {
            logActivity("STATUS-301", "Key Registered: " + keyName);
        }
    } else {
        logActivity("ERROR-101", "Write Failure: Keybind_Log is inaccessible");
    }
}

// Mapping Logic ==================================================================================

void processMovementAction(string keyId, char mode) {
    if (mode == 'P' && keyId == lastKeyFired) {
        logActivity("ERROR-305", "Mode Specific Block: Pulse Mode repeat prevented for " + keyId);
        logKeyData('F', "--", keyId, 'E', 'P');
        return;
    }

    // Translation Mappings
    if (keyId == "W")      { logKeyData('T', "+X", "W", 'N', mode); setGPIOPin(0, 1); setGPIOPin(8, 1); }
    else if (keyId == "S") { logKeyData('T', "-X", "S", 'N', mode); setGPIOPin(5, 1); setGPIOPin(11, 1); }
    else if (keyId == "A") { logKeyData('T', "-Y", "A", 'N', mode); setGPIOPin(2, 1); setGPIOPin(6, 1); }
    else if (keyId == "D") { logKeyData('T', "+Y", "D", 'N', mode); setGPIOPin(1, 1); setGPIOPin(9, 1); }
    else if (keyId == "Q") { logKeyData('T', "-Z", "Q", 'N', mode); setGPIOPin(3, 1); setGPIOPin(9, 1); }
    else if (keyId == "E") { logKeyData('T', "+Z", "E", 'N', mode); setGPIOPin(4, 1); setGPIOPin(10, 1); }
    
    // Rotation Mappings
    else if (keyId == "I") { logKeyData('R', "+P", "I", 'N', mode); setGPIOPin(9, 1); setGPIOPin(4, 1); }
    else if (keyId == "K") { logKeyData('R', "-P", "K", 'N', mode); setGPIOPin(3, 1); setGPIOPin(10, 1); }
    else if (keyId == "J") { logKeyData('R', "-Y", "J", 'N', mode); setGPIOPin(2, 1); setGPIOPin(9, 1); }
    else if (keyId == "L") { logKeyData('R', "+Y", "L", 'N', mode); setGPIOPin(6, 1); setGPIOPin(1, 1); }
    else if (keyId == "U") { logKeyData('R', "-R", "U", 'N', mode); setGPIOPin(2, 1); setGPIOPin(9, 1); }
    else if (keyId == "O") { logKeyData('R', "+R", "O", 'N', mode); setGPIOPin(6, 1); setGPIOPin(1, 1); }
    
    else { 
        logKeyData('F', "--", keyId, 'E', mode); 
        logActivity("ERROR-300", "Incorrect Keybind"); 
    }
    lastKeyFired = keyId;
}

// UI =============================================================================================

void displayMenu() {
    logActivity("STATUS-402", "UI Redraw: Menu refreshed");
    if (programMode == 0) {
        cout << "\n=================================================\nDTO Program\n-------------------------------------------------\nProgram Modes:\n- 0   : Menu Mode\n- 1   : Startup Sequence Mode\n- 2   : Operational Mode\n- Esc : Quit Mode\n=================================================\n>> " << flush;
    } else if (programMode == 1) {
        cout << "\n=================================================\nStartup Sequence Mode\n-------------------------------------------------\nProgram Modes:\n- Esc : Quit Mode\n=================================================\n>> " << flush;
    } else if (programMode == 2) {
        cout << "\n=================================================\nOperational Mode (2)\n-------------------------------------------------\nProgram Modes:\n- 1   : Continuous Mode\n- 2   : Pulse Mode\n- Esc : Quit Program\n=================================================\n>> " << flush;
    }
}

// Main ===========================================================================================

int main() {
    if (system(("mkdir -p " + logDirectoryPath).c_str()) == 0) {
        logActivity("STATUS-103", "Directory Created: Log storage ready");
    } else {
        logActivity("ERROR-103", "Dir Creation Fail: Check permissions");
    }

    initGPIO();
    ofstream keybindLogFile(logDirectoryPath + "Keybind_Log.csv", ios::trunc);
    ofstream activityLogFile(logDirectoryPath + "Activity_Log.csv", ios::trunc);
    if (keybindLogFile.is_open() && activityLogFile.is_open()) {
        logActivity("STATUS-101", "Path Validated: File Open Successful");
        keybindLogFile << "Time(s),Mode,Type,Direction,Key" << endl;
        activityLogFile << "Time(s),Code,Description" << endl;
        keybindLogFile.close(); activityLogFile.close();
        logActivity("STATUS-000", "Startup Successful: Files Ready");
    } else { 
        logActivity("ERROR-000", "Startup Failure: Path Inaccessible");
        return 1; 
    }

    logActivity("STATUS-001", "Session Started");
    displayMenu();

    while (true) {
        if (checkKeyboardInput()) {
            unsigned char charInput = getchar();
            if (charInput == 27) break; // ESC

            if (programMode == 0) {
                if (charInput == '1') {
                    programMode = 1; displayMenu();
                    logActivity("STATUS-302", "Sequence Initiated");
                    int rackConnectors[3][4] = {{0,3,4,1}, {8,9,10,9}, {2,6,5,11}};
                    for (int rackIndex = 0; rackIndex < 3; rackIndex++) {
                        logActivity("STATUS-303", "Testing Rack Connector " + to_string(rackIndex+1));
                        for (int gpioIndex = 0; gpioIndex < 4; gpioIndex++) {
                            setGPIOPin(rackConnectors[rackIndex][gpioIndex], 1);
                            logActivity("STATUS-304", "GPIO " + to_string(rackConnectors[rackIndex][gpioIndex]) + " ON");
                            this_thread::sleep_for(chrono::milliseconds(250));
                            setGPIOPin(rackConnectors[rackIndex][gpioIndex], 0);
                            logActivity("STATUS-304", "GPIO " + to_string(rackConnectors[rackIndex][gpioIndex]) + " OFF");
                            this_thread::sleep_for(chrono::milliseconds(250));
                        }
                    }
                    logActivity("STATUS-305", "Sequence Complete");
                    programMode = 0; displayMenu();
                } else if (charInput == '2') {
                    programMode = 2;
                    firingMode = 'C'; // Automatically start in Continuous Mode
                    logActivity("STATUS-300", "Mode Changed: Operational Mode Active");
                    displayMenu();
                }
            } else if (programMode == 2) {
                if (charInput == '1') { firingMode = 'C'; logActivity("STATUS-300", "Mode Changed: Continuous Mode"); }
                else if (charInput == '2') { firingMode = 'P'; logActivity("STATUS-300", "Mode Changed: Pulse Mode"); }
                else if (charInput == '0') { programMode = 0; displayMenu(); }
                else {
                    string keyIdString = string(1, toupper(charInput));
                    processMovementAction(keyIdString, firingMode);
                }
            }
        } else {
            if (programMode == 2) {
                logKeyData('F', "--", "-", 'N', firingMode);
                lastKeyFired = "";
            }
        }
        if (programMode == 2) timeCounter += 0.1;
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    logActivity("STATUS-002", "Session Ended");
    logActivity("STATUS-003", "Shutdown Successful");
    if(gpioChip) gpiod_chip_close(gpioChip);
    return 0;
}