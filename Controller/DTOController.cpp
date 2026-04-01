/*
    DTO Controller - Raspberry Pi 5 Optimized Port
    
    Features:
    - Logs key events with timestamps, types, directions, and modes to Keybind_Log.csv.
    - Supports Continuous (Caps Lock OFF) and Pulse (Caps Lock ON) modes.
    - Maps specific keys to translation and rotation movements.
    - Activity_Log.csv tracks high-level Application Codes (STATUS and ERROR).
    - Toggle Modes: ~ + S (Startup Sequence) | ~ + O (Operational Mode).

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

    struct gpiod_chip_info* info = gpiod_chip_get_info(chip);
    if (info) {
        const char* name = gpiod_chip_info_get_name(info);
        cout << "[STATUS-00] GPIO System Online: " << (name ? name : "Unknown Chip") << endl;
        gpiod_chip_info_free(info);
    }
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
        term.c_lflag &= ~ECHO; 
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

void processAction(string key_id, char mode) {
    // Translation
    if (key_id == "W") { logData('T', "+X", "W", 'N', mode); setGPIO(0, 1); setGPIO(8, 1); }
    else if (key_id == "S") { logData('T', "-X", "S", 'N', mode); setGPIO(5, 1); setGPIO(11, 1); }
    else if (key_id == "A") { logData('T', "-Y", "A", 'N', mode); setGPIO(2, 1); setGPIO(6, 1); }
    else if (key_id == "D") { logData('T', "+Y", "D", 'N', mode); setGPIO(1, 1); setGPIO(9, 1); }
    else if (key_id == "SPACE") { logData('T', "+Z", "Space", 'N', mode); setGPIO(4, 1); setGPIO(10, 1); }
    else if (key_id == "X") { logData('T', "-Z", "X", 'N', mode); setGPIO(3, 1); setGPIO(9, 1); }
    
    // Rotation (Mapped to Arrows)
    else if (key_id == "UP") { logData('R', "+P", "UpArrow", 'N', mode); setGPIO(9, 1); setGPIO(4, 1); }
    else if (key_id == "DOWN") { logData('R', "-P", "DownArrow", 'N', mode); setGPIO(3, 1); setGPIO(10, 1); }
    else if (key_id == "LEFT") { logData('R', "-Y", "LeftArrow", 'N', mode); setGPIO(2, 1); setGPIO(9, 1); }
    else if (key_id == "RIGHT") { logData('R', "+Y", "RightArrow", 'N', mode); setGPIO(6, 1); setGPIO(1, 1); }
    
    // Rotation (Mapped to Ctrl+Arrows/Rotation Keys)
    else if (key_id == "I") { logData('R', "+P", "I", 'N', mode); setGPIO(9, 1); setGPIO(4, 1); }
    else if (key_id == "K") { logData('R', "-P", "K", 'N', mode); setGPIO(3, 1); setGPIO(10, 1); }
    else { logActivity("ERROR-03", "Incorrect Keybind Detected: " + key_id); }
}

// Main Logic =====================================================================================

int main() {
    system(("mkdir -p " + LOG_PATH).c_str());
    initGPIO();

    ofstream resetFile(LOG_PATH + "Keybind_Log.csv", ios::trunc);
    ofstream resetActivity(LOG_PATH + "Activity_Log.csv", ios::trunc);
    if (!resetFile.is_open() || !resetActivity.is_open()) {
        cerr << "[ERROR-00] Startup Failure." << endl;
        return 1;
    }
    resetFile << "Time(s),Mode,Type,Direction,Key" << endl;
    resetActivity << "Time(s),Code,Description" << endl;
    resetFile.close(); resetActivity.close();

    cout << "\n====================================================" << endl;
    cout << " DTO MISSION CONTROL - RPi5 ACTIVE" << endl;
    cout << " - W,A,S,D : Translation | SPACE/X : Up/Down" << endl;
    cout << " - ARROWS  : Rotation (Pitch/Yaw)" << endl;
    cout << " - ESC     : Shutdown" << endl;
    cout << "====================================================\n" << endl;

    logActivity("STATUS-02", "Session Started");

    while (true) {
        if (kbhit()) {
            string key_pressed = "";
            char ch = getchar();

            if (ch == 27) { // ESC or Escape Sequence
                if (kbhit()) { // It's an escape sequence (Arrow keys)
                    getchar(); // skip '['
                    char sub = getchar();
                    if (sub == 'A') key_pressed = "UP";
                    else if (sub == 'B') key_pressed = "DOWN";
                    else if (sub == 'C') key_pressed = "RIGHT";
                    else if (sub == 'D') key_pressed = "LEFT";
                } else {
                    break; // Actual ESC key
                }
            } else if (ch == '~') {
                char next = getchar();
                if (toupper(next) == 'S') {
                    logActivity("STATUS-10", "Sequence Initiated");
                    int connectors[3][4] = {{0,3,4,1}, {8,9,10,9}, {2,6,5,11}};
                    for (int r = 0; r < 3; r++) {
                        logActivity("STATUS-11", "Testing Rack Connector " + to_string(r+1));
                        for (int g = 0; g < 4; g++) {
                            setGPIO(connectors[r][g], 1);
                            this_thread::sleep_for(chrono::milliseconds(200));
                            setGPIO(connectors[r][g], 0);
                            this_thread::sleep_for(chrono::milliseconds(200));
                        }
                    }
                    logActivity("STATUS-14", "All GPIO Successfully Activated");
                } else if (toupper(next) == 'O') {
                    isOperationalMode = true;
                    logActivity("STATUS-05", "Operational Mode Active");
                }
            } else {
                key_pressed = string(1, toupper(ch));
                if (ch == ' ') key_pressed = "SPACE";
            }

            if (isOperationalMode && key_pressed != "") {
                char mode = isCapsLockOn() ? 'P' : 'C';
                processAction(key_pressed, mode);
            }
        }
        if (isOperationalMode) time_counter += 0.1;
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    logActivity("STATUS-04", "Shutdown Successful");
    if(chip) gpiod_chip_close(chip);
    return 0;
}