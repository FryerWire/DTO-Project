/*
DTO Controller Software

Features:
- Controls 12 thrusters via GPIO pins on a Raspberry Pi 5, with support for active-low or active-high relay configurations.
- Logs key events and high-level activities to separate CSV files with timestamps and standardized codes.
- Supports a menu system with a Startup Sequence mode for testing GPIO activation and an Operational Mode for real-time control of the thrusters.
- Implements error handling for file access, incorrect keybinds, and mode-specific constraints.
- Uses non-blocking input handling to allow for real-time processing of key events without freezing the program.
- Uses hardware-based timing to prevent lag issues from affecting key press duration tracking.

Functions:
- main(): Initializes the program, handles the main loop for input processing and mode management, and performs cleanup on exit.
- initGPIO(): Initializes the GPIO chip for controlling the thrusters, with error handling for chip access.
- setGPIOPin(int pinOffset, int pinValue): Sets the specified GPIO pin to the desired value, with error handling for chip access and relay logic.
- checkKeyboardInput(): Checks for available keyboard input without blocking program execution, with initialization of terminal settings for non-blocking input.
- logActivity(string code, string description): Logs a high-level activity or event with a standardized code and description, along with a timestamp.
- logKeyData(char type, string direction, string keyName, char statusChar, char mode): Logs detailed information about key events, including type, direction, key name, status, and mode, with error handling for file access.
- processMovementAction(string keyId, char mode): Processes a key event for movement or rotation based on the key identifier and current mode, with error handling for invalid keybinds and Pulse Mode repeat prevention.
- displayMenu(): Displays the appropriate menu options based on the current program mode, with logging for UI redraws.

Codes:
- STATUS-000: Startup Successful: Files Ready
- STATUS-001: Session Started
- STATUS-101: Path Validated: File Open Successful
- STATUS-103: Directory Created: Log storage ready
- STATUS-300: Mode Changed: Operational Mode Active
- STATUS-301: Key Registered: [KeyName]
- STATUS-302: Sequence Initiated
- STATUS-304: GPIO [PinNumber] ON/OFF
- STATUS-305: Sequence Complete
- STATUS-402: UI Redraw: Menu refreshed
- ERROR-000: Startup Failure: Path Inaccessible
- ERROR-101: Write Failure: Keybind_Log is inaccessible
- ERROR-103: Dir Creation Fail: Check permissions
- ERROR-300: Incorrect Keybind
- ERROR-303: GPIO Chip Failure: Cannot find chip4 or chip0
- ERROR-305: Mode Specific Block: Pulse Mode repeat prevented for [KeyName]
- ERROR-306: GPIO Init Failure: Controller not linked to hardware
- ERROR-402: UI Redraw Failure: Console output error
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
                 This function first checks if the gpioChip is initialized. If not, it returns immediately. 
                 It then creates line settings for the specified pin, setting it as an output and applying the appropriate output value based on relay logic. 
                 It configures a line request for the specified pin and releases it immediately after setting the value. 
                 Finally, it frees all allocated resources related to the line settings and request configuration.

    Parameters:
    - pinOffset (int) : The offset of the GPIO pin to set (0-11 for this controller)
    - pinValue (int)  : The desired state of the pin, where 1 represents ON and 0 represents OFF. 
                        This value is processed through getRelayValue to account for active-low or active-high relay configurations.
*/
void setGPIOPin(int pinOffset, int pinValue) {
    // Error handling for GPIO chip access ----------------------------------------------------------------------------------------------------------
    if (!gpioChip) return;
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
    if (lineRequest) gpiod_line_request_release(lineRequest);
    gpiod_request_config_free(requestConfig);
    gpiod_line_config_free(lineConfig);
    gpiod_line_settings_free(lineSettings);
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
    - statusChar (char)   : A character representing the status of the key event ('N' for new press, 'E' for error)
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
            logActivity("STATUS-301", "Key Registered: " + keyName);
        }
    } else {
        logActivity("ERROR-101", "Write Failure: Keybind_Log is inaccessible");
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
        logActivity("ERROR-305", "Mode Specific Block: Pulse Mode repeat prevented for " + keyId);
        logKeyData('F', "--", keyId, 'E', 'P');
        return;
    }

    // Translation Mappings -------------------------------------------------------------------------------------------------------------------------
    if (keyId == "W")      { logKeyData('T', "+X", "W", 'N', mode); setGPIOPin(0, 1); setGPIOPin(8, 1); }   // A1, A2
    else if (keyId == "S") { logKeyData('T', "-X", "S", 'N', mode); setGPIOPin(5, 1); setGPIOPin(11, 1); }  // F1, F2
    else if (keyId == "A") { logKeyData('T', "+Y", "A", 'N', mode); setGPIOPin(1, 1); setGPIOPin(7, 1); }   // S1, S2
    else if (keyId == "D") { logKeyData('T', "-Y", "D", 'N', mode); setGPIOPin(2, 1); setGPIOPin(6, 1); }   // P1, P2
    else if (keyId == "E") { logKeyData('T', "+Z", "E", 'N', mode); setGPIOPin(4, 1); setGPIOPin(10, 1); }  // B1, B2
    else if (keyId == "Q") { logKeyData('T', "-Z", "Q", 'N', mode); setGPIOPin(3, 1); setGPIOPin(9, 1); }   // T1, T2
    
    // Rotation Mappings ----------------------------------------------------------------------------------------------------------------------------
    else if (keyId == "K") { logKeyData('R', "-P", "K", 'N', mode); setGPIOPin(9, 1); setGPIOPin(4, 1); }   // T2, B1
    else if (keyId == "I") { logKeyData('R', "+P", "I", 'N', mode); setGPIOPin(3, 1); setGPIOPin(10, 1); }  // T1, B2
    else if (keyId == "U") { logKeyData('R', "-R", "U", 'N', mode); setGPIOPin(6, 1); setGPIOPin(1, 1); }   // P2, S1
    else if (keyId == "O") { logKeyData('R', "+R", "O", 'N', mode); setGPIOPin(2, 1); setGPIOPin(7, 1); }   // P1, S2
    else if (keyId == "J") { logKeyData('R', "-Y", "J", 'N', mode); setGPIOPin(0, 1); setGPIOPin(11, 1); }  // A1, F2
    else if (keyId == "L") { logKeyData('R', "+Y", "L", 'N', mode); setGPIOPin(8, 1); setGPIOPin(5, 1); }   // A2, F1
    
    else { 
        logKeyData('F', "--", keyId, 'E', mode); 
        logActivity("ERROR-300", "Incorrect Keybind"); 
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
    logActivity("STATUS-402", "UI Redraw: Menu refreshed");
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
        logActivity("STATUS-103", "Directory Created: Log storage ready");
    } else {
        logActivity("ERROR-103", "Dir Creation Fail: Check permissions");
    }

    // GPIO Initialization and Log File Preparation -------------------------------------------------------------------------------------------------
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
                    programMode = 1; displayMenu();
                    logActivity("STATUS-302", "Sequence Initiated");
                    int sequenceOrder[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
                    // Startup Sequence: Activates each thruster in order with a delay, then returns to Menu Mode -----------------------------------
                    for (int gpioPin : sequenceOrder) {
                        setGPIOPin(gpioPin, 1);
                        logActivity("STATUS-304", "GPIO " + to_string(gpioPin) + " ON");
                        this_thread::sleep_for(chrono::milliseconds(250));
                        setGPIOPin(gpioPin, 0);
                        logActivity("STATUS-304", "GPIO " + to_string(gpioPin) + " OFF");
                        this_thread::sleep_for(chrono::milliseconds(250));
                    }
                    logActivity("STATUS-305", "Sequence Complete");
                    programMode = 0; displayMenu();
                // Operational Mode: Accepts mode selection inputs and transitions to mode-specific menu --------------------------------------------
                } else if (charInput == '2') {
                    programMode = 2;
                    firingMode = 'C'; 
                    logActivity("STATUS-300", "Mode Changed: Operational Mode Active");
                    displayMenu();
                }
            // Operational Mode: Accepts mode selection and movement/rotation inputs ----------------------------------------------------------------
            } else if (programMode == 2) {
                // In Operational Mode, handle mode switching and movement/rotation key processing --------------------------------------------------
                if (charInput == '1') { firingMode = 'C'; logActivity("STATUS-300", "Mode Changed: Continuous Mode"); }
                else if (charInput == '2') { firingMode = 'P'; logActivity("STATUS-300", "Mode Changed: Pulse Mode"); }
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
                    // Turn off all thrusters when no key is pressed
                    for(int i = 0; i <= 11; i++) setGPIOPin(i, 0);
                    currentKeyPressed = "";
                    lastKeyFired = "";
                }
            }
        }

        // Small delay to prevent excessive CPU usage
        this_thread::sleep_for(chrono::milliseconds(50));
    }
    
    // Cleanup and Exit ---------------------------------------------------------------------------
    for(int i = 0; i <= 11; i++) setGPIOPin(i, 0);
    logActivity("STATUS-002", "Session Ended");
    logActivity("STATUS-003", "Shutdown Successful");
    if(gpioChip) gpiod_chip_close(gpioChip);
    return 0;
}
