/*
    DTO Controller - Raspberry Pi 5 Optimized Port
    
    UI Flow:
    - 0 : Menu Mode
    - 1 : Startup Sequence Mode
    - 2 : Operational Mode (Starts in Continuous 'C')
    - Esc : Quit Program
    
    Mappings:
    - Translation: W/S (+/-X), A/D (-/+Y), Q/E (-/+Z)
    - Rotation:    I/K (+/-Pitch), J/L (-/+Yaw), U/O (-/+Roll)
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
char current_firing_mode = 'C'; 
int current_program_mode = 0; 
const string LOG_PATH = "./Logs/"; 
struct gpiod_chip* chip;
const char* chip_path = "/dev/gpiochip4"; 

// Forward Declarations
void logActivity(string code, string description);
void logData(char type, string direction, string keyname, char statusChar, char mode);
void displayMenu();
void processAction(string key_id, char mode);

// Helper for 0:10 Timestamp Format
string formatTime(double t) {
    int whole = (int)t;
    int decimal = (int)((t - whole) * 100 + 0.5);
    string dStr = (decimal < 10) ? "0" + to_string(decimal) : to_string(decimal);
    return to_string(whole) + ":" + dStr;
}

// GPIO Logic =====================================================================================

void initGPIO() {
    chip = gpiod_chip_open(chip_path);
    if (!chip) {
        chip = gpiod_chip_open("/dev/gpiochip0"); 
        if (!chip) return;
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

// Logging ========================================================================================

void logActivity(string code, string description) {
    string timeStr = formatTime(time_counter);
    ofstream activityFile(LOG_PATH + "Activity_Log.csv", ios_base::app);
    if (activityFile.is_open()) {
        activityFile << timeStr << "," << code << "," << description << endl;
        activityFile.close();
    }
    cout << timeStr << "," << code << "," << description << endl;
}

void logData(char type, string direction, string keyname, char statusChar, char mode) {
    string timeStr = formatTime(time_counter);
    ofstream outFile(LOG_PATH + "Keybind_Log.csv", ios_base::app);
    if (outFile.is_open()) {  
        outFile << timeStr << "," << mode << "," << type << "," << direction << "," << keyname << endl;
        outFile.close();
        if (statusChar == 'N' && keyname != "-") {
            logActivity("STATUS-301", "Key Registered: " + keyname);
        }
    }
}

// Mapping Logic ==================================================================================

void processAction(string key_id, char mode) {
    if (mode == 'P' && key_id == last_key_fired) {
        logActivity("ERROR-300", "Key cannot be operated because it is in pulse mode");
        logData('F', "--", key_id, 'E', 'P');
        return;
    }

    // Translation Mappings
    if (key_id == "W")      { logData('T', "+X", "W", 'N', mode); setGPIO(0, 1); setGPIO(8, 1); }
    else if (key_id == "S") { logData('T', "-X", "S", 'N', mode); setGPIO(5, 1); setGPIO(11, 1); }
    else if (key_id == "A") { logData('T', "-Y", "A", 'N', mode); setGPIO(2, 1); setGPIO(6, 1); }
    else if (key_id == "D") { logData('T', "+Y", "D", 'N', mode); setGPIO(1, 1); setGPIO(9, 1); }
    else if (key_id == "Q") { logData('T', "-Z", "Q", 'N', mode); setGPIO(3, 1); setGPIO(9, 1); }
    else if (key_id == "E") { logData('T', "+Z", "E", 'N', mode); setGPIO(4, 1); setGPIO(10, 1); }
    
    // Rotation Mappings
    else if (key_id == "I") { logData('R', "+P", "I", 'N', mode); setGPIO(9, 1); setGPIO(4, 1); }
    else if (key_id == "K") { logData('R', "-P", "K", 'N', mode); setGPIO(3, 1); setGPIO(10, 1); }
    else if (key_id == "J") { logData('R', "-Y", "J", 'N', mode); setGPIO(2, 1); setGPIO(9, 1); }
    else if (key_id == "L") { logData('R', "+Y", "L", 'N', mode); setGPIO(6, 1); setGPIO(1, 1); }
    else if (key_id == "U") { logData('R', "-R", "U", 'N', mode); setGPIO(2, 1); setGPIO(9, 1); }
    else if (key_id == "O") { logData('R', "+R", "O", 'N', mode); setGPIO(6, 1); setGPIO(1, 1); }
    
    else { 
        logData('F', "--", key_id, 'E', mode); 
        logActivity("ERROR-300", "Incorrect Keybind"); 
    }
    last_key_fired = key_id;
}

// UI =============================================================================================

void displayMenu() {
    if (current_program_mode == 0) {
        cout << "\n=================================================\nDTO Program\n-------------------------------------------------\nProgram Modes:\n- 0   : Menu Mode\n- 1   : Startup Sequence Mode\n- 2   : Operational Mode\n- Esc : Quit Mode\n=================================================\n>> " << flush;
    } else if (current_program_mode == 1) {
        cout << "\n=================================================\nStartup Sequence Mode\n-------------------------------------------------\nProgram Modes:\n- Esc : Quit Mode\n=================================================\n>> " << flush;
    } else if (current_program_mode == 2) {
        cout << "\n=================================================\nOperational Mode (2)\n-------------------------------------------------\nProgram Modes:\n- 1   : Continuous Mode\n- 2   : Pulse Mode\n- Esc : Quit Program\n=================================================\n>> " << flush;
    }
}

// Main ===========================================================================================

int main() {
    system(("mkdir -p " + LOG_PATH).c_str());
    initGPIO();
    ofstream r1(LOG_PATH + "Keybind_Log.csv", ios::trunc);
    ofstream r2(LOG_PATH + "Activity_Log.csv", ios::trunc);
    if (r1.is_open() && r2.is_open()) {
        logActivity("STATUS-101", "Path Validated: File Open Successful");
        r1 << "Time(s),Mode,Type,Direction,Key" << endl;
        r2 << "Time(s),Code,Description" << endl;
        r1.close(); r2.close();
        logActivity("STATUS-000", "Startup Successful: Files Ready");
    } else { return 1; }

    logActivity("STATUS-001", "Session Started");
    displayMenu();

    while (true) {
        if (kbhit()) {
            unsigned char ch = getchar();
            if (ch == 27) break; // ESC

            if (current_program_mode == 0) {
                if (ch == '1') {
                    current_program_mode = 1; displayMenu();
                    logActivity("STATUS-302", "Sequence Initiated");
                    int connectors[3][4] = {{0,3,4,1}, {8,9,10,9}, {2,6,5,11}};
                    for (int r = 0; r < 3; r++) {
                        logActivity("STATUS-303", "Testing Rack Connector " + to_string(r+1));
                        for (int g = 0; g < 4; g++) {
                            setGPIO(connectors[r][g], 1);
                            logActivity("STATUS-304", "GPIO " + to_string(connectors[r][g]) + " ON");
                            this_thread::sleep_for(chrono::milliseconds(250));
                            setGPIO(connectors[r][g], 0);
                            logActivity("STATUS-304", "GPIO " + to_string(connectors[r][g]) + " OFF");
                            this_thread::sleep_for(chrono::milliseconds(250));
                        }
                    }
                    logActivity("STATUS-305", "Sequence Complete");
                    current_program_mode = 0; displayMenu();
                } else if (ch == '2') {
                    current_program_mode = 2;
                    current_firing_mode = 'C'; // Automatically start in Continuous Mode
                    logActivity("STATUS-300", "Mode Changed: Operational Mode Active");
                    displayMenu();
                }
            } else if (current_program_mode == 2) {
                if (ch == '1') { current_firing_mode = 'C'; logActivity("STATUS-300", "Mode Changed: Continuous Mode"); }
                else if (ch == '2') { current_firing_mode = 'P'; logActivity("STATUS-300", "Mode Changed: Pulse Mode"); }
                else if (ch == '0') { current_program_mode = 0; displayMenu(); }
                else {
                    string key_id = string(1, toupper(ch));
                    processAction(key_id, current_firing_mode);
                }
            }
        } else {
            if (current_program_mode == 2) {
                logData('F', "--", "-", 'N', current_firing_mode);
                last_key_fired = "";
            }
        }
        if (current_program_mode == 2) time_counter += 0.1;
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    logActivity("STATUS-002", "Session Ended");
    logActivity("STATUS-003", "Shutdown Successful");
    if(chip) gpiod_chip_close(chip);
    return 0;
}