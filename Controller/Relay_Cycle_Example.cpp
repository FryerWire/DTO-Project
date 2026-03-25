
/*
    Relay Cycle

    Cycle all 24 GPIOs on a relay board attached to a Raspberry Pi 5 using libgpiod.
    This script ensures all relays are forced OFF at start, iterates through them one by one,
    and cleanly shuts them all OFF when a SIGINT (Ctrl+C) is caught.
*/



#include <gpiod.h>   // For GPIO control using libgpiod
#include <cstdio>    // C++ version of stdio.h which is compatible with C-style I/O functions like printf
#include <cstdlib>   // C++ version of stdlib.h which is compatible with C-style functions like exit
#include <unistd.h>  // For sleep()
#include <csignal>   // C++ version of signal.h for handling system signals like SIGINT (Ctrl+C)



// Function Prototypes ============================================================================
void handle_sigint(int sig);
enum gpiod_line_value relay_on_value();
enum gpiod_line_value relay_off_value();
int set_all(struct gpiod_line_request *req, const unsigned int *offsets, size_t n, enum gpiod_line_value val);
bool setup_gpio();
void cleanup_gpio();



// Global Variables ===============================================================================
const unsigned int NUM_RELAYS = 24;                  // Total number of relays/GPIO pins to cycle through
const unsigned int DELAY_S = 1;                      // Delay in seconds between each relay toggle
const int ACTIVE_LOW = 0;                            // Flag to determine relay polarity (0 for Active High, 1 for Active Low)
const char *chip_path = "/dev/gpiochip0";            // Path to the GPIO chip device
const char *consumer = "relay-cycle";                // Consumer name for debugging in gpioinfo

unsigned int offsets[NUM_RELAYS];                    // Array to hold the GPIO offsets (0 through 23)
static volatile std::sig_atomic_t keep_running = 1;  // Atomic flag to ensure thread-safe signal handling for loop exit



// Libgpiod Objects ===============================================================================
struct gpiod_chip *chip = nullptr;               // Pointer to the GPIO chip object
struct gpiod_request_config *req_cfg = nullptr;  // Pointer to the request configuration object which holds settings for the line request
struct gpiod_line_config *line_cfg = nullptr;    // Pointer to the line configuration object which holds settings for individual lines
struct gpiod_line_settings *settings = nullptr;  // Pointer to the line settings object which holds settings for line direction and initial value
struct gpiod_line_request *request = nullptr;    // Pointer to the line request object which represents the actual request for control of the GPIO lines



/*
    main() - Initializes the GPIO chip and configures the 24 lines as outputs. 
             It forces all relays OFF, then loops through turning each ON and OFF in sequence.
             The loop listens for a SIGINT (Ctrl+C), breaking out to gracefully turn all relays OFF
             and clean up memory before exiting.
*/
int main() {
    std::signal(SIGINT, handle_sigint);  // Register the Ctrl+C signal handler to catch exit requests

    if (!setup_gpio()) {                                   // Attempt to set up the GPIO lines, and if it fails, clean up and exit
        fprintf(stderr, "Failed to set up GPIO lines\n");
        cleanup_gpio();                                    // Clean up any allocated resources to ensure a clean exit

        return 1;
    }

    set_all(request, offsets, NUM_RELAYS, relay_off_value());                      // Safety: Ensure all relays are explicitly forced OFF on startup
    printf("Startup reset: all relays OFF\n");
    while (keep_running) {                                                         // Loop indefinitely to cycle through the relays until Ctrl+C is pressed
        for (unsigned int i = 0; i < NUM_RELAYS && keep_running; i++) {            // Iterate through each of the 24 relays (aborting mid-loop if signaled)
            gpiod_line_request_set_value(request, offsets[i], relay_on_value());   // Turn the current relay ON
            printf("Relay %u ON\n", i);
            sleep(DELAY_S);                                                        // Wait for the defined delay period

            gpiod_line_request_set_value(request, offsets[i], relay_off_value());  // Turn the current relay OFF before moving to the next
        }
    }

    set_all(request, offsets, NUM_RELAYS, relay_off_value());  // Safety: Ensure all relays are explicitly forced OFF before exiting
    printf("\nShutdown complete.\n");

    cleanup_gpio();  // Clean up all allocated resources and close the GPIO chip before exiting

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
    set_all() - Helper function to apply a specific value (ON or OFF) to a bulk array of pins.
                Iterates through the provided array and applies the value using the libgpiod request.
*/
int set_all(struct gpiod_line_request *req, const unsigned int *offsets, size_t n, enum gpiod_line_value val) {
    for (size_t i = 0; i < n; i++) {                                                                             // Loop through the array of pins
        if (gpiod_line_request_set_value(req, offsets[i], val) < 0) {                                            // Attempt to set the desired value
            perror("gpiod_line_request_set_value");                                                              // Print error if the line value could not be set

            return -1;
        }
    }
    return 0;
}



/*
    setup_gpio() - This function initializes the GPIO chip and configures the array of 24 lines as outputs. 
                   It populates the offset array, sets the lines to output mode, and requests control.
                   If any step fails, it returns false to trigger a clean exit.
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

    gpiod_request_config_set_consumer(req_cfg, consumer);                                // Set the consumer name for the request configuration
    if (gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT) < 0) {  // Set the line direction to output and check for errors
        perror("gpiod_line_settings_set_direction");

        return false;                                                                    // If setting the line direction failed, return false to trigger cleanup
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
    
    return true;
}



/*
    cleanup_gpio() - This function releases the line request to free control of the GPIO lines, frees all allocated libgpiod objects, and closes the GPIO chip. 
                     It ensures that all resources are properly released when the program exits.
*/
void cleanup_gpio() {
    if (request) gpiod_line_request_release(request);  // Release the line request to free control of the GPIO lines
    if (settings) gpiod_line_settings_free(settings);  // Free the line settings object
    if (line_cfg) gpiod_line_config_free(line_cfg);    // Free the line configuration object
    if (req_cfg) gpiod_request_config_free(req_cfg);   // Free the request configuration object
    if (chip) gpiod_chip_close(chip);                  // Close the GPIO chip
}
