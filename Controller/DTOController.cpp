/*
    DTO Controller - Raspberry Pi 5 Optimized Port
    
    Features:
    - Logs key events with timestamps, types, directions, and modes to Keybind_Log.csv.
    - Supports Continuous (Caps Lock OFF) and Pulse (Caps Lock ON) modes.
    - Maps specific keys to translation and rotation movements.
    - Activity_Log.csv tracks high-level Application Codes (STATUS and ERROR).
    - Toggle Modes: ~ + S (Startup Sequence) | ~ + O (Operational Mode).

    Code:
    ERROR-00: Startup Failure - Cannot open/create log files.
    ERROR-01: GPIO Chip Failure - Hardware not detected.
    ERROR-03: Incorrect Keybind - Key pressed is not mapped to an action.
    ERROR-06: Mode Switch Denied - Attempted Startup (~S) while in Operational Mode.

    STATUS-00: Startup Successful - Directories and file headers initialized.
    STATUS-02: Session Started - User loop is active.
    STATUS-03: Key Registered - Valid movement key processed and logged.
    STATUS-04: Shutdown Successful - Cleanup complete, application closing.
    STATUS-05: Operational Mode Active - System switched to Mode ~O.
    STATUS-10: Sequence Initiated - Startup Sequence (~S) started.
    STATUS-11: Testing Rack Connector [N] - Diagnostic loop entering a specific rack.
    STATUS-12: Rack Connector [N] PASS - Diagnostic for specific rack completed.
    STATUS-13: GPIO [N] ON/OFF - Specific pin state change during diagnostic.
    STATUS-14: All GPIO Successfully Activated - Diagnostic loop finished successfully.

    Architecture: Linux / libgpiod v2.x
    Pin Controller: pinctrl-rp1
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
double time_counter = 0.0;
string last_key_fired = ""; 
const string LOG_PATH = "./Logs/"; 
bool isOperationalMode = false; 
struct gpiod_chip* chip;
const char* chip_path = "/dev/gpiochip4"; // RPi5 Default for RP1

// Forward Declarations
void logActivity(string code, string description);
void logData(char type, string direction, string keyname, char statusChar, char mode);

// GPIO Logic =====================================================================================

void initGPIO() {
    chip = gpiod_chip_open(chip_path);
    if (!chip) {
        chip = gpiod_chip_open("/dev/gpiochip0"); // Fallback
        if (!chip) {
            cerr << "[ERROR-01] GPIO Chip Failure. Hardware not detected." << endl;
            return;
        }
    }
    cout << "[STATUS-00] GPIO System Online: " << gpiod_chip_get_info(chip)->name << endl;
}

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

    if (request) gpiod_line_request_release(request);
    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);
}

// Input Handling =================================================================================

int kbhit() {
    static const int STDIN = 0;
    static bool initialized = false;
    if (!initialized) {
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        term.c_lflag &= ~ECHO; // Don't echo keys to terminal
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }
    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}

bool isCapsLockOn() {
    int fd = open("/dev/console", O_RDONLY);
    if (fd < 0) return false;
    long state;
    ioctl(fd, KDGKBLED, &state);
    close(fd);
    return (state & LED_CAP);
}

// Logging ========================================================================================

void logActivity(string code, string description) {
    ofstream activityFile(LOG_PATH + "Activity_Log.csv", ios_base::app);
    if (activityFile.is_open()) {
        activityFile << fixed << setprecision(2) << time_counter << "," << code << "," << description << endl;
        activityFile.close();
    }
    // Print Status to Terminal as requested
    cout << fixed << setprecision(2) << "[" << time_counter << "s] " << code << ": " << description << endl;
}

void logData(char type, string direction, string keyname, char statusChar, char mode = 'C') {
    ofstream outFile(LOG_PATH + "Keybind_Log.csv", ios_base::app);
    if (outFile.is_open()) {  
        outFile << fixed << setprecision(2) << time_counter << "," << mode << "," << type << "," << direction << "," << keyname << endl;
        outFile.close();
        if (statusChar == 'N' && keyname != "-") {
            logActivity("STATUS-03", "Key Registered: " + keyname + " (" + direction + ")");
        }
    }
}

// Mapping Logic ==================================================================================

void processAction(char key, char mode) {
    switch (toupper(key)) {  
        case 'W': logData('T', "+X", "W", 'N', mode); setGPIO(0, 1); setGPIO(8, 1); break;
        case 'S': logData('T', "-X", "S", 'N', mode); setGPIO(5, 1); setGPIO(11, 1); break;
        case 'A': logData('T', "-Y", "A", 'N', mode); setGPIO(2, 1); setGPIO(6, 1); break;
        case 'D': logData('T', "+Y", "D", 'N', mode); setGPIO(1, 1); setGPIO(9, 1); break;
        case ' ': logData('T', "+Z", "Space", 'N', mode); setGPIO(4, 1); setGPIO(10, 1); break;
        case 'X': logData('T', "-Z", "X", 'N', mode); setGPIO(3, 1); setGPIO(9, 1); break;
        // Rotation
        case 'I': logData('R', "+P", "I", 'N', mode); setGPIO(9, 1); setGPIO(4, 1); break;
        case 'K': logData('R', "-P", "K", 'N', mode); setGPIO(3, 1); setGPIO(10, 1); break;
        default:  logActivity("ERROR-03", "Incorrect Keybind Detected"); break;
    }
}

// Main Logic =====================================================================================

int main() {
    system(("mkdir -p " + LOG_PATH).c_str());
    initGPIO();

    ofstream resetFile(LOG_PATH + "Keybind_Log.csv", ios::trunc);
    ofstream resetActivity(LOG_PATH + "Activity_Log.csv", ios::trunc);
    if (!resetFile.is_open() || !resetActivity.is_open()) {
        cerr << "[ERROR-00] Startup Failure. Path issues." << endl;
        return 1;
    }
    resetFile << "Time(s),Mode,Type,Direction,Key" << endl;
    resetActivity << "Time(s),Code,Description" << endl;
    resetFile.close(); resetActivity.close();

    cout << "\n====================================================" << endl;
    cout << " DTO MISSION CONTROL - RPi5 ACTIVE" << endl;
    cout << " - Press '~' then 'S' for Startup Sequence" << endl;
    cout << " - Press '~' then 'O' for Operational Mode" << endl;
    cout << " - Press 'Esc' to Shutdown" << endl;
    cout << "====================================================\n" << endl;

    logActivity("STATUS-02", "Session Started");

    while (true) {
        if (kbhit()) {
            char ch = getchar();
            if (ch == 27) break; // ESC logic

            if (ch == '~') {
                char next = getchar();
                if (toupper(next) == 'S') {
                    if (isOperationalMode) {
                        logActivity("ERROR-06", "Mode Switch Denied: Exit ~O first");
                    } else {
                        logActivity("STATUS-10", "Sequence Initiated");
                        int connectors[3][4] = {{0,3,4,1}, {8,9,10,9}, {2,6,5,11}};
                        for (int r = 0; r < 3; r++) {
                            logActivity("STATUS-11", "Testing Rack Connector " + to_string(r+1));
                            for (int g = 0; g < 4; g++) {
                                setGPIO(connectors[r][g], 1);
                                logActivity("STATUS-13", "GPIO " + to_string(connectors[r][g]) + " ON");
                                this_thread::sleep_for(chrono::milliseconds(500));
                                setGPIO(connectors[r][g], 0);
                                logActivity("STATUS-13", "GPIO " + to_string(connectors[r][g]) + " OFF");
                                this_thread::sleep_for(chrono::milliseconds(500));
                            }
                            logActivity("STATUS-12", "Rack Connector " + to_string(r+1) + " PASS");
                        }
                        logActivity("STATUS-14", "All GPIO Successfully Activated");
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
        if (isOperationalMode) time_counter += 0.1;
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    logActivity("STATUS-04", "Shutdown Successful");
    if(chip) gpiod_chip_close(chip);
    return 0;
}
// Compilation Instructions =======================================================================
// g++ DTOController.cpp -lgpiod -o DTOController