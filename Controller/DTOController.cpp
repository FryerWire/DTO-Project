/*
DTOController.cpp
DTO Controller Software - Raspberry Pi 5 GPIO Thruster Control System

Features:
- Controls 12 thrusters via GPIO pins on a Raspberry Pi 5 using libgpiod, with configurable active-low or active-high relay logic via the ACTIVE_LOW constant.
- Logs all key events and high-level application activities to separate CSV files (Keybind_Log.csv and Activity_Log.csv) with hardware-based elapsed timestamps and standardized STATUS/ERROR codes.
- Implements a three-mode menu system: Menu Mode (0) for mode selection, Startup Sequence Mode (1) for hardware validation across three rack connectors, and Operational Mode (2) for real-time thruster control.
- Startup Sequence Mode executes a two-phase GPIO test per rack connector: Phase 1 performs four individual 1-second ON/OFF pulses, and Phase 2 performs four double 0.5-second pulse pairs, tracking and reporting any GPIO failures separately per rack.
- Operational Mode fires thrusters continuously for the duration of a key hold and deactivates all thrusters when the key is released.
- Maps 12 keybinds (W, S, A, D, E, Q for translation; I, K, U, O, J, L for rotation) to specific GPIO pin pairs controlling thruster pairs across all six degrees of freedom.
- Automatically deactivates all 12 thrusters when no key is detected in Operational Mode, ensuring failsafe thruster shutdown on key release.
- Uses non-blocking terminal input via POSIX termios and O_NONBLOCK fcntl configuration, with IOCTL byte-available checking to allow real-time key polling without freezing execution.
- Uses a configurable no-input cycle counter (RELEASE_CYCLES) to bridge terminal key repeat delay gaps, preventing false release/re-fire cycling when a key is held down.
- Uses chrono::high_resolution_clock for hardware-based timing, preventing OS scheduling lag from affecting key press duration tracking or log timestamps.
- Performs safe GPIO cleanup on ESC exit, deactivating all thrusters and closing the gpiod chip handle before returning.

Functions:
- main(): Initializes the log directory, GPIO chip, and log files; runs the primary input polling loop for mode management and thruster control; performs full GPIO shutdown and log finalization on ESC exit.
- initGPIO(): Attempts to open the GPIO chip at /dev/gpiochip4, falls back to /dev/gpiochip0 on failure, and logs the result; sets the global gpioChip handle used by all pin operations.
- setGPIOPin(int pinOffset, int pinValue): Allocates a gpiod line settings and request configuration for a single pin, applies the correct active-high or active-low output value via getRelayValue(), releases the line immediately after setting, and frees all resources; returns true on success, false on failure.
- requestGPIOPin(int pinOffset, int pinValue): Same as setGPIOPin but returns the line request handle WITHOUT releasing it, keeping the pin held in the set state; used for sustained pulses in the startup sequence. Caller must release via releaseGPIORequest().
- releaseGPIORequest(gpiod_line_request* request): Releases a held GPIO line request obtained from requestGPIOPin(), returning the pin to its default state.
- getRelayValue(int val): Translates a logical ON (1) or OFF (0) pin value into the correct gpiod_line_value enum considering the ACTIVE_LOW relay configuration constant.
- checkKeyboardInput(): Initializes the terminal to raw non-blocking mode on the first call (disabling ICANON and ECHO, setting O_NONBLOCK); on all calls returns the number of bytes available on stdin via IOCTL FIONREAD without consuming any input.
- logActivity(string code, string description): Appends a timestamped STATUS or ERROR code entry to Activity_Log.csv and echoes it to stdout; silently skips the file write if the log is inaccessible but still prints to console.
- logKeyData(char type, string direction, string keyName, char statusChar): Appends a timestamped key event row to Keybind_Log.csv with type, direction, and key name fields; triggers a STATUS-011 logActivity entry for valid new key presses; logs ERROR-002 if the file is inaccessible.
- processMovementAction(string keyId): Maps the key to a translation or rotation action, calls logKeyData and setGPIOPin for the two associated thruster pins, and updates currentKeyPressed; logs ERROR-004 for unmapped keys.
- displayMenu(): Logs a STATUS-019 UI redraw event and prints the mode-appropriate menu to stdout based on the current value of programMode.
- getElapsedTime(): Returns the number of seconds elapsed since programStartTime using chrono::high_resolution_clock for sub-millisecond precision.
- formatTimestamp(double t): Formats a floating-point elapsed time value into a "seconds:centiseconds" string (e.g., "12:07") for use in log entries.

Codes:
- STATUS-001: Session Started: main loop entered after all file and GPIO initialization succeeded
- STATUS-002: Session Ended: ESC received, main loop exited, cleanup beginning
- STATUS-003: Shutdown Successful: all GPIO pins deactivated, chip handle closed, program returning 0
- STATUS-005: Log Dir Created: mkdir -p returned 0 for logDirectoryPath
- STATUS-008: Log Files Opened: Keybind_Log.csv and Activity_Log.csv created with headers
- STATUS-009: Startup Complete: all files ready for logging, session logging active
- STATUS-011: Key Registered: valid keybind matched in processMovementAction and written to Keybind_Log.csv
- STATUS-012: Startup Initiated: user pressed '1' in Menu Mode, GPIO validation starting
- STATUS-013: All GPIO Activated: all 12 GPIO successfully activated, no pins in failedGPIOPins
- STATUS-014: Rack Test Started: outer rack loop entered for this connector, Phase 1 and Phase 2 pending
- STATUS-015: Rack Test Passed: all four pins in this rack activated without failure in both phases
- STATUS-016: GPIO ON: requestGPIOPin returned a valid handle for this pin in Phase 1 or Phase 2
- STATUS-017: GPIO OFF: releaseGPIORequest called for this pin after its pulse duration elapsed
- STATUS-018: Partial Activation: startup complete with some failures, one or more pins in failedGPIOPins
- STATUS-019: Menu Refreshed: displayMenu() called, mode-appropriate menu text printed to stdout
- STATUS-020: Operational Mode: user pressed '2' in Menu Mode, real-time keybind polling now active
- STATUS-100: GPIO Chip Primary Path: gpiod_chip_open(/dev/gpiochip4) succeeded, gpioChip handle valid
- STATUS-101: GPIO Chip Fallback Path: primary /dev/gpiochip4 failed, gpiod_chip_open(/dev/gpiochip0) succeeded
- STATUS-102: Raw Input Configured: tcsetattr() disabled ICANON and ECHO on stdin
- STATUS-103: stdin Non-Blocking: fcntl() set O_NONBLOCK flag, getchar() will not block
- STATUS-104: Translation Dispatched: key matched a translation action, both GPIO thruster pins set to ON
- STATUS-105: Rotation Dispatched: key matched a rotation action, both GPIO thruster pins set to ON
- STATUS-106: Thrusters Deactivated (Key Release): noInputCount reached RELEASE_CYCLES threshold, key release confirmed
- STATUS-107: Thrusters Deactivated (Exit): ESC cleanup path set all 12 pins to 0 before shutdown
- STATUS-108: GPIO Line Requested: requestGPIOPin returned a valid handle to be released after sleep
- STATUS-109: GPIO Line Released: releaseGPIORequest called to return pin to default state
- STATUS-110: Phase 1 Started: four individual 1-second ON/OFF pulses beginning on this connector
- STATUS-111: Phase 2 Started: four double 0.5-second pulse pairs beginning on this connector
- STATUS-112: Duplicate Failure Suppressed: pin already present in failedGPIOPins, std::find blocked re-insertion
- STATUS-113: Force Exit During Startup: ESC byte detected via checkKeyboardInput, force_exit set to true
- STATUS-114: Returned to Menu: '0' pressed, all thrusters deactivated, programMode reset to 0
- ERROR-000: Log Open Failed: keybindLogFile or activityLogFile not is_open(), returning 1
- ERROR-002: Keybind Log Write Failed: Keybind_Log.csv could not be opened in append mode inside logKeyData()
- ERROR-003: Log Dir Creation Failed: system(mkdir -p) returned non-zero, log directory may not exist
- ERROR-004: Invalid Keybind: key received in Operational Mode has no matching entry in processMovementAction
- ERROR-005: Startup Aborted by ESC: ESC detected during startup sequence, force_exit was set and sequence halted
- ERROR-006: GPIO Activation Failed: requestGPIOPin returned null for a pin in this rack during Phase 1 or Phase 2
- ERROR-007: Failed Pin Summary: one or more pins remain in failedGPIOPins after all three rack tests completed
- ERROR-100: GPIO Init Failed: both gpiod_chip_open(/dev/gpiochip4) and /dev/gpiochip0 returned null
- ERROR-101: GPIO Primary Path Failed: gpiod_chip_open(/dev/gpiochip4) returned null, fallback to /dev/gpiochip0 initiated
- ERROR-102: GPIO Not Initialized: gpioChip handle is null in setGPIOPin or requestGPIOPin, returning false immediately
- ERROR-103: GPIO Line Request Failed: gpiod_chip_request_lines returned null for the target pin offset in setGPIOPin
- ERROR-104: GPIO Request Failed (Startup): requestGPIOPin returned null, pin added to failedGPIOPins
- ERROR-105: Invalid Menu Input: character received in Menu Mode does not match '1', '2', or ESC, discarded silently
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

// Logging and Input Tracking Variables -------------------------------------------------------------------------------------------------------------
chrono::high_resolution_clock::time_point programStartTime;  // Track actual program start time
string currentKeyPressed = "";                // Tracks currently pressed key
int noInputCount = 0;                         // Counts consecutive no-input poll cycles for release confirmation
const int RELEASE_CYCLES = 15;                // Number of consecutive no-input cycles (x50ms = 750ms) before confirming key release.
                                              // Must exceed the terminal key repeat delay (~250-660ms) divided by poll interval (50ms).
                                              // RPi5 key repeat delay is ~600ms, requiring at least 12 cycles; 15 provides a safe buffer.
                                              // Too low: false release/re-fire cycling on key hold. Too high: slow thruster shutdown on release.
vector<int> failedGPIOPins;                   // Tracks GPIO pins that failed during startup
struct gpiod_chip* gpioChip;                  // GPIO chip handle
const int ACTIVE_LOW = 0;                     // Set to 1 if using active-low relays, 0 for active-high relays
const string logDirectoryPath = "./Logs/";    // Data logging path
const char* gpioChipPath = "/dev/gpiochip4";  // GPIO chip path



// Function Prototypes ==============================================================================================================================
void displayMenu();
void logActivity(string code, string description);
void processMovementAction(string keyId);
void logKeyData(char type, string direction, string keyName, char statusChar);
struct gpiod_line_request* requestGPIOPin(int pinOffset, int pinValue);
void releaseGPIORequest(struct gpiod_line_request* request);
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
        logActivity("ERROR-101", "GPIO Primary Path Failed: /dev/gpiochip4 inaccessible, attempting fallback to /dev/gpiochip0");
        gpioChip = gpiod_chip_open("/dev/gpiochip0");
        if (!gpioChip) {
            logActivity("ERROR-100", "GPIO Init Failed: Both /dev/gpiochip4 and /dev/gpiochip0 returned null");
            return;
        }
        logActivity("STATUS-101", "GPIO Chip Fallback Path: /dev/gpiochip0 opened successfully");
        return;
    }
    logActivity("STATUS-100", "GPIO Chip Primary Path: /dev/gpiochip4 opened successfully");
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
    requestGPIOPin - Requests a GPIO pin and sets it to the desired value, returning the line request handle WITHOUT releasing it
                     This function is used when the pin must remain in the set state for a sustained duration (e.g., startup sequence pulses).
                     The caller is responsible for calling releaseGPIORequest() when the pin should be released.
                     Returns nullptr on failure (chip not initialized or line request failed).

    Parameters:
    - pinOffset (int) : The offset of the GPIO pin to set (0-11 for this controller)
    - pinValue (int)  : The desired state of the pin, where 1 represents ON and 0 represents OFF

    Returns:
    - gpiod_line_request* : The active line request handle, or nullptr on failure
*/
struct gpiod_line_request* requestGPIOPin(int pinOffset, int pinValue) {
    if (!gpioChip) return nullptr;
    struct gpiod_line_settings* lineSettings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(lineSettings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(lineSettings, getRelayValue(pinValue));
    struct gpiod_line_config* lineConfig = gpiod_line_config_new();
    unsigned int offset = (unsigned int)pinOffset;
    gpiod_line_config_add_line_settings(lineConfig, &offset, 1, lineSettings);
    struct gpiod_request_config* requestConfig = gpiod_request_config_new();
    gpiod_request_config_set_consumer(requestConfig, "DTO_Controller");
    struct gpiod_line_request* lineRequest = gpiod_chip_request_lines(gpioChip, requestConfig, lineConfig);

    gpiod_request_config_free(requestConfig);
    gpiod_line_config_free(lineConfig);
    gpiod_line_settings_free(lineSettings);
    return lineRequest;  // Caller must release via releaseGPIORequest()
}



/*
    releaseGPIORequest - Releases a held GPIO line request, returning the pin to its default state
                         This function safely releases a line request obtained from requestGPIOPin().
                         If the request is null, the function does nothing.

    Parameters:
    - request (gpiod_line_request*) : The line request handle to release, or nullptr
*/
void releaseGPIORequest(struct gpiod_line_request* request) {
    if (request) gpiod_line_request_release(request);
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
        logActivity("STATUS-102", "Raw Input Configured: tcsetattr() disabled ICANON and ECHO");

        int oldflags = fcntl(STDIN_FILENO_ID, F_GETFL, 0);
        fcntl(STDIN_FILENO_ID, F_SETFL, oldflags | O_NONBLOCK);
        logActivity("STATUS-103", "stdin Non-Blocking: fcntl() set O_NONBLOCK flag");

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
    logKeyData - Logs detailed information about key events, including type, direction, and key name, with error handling for file access
                 This function formats the current elapsed time into a string, opens the Keybind_Log.csv file in append mode, and writes a new line containing the timestamp, type of event (translation, rotation, or release), direction of movement or rotation, and key name. 
                 If the status character indicates a new key press ('N') and the key name is not "-", it also logs a high-level activity indicating that a key was registered. 
                 If the file cannot be opened for logging key data, it logs an error activity indicating that the Keybind_Log is inaccessible.

    Parameters:
    - type (char)          : A character representing the type of event being logged ('T' for translation, 'R' for rotation/release)
    - direction (string)   : A string indicating the direction of movement or rotation associated with the key event (e.g., "+X", "-P", "--" for release)
    - keyName (string)     : The name of the key that triggered the event (e.g., "W", "A", "K", "-" for release)
    - statusChar (char)    : A character representing the status of the key event ('N' for new press, 'R' for release, 'E' for error)
*/
void logKeyData(char type, string direction, string keyName, char statusChar) {
    double timeCounter = getElapsedTime();
    string timeString = formatTimestamp(timeCounter);
    ofstream keyLogFile(logDirectoryPath + "Keybind_Log.csv", ios_base::app);
    if (keyLogFile.is_open()) {  
        keyLogFile << timeString << "," << type << "," << direction << "," << keyName << endl;
        keyLogFile.close();
        if (statusChar == 'N' && keyName != "-") {
            logActivity("STATUS-011", "Key Registered: " + keyName);
        }
    } else {
        logActivity("ERROR-002", "Keybind Log Write Failed: Keybind_Log.csv inaccessible");
    }
}



/*
    processMovementAction - Processes a key event for movement or rotation based on the key identifier, with error handling for invalid keybinds
                            This function checks the key identifier against known movement and rotation keys, logs the corresponding key data, and sets the appropriate GPIO pins to activate the thrusters. 
                            If the key identifier does not match any known actions, it logs an error entry in the Keybind_Log and logs an error activity indicating an incorrect keybind.

    Parameters:
    - keyId (string) : The identifier of the key that triggered the event (e.g., "W", "A", "K")
*/
void processMovementAction(string keyId) {
    // Translation Mappings -------------------------------------------------------------------------------------------------------------------------
    if (keyId == "W")      { logKeyData('T', "+X", "W", 'N'); (void)setGPIOPin(0, 1); (void)setGPIOPin(8, 1); }   // A1, A2
    else if (keyId == "S") { logKeyData('T', "-X", "S", 'N'); (void)setGPIOPin(5, 1); (void)setGPIOPin(11, 1); }  // F1, F2
    else if (keyId == "A") { logKeyData('T', "+Y", "A", 'N'); (void)setGPIOPin(1, 1); (void)setGPIOPin(7, 1); }   // S1, S2
    else if (keyId == "D") { logKeyData('T', "-Y", "D", 'N'); (void)setGPIOPin(2, 1); (void)setGPIOPin(6, 1); }   // P1, P2
    else if (keyId == "E") { logKeyData('T', "+Z", "E", 'N'); (void)setGPIOPin(4, 1); (void)setGPIOPin(10, 1); }  // B1, B2
    else if (keyId == "Q") { logKeyData('T', "-Z", "Q", 'N'); (void)setGPIOPin(3, 1); (void)setGPIOPin(9, 1); }   // T1, T2
    
    // Rotation Mappings ----------------------------------------------------------------------------------------------------------------------------
    else if (keyId == "K") { logKeyData('R', "-P", "K", 'N'); (void)setGPIOPin(9, 1); (void)setGPIOPin(4, 1); }   // T2, B1
    else if (keyId == "I") { logKeyData('R', "+P", "I", 'N'); (void)setGPIOPin(3, 1); (void)setGPIOPin(10, 1); }  // T1, B2
    else if (keyId == "U") { logKeyData('R', "-R", "U", 'N'); (void)setGPIOPin(6, 1); (void)setGPIOPin(1, 1); }   // P2, S1
    else if (keyId == "O") { logKeyData('R', "+R", "O", 'N'); (void)setGPIOPin(2, 1); (void)setGPIOPin(7, 1); }   // P1, S2
    else if (keyId == "J") { logKeyData('R', "-Y", "J", 'N'); (void)setGPIOPin(0, 1); (void)setGPIOPin(11, 1); }  // A1, F2
    else if (keyId == "L") { logKeyData('R', "+Y", "L", 'N'); (void)setGPIOPin(8, 1); (void)setGPIOPin(5, 1); }   // A2, F1
    
    else { 
        logKeyData('F', "--", keyId, 'E'); 
        logActivity("ERROR-004", "Invalid Keybind");
    }
}



/*
    displayMenu - Displays the appropriate menu options based on the current program mode, with logging for UI redraws
                  This function logs a status activity indicating that the menu is being refreshed. 
                  It then checks the current program mode and outputs the corresponding menu options to the console. 
                  The menu options include available modes and actions for each mode, along with instructions for quitting the program. 
                  Finally, it prompts the user for input with a ">> " symbol.
*/
void displayMenu() {
    logActivity("STATUS-019", "Menu Refreshed: displayMenu() called");
    if (programMode == 0) {
        cout << "\n=================================================\nDTO Program\n-------------------------------------------------\nProgram Modes:\n- 0   : Menu Mode\n- 1   : Startup Sequence Mode\n- 2   : Operational Mode\n- Esc : Quit Mode\n=================================================\n>> " << flush;
    } else if (programMode == 1) {
        cout << "\n=================================================\nStartup Sequence Mode\n-------------------------------------------------\nProgram Modes:\n- Esc : Quit Mode\n=================================================\n>> " << flush;
    } else if (programMode == 2) {
        cout << "\n=================================================\nOperational Mode (2)\n-------------------------------------------------\nProgram Modes:\n- 0   : Menu Mode\n- Esc : Quit Program\n=================================================\n>> " << flush;
    }
}



/*
    main - The main entry point of the DTO Controller software, responsible for initialization, main program loop, and cleanup on exit
           This function first attempts to create the log directory and logs the result. It then initializes the GPIO chip and prepares the log files for keybinds and activities, logging the startup status. 
           It enters a main loop where it checks for keyboard input and processes it based on the current program mode. In Operational Mode, it also handles turning off thrusters when no keys are pressed. 
           A no-input cycle counter (noInputCount) prevents false release/re-fire cycling caused by gaps in terminal key repeat timing. Thrusters only deactivate after RELEASE_CYCLES consecutive empty polls.
           The loop continues until the user presses the ESC key, at which point it performs cleanup by turning off all thrusters, logging session end and shutdown status, closing the GPIO chip if it was initialized, and returning from the program.

    Returns:
    - int : The exit status of the program, where 0 indicates successful execution and 1 indicates an error during initialization (e.g., log file access failure)
*/
int main() {
    // Initialize high-resolution timer
    programStartTime = chrono::high_resolution_clock::now();

    // Initialization and Setup ---------------------------------------------------------------------------------------------------------------------
    if (system(("mkdir -p " + logDirectoryPath).c_str()) == 0) {
        logActivity("STATUS-005", "Log Dir Created: Logs/ directory created successfully");
    } else {
        logActivity("ERROR-003", "Log Dir Creation Failed: mkdir returned non-zero exit code");
    }

    // GPIO Initialization and Log File Preparation -------------------------------------------------------------------------------------------------
    initGPIO();
    ofstream keybindLogFile(logDirectoryPath + "Keybind_Log.csv", ios::trunc);
    ofstream activityLogFile(logDirectoryPath + "Activity_Log.csv", ios::trunc);
    if (keybindLogFile.is_open() && activityLogFile.is_open()) {
        logActivity("STATUS-008", "Log Files Opened: Both CSV files opened with headers written");
        keybindLogFile << "Time(s),Type,Direction,Key" << endl;
        activityLogFile << "Time(s),Code,Description" << endl;
        keybindLogFile.close(); activityLogFile.close();
        logActivity("STATUS-009", "Startup Complete: All files ready for logging");
    } else {
        logActivity("ERROR-000", "Log Open Failed: Log files could not be opened; exit code 1");
        return 1; 
    }

    // Main Program Loop ----------------------------------------------------------------------------------------------------------------------------
    logActivity("STATUS-001", "Session Started");
    displayMenu();
    
    while (true) {
        // Keyboard Input Handling ------------------------------------------------------------------------------------------------------------------
        if (checkKeyboardInput()) {
            unsigned char charInput = getchar();
            // Reset no-input counter on any input received
            noInputCount = 0;
            // Handle ESC key for quitting the program ----------------------------------------------------------------------------------------------
            if (charInput == 27) break; // ESC

            // Handle input based on current program mode -------------------------------------------------------------------------------------------
            if (programMode == 0) {
                // Menu Mode: Only accepts mode selection inputs ------------------------------------------------------------------------------------
                if (charInput == '1') {
                    programMode = 1;
                    programStartTime = chrono::high_resolution_clock::now();
                    logActivity("STATUS-012", "Startup Initiated: GPIO validation starting");
                    
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
                        logActivity("STATUS-014", "Rack Test Started: Rack connector " + to_string(rackNum + 1) + " test beginning");
                        
                        // Phase 1: Individual 1 second ON/OFF pulses for each GPIO
                        for (int g = 0; g < 4; g++) {
                            if (checkKeyboardInput() && getchar() == 27) { force_exit = true; break; } // ESC check
                            
                            int gpioPin = connectors[rackNum][g];
                            double phaseTime = g * 2.0;
                            
                            // GPIO ON for 1 second (held open for full duration)
                            cout << fixed << setprecision(2) << phaseTime << " GPIO " << gpioPin << " On" << endl;
                            struct gpiod_line_request* holdRequest = requestGPIOPin(gpioPin, 1);
                            if (!holdRequest) {
                                failedGPIOPins.push_back(gpioPin);
                            }
                            logActivity("STATUS-016", "GPIO ON: GPIO pin " + to_string(gpioPin) + " activated");
                            this_thread::sleep_for(chrono::milliseconds(1000));

                            // GPIO OFF for 1 second (release the held request)
                            phaseTime += 1.0;
                            cout << fixed << setprecision(2) << phaseTime << " GPIO " << gpioPin << " Off" << endl;
                            releaseGPIORequest(holdRequest);
                            logActivity("STATUS-017", "GPIO OFF: GPIO pin " + to_string(gpioPin) + " deactivated");
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
                                
                                // GPIO ON for 0.5 seconds (held open for full duration)
                                cout << fixed << setprecision(2) << phaseTime << " GPIO " << gpioPin << " On" << endl;
                                struct gpiod_line_request* holdRequest = requestGPIOPin(gpioPin, 1);
                                if (!holdRequest) {
                                    // Only add to failed list if not already there
                                    if (find(failedGPIOPins.begin(), failedGPIOPins.end(), gpioPin) == failedGPIOPins.end()) {
                                        failedGPIOPins.push_back(gpioPin);
                                    }
                                }
                                logActivity("STATUS-016", "GPIO ON: GPIO pin " + to_string(gpioPin) + " activated (pulse)");
                                this_thread::sleep_for(chrono::milliseconds(500));

                                // GPIO OFF for 0.5 seconds (release the held request)
                                phaseTime += 0.5;
                                cout << fixed << setprecision(2) << phaseTime << " GPIO " << gpioPin << " Off" << endl;
                                releaseGPIORequest(holdRequest);
                                logActivity("STATUS-017", "GPIO OFF: GPIO pin " + to_string(gpioPin) + " deactivated (pulse)");
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
                                logActivity("ERROR-006", "GPIO Activation Failed: Rack " + to_string(rackNum + 1) + " GPIO " + to_string(failedPin) + " failed");
                            }
                            cout << endl;
                        } else {
                            cout << "Rack Connector " << (rackNum + 1) << " Test Successfully Completed.\n" << endl;
                            logActivity("STATUS-015", "Rack Test Passed: Rack connector " + to_string(rackNum + 1) + " all phases completed");
                        }
                    }
                    
                    if (!force_exit) {
                        // Report final status
                        int successCount = 12 - failedGPIOPins.size();
                        if (failedGPIOPins.empty()) {
                            cout << "All 12/12 GPIO Successfully Activated." << endl;
                            logActivity("STATUS-013", "All GPIO Activated: All 12 pins activated without error");
                        } else {
                            cout << successCount << "/12 GPIO Successfully Activated." << endl;
                            for (int pin : failedGPIOPins) {
                                cout << "Rack Connector " << ((pin / 4) + 1) << " Test Failed: GPIO " << pin << " Connection issue" << endl;
                                logActivity("ERROR-007", "Failed Pin Summary: GPIO " + to_string(pin) + " in failedGPIOPins");
                            }
                            logActivity("STATUS-018", "Partial Activation: " + to_string(successCount) + "/12 GPIO activated, " + to_string(failedGPIOPins.size()) + " failures detected");
                        }
                    }
                    
                    programMode = 0;
                    programStartTime = chrono::high_resolution_clock::now();
                    displayMenu();
                // Operational Mode: Accepts mode selection inputs and transitions to Operational Mode -----------------------------------------------
                } else if (charInput == '2') {
                    programMode = 2;
                    programStartTime = chrono::high_resolution_clock::now();
                    logActivity("STATUS-020", "Operational Mode: User pressed '2'; polling loop ready");
                    displayMenu();
                }
            // Operational Mode: Accepts movement/rotation inputs -----------------------------------------------------------------------------------
            } else if (programMode == 2) {
                if (charInput == '0') { 
                    // Return to menu - deactivate all thrusters first
                    if (!currentKeyPressed.empty()) {
                        for(int i = 0; i <= 11; i++) (void)setGPIOPin(i, 0);
                        logKeyData('N', "--", "-", 'R');
                        logActivity("STATUS-106", "Thrusters Deactivated (Key Release): " + currentKeyPressed);
                        currentKeyPressed = "";
                    }
                    programMode = 0;
                    programStartTime = chrono::high_resolution_clock::now();
                    logActivity("STATUS-114", "Returned to Menu: GPIO deactivated, menu shown");
                    displayMenu();
                }
                else {
                    string keyIdString = string(1, toupper(charInput));
                    // Only process if it's a different key than currently tracked
                    if (keyIdString != currentKeyPressed) {
                        processMovementAction(keyIdString);
                        currentKeyPressed = keyIdString;
                    }
                }
            }
        // Handle thruster deactivation when no keys are pressed in Operational Mode ----------------------------------------------------------------
        } else {
            // In Operational Mode, if no keys are pressed, count consecutive no-input cycles.
            // Only deactivate thrusters after RELEASE_CYCLES consecutive empty polls to bridge
            // the gap between the first key character and when terminal key repeat kicks in.
            // Without this, holding a key causes: fire -> deactivate (during repeat delay gap) -> re-fire (on repeat).
            if (programMode == 2 && !currentKeyPressed.empty()) {
                noInputCount++;
                if (noInputCount >= RELEASE_CYCLES) {
                    logKeyData('N', "--", "-", 'R');
                    for(int i = 0; i <= 11; i++) (void)setGPIOPin(i, 0);
                    logActivity("STATUS-106", "Thrusters Deactivated (Key Release): " + currentKeyPressed);
                    currentKeyPressed = "";
                    noInputCount = 0;
                }
            }
        }

        // Small delay to prevent excessive CPU usage
        this_thread::sleep_for(chrono::milliseconds(50));
    }
    
    // Cleanup and Exit ---------------------------------------------------------------------------
    for(int i = 0; i <= 11; i++) (void)setGPIOPin(i, 0);
    logActivity("STATUS-107", "Thrusters Deactivated (Exit): ESC exit path; all 12 OFF");
    logActivity("STATUS-002", "Session Ended");
    logActivity("STATUS-003", "Shutdown Successful");
    if(gpioChip) gpiod_chip_close(gpioChip);
    return 0;
}