
/*
    DTO Controller - Raspberry Pi 5 Port

    Features:
    - Logs key events with timestamps, types, directions, and modes to Keybind_Log.csv.
    - Supports Continuous (Caps Lock OFF) and Pulse (Caps Lock ON) modes.
    - Maps specific keys to translation and rotation movements.
    - Activity_Log.csv tracks high-level Application Codes (STATUS and ERROR).
    - Toggle Modes: ~ + S (Startup Sequence) | ~ + O (Operational Mode).

    Note: GPIO control uses libgpiod as per RPi 5 architecture[cite: 758, 918].
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
const char* chip_path = "/dev/gpiochip0";  // GPIO Setup for RPi 5
struct gpiod_chip* chip;
struct gpiod_line_bulk lines;



/*
    initGPIO() - Initializes GPIO chip for RPi 5.
*/
void initGPIO() {
    chip = gpiod_chip_open(chip_path);
    if (!chip) {
        cerr << "ERROR: Could not open GPIO chip" << endl;
    }
}



/*
    setGPIO() - Sets a specific GPIO pin to high (1) or low (0).

    Parameters:
    - pin (int)   : The GPIO pin number to control.
    - value (int) : 1 to set the pin high, 0 to set it low.
*/
void setGPIO(int pin, int value) {
    struct gpiod_line* line = gpiod_chip_get_line(chip, pin);
    if (line) {
        gpiod_line_request_output(line, "DTO_Controller", value);
        gpiod_line_release(line);
    }
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
    char buf = 0;                                                        // Buffer to hold the read character
    struct termios old = {0};                                            // Structure to hold old terminal settings
    if (tcgetattr(0, &old) < 0) perror("tcsetattr()");                   // Get current terminal attributes and check for errors
    old.c_lflag &= ~ICANON;                                              // Disable canonical mode to allow reading input without waiting for a newline
    old.c_lflag &= ~ECHO;                                                // Disable echo to prevent the character from being displayed on the terminal
    old.c_cc[VMIN] = 1;                                                  // Set minimum number of characters to read
    old.c_cc[VTIME] = 0;                                                 // Set timeout to 0 (no timeout)
    if (tcsetattr(0, TCSANOW, &old) < 0) perror("tcsetattr ICANON");     // Apply new terminal settings immediately and check for errors
    if (read(0, &buf, 1) < 0) perror("read()");                          // Read a single character from stdin and check for errors
    old.c_lflag |= ICANON;                                               // Restore canonical mode
    old.c_lflag |= ECHO;                                                 // Restore echo
    if (tcsetattr(0, TCSADRAIN, &old) < 0) perror("tcsetattr ~ICANON");  // Restore old terminal settings after reading input and check for errors
    
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
        case 'W': logData('T', "+X", "W", 'N', mode); break;
        case 'S': logData('T', "-X", "S", 'N', mode); break;
        case 'D': logData('T', "+Y", "D", 'N', mode); break;
        case 'A': logData('T', "-Y", "A", 'N', mode); break;
        case ' ': logData('T', "+Z", "Space", 'N', mode); break;
        default:
             logData('F', "--", string(1, key), 'E', mode); logError("ERROR-03", "Incorrect Keybind"); break;
    }
}



/*
    main() - The entry point of the program.
*/
int main() {
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
                        logActivity("STATUS-10", "Sequence Initiated");
                        
                        // Startup Sequence logic -------------------------------------------------
                        int connectors[3][4] = {{0,1,2,3}, {4,5,6,7}, {8,11,12,13}};
                        // Phase 1: 1.0s Pulses with Real-Time Delay --------------------------------------
                        for (int r = 0; r < 3; r++) {
                            logActivity("STATUS-11", "Testing Rack " + to_string(r+1));
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
                        isStartupMode = false;
                    }
                } else if (toupper(next) == 'O') {
                    isOperationalMode = true;
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
    
    // Final exit sequence ------------------------------------------------------------------------
    if(chip) gpiod_chip_close(chip);
    return 0;
}

// Compilation Instructions =======================================================================
// g++ DTOController.cpp -lgpiod -o DTOController
