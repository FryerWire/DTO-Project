/*
    DTO Controller - Raspberry Pi 5 Port

    Features:
    - Logs key events with timestamps, types, directions, and modes to Keybind_Log.csv.
    - Supports Continuous (Caps Lock OFF) and Pulse (Caps Lock ON) modes.
    - Maps specific keys to translation and rotation movements.
    - Activity_Log.csv tracks high-level Application Codes (STATUS and ERROR).
    - Toggle Modes: ~ + S (Startup Sequence) | ~ + O (Operational Mode).

    Code:
    ERROR-00: Startup Failure - Cannot open/create log files.
    ERROR-01: GPIO Chip Failure - Could not open /dev/gpiochip0.
    ERROR-02: Write Failure - Keybind CSV file is locked or inaccessible.
    ERROR-03: Incorrect Keybind - Key pressed is not mapped to an action.
    ERROR-06: Mode Switch Denied - Attempted Startup (~S) while in Operational Mode.

    STATUS-00: Startup Successful - Directories and file headers initialized.
    STATUS-01: Session Ended - User exited the application loop.
    STATUS-02: Session Started - User loop is active.
    STATUS-03: Key Registered - Valid movement key processed and logged.
    STATUS-04: Shutdown Successful - Cleanup complete, application closing.
    STATUS-05: Operational Mode Active - System switched to Mode ~O.
    STATUS-10: Sequence Initiated - Startup Sequence (~S) started.
    STATUS-11: Testing Rack [N] - Diagnostic loop entering a specific rack.
    STATUS-13: GPIO [N] ON/OFF - Specific pin state change during diagnostic.
    STATUS-14: Sequence Complete - Diagnostic loop finished successfully.

    ACRONYM DEFINITIONS:
    - GPIO: General Purpose Input/Output
    - SDA:  Serial Data (I2C)
    - SCL:  Serial Clock (I2C)
    - TXD:  Transmit Data (UART)
    - RXD:  Receive Data (UART)
    - PCM:  Pulse Code Modulation (Audio)
    - MOSI: Master Out Slave In (SPI)
    - SCLK: Serial Clock (SPI)
    - CE:    Chip Enable (SPI)
    - PWM:  Pulse Width Modulation
    - ID_SD/SC: Identification System Data/Clock (EEPROM)

    GPIO Pin Layout:
    - Pin 3  : GPIO 2 (SDA)       : P1
    - Pin 5  : GPIO 3 (SCL)       : T1
    - Pin 7  : GPIO 4 (GPCLK0)    : B1
    - Pin 8  : GPIO 14 (TXD)      :
    - Pin 10 : GPIO 15 (RXD)      :
    - Pin 11 : GPIO 17            : 
    - Pin 12 : GPIO 18 (PCM_CLK)  : 
    - Pin 13 : GPIO 27            :
    - Pin 15 : GPIO 22            :
    - Pin 16 : GPIO 23            :
    - Pin 18 : GPIO 24            :
    - Pin 19 : GPIO 10 (MOSI)     : B2
    - Pin 21 : GPIO 9 (MISO)      : T2
    - Pin 22 : GPIO 25            :
    - Pin 23 : GPIO 11 (SCLK)     : F2
    - Pin 24 : GPIO 8 (CE0)       : A2
    - Pin 26 : GPIO 7 (CE1)       : P2
    - Pin 27 : GPIO 0 (ID_SD)     : A1
    - Pin 28 : GPIO 1 (ID_SC)     : S1
    - Pin 29 : GPIO 5             : F1
    - Pin 31 : GPIO 6             : P2
    - Pin 32 : GPIO 12 (PWM0)     :
    - Pin 33 : GPIO 13 (PWM1)     :
    - Pin 35 : GPIO 19 (PCM_FS)   :
    - Pin 36 : GPIO 16            :
    - Pin 37 : GPIO 26            :
    - Pin 38 : GPIO 20 (PCM_DIN)  :
    - Pin 40 : GPIO 21 (PCM_DOUT) :

    Translation Thrusters:
    - FWD (+X)   : A1, A2
    - Back (-X)  : F1, F2
    - Left (+Y)  : S1, S2
    - Right (-Y) : P1, P2
    - Up (+Z)    : B1, B2
    - Down (-Z)  : T1, T2

    Rotation Thruster:
    - CW P  : T2, B1
    - CCW P : T1, B2
    - CCW R : P1, S2
    - CW R  : P2, S1
    - CW Y  : A1, F2
    - CCW Y : A2, F1
*/



#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <string>
#include <iomanip>
#include <vector>
#include <map>

// Linux/RPi5 Specific Headers
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <gpiod.h> 



using namespace std;



// Global Variables ===============================================================================
double time_counter = 0.0;
string last_key_fired = ""; 
const string LOG_PATH = "./Logs/";  // Path adjusted for Linux environment 
bool isStartupMode = false;    
bool isOperationalMode = false; 
const char* chip_path = "/dev/gpiochip4";  // RPi 5 RP1 controller is usually chip 4
struct gpiod_chip* chip;



/*
    initGPIO() - Initializes GPIO chip for RPi 5.
*/
void initGPIO() {
    cout << ">> Initializing GPIO Chip..." << endl;
    chip = gpiod_chip_open(chip_path);
    if (!chip) {
        cout << ">> Chip 4 failed, attempting Chip 0 fallback..." << endl;
        chip = gpiod_chip_open("/dev/gpiochip0");
        if (!chip) {
            cerr << "ERROR-01: Could not open any GPIO chip!" << endl;
            return;
        }
    }
    cout << ">> GPIO System Online." << endl;
}



/*
    setGPIO() - Sets a specific GPIO pin to high (1) or low (0) using libgpiod v2.

    Parameters:
    - pin (int)   : The GPIO pin number to control.
    - value (int) : 1 to set the pin high, 0 to set it low.
*/
void setGPIO(int pin, int value) {
    if (!chip) return;

    struct gpiod_line_settings* settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, value ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);

    struct gpiod_line_config* line_cfg = gpiod_line_config_new();
    unsigned int offset = (unsigned int)pin;
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);

    struct gpiod_request_config* req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "DTO_Controller");

    struct gpiod_line_request* request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    // Cleanup resources
    if (request) gpiod_line_request_release(request);
    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);
}



/*
    kbhit() - Linux implementation of Windows _kbhit() using termios.
*/
int kbhit() {
    static const int STDIN = 0;
    static bool initialized = false;
    if (!initialized) {
        termios term;                      // Disable canonical mode and echo for non-blocking input
        tcgetattr(STDIN, &term);           // Get current terminal attributes
        term.c_lflag &= ~ICANON;           // Disable canonical mode
        tcsetattr(STDIN, TCSANOW, &term);  // Apply new attributes immediately
        setbuf(stdin, NULL);               // Disable buffering for stdin
        initialized = true;                // Mark initialization complete
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);

    return bytesWaiting;
}



/*
    getch_linux() - Linux implementation of _getch().
*/
char getch_linux() {
    char buf = 0;                                                                        // Buffer to hold the read character
    struct termios old = {0};                                                            // Structure to hold old terminal settings
    if (tcgetattr(0, &old) < 0) perror("tcsetattr()");                                   // Get current terminal attributes and check for errors
    old.c_lflag &= ~ICANON;                                                              // Disable canonical mode to allow reading input without waiting for a newline
    old.c_lflag &= ~ECHO;                                                                // Disable echo to prevent the character from being displayed on the terminal
    old.c_cc[VMIN] = 1;                                                                  // Set minimum number of characters to read
    old.c_cc[VTIME] = 0;                                                                 // Set timeout to 0 (no timeout)
    if (tcsetattr(0, TCSANOW, &old) < 0) perror("tcsetattr ICANON");                     // Apply new terminal settings immediately and check for errors
    if (read(0, &buf, 1) < 0) perror("read()");                                          // Read a single character from stdin and check for errors
    old.c_lflag |= ICANON;                                                               // Restore canonical mode
    old.c_lflag |= ECHO;                                                                 // Restore echo
    if (tcsetattr(0, TCSADRAIN, &old) < 0) perror("tcsetattr ~ICANON");                  // Restore old terminal settings after reading input and check for errors
    
    return buf;
}



/*
    isCapsLockOn() - Checks keyboard LED state on Linux.
*/
bool isCapsLockOn() {
    int fd = open("/dev/console", O_RDONLY);  // Open the console device to read keyboard LED states; returns a file descriptor or -1 on error
    if (fd < 0) return false;
    long state;                               // Variable to hold the LED state
    ioctl(fd, KDGKBLED, &state);              // Get the current state of the keyboard LEDs and store it in the 'state' variable
    close(fd);

    return (state & LED_CAP);
}



/*
    getVirtualKeyName() - Converts virtual key codes to human-readable names for logging.

    Parameters:
    - vkCode (int) : The virtual key code of the pressed key.
*/
void logActivity(string code, string description) {
    ofstream activityFile(LOG_PATH + "Activity_Log.csv", ios_base::app);
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
    cout << ">> " << errorCode << ": " << title << endl;
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
    ofstream outFile(LOG_PATH + "Keybind_Log.csv", ios_base::app);
    if (outFile.is_open()) {  
        outFile << fixed << setprecision(2)
                << time_counter << "," << mode << "," << type << "," << direction << "," << keyname << endl;
        outFile.close();
        
        if (statusChar == 'N' && keyname != "-") {
            logActivity("STATUS-03", "Key Registered: " + keyname);
            cout << ">> Key Pressed: " << keyname << " (" << direction << ") | Mode: " << mode << endl;
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
void processAction(char key, char mode) {
    switch (toupper(key)) {  
        // Translation Mappings
        case 'W': 
            logData('T', "+X", "W", 'N', mode); 
            setGPIO(0, 1); setGPIO(8, 1); // A1, A2
            break;
        case 'S': 
            logData('T', "-X", "S", 'N', mode); 
            setGPIO(5, 1); setGPIO(11, 1); // F1, F2
            break;
        case 'A': 
            logData('T', "-Y", "A", 'N', mode); 
            setGPIO(2, 1); setGPIO(6, 1); // P1, P2
            break;
        case 'D': 
            logData('T', "+Y", "D", 'N', mode); 
            setGPIO(1, 1); setGPIO(9, 1); // S1, S2
            break;
        case ' ': 
            logData('T', "+Z", "Space", 'N', mode); 
            setGPIO(4, 1); setGPIO(10, 1); // B1, B2
            break;
        case 'X': // Assigned X to Shift-equivalent for Down (-Z)
            logData('T', "-Z", "X", 'N', mode); 
            setGPIO(3, 1); setGPIO(9, 1); // T1, T2
            break;

        // Rotation Mappings
        case 'I': // Pitch Up (CW P)
            logData('R', "CW P", "I", 'N', mode);
            setGPIO(9, 1); setGPIO(4, 1); // T2, B1
            break;
        case 'K': // Pitch Down (CCW P)
            logData('R', "CCW P", "K", 'N', mode);
            setGPIO(3, 1); setGPIO(10, 1); // T1, B2
            break;
        case 'J': // Roll Left (CCW R)
            logData('R', "CCW R", "J", 'N', mode);
            setGPIO(2, 1); setGPIO(9, 1); // P1, S2
            break;
        case 'L': // Roll Right (CW R)
            logData('R', "CW R", "L", 'N', mode);
            setGPIO(6, 1); setGPIO(1, 1); // P2, S1
            break;
        case 'U': // Yaw Left (CW Y)
            logData('R', "CW Y", "U", 'N', mode);
            setGPIO(0, 1); setGPIO(11, 1); // A1, F2
            break;
        case 'O': // Yaw Right (CCW Y)
            logData('R', "CCW Y", "O", 'N', mode);
            setGPIO(8, 1); setGPIO(5, 1); // A2, F1
            break;

        default:
             logData('F', "--", string(1, key), 'E', mode); logError("ERROR-03", "Incorrect Keybind"); break;
    }
}



/*
    main() - The entry point of the program.
*/
int main() {
    cout << "========================================" << endl;
    cout << "     DTO CONTROLLER - RPi5 EDITION      " << endl;
    cout << "========================================" << endl;

    initGPIO();
    system(("mkdir -p " + LOG_PATH).c_str());

    ofstream resetFile(LOG_PATH + "Keybind_Log.csv", ios::trunc);
    ofstream resetActivity(LOG_PATH + "Activity_Log.csv", ios::trunc);

    // Check if files opened successfully before writing headers ----------------------------------
    if (!resetFile.is_open() || !resetActivity.is_open()) {
        cerr << "ERROR-00: Startup Failure. Path: " << LOG_PATH << endl;
        return 1;
    }

    resetFile << "Time(s),Mode,Type,Direction,Key" << endl;
    resetActivity << "Time(s),Code,Description" << endl;
    resetFile.close();
    resetActivity.close();

    logActivity("STATUS-00", "Startup Successful: Files Ready");
    logActivity("STATUS-02", "Session Started");

    cout << ">> System Ready. Press '~' + 'O' to begin." << endl;

    // Welcome Message ----------------------------------------------------------------------------
    while (true) {
        // Exit check -----------------------------------------------------------------------------
        if (kbhit()) {
            char ch = getch_linux();
            // Exit check for ESC key -------------------------------------------------------------
            if (ch == 27) break; // ESC

            // Check for Mode Changes (~ + S or ~ + O) --------------------------------------------
            if (ch == '~') {
                char next = getch_linux();
                // Handle Mode Toggles with Constraints --------------------------------------------
                if (toupper(next) == 'S') {
                    // Constraint: Cannot enter ~S if already in ~O. Must Esc and restart. --------
                    if (isOperationalMode) {
                        logError("ERROR-06", "Mode Switch Denied");
                    } else {
                        isStartupMode = true;
                        cout << ">> STARTUP SEQUENCE INITIATED..." << endl;
                        logActivity("STATUS-10", "Sequence Initiated");
                        
                        // Startup Sequence logic based on your Pin Layout mapping ----------------
                        // Rack 1: A1, T1, B1, S1 | Rack 2: A2, T2, B2, S2 | Rack 3: P1, P2, F1, F2
                        int connectors[3][4] = {{0,3,4,1}, {8,9,10,9}, {2,6,5,11}};

                        for (int r = 0; r < 3; r++) {
                            logActivity("STATUS-11", "Testing Rack " + to_string(r+1));
                            cout << "   Testing Rack " << (r+1) << "..." << endl;
                            // Phase 1: 1.0s Pulses with Real-Time Delay --------------------------
                            for (int g = 0; g < 4; g++) {
                                setGPIO(connectors[r][g], 1);
                                logActivity("STATUS-13", "GPIO " + to_string(connectors[r][g]) + " ON");
                                this_thread::sleep_for(chrono::milliseconds(1000));
                                setGPIO(connectors[r][g], 0);
                                logActivity("STATUS-13", "GPIO " + to_string(connectors[r][g]) + " OFF");
                                this_thread::sleep_for(chrono::milliseconds(1000));
                            }
                        }
                        logActivity("STATUS-14", "Sequence Complete");
                        cout << ">> STARTUP SEQUENCE COMPLETE." << endl;
                        isStartupMode = false;
                    }
                } else if (toupper(next) == 'O') {
                    isOperationalMode = true;
                    cout << ">> OPERATIONAL MODE ACTIVE. LOGGING ENABLED." << endl;
                    logActivity("STATUS-05", "Operational Mode Active");
                }
            } else if (isOperationalMode) {
                char mode = isCapsLockOn() ? 'P' : 'C';
                processAction(ch, mode);
            }
        }
        
        this_thread::sleep_for(chrono::milliseconds(100));

        // Only increment time in Operational Mode ------------------------------------------------
        if (isOperationalMode) time_counter += 0.1;
    }

    logActivity("STATUS-01", "Session Ended");
    logActivity("STATUS-04", "Shutdown Successful");
    cout << ">> Shutdown Complete. Goodbye." << endl;
    
    // Final exit sequence ------------------------------------------------------------------------
    if(chip) gpiod_chip_close(chip);
    return 0;
}

// Compilation Instructions =======================================================================
// g++ DTOController.cpp -lgpiod -o DTOController