
/*
    GPIO Simple Toggle

    Simple GPIO call to Raspberry Pi 5 GPIOs using libgpiod (C++ Wrapper around C API)
    This simply calls GPIO0, which is the first switch on the board and toggles it.
*/



#include <gpiod.h>   // For GPIO control using libgpiod
#include <cstdio>    // C++ version of stdio.h which is compatible with C-style I/O functions like printf
#include <cstdlib>   // C++ version of stdlib.h which is compatible with C-style functions like exit
#include <unistd.h>  // For sleep()



// Function Prototypes ============================================================================
bool setup_gpio();
void cleanup_gpio();



// Global Variables ===============================================================================
const unsigned int pin0 = 0;               // GPIO-0
const unsigned int pin1 = 1;               // GPIO-1
const unsigned int pin2 = 2;               // GPIO-2
const unsigned int pin3 = 3;               // GPIO-3
const char *chip_path = "/dev/gpiochip0";  // Path to the GPIO chip device
const char *consumer = "relay-demo";       // Consumer name for debugging in gpioinfo



// Libgpiod Objects ===============================================================================
struct gpiod_chip *chip = nullptr;               // Pointer to the GPIO chip object
struct gpiod_request_config *req_cfg = nullptr;  // Pointer to the request configuration object which holds settings for the line request
struct gpiod_line_config *line_cfg = nullptr;    // Pointer to the line configuration object which holds settings for individual lines
struct gpiod_line_settings *settings = nullptr;  // Pointer to the line settings object which holds settings for line direction and initial value
struct gpiod_line_request *request = nullptr;    // Pointer to the line request object which represents the actual request for control of the GPIO lines



/*
    main() - The main function initializes the GPIO chip and configures the specified GPIO lines as outputs. 
             It then enters an infinite loop where it toggles the specified GPIO lines on and off every second. 
             The program can be exited gracefully by releasing the line request and closing the GPIO chip in the cleanup section.
*/
int main() {
    if (!setup_gpio()) {                                   // Attempt to set up the GPIO lines, and if it fails, print an error message and clean up resources before exiting
        fprintf(stderr, "Failed to set up GPIO lines\n");  
        cleanup_gpio();                                    // Clean up any allocated resources to ensure a clean exit

        return 1;
    }

    while (true) {                                                               // Loop indefinitely to toggle the GPIO lines on and off
        gpiod_line_request_set_value(request, pin0, GPIOD_LINE_VALUE_ACTIVE);    // Set GPIO-0 to active (usually 3.3V)
        gpiod_line_request_set_value(request, pin1, GPIOD_LINE_VALUE_ACTIVE);    // Set GPIO-1 to active (usually 3.3V)
        gpiod_line_request_set_value(request, pin2, GPIOD_LINE_VALUE_ACTIVE);    // Set GPIO-2 to active (usually 3.3V)
        gpiod_line_request_set_value(request, pin3, GPIOD_LINE_VALUE_ACTIVE);    // Set GPIO-3 to active (usually 3.3V)
        printf("ON\n");
        sleep(1);                                                                // Wait for 1 second

        gpiod_line_request_set_value(request, pin0, GPIOD_LINE_VALUE_INACTIVE);  // Set GPIO-0 to inactive (usually 0V)
        gpiod_line_request_set_value(request, pin1, GPIOD_LINE_VALUE_INACTIVE);  // Set GPIO-1 to inactive (usually 0V)
        gpiod_line_request_set_value(request, pin2, GPIOD_LINE_VALUE_INACTIVE);  // Set GPIO-2 to inactive (usually 0V)
        gpiod_line_request_set_value(request, pin3, GPIOD_LINE_VALUE_INACTIVE);  // Set GPIO-3 to inactive (usually 0V)
        printf("OFF\n");
        sleep(1);
    }
    
    cleanup_gpio();

    return 0;
}



/*
    setup_gpio() - This function initializes the GPIO chip and configures the specified GPIO lines as outputs. 
                   It sets the initial output value for the lines to inactive (usually 0V) and requests control of the lines from the chip. 
                   If any step fails, it returns false to indicate that setup was unsuccessful, allowing the main function to handle cleanup and exit gracefully.
*/
bool setup_gpio() {
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

    gpiod_request_config_set_consumer(req_cfg, consumer);                                // Set the consumer name for the request configuration, which is useful for debugging in tools like gpioinfo
    if (gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT) < 0) {  // Set the line direction to output and check for errors
        perror("gpiod_line_settings_set_direction");

        return false;                                                                    // If setting the line direction failed, return false to trigger cleanup
    }

    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);  // Set the initial output value for the lines to inactive (usually 0V)
    gpiod_line_config_add_line_settings(line_cfg, &pin0, 1, settings);           // Add the settings for GPIO-0 to the line configuration
    gpiod_line_config_add_line_settings(line_cfg, &pin1, 1, settings);           // Add the settings for GPIO-1 to the line configuration
    gpiod_line_config_add_line_settings(line_cfg, &pin2, 1, settings);           // Add the settings for GPIO-2 to the line configuration
    gpiod_line_config_add_line_settings(line_cfg, &pin3, 1, settings);           // Add the settings for GPIO-3 to the line configuration

    request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);  // Request control of the specified lines from the chip using the configured settings
    if (!request) {                                               // Check if the line request was successful
        perror("gpiod_chip_request_lines");

        return false;                                             // If the line request failed, return false to trigger cleanup
    }
    
    return true;
}



/*
    cleanup_gpio() - This function releases the line request to free control of the GPIO lines, frees all allocated libgpiod objects, and closes the GPIO chip. 
                     It ensures that all resources are properly released when the program exits, preventing resource leaks and allowing other programs to use the GPIO lines without issues.
*/
void cleanup_gpio() {
    if (request) gpiod_line_request_release(request);  // Release the line request to free control of the GPIO lines
    if (settings) gpiod_line_settings_free(settings);  // Free the line settings object
    if (line_cfg) gpiod_line_config_free(line_cfg);    // Free the line configuration object
    if (req_cfg) gpiod_request_config_free(req_cfg);   // Free the request configuration object
    if (chip) gpiod_chip_close(chip);                  // Close the GPIO chip
}