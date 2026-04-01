/*
    DTO Controller - Raspberry Pi 5 Optimized Port
    
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
bool isKeyHeld = false;
const string LOG_PATH = "./Logs/"; 
bool isOperationalMode = false; 
struct gpiod_chip* chip;
const char* chip_path = "/dev/gpiochip4"; 

// Forward Declarations
void logActivity(string code, string description);
void logData(char type, string direction, string keyname, char statusChar, char mode);

// GPIO Logic =====================================================================================

void initGPIO() {
    chip = gpiod_chip_open(chip_path);
    if (!chip) {
        chip = gpiod_chip_open("/dev/gpiochip0"); 
        if (!chip) {
            cerr << "[ERROR-000] GPIO Chip Failure." << endl;
            return;
        }
    }
    struct gpiod_chip_info* info = gpiod_chip_get_info(chip);
    if (info) {
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
    // Formatted Terminal Output
    cout << fixed << setprecision(2) << time_counter << "," << code << "," << description << endl;
}

void logData(char type, string direction, string keyname, char statusChar, char mode = 'C') {
    ofstream outFile(LOG_PATH + "Keybind_Log.csv", ios_base::app);
    if (outFile.is_open()) {  
        outFile << fixed << setprecision(2) << time_counter << "," << mode << "," << type << "," << direction << "," << keyname << endl;
        outFile.close();
        if (statusChar == 'N' && keyname != "-") {
            logActivity("STATUS-301", "Key Registered: " + keyname);
        }
    }
}

// Mapping Logic ==================================================================================

void processAction(string key_id, char mode) {
    if (mode == 'P') {
        if (key_id == last_key_fired) {
            logActivity("ERROR-300", "Hold Denied in Pulse Mode");
            logData('F', "--", key_id, 'E', 'P');
            return;
        }
    }

    if (key_id == "W") { logData('T', "+X", "W", 'N', mode); setGPIO(0, 1); setGPIO(8, 1); }
    else if (key_id == "S") { logData('T', "-X", "S", 'N', mode); setGPIO(5, 1); setGPIO(11, 1); }
    else if (key_id == "A") { logData('T', "-Y", "A", 'N', mode); setGPIO(2, 1); setGPIO(6, 1); }
    else if (key_id == "D") { logData('T', "+Y", "D", 'N', mode); setGPIO(1, 1); setGPIO(9, 1); }
    else if (key_id == "SPACE") { logData('T', "+Z", "Space", 'N', mode); setGPIO(4, 1); setGPIO(10, 1); }
    else if (key_id == "TAB") { logData('T', "-Z", "Tab", 'N', mode); setGPIO(3, 1); setGPIO(9, 1); }
    else if (key_id == "UP") { logData('R', "+P", "UpArrow", 'N', mode); setGPIO(9, 1); setGPIO(4, 1); }
    else if (key_id == "DOWN") { logData('R', "-P", "DownArrow", 'N', mode); setGPIO(3, 1); setGPIO(10, 1); }
    else if (key_id == "LEFT") { logData('R', "-Y", "LeftArrow", 'N', mode); setGPIO(2, 1); setGPIO(9, 1); }
    else if (key_id == "RIGHT") { logData('R', "+Y", "RightArrow", 'N', mode); setGPIO(6, 1); setGPIO(1, 1); }
    else if (key_id == "[") { logData('R', "-R", "RollLeft", 'N', mode); setGPIO(2, 1); setGPIO(9, 1); }
    else if (key_id == "]") { logData('R', "+R", "RollRight", 'N', mode); setGPIO(6, 1); setGPIO(1, 1); }
    else { 
        logData('F', "--", key_id, 'E', mode); 
        logActivity("ERROR-300", "Incorrect Keybind"); 
    }
    
    last_key_fired = key_id;
}

// Main Loop ======================================================================================

int main() {
    system(("mkdir -p " + LOG_PATH).c_str());
    initGPIO();
    ofstream r1(LOG_PATH + "Keybind_Log.csv", ios::trunc);
    ofstream r2(LOG_PATH + "Activity_Log.csv", ios::trunc);
    r1 << "Time(s),Mode,Type,Direction,Key" << endl;
    r2 << "Time(s),Code,Description" << endl;
    r1.close(); r2.close();

    logActivity("STATUS-000", "Startup Successful: Files Ready");
    logActivity("STATUS-001", "Session Started");

    while (true) {
        char mode = isCapsLockOn() ? 'P' : 'C';

        if (kbhit()) {
            string key_pressed = "";
            unsigned char ch = getchar();

            if (ch == 27) { // Escape Sequence for Arrows
                if (kbhit()) {
                    getchar(); // skip '['
                    unsigned char sub = getchar();
                    if (sub == 'A') key_pressed = "UP";
                    else if (sub == 'B') key_pressed = "DOWN";
                    else if (sub == 'C') key_pressed = "RIGHT";
                    else if (sub == 'D') key_pressed = "LEFT";
                } else break; // Actual ESC key
            } else if (ch == '~') {
                char next = getchar();
                if (toupper(next) == 'O') {
                    isOperationalMode = true;
                    logActivity("STATUS-300", "Mode Changed: Operational Mode");
                }
            } else if (ch == '\t') {
                key_pressed = "TAB";
            } else if (ch == ' ') {
                key_pressed = "SPACE";
            } else {
                key_pressed = string(1, toupper(ch));
            }

            if (isOperationalMode && key_pressed != "") {
                processAction(key_pressed, mode);
            }
        } else {
            if (isOperationalMode) {
                // Heartbeat logging for Free Fall state
                logData('F', "--", "-", 'N', mode);
                last_key_fired = ""; 
            }
        }

        if (isOperationalMode) time_counter += 0.1;
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    logActivity("STATUS-002", "Session Ended");
    logActivity("STATUS-003", "Shutdown Successful");
    if(chip) gpiod_chip_close(chip);
    return 0;
}