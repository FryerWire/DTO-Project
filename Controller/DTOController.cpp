/*
DTOController.cpp
DTO Controller Software - Raspberry Pi 5 GPIO Thruster Control System

Features:
- Controls 12 thrusters via GPIO pins on a Raspberry Pi 5 using libgpiod, with configurable active-low or active-high relay logic via the ACTIVE_LOW constant.
- Logs all key events and high-level application activities to separate CSV files (Keybind_Log.csv and Activity_Log.csv) with hardware-based elapsed timestamps and standardized STATUS/ERROR codes.
- Implements a three-mode menu system: Menu Mode (0) for mode selection, Startup Sequence Mode (1) for hardware validation across three rack connectors, and Operational Mode (2) for real-time thruster control.
- Startup Sequence Mode executes a two-phase GPIO test per rack connector: Phase 1 performs four individual 1-second ON/OFF pulses, and Phase 2 performs four double 0.5-second pulse pairs, tracking and reporting any GPIO failures separately per rack.
- Operational Mode supports two firing sub-modes: Continuous Mode ('C'), where thrusters fire for the duration of a key hold, and Pulse Mode ('P'), where each key fires only once per new press, preventing repeat firings on hold.
- Maps 12 keybinds (W, S, A, D, E, Q for translation; I, K, U, O, J, L for rotation) to specific GPIO pin pairs controlling thruster pairs across all six degrees of freedom.
- Automatically deactivates all 12 thrusters when no key is detected in Operational Mode, ensuring failsafe thruster shutdown on key release.
- Uses non-blocking terminal input via POSIX termios and O_NONBLOCK fcntl configuration, with IOCTL byte-available checking to allow real-time key polling without freezing execution.
- Uses chrono::high_resolution_clock for hardware-based timing, preventing OS scheduling lag from affecting key press duration tracking or log timestamps.
- Performs safe GPIO cleanup on ESC exit, deactivating all thrusters and closing the gpiod chip handle before returning.

Functions:
- main(): Initializes the log directory, GPIO chip, and log files; runs the primary input polling loop for mode management and thruster control; performs full GPIO shutdown and log finalization on ESC exit.
- initGPIO(): Attempts to open the GPIO chip at /dev/gpiochip4, falls back to /dev/gpiochip0 on failure, and logs the result; sets the global gpioChip handle used by all pin operations.
- setGPIOPin(int pinOffset, int pinValue): Allocates a gpiod line settings and request configuration for a single pin, applies the correct active-high or active-low output value via getRelayValue(), releases the line immediately after setting, and frees all resources; returns true on success, false on failure.
- getRelayValue(int val): Translates a logical ON (1) or OFF (0) pin value into the correct gpiod_line_value enum considering the ACTIVE_LOW relay configuration constant.
- checkKeyboardInput(): Initializes the terminal to raw non-blocking mode on the first call (disabling ICANON and ECHO, setting O_NONBLOCK); on all calls returns the number of bytes available on stdin via IOCTL FIONREAD without consuming any input.
- logActivity(string code, string description): Appends a timestamped STATUS or ERROR code entry to Activity_Log.csv and echoes it to stdout; silently skips the file write if the log is inaccessible but still prints to console.
- logKeyData(char type, string direction, string keyName, char statusChar, char mode): Appends a timestamped key event row to Keybind_Log.csv with mode, type, direction, and key name fields; triggers a STATUS-011 logActivity entry for valid new key presses; logs ERROR-002 if the file is inaccessible.
- processMovementAction(string keyId, char mode): Validates the key against Pulse Mode repeat rules, maps the key to a translation or rotation action, calls logKeyData and setGPIOPin for the two associated thruster pins, and updates lastKeyFired; logs ERROR-004 for unmapped keys.
- displayMenu(): Logs a STATUS-019 UI redraw event and prints the mode-appropriate menu to stdout based on the current value of programMode.
- getElapsedTime(): Returns the number of seconds elapsed since programStartTime using chrono::high_resolution_clock for sub-millisecond precision.
- formatTimestamp(double t): Formats a floating-point elapsed time value into a "seconds:centiseconds" string (e.g., "12:07") for use in log entries.

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
- STATUS-100: GPIO Chip Opened Successfully at Primary Path (gpiochip4)
- STATUS-101: GPIO Chip Opened Successfully via Fallback Path (gpiochip0)
- STATUS-102: GPIO Line Settings Object Allocated
- STATUS-103: GPIO Line Configured as Output with Target Value
- STATUS-104: GPIO Line Request Submitted to Chip
- STATUS-105: GPIO Line Request Released After Set
- STATUS-106: All 12 Thrusters Deactivated on Key Release
- STATUS-107: All 12 Thrusters Deactivated on Program Exit
- STATUS-108: Terminal Configured for Raw Non-Blocking Input Mode
- STATUS-109: stdin Set to O_NONBLOCK: Non-Blocking Reads Active
- STATUS-110: Translation Action Processed: GPIO Pair Activated
- STATUS-111: Rotation Action Processed: GPIO Pair Activated
- STATUS-112: Continuous Firing Mode Active
- STATUS-113: Pulse Firing Mode Active
- STATUS-114: Startup Sequence Phase 1 Started: 1-Second Pulses
- STATUS-115: Startup Sequence Phase 2 Started: Double 0.5-Second Pulses
- STATUS-116: Rack Connector Failure Summary Logged
- STATUS-117: Duplicate GPIO Failure Entry Suppressed
- STATUS-118: Force Exit Triggered by ESC During Startup Sequence
- ERROR-000: Startup Failure: Log Path Inaccessible or Cannot Be Created
- ERROR-001: Activity Log Write Failure: File Inaccessible
- ERROR-002: Keybind Log Write Failure: File Inaccessible
- ERROR-003: Log Directory Creation Failed: Check Permissions
- ERROR-004: Incorrect Keybind: No Mapping Found for Key
- ERROR-005: Startup Sequence Aborted by User via ESC
- ERROR-006: Rack Connector Test Failed: GPIO Connection Issue on Pin
- ERROR-007: GPIO Pin Activation Failed: setGPIOPin Returned False
- ERROR-100: GPIO Chip Access Failed: gpiochip4 Not Found at Primary Path
- ERROR-101: GPIO Chip Access Failed: gpiochip0 Fallback Also Failed
- ERROR-102: GPIO Not Initialized: gpioChip Handle is Null, Cannot Set Pin
- ERROR-103: GPIO Line Request Failed: gpiod_chip_request_lines Returned Null
- ERROR-104: GPIO Pin Activation Failure Recorded During Startup Sequence
- ERROR-105: Pulse Mode Repeat Blocked: Key Already Fired This Press
- ERROR-106: UI Redraw Failed: Console Output Error
- ERROR-107: Mode Entry Rejected: Invalid Mode Character Received
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

// Linux Specific Headers ---------------------------------------------------------------------------------------------------------------------------
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <gpiod.h> 



using namespace std;



// Global Variables =================================================================================================================================
// Program State Variables --------------------------------------------------------------------------------------------------------------------------
int programMode = 0;                          // 0 = Menu, 1 = Startup Sequence, 2 = Operational Mode
char firingMode = 'C';                        // 'C' for Continuous, 'P' for Pulse Mode in Operational Mode

// Logging and Input Tracking Variables -------------------------------------------------------------------------------------------------------------
chrono::high_resolution_clock::time_point programStartTime;  // Track actual program start time
string lastKeyFired = "";                     // Tracks last key for Pulse Mode repeat prevention
string currentKeyPressed = "";                // Tracks currently pressed key
vector<int> failedGPIOPins;                   // Tracks GPIO pins that failed during startup
struct gpiod_chip* gpioChip;                  // GPIO chip handle
const int ACTIVE_LOW = 0;                     // Set to 1 if using active-low relays, 0 for active-high relays
const string logDirectoryPath = "./Logs/";    // Data logging path
const char* gpioChipPath = "/dev/gpiochip4";  // GPIO chip path



// Function Prototypes ==============================================================================================================================
void displayMenu();
void logActivity(string code, string description);
void processMovementAction(string keyId, char mode);
void logKeyData(char type, string direction, string keyName, char statusChar, char mode);
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
    formatTimestamp - Converts a double time value into a string formatted as "seconds:decimal"

    Parameters:
    - t (double) : The time value to format, where the whole number represents seconds and the decimal represents hundredths of a second.
*/
string formatTimestamp(double t) {
    int wholeSeconds = (int)t;
    int decimalSeconds = (int)((t - wholeSeconds) * 100 + 0.5);
    string decimalString = (decimalSeconds < 10) ? "0" + to_string(decimalSeconds) : to_string(decimalSeconds);
    return to_string(wholeSeconds) + ":" + decimalString;
}



/*
    getRelayValue - Determines the appropriate GPIO line value based on the desired pin state and relay logic

    Parameters:
    - val (int) : The desired pin state, where 1 represents ON and 0 represents OFF

    Returns:
    - gpiod_line_value : The corresponding GPIO line value to set, taking into account whether the relays are active-low or active-high
*/
enum gpiod_line_value getRelayValue(int val) {
    if (val == 1) {
        return ACTIVE_LOW ? GPIOD_LINE_VALUE_INACTIVE : GPIOD_LINE_VALUE_ACTIVE;  // For val == 1, return the opposite value based on relay logic
    }

    return ACTIVE_LOW ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;      // For val == 0, return the opposite value based on relay logic
}



/*
    initGPIO - Initializes the GPIO chip for controlling the thrusters, with error handling for chip access
               This function attempts to open the specified GPIO chip at gpioChipPath. 
               If it fails, it falls back to trying "/dev/gpiochip0". 
               If both attempts fail, it logs an error and returns without initializing the GPIO chip. 
               If successful, it logs a status message indicating that the GPIO initialization was successful 
               and that the controller is linked to the hardware.
*/
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



/*
    setGPIOPin - Sets the specified GPIO pin to the desired value, with error handling for chip access and relay logic
                 This function first checks if the gpioChip is initialized. If not, it returns false. 
                 It then creates line settings for the specified pin, setting it as an output and applying the appropriate output value based on relay logic. 
                 It configures a line request for the specified pin and releases it immediately after setting the value. 
                 Finally, it frees all allocated resources related to the line settings and request configuration.
                 Returns true if the operation was successful, false if there was an error.

    Parameters:
    - pinOffset (int) : The offset of the GPIO pin to set (0-11 for this controller)
    - pinValue (int)  : The desired state of the pin, where 1 represents ON and 0 represents OFF. 
                        This value is processed through getRelayValue to account for active-low or active-high relay configurations.

    Returns:
    - bool : true if the GPIO pin was successfully set, false if there was an error (chip not initialized or line request failed)
*/
bool setGPIOPin(int pinOffset, int pinValue) {
    // Error handling for GPIO chip access ----------------------------------------------------------------------------------------------------------
    if (!gpioChip) return false;
    struct gpiod_line_settings* lineSettings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(lineSettings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(lineSettings, getRelayValue(pinValue));
    struct gpiod_line_config* lineConfig = gpiod_line_config_new();
    unsigned int offset = (unsigned int)pinOffset;
    gpiod_line_config_add_line_settings(lineConfig, &offset, 1, lineSettings);                           
    struct gpiod_request_config* requestConfig = gpiod_request_config_new();                              
    gpiod_request_config_set_consumer(requestConfig, "DTO_Controller");                              
    struct gpiod_line_request* lineRequest = gpiod_chip_request_lines(gpioChip, requestConfig, lineConfig);  

    // Error handling for line request --------------------------------------------------------------------------------------------------------------
    bool success = (lineRequest != nullptr);
    if (lineRequest) gpiod_line_request_release(lineRequest);
    gpiod_request_config_free(requestConfig);
    gpiod_line_config_free(lineConfig);
    gpiod_line_settings_free(lineSettings);
    return success;
}



/*
    checkKeyboardInput - Checks for available keyboard input without blocking program execution, with initialization of terminal settings for non-blocking input
                         This function initializes the terminal settings for non-blocking input on the first call, configuring the terminal to disable canonical mode and echo. 
                         It also sets the standard input to non-blocking mode. On subsequent calls, it checks how many bytes of input are available and returns that count, allowing the main loop to read input without blocking if no keys are pressed.

    Returns:
    - int : The number of bytes of keyboard input currently available to read. A value of 0 indicates no input is available.
*/
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
        
        int oldflags = fcntl(STDIN_FILENO_ID, F_GETFL, 0);
        fcntl(STDIN_FILENO_ID, F_SETFL, oldflags | O_NONBLOCK);
        
        isInputInitialized = true;
    }

    int bytesAvailable;
    ioctl(STDIN_FILENO_ID, FIONREAD, &bytesAvailable);
    
    return bytesAvailable;
}



/*
    logActivity - Logs a high-level activity or event with a standardized code and description, along with a timestamp
                  This function formats the current elapsed time into a string, opens the Activity_Log.csv file in append mode, and writes a new line containing the timestamp, activity code, and description. 
                  It also prints the same information to the console for real-time monitoring. If the file cannot be opened, it does not log the activity but still outputs to the console.

    Parameters:
    - code (string)        : A standardized code representing the type of activity or event being logged (e.g., "STATUS-101", "ERROR-000")
    - description (string) : A human-readable description providing additional context about the activity or event being logged
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
    logKeyData - Logs detailed information about key events, including type, direction, key name, status, and mode, with error handling for file access
                 This function formats the current elapsed time into a string, opens the Keybind_Log.csv file in append mode, and writes a new line containing the timestamp, mode, type of event (translation, rotation, or firing), direction of movement or rotation, key name, and status character. 
                 If the status character indicates a new key press ('N') and the key name is not "-", it also logs a high-level activity indicating that a key was registered. 
                 If the file cannot be opened for logging key data, it logs an error activity indicating that the Keybind_Log is inaccessible.

    Parameters:
    - type (char)          : A character representing the type of event being logged ('T' for translation, 'R' for rotation, 'F' for firing)
    - direction (string)   : A string indicating the direction of movement or rotation associated with the key event (e.g., "+X", "-P")
    - keyName (string)    : The name of the key that triggered the event (e.g., "W", "A", "K")
    - statusChar (char)   : A character representing the status of the key event ('N' for new press, 'R' for release, 'E' for error)
    - mode (char)         : A character representing the current firing mode ('C' for Continuous Mode, 'P' for Pulse Mode)
*/
void logKeyData(char type, string direction, string keyName, char statusChar, char mode) {
    double timeCounter = getElapsedTime();
    string timeString = formatTimestamp(timeCounter);
    ofstream keyLogFile(logDirectoryPath + "Keybind_Log.csv", ios_base::app);
    if (keyLogFile.is_open()) {  
        keyLogFile << timeString << "," << mode << "," << type << "," << direction << "," << keyName << endl;
        keyLogFile.close();
        if (statusChar == 'N' && keyName != "-") {
            logActivity("STATUS-011", "Key Registered: " + keyName);
        }
    } else {
        logActivity("ERROR-002", "Write Failure: Keybind_Log is inaccessible");
    }
}



/*
    processMovementAction - Processes a key event for movement or rotation based on the key identifier and current mode, with error handling for invalid keybinds and Pulse Mode repeat prevention
                            This function first checks if the current mode is Pulse Mode ('P') and if the key being processed is the same as the last key fired. If both conditions are true, it logs an error activity indicating that a repeat action was prevented for that key in Pulse Mode, logs an error entry in the Keybind_Log, and returns without processing the action. 
                            If the action is valid, it checks the key identifier against known movement and rotation keys, logs the corresponding key data, and sets the appropriate GPIO pins to activate the thrusters. 
                            If the key identifier does not match any known actions, it logs an error entry in the Keybind_Log and logs an error activity indicating an incorrect keybind. 
                            Finally, it updates lastKeyFired to track the most recent key event.

    Parameters:
    - keyId (string) : The identifier of the key that triggered the event (e.g., "W", "A", "K")
    - mode (char)    : The current firing mode ('C' for Continuous Mode, 'P' for Pulse Mode) that affects how the action is processed and logged
*/
void processMovementAction(string keyId, char mode) {
    if (mode == 'P' && keyId == lastKeyFired) {
        logActivity("ERROR-105", "Mode Specific Block: Pulse Mode repeat prevented for " + keyId);
        logKeyData('F', "--", keyId, 'E', 'P');
        return;
    }

    // Translation Mappings -------------------------------------------------------------------------------------------------------------------------
    if (keyId == "W")      { logKeyData('T', "+X", "W", 'N', mode); (void)setGPIOPin(0, 1); (void)setGPIOPin(8, 1); }   // A1, A2
    else if (keyId == "S") { logKeyData('T', "-X", "S", 'N', mode); (void)setGPIOPin(5, 1); (void)setGPIOPin(11, 1); }  // F1, F2
    else if (keyId == "A") { logKeyData('T', "+Y", "A", 'N', mode); (void)setGPIOPin(1, 1); (void)setGPIOPin(7, 1); }   // S1, S2
    else if (keyId == "D") { logKeyData('T', "-Y", "D", 'N', mode); (void)setGPIOPin(2, 1); (void)setGPIOPin(6, 1); }   // P1, P2
    else if (keyId == "E") { logKeyData('T', "+Z", "E", 'N', mode); (void)setGPIOPin(4, 1); (void)setGPIOPin(10, 1); }  // B1, B2
    else if (keyId == "Q") { logKeyData('T', "-Z", "Q", 'N', mode); (void)setGPIOPin(3, 1); (void)setGPIOPin(9, 1); }   // T1, T2
    
    // Rotation Mappings ----------------------------------------------------------------------------------------------------------------------------
    else if (keyId == "K") { logKeyData('R', "-P", "K", 'N', mode); (void)setGPIOPin(9, 1); (void)setGPIOPin(4, 1); }   // T2, B1
    else if (keyId == "I") { logKeyData('R', "+P", "I", 'N', mode); (void)setGPIOPin(3, 1); (void)setGPIOPin(10, 1); }  // T1, B2
    else if (keyId == "U") { logKeyData('R', "-R", "U", 'N', mode); (void)setGPIOPin(6, 1); (void)setGPIOPin(1, 1); }   // P2, S1
    else if (keyId == "O") { logKeyData('R', "+R", "O", 'N', mode); (void)setGPIOPin(2, 1); (void)setGPIOPin(7, 1); }   // P1, S2
    else if (keyId == "J") { logKeyData('R', "-Y", "J", 'N', mode); (void)setGPIOPin(0, 1); (void)setGPIOPin(11, 1); }  // A1, F2
    else if (keyId == "L") { logKeyData('R', "+Y", "L", 'N', mode); (void)setGPIOPin(8, 1); (void)setGPIOPin(5, 1); }   // A2, F1
    
    else { 
        logKeyData('F', "--", keyId, 'E', mode); 
        logActivity("ERROR-004", "Incorrect Keybind"); 
    }
    lastKeyFired = keyId;
}



/*
    displayMenu - Displays the appropriate menu options based on the current program mode, with logging for UI redraws
                  This function logs a status activity indicating that the menu is being refreshed. 
                  It then checks the current program mode and outputs the corresponding menu options to the console. 
                  The menu options include available modes and actions for each mode, along with instructions for quitting the program. 
                  Finally, it prompts the user for input with a ">> " symbol.
*/
void displayMenu() {
    logActivity("STATUS-019", "UI Redraw: Menu refreshed");
    if (programMode == 0) {
        cout << "\n=================================================\nDTO Program\n-------------------------------------------------\nProgram Modes:\n- 0   : Menu Mode\n- 1   : Startup Sequence Mode\n- 2   : Operational Mode\n- Esc : Quit Mode\n=================================================\n>> " << flush;
    } else if (programMode == 1) {
        cout << "\n=================================================\nStartup Sequence Mode\n-------------------------------------------------\nProgram Modes:\n- Esc : Quit Mode\n=================================================\n>> " << flush;
    } else if (programMode == 2) {
        cout << "\n=================================================\nOperational Mode (2)\n-------------------------------------------------\nProgram Modes:\n- 1   : Continuous Mode\n- 2   : Pulse Mode\n- Esc : Quit Program\n=================================================\n>> " << flush;
    }
}



/*
    main - The main entry point of the DTO Controller software, responsible for initialization, main program loop, and cleanup on exit
           This function first attempts to create the log directory and logs the result. It then initializes the GPIO chip and prepares the log files for keybinds and activities, logging the startup status. 
           It enters a main loop where it checks for keyboard input and processes it based on the current program mode. In Operational Mode, it also handles turning off thrusters when no keys are pressed. 
           The loop continues until the user presses the ESC key, at which point it performs cleanup by turning off all thrusters, logging session end and shutdown status, closing the GPIO chip if it was initialized, and returning from the program.

    Returns:
    - int : The exit status of the program, where 0 indicates successful execution and 1 indicates an error during initialization (e.g., log file access failure)
*/
int main() {
    // Initialize high-resolution timer
    programStartTime = chrono::high_resolution_clock::now();

    // Initialization and Setup ---------------------------------------------------------------------------------------------------------------------
    if (system(("mkdir -p " + logDirectoryPath).c_str()) == 0) {
        logActivity("STATUS-005", "Directory Created: Log storage ready");
    } else {
        logActivity("ERROR-003", "Dir Creation Fail: Check permissions");
    }

    // GPIO Initialization and Log File Preparation -------------------------------------------------------------------------------------------------
    initGPIO();
    ofstream keybindLogFile(logDirectoryPath + "Keybind_Log.csv", ios::trunc);
    ofstream activityLogFile(logDirectoryPath + "Activity_Log.csv", ios::trunc);
    if (keybindLogFile.is_open() && activityLogFile.is_open()) {
        logActivity("STATUS-008", "Path Validated: File Open Successful");
        keybindLogFile << "Time(s),Mode,Type,Direction,Key" << endl;
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
            unsigned char charInput = getchar();
            // Handle ESC key for quitting the program ----------------------------------------------------------------------------------------------
            if (charInput == 27) break; // ESC

            // Handle input based on current program mode -------------------------------------------------------------------------------------------
            if (programMode == 0) {
                // Menu Mode: Only accepts mode selection inputs ------------------------------------------------------------------------------------
                if (charInput == '1') {                                                            
                    programMode = 1; 
                    logActivity("STATUS-012", "Sequence Initiated");
                    
                    // Clear previous test failures
                    failedGPIOPins.clear();
                    
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
                        
                        // Phase 1: Individual 1 second ON/OFF pulses for each GPIO
                        for (int g = 0; g < 4; g++) {
                            if (checkKeyboardInput() && getchar() == 27) { force_exit = true; break; } // ESC check
                            
                            int gpioPin = connectors[rackNum][g];
                            double phaseTime = g * 2.0;
                            
                            // GPIO ON for 1 second
                            cout << fixed << setprecision(2) << phaseTime << " GPIO " << gpioPin << " On" << endl;
                            if (!setGPIOPin(gpioPin, 1)) {
                                failedGPIOPins.push_back(gpioPin);
                            }
                            logActivity("STATUS-016", "GPIO " + to_string(gpioPin) + " ON");
                            this_thread::sleep_for(chrono::milliseconds(1000));
                            
                            // GPIO OFF for 1 second
                            phaseTime += 1.0;
                            cout << fixed << setprecision(2) << phaseTime << " GPIO " << gpioPin << " Off" << endl;
                            setGPIOPin(gpioPin, 0);
                            logActivity("STATUS-017", "GPIO " + to_string(gpioPin) + " OFF");
                            this_thread::sleep_for(chrono::milliseconds(1000));
                        }
                        
                        // Phase 2: Double 0.5 second pulses for each GPIO
                        for (int g = 0; g < 4; g++) {
                            if (force_exit) break;
                            
                            int gpioPin = connectors[rackNum][g];
                            double phaseTime = 8.0 + (g * 2.0);
                            
                            // Two quick pulses
                            for (int pulse = 0; pulse < 2; pulse++) {
                                if (checkKeyboardInput() && getchar() == 27) { force_exit = true; break; } // ESC check
                                
                                // GPIO ON for 0.5 seconds
                                cout << fixed << setprecision(2) << phaseTime << " GPIO " << gpioPin << " On" << endl;
                                if (!setGPIOPin(gpioPin, 1)) {
                                    // Only add to failed list if not already there
                                    if (find(failedGPIOPins.begin(), failedGPIOPins.end(), gpioPin) == failedGPIOPins.end()) {
                                        failedGPIOPins.push_back(gpioPin);
                                    }
                                }
                                logActivity("STATUS-016", "GPIO " + to_string(gpioPin) + " ON (PULSE)");
                                this_thread::sleep_for(chrono::milliseconds(500));
                                
                                // GPIO OFF for 0.5 seconds
                                phaseTime += 0.5;
                                cout << fixed << setprecision(2) << phaseTime << " GPIO " << gpioPin << " Off" << endl;
                                setGPIOPin(gpioPin, 0);
                                logActivity("STATUS-017", "GPIO " + to_string(gpioPin) + " OFF (PULSE)");
                                this_thread::sleep_for(chrono::milliseconds(500));
                                phaseTime += 0.5;
                            }
                        }
                        
                        // Check if any GPIO in this rack failed and report all failures
                        vector<int> rackFailedPins;
                        for (int pin : failedGPIOPins) {
                            if (pin >= connectors[rackNum][0] && pin <= connectors[rackNum][3]) {
                                rackFailedPins.push_back(pin);
                            }
                        }
                        
                        if (!rackFailedPins.empty()) {
                            for (int failedPin : rackFailedPins) {
                                cout << "Rack Connector " << (rackNum + 1) << " Test Failed: GPIO " << failedPin << " Connection issue" << endl;
                                logActivity("ERROR-006", "Rack Connector " + to_string(rackNum + 1) + " Test Failed: GPIO " + to_string(failedPin) + " Connection issue");
                            }
                            cout << endl;
                        } else {
                            cout << "Rack Connector " << (rackNum + 1) << " Test Successfully Completed.\n" << endl;
                            logActivity("STATUS-015", "Rack Connector " + to_string(rackNum + 1) + " Test Successfully Completed");
                        }
                    }
                    
                    if (!force_exit) {
                        // Report final status
                        int successCount = 12 - failedGPIOPins.size();
                        if (failedGPIOPins.empty()) {
                            cout << "All 12/12 GPIO Successfully Activated." << endl;
                            logActivity("STATUS-013", "Sequence Complete: All 12/12 GPIO Successfully Activated");
                        } else {
                            cout << successCount << "/12 GPIO Successfully Activated." << endl;
                            for (int pin : failedGPIOPins) {
                                cout << "Rack Connector " << ((pin / 4) + 1) << " Test Failed: GPIO " << pin << " Connection issue" << endl;
                                logActivity("ERROR-007", "Connection Failed: GPIO " + to_string(pin));
                            }
                            logActivity("STATUS-018", "Sequence Complete: " + to_string(successCount) + "/12 GPIO activated, " + to_string(failedGPIOPins.size()) + " connection issues detected");
                        }
                    }
                    
                    programMode = 0; 
                    displayMenu();
                // Operational Mode: Accepts mode selection inputs and transitions to mode-specific menu --------------------------------------------
                } else if (charInput == '2') {
                    programMode = 2;
                    firingMode = 'C'; 
                    logActivity("STATUS-020", "Mode Changed: Operational Mode Active");
                    displayMenu();
                }
            // Operational Mode: Accepts mode selection and movement/rotation inputs ----------------------------------------------------------------
            } else if (programMode == 2) {
                // In Operational Mode, handle mode switching and movement/rotation key processing --------------------------------------------------
                if (charInput == '1') { firingMode = 'C'; logActivity("STATUS-112", "Mode Changed: Continuous Mode"); }
                else if (charInput == '2') { firingMode = 'P'; logActivity("STATUS-113", "Mode Changed: Pulse Mode"); }
                else if (charInput == '0') { programMode = 0; displayMenu(); }
                else {
                    string keyIdString = string(1, toupper(charInput));
                    // Only process if it's a different key than currently tracked
                    if (keyIdString != currentKeyPressed) {
                        processMovementAction(keyIdString, firingMode);
                        currentKeyPressed = keyIdString;
                    }
                }
            }
        // Handle thruster deactivation when no keys are pressed in Operational Mode ----------------------------------------------------------------
        } else {
            // In Operational Mode, if no keys are pressed, ensure all thrusters are turned off
            if (programMode == 2) {
                // Only turn off and reset if we were previously tracking a key
                if (!currentKeyPressed.empty()) {
                    // Log the key release event before deactivating thrusters
                    logKeyData('F', "--", currentKeyPressed, 'R', firingMode);
                    // Turn off all thrusters when no key is pressed
                    for(int i = 0; i <= 11; i++) (void)setGPIOPin(i, 0);
                    logActivity("STATUS-106", "All Thrusters Deactivated: Key Released (" + currentKeyPressed + ")");
                    currentKeyPressed = "";
                    lastKeyFired = "";
                }
            }
        }

        // Small delay to prevent excessive CPU usage
        this_thread::sleep_for(chrono::milliseconds(50));
    }
    
    // Cleanup and Exit ---------------------------------------------------------------------------
    for(int i = 0; i <= 11; i++) (void)setGPIOPin(i, 0);
    logActivity("STATUS-107", "All Thrusters Deactivated on Program Exit");
    logActivity("STATUS-002", "Session Ended");
    logActivity("STATUS-003", "Shutdown Successful");
    if(gpioChip) gpiod_chip_close(gpioChip);
    return 0;
}