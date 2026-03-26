
/*
    Relay WASD Toggle

    Simple WASD interactive example using a Raspberry Pi 5.
    Listens for non-blocking keyboard input to toggle specific GPIOs: 
    W - GPIO0, A - GPIO1, S - GPIO2, D - GPIO3.
*/



#include <gpiod.h>    // For GPIO control using libgpiod
#include <cstdio>     // C++ version of stdio.h which is compatible with C-style I/O functions like printf
#include <cstdlib>    // C++ version of stdlib.h which is compatible with C-style functions like exit
#include <unistd.h>   // For sleep() and usleep()
#include <csignal>    // C++ version of signal.h for handling system signals like SIGINT (Ctrl+C)
#include <termios.h>  // For terminal I/O control to read characters without hitting 'Enter'
#include <fcntl.h>    // For file control to enable non-blocking input reading



// Function Prototypes ============================================================================
void handle_sigint(int sig);
enum gpiod_line_value relay_on_value();
enum gpiod_line_value relay_off_value();
bool setup_gpio();
void cleanup_gpio();



// Global Variables ===============================================================================
const unsigned int NUM_RELAYS = 24;                  // Total number of relays/GPIO pins available
const int ACTIVE_LOW = 0;                            // Flag to determine relay polarity (0 for Active High, 1 for Active Low)
const char *chip_path = "/dev/gpiochip0";            // Path to the GPIO chip device
const char *consumer = "relay-wasd";                 // Consumer name for debugging in gpioinfo
unsigned int offsets[NUM_RELAYS];                    // Array to hold the GPIO offsets (0 through 23)
int state[NUM_RELAYS] = {0};                         // Array to track if a specific relay is currently ON (1) or OFF (0)
static volatile std::sig_atomic_t keep_running = 1;  // Atomic flag to ensure thread-safe signal handling for loop exit
struct termios oldt;                                 // Structure to store the original terminal settings for restoration on exit



// Libgpiod Objects ===============================================================================
struct gpiod_chip *chip = nullptr;               // Pointer to the GPIO chip object
struct gpiod_request_config *req_cfg = nullptr;  // Pointer to the request configuration object which holds settings for the line request
struct gpiod_line_config *line_cfg = nullptr;    // Pointer to the line configuration object which holds settings for individual lines
struct gpiod_line_settings *settings = nullptr;  // Pointer to the line settings object which holds settings for line direction and initial value
struct gpiod_line_request *request = nullptr;    // Pointer to the line request object which represents the actual request for control of the GPIO lines



/*
    main() - Initializes the GPIO chip, configures the pins as outputs, and modifies terminal behavior.
             Enters a continuous, non-blocking loop listening for W, A, S, or D keystrokes.
             When a valid key is pressed, it toggles the corresponding relay state.
             Gracefully restores the terminal and closes the chip when Ctrl+C is caught.
*/
int main() {
    std::signal(SIGINT, handle_sigint);  // Register the Ctrl+C signal handler to catch exit requests

    if (!setup_gpio()) {                                               // Attempt to set up the GPIO lines and terminal, clean up if it fails
        fprintf(stderr, "Failed to set up GPIO lines or terminal\n");
        cleanup_gpio();                                                // Clean up any allocated resources to ensure a clean exit

        return 1;
    }

    printf("WASD toggles GPIO0..GPIO3. Ctrl+C to exit.\n");

    while (keep_running) {                                     // Loop indefinitely to listen for keystrokes until Ctrl+C is pressed
        char ch;
        int n = static_cast<int>(read(STDIN_FILENO, &ch, 1));  // Read 1 byte from standard input (non-blocking)
        if (n > 0) {                                           // If a character was successfully read from the keyboard
            int idx = -1;                                      // Initialize index to an invalid state

            if (ch == 'w' || ch == 'W') idx = 0;               // Map 'W' key to GPIO 0
            else if (ch == 'a' || ch == 'A') idx = 1;          // Map 'A' key to GPIO 1
            else if (ch == 's' || ch == 'S') idx = 2;          // Map 'S' key to GPIO 2
            else if (ch == 'd' || ch == 'D') idx = 3;          // Map 'D' key to GPIO 3

            if (idx >= 0) {                                                                   // If a valid mapped key was pressed
                state[idx] = !state[idx];                                                     // Toggle the tracked state array for that specific relay (0 to 1, or 1 to 0)
                enum gpiod_line_value v = state[idx] ? relay_on_value() : relay_off_value();  // Determine if we are sending an ON or OFF signal based on the new state
                
                gpiod_line_request_set_value(request, offsets[idx], v);   // Apply the new value to the physical hardware pin
                printf("Relay %d %s\n", idx, state[idx] ? "ON" : "OFF");  // Print the state change to the console
                fflush(stdout);                                           // Force the output to print immediately (necessary because we disabled standard terminal buffering)
            }
        }
        
        usleep(5000);  // Sleep for 5 milliseconds to prevent the loop from consuming 100% CPU
    }

    printf("\nShutdown complete.\n");
    cleanup_gpio();                    // Restore terminal settings and clean up hardware requests

    return 0;
}



/*
    handle_sigint() - Signal handler caught by the OS when the user presses Ctrl+C.
                      It toggles the keep_running flag to 0, which breaks the main while loop.
*/
void handle_sigint(int sig) {
    (void)sig;         // Suppress unused parameter warning from the compiler
    keep_running = 0;  // Set flag to 0 to safely break the main execution loop
}



/*
    relay_on_value() - Helper function that returns the correct libgpiod value for "ON"
                       based on whether the relay board uses active-low or active-high logic.
*/
enum gpiod_line_value relay_on_value() {
    return ACTIVE_LOW ? GPIOD_LINE_VALUE_INACTIVE : GPIOD_LINE_VALUE_ACTIVE;  // Return inactive if active-low, otherwise active
}



/*
    relay_off_value() - Helper function that returns the correct libgpiod value for "OFF"
                        based on whether the relay board uses active-low or active-high logic.
*/
enum gpiod_line_value relay_off_value() {
    return ACTIVE_LOW ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;  // Return active if active-low, otherwise inactive
}



/*
    setup_gpio() - Initializes the GPIO chip and configures the terminal for live input. 
                   Populates offsets, sets lines to output mode, and requests control.
                   Then modifies terminal attributes to be non-blocking and character-immediate.
*/
bool setup_gpio() {
    for (unsigned int i = 0; i < NUM_RELAYS; i++) {  // Populate the array with GPIO offsets 0 through 23
        offsets[i] = i;
    }

    chip = gpiod_chip_open(chip_path);  // Open the GPIO chip
    if (!chip) {                        // Check if the chip was opened successfully
        perror("gpiod_chip_open");      // Print an error message if the chip could not be opened

        return false;
    }

    req_cfg = gpiod_request_config_new();                          // Create a new request configuration object
    line_cfg = gpiod_line_config_new();                            // Create a new line configuration object
    settings = gpiod_line_settings_new();                          // Create a new line settings object
    if (!req_cfg || !line_cfg || !settings) {                      // Check if any of the objects were created successfully
        fprintf(stderr, "Failed to allocate libgpiod objects\n");

        return false;                                              // If any object creation failed, return false to trigger cleanup
    }

    gpiod_request_config_set_consumer(req_cfg, consumer);                                    // Set the consumer name for the request configuration
    if (gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT) < 0) {      // Set the line direction to output and check for errors
        perror("gpiod_line_settings_set_direction");

        return false;                                                                        // If setting the line direction failed, return false to trigger cleanup
    }

    gpiod_line_settings_set_output_value(settings, relay_off_value());                       // Set the initial output value for the lines to the defined OFF state
    if (gpiod_line_config_add_line_settings(line_cfg, offsets, NUM_RELAYS, settings) < 0) {  // Add the settings for all 24 offsets to the line configuration in bulk
        perror("gpiod_line_config_add_line_settings");

        return false;
    }

    request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);  // Request control of the specified lines from the chip using the configured settings
    if (!request) {                                               // Check if the line request was successful
        perror("gpiod_chip_request_lines");

        return false;                                             // If the line request failed, return false to trigger cleanup
    }

    struct termios newt;                                  // Create a temporary structure for new terminal settings
    tcgetattr(STDIN_FILENO, &oldt);                       // Save current terminal settings to the global oldt variable for later restoration
    newt = oldt;                                          // Copy the old settings to our new struct to modify them
    newt.c_lflag &= ~(ICANON | ECHO);                     // Disable canonical buffering (hitting Enter) and echoing of typed characters
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);              // Apply the new terminal settings immediately

    int oldflags = fcntl(STDIN_FILENO, F_GETFL, 0);       // Get the current flags for standard input
    fcntl(STDIN_FILENO, F_SETFL, oldflags | O_NONBLOCK);  // Apply the non-blocking flag so our read() function doesn't freeze the loop waiting for a key
    
    return true;
}



/*
    cleanup_gpio() - Restores original terminal behavior so the command line works normally after exit.
                     Releases the line request to free control of the GPIO lines, frees all allocated 
                     libgpiod objects, and closes the GPIO chip.
*/
void cleanup_gpio() {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);           // Restore the original terminal settings so the command line behaves normally after exit
    
    if (request) gpiod_line_request_release(request);  // Release the line request to free control of the GPIO lines
    if (settings) gpiod_line_settings_free(settings);  // Free the line settings object
    if (line_cfg) gpiod_line_config_free(line_cfg);    // Free the line configuration object
    if (req_cfg) gpiod_request_config_free(req_cfg);   // Free the request configuration object
    if (chip) gpiod_chip_close(chip);                  // Close the GPIO chip
}