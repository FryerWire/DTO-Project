
/*
    12-Thruster Desktop Simulator & Logger

    Standard C++ (Windows) simulator for testing WASD thruster control logic.
    Monitors real-time keypresses (W, A, S, D) to simulate activating specific 
    groups of 3 thrusters per direction. 
    Logs the active thrusters every second to 'thruster_log.txt'.
    Press 'Q' to trigger a graceful shutdown and save the log file.
*/



#include <iostream>   // For standard console input/output (std::cout, std::cerr)
#include <fstream>    // For file stream operations to write the log file (std::ofstream)
#include <chrono>     // For high-resolution time tracking (std::chrono::steady_clock)
#include <thread>     // For sleeping the execution thread to reduce CPU usage (std::this_thread::sleep_for)
#include <windows.h>  // Required for GetAsyncKeyState to read real-time keyboard state non-blockingly



// Function Prototypes ============================================================================
bool setup_logger();
void cleanup_logger();
void log_current_state();
void check_state_changes();



// Global Variables ===============================================================================
bool keep_running = true;                          // Flag to ensure the main loop continues running until 'Q' is pressed
bool prev_w = false;                               // Track the previous state of the 'W' key to avoid spamming the console
bool prev_a = false;                               // Track the previous state of the 'A' key to avoid spamming the console
bool prev_s = false;                               // Track the previous state of the 'S' key to avoid spamming the console
bool prev_d = false;                               // Track the previous state of the 'D' key to avoid spamming the console
bool is_w_held = false;                            // Track the current real-time state of the 'W' key
bool is_a_held = false;                            // Track the current real-time state of the 'A' key
bool is_s_held = false;                            // Track the current real-time state of the 'S' key
bool is_d_held = false;                            // Track the current real-time state of the 'D' key
int next_second = 0;                               // Counter to track the next second interval for logging
std::ofstream log_file;                            // Global file stream object for writing to the log file
std::chrono::steady_clock::time_point start_time;  // Global variable to store the exact time the simulation started



/*
    main() - Initializes the log file and starts the simulation timer.
    Enters a continuous loop that checks for the 'Q' quit command,
    reads the real-time state of the WASD keys, prints any state changes to the console,
    and logs the currently held keys exactly once per second.
    Safely closes the log file upon exit.
*/
int main() {
    if (!setup_logger()) {                                            // Attempt to open the log file, and if it fails, exit immediately
        std::cerr << "Error: Could not open log file." << std::endl;
        
        return 1;
    }

    std::cout << "--- Desktop Thruster Simulator ---" << std::endl;
    std::cout << "Hold W/A/S/D to activate thruster groups." << std::endl;
    std::cout << "Press 'Q' to quit and save the log." << std::endl;
    std::cout << "Logging started...\n" << std::endl;

    while (keep_running) {                                                        // Loop indefinitely to read keystrokes until 'Q' is pressed
        if (GetAsyncKeyState('Q') & 0x8000) {                                     // Check if the 'Q' key is currently being pressed (0x8000 checks the most significant bit)
            std::cout << "\nShutdown command received. Exiting..." << std::endl;
            keep_running = false;                                                 // Set flag to false to safely break the main execution loop

            break;
        }

        is_w_held = GetAsyncKeyState('W') & 0x8000;  // Read the real-time state of the 'W' key
        is_a_held = GetAsyncKeyState('A') & 0x8000;  // Read the real-time state of the 'A' key
        is_s_held = GetAsyncKeyState('S') & 0x8000;  // Read the real-time state of the 'S' key
        is_d_held = GetAsyncKeyState('D') & 0x8000;  // Read the real-time state of the 'D' key

        check_state_changes();                                       // Compare current keys against previous keys to print console updates only when a change occurs
        log_current_state();                                         // Check if a full second has passed, and if so, write the currently held keys to the text file

        std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Sleep for 10 milliseconds to prevent the loop from consuming 100% of the CPU
    }

    cleanup_logger();                                                // Ensure the log file is properly closed and saved before the program terminates

    return 0;
}



/*
    check_state_changes() - Compares the current state of the WASD keys against their state during the previous loop iteration.
    If a key has been pressed or released, it prints a status update to the console simulating
    which thrusters are firing. Updates the tracking variables for the next loop.
*/
void check_state_changes() {
    if (is_w_held != prev_w) {                                                                    // If the state of 'W' has changed since the last check
        std::cout << "Thrusters [0,1,2] " << (is_w_held ? "ON (Forward)" : "OFF") << std::endl;
        prev_w = is_w_held;                                                                       // Update the tracking variable to the new state
    }
    
    if (is_a_held != prev_a) {                                                                    // If the state of 'A' has changed since the last check
        std::cout << "Thrusters [3,4,5] " << (is_a_held ? "ON (Left)" : "OFF") << std::endl;
        prev_a = is_a_held;                                                                       // Update the tracking variable to the new state
    }
    
    if (is_s_held != prev_s) {                                                                    // If the state of 'S' has changed since the last check
        std::cout << "Thrusters [6,7,8] " << (is_s_held ? "ON (Backward)" : "OFF") << std::endl;
        prev_s = is_s_held;                                                                       // Update the tracking variable to the new state
    }
    
    if (is_d_held != prev_d) {                                                                    // If the state of 'D' has changed since the last check
        std::cout << "Thrusters [9,10,11] " << (is_d_held ? "ON (Right)" : "OFF") << std::endl;
        prev_d = is_d_held;                                                                       // Update the tracking variable to the new state
    }
}



/*
    log_current_state() - Calculates the elapsed time since the simulation started.
    If the elapsed time (in seconds) meets or exceeds the next target second, 
    it formats a string with the currently active keys and writes it to the log file.
*/
void log_current_state() {
    auto now = std::chrono::steady_clock::now();                                                // Get the current precise time
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();  // Calculate the total elapsed seconds since the program started

    if (elapsed >= next_second) {                            // If the elapsed time has reached the next logging interval
        std::string log_line = std::to_string(next_second);  // Start the log line with the current second timestamp
        
        if (is_w_held) log_line += " W";  // Append 'W' if the forward thrusters are active
        if (is_a_held) log_line += " A";  // Append 'A' if the left thrusters are active
        if (is_s_held) log_line += " S";  // Append 'S' if the backward thrusters are active
        if (is_d_held) log_line += " D";  // Append 'D' if the right thrusters are active

        log_file << log_line << "\n";  // Write the formatted line to the log file
        log_file.flush();              // Force save to disk immediately in case the program crashes unexpectedly
        
        next_second++;                 // Increment the target second for the next logging event
    }
}



/*
    setup_logger() - Attempts to create and open 'thruster_log.txt'. 
    If successful, initializes the start time for the simulation timer.
    Returns true if the file opened successfully, false otherwise.
*/
bool setup_logger() {
    log_file.open("thruster_log.txt");  // Attempt to open or create the log file in the current directory
    
    if (!log_file.is_open()) {  // Check if the file failed to open (e.g., due to permission issues)
        return false;
    }

    start_time = std::chrono::steady_clock::now();  // Record the exact time the simulation started to use as baseline for logging

    return true;
}



/*
    cleanup_logger() - Safely closes the log file to ensure all data is written to disk
    and prints a confirmation message to the console.
*/
void cleanup_logger() {
    if (log_file.is_open()) {  // Ensure the file is actually open before attempting to close it
        log_file.close();      // Close the file stream to release the system resource
    }
    
    std::cout << "Log saved to thruster_log.txt. Safe shutdown complete." << std::endl;
}
