// GPIO Simple Toggle
/*
 * Simple GPIO call to Raspberry Pi 5 
 * GPIOs using libgpiod (C++ Wrapper around C API)
 * This simply calls GPIO0, which is the first
 * switch on the board and toggles it.
 */
#include <gpiod.h>
#include <cstdio>   // C++ version of stdio.h
#include <cstdlib>  // C++ version of stdlib.h
#include <unistd.h>  // For sleep()

int main() 
{
    // Define the path to the GPIO chip and the line offsets
    const char *chip_path = "/dev/gpiochip0";
    const unsigned int sw1 = 0;  // GPIO0
    const unsigned int sw2 = 1;  // GPIO1
    const unsigned int sw3 = 2;  // GPIO2
    const unsigned int sw4 = 3;  // GPIO3
    
    const char *consumer = "relay-demo";

    // Initialize pointers for libgpiod objects to NULL
    struct gpiod_chip *chip = nullptr;
    struct gpiod_request_config *req_cfg = nullptr;
    struct gpiod_line_config *line_cfg = nullptr;
    struct gpiod_line_settings *settings = nullptr;
    struct gpiod_line_request *request = nullptr;

    // Open the GPIO chip
    chip = gpiod_chip_open(chip_path);
    if (!chip) {
        perror("gpiod_chip_open");
        return 1;
    }

    // Allocate necessary configuration objects
    req_cfg = gpiod_request_config_new();
    line_cfg = gpiod_line_config_new();
    settings = gpiod_line_settings_new();
    
    // Check if allocation failed
    if (!req_cfg || !line_cfg || !settings) {
        fprintf(stderr, "Failed to allocate libgpiod objects\n");
        goto cleanup;
    }

    // Set the consumer name (useful for debugging in gpioinfo)
    gpiod_request_config_set_consumer(req_cfg, consumer);

    // Set the line direction to output
    if (gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT) < 0) {
        perror("gpiod_line_settings_set_direction");
        goto cleanup;
    }

    // Initialize the line to be inactive (usually 0V)
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

    // Add settings for each specific GPIO pin
    gpiod_line_config_add_line_settings(line_cfg, &sw1, 1, settings);
    gpiod_line_config_add_line_settings(line_cfg, &sw2, 1, settings);
    gpiod_line_config_add_line_settings(line_cfg, &sw3, 1, settings);
    gpiod_line_config_add_line_settings(line_cfg, &sw4, 1, settings);

    // Request the lines from the chip to gain control
    request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (!request) {
        perror("gpiod_chip_request_lines");
        goto cleanup;
    }

    // Infinite loop to toggle pins ON and OFF
    while (true) {
        // Set all pins to ACTIVE (Logic High/ON)
        gpiod_line_request_set_value(request, sw1, GPIOD_LINE_VALUE_ACTIVE);
        gpiod_line_request_set_value(request, sw2, GPIOD_LINE_VALUE_ACTIVE);
        gpiod_line_request_set_value(request, sw3, GPIOD_LINE_VALUE_ACTIVE);
        gpiod_line_request_set_value(request, sw4, GPIOD_LINE_VALUE_ACTIVE);
        printf("ON\n");
        sleep(1);

        // Set all pins to INACTIVE (Logic Low/OFF)
        gpiod_line_request_set_value(request, sw1, GPIOD_LINE_VALUE_INACTIVE);
        gpiod_line_request_set_value(request, sw2, GPIOD_LINE_VALUE_INACTIVE);
        gpiod_line_request_set_value(request, sw3, GPIOD_LINE_VALUE_INACTIVE);
        gpiod_line_request_set_value(request, sw4, GPIOD_LINE_VALUE_INACTIVE);
        printf("OFF\n");
        sleep(1);
    }
    
cleanup:
    // Free resources in reverse order of allocation
    if (request) gpiod_line_request_release(request);
    if (settings) gpiod_line_settings_free(settings);
    if (line_cfg) gpiod_line_config_free(line_cfg);
    if (req_cfg) gpiod_request_config_free(req_cfg);
    if (chip) gpiod_chip_close(chip);

    return 0;
}

// Relay Cylee, can iterate through 24 relays and handle clean shutown via SIGINT
/*
 * Cycle all GPIOs on relay board to Raspberry Pi 5 
 * This ensures all are off at start and end.
 */
#include <gpiod.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <csignal> // C++ version of signal.h

#define CHIP_PATH "/dev/gpiochip0"
#define NUM_RELAYS 24
#define DELAY_S 1
#define ACTIVE_LOW 0

// Atomic flag to ensure thread-safe signal handling for loop exit
static volatile std::sig_atomic_t keep_running = 1;

// Signal handler to catch Ctrl+C
static void handle_sigint(int sig) {
    (void)sig; // Suppress unused parameter warning
    keep_running = 0;
}

// Helper: Returns what "ON" means based on relay polarity
static enum gpiod_line_value relay_on_value() {
    return ACTIVE_LOW ? GPIOD_LINE_VALUE_INACTIVE : GPIOD_LINE_VALUE_ACTIVE;
}

// Helper: Returns what "OFF" means based on relay polarity
static enum gpiod_line_value relay_off_value() {
    return ACTIVE_LOW ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;
}

// Helper: Bulk set values for a group of offsets
static int set_all(struct gpiod_line_request *req, const unsigned int *offsets, size_t n, enum gpiod_line_value val) {
    for (size_t i = 0; i < n; i++) {
        if (gpiod_line_request_set_value(req, offsets[i], val) < 0) {
            perror("gpiod_line_request_set_value");
            return -1;
        }
    }
    return 0;
}

int main() {
    // Register the Ctrl+C signal handler
    std::signal(SIGINT, handle_sigint);

    // Initialize array of GPIO offsets (0 through 23)
    unsigned int offsets[NUM_RELAYS];
    for (unsigned int i = 0; i < NUM_RELAYS; i++) offsets[i] = i;

    // Open the chip
    struct gpiod_chip *chip = gpiod_chip_open(CHIP_PATH);
    if (!chip) { perror("gpiod_chip_open"); return 1; }

    // Allocate configuration objects
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    struct gpiod_line_settings *settings = gpiod_line_settings_new();

    if (!req_cfg || !line_cfg || !settings) {
        fprintf(stderr, "Failed to allocate objects\n");
        return 1;
    }

    // Configure request and line settings
    gpiod_request_config_set_consumer(req_cfg, "relay-cycle");
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, relay_off_value());

    // Apply settings to all 24 offsets
    if (gpiod_line_config_add_line_settings(line_cfg, offsets, NUM_RELAYS, settings) < 0) {
        perror("gpiod_line_config_add_line_settings");
        return 1;
    }

    // Finalize the request
    struct gpiod_line_request *req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (!req) { perror("gpiod_chip_request_lines"); return 1; }

    // Safety: Ensure everything starts OFF
    set_all(req, offsets, NUM_RELAYS, relay_off_value());
    printf("Startup reset: all relays OFF\n");

    // Loop until user hits Ctrl+C
    while (keep_running) {
        for (unsigned int i = 0; i < NUM_RELAYS && keep_running; i++) {
            gpiod_line_request_set_value(req, offsets[i], relay_on_value());
            printf("Relay %u ON\n", i);
            sleep(DELAY_S);

            gpiod_line_request_set_value(req, offsets[i], relay_off_value());
        }
    }

    // Safety: Ensure everything is OFF before exiting
    set_all(req, offsets, NUM_RELAYS, relay_off_value());
    printf("\nShutdown complete.\n");

    // Cleanup
    gpiod_line_request_release(req);
    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);
    gpiod_chip_close(chip);

    return 0;
}

// Relay WASD Toggle
/*
 * Simple WASD Example using RPI5 
 * W - GPIO0, A - GPIO1, S - GPIO2, D - GPIO3
 */
#include <gpiod.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <termios.h> // For terminal I/O control
#include <fcntl.h>   // For file control (non-blocking)
#include <csignal>

#define CHIP_PATH "/dev/gpiochip0"
#define NUM_RELAYS 24
#define ACTIVE_LOW 0

static volatile std::sig_atomic_t keep_running = 1;

static void handle_sigint(int sig) {
    (void)sig;
    keep_running = 0;
}

static enum gpiod_line_value relay_on_value() {
    return ACTIVE_LOW ? GPIOD_LINE_VALUE_INACTIVE : GPIOD_LINE_VALUE_ACTIVE;
}

static enum gpiod_line_value relay_off_value() {
    return ACTIVE_LOW ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;
}

int main() {
    std::signal(SIGINT, handle_sigint);

    unsigned int offsets[NUM_RELAYS];
    for (unsigned int i = 0; i < NUM_RELAYS; i++) offsets[i] = i;

    // Array to track if relay is currently ON (1) or OFF (0)
    int state[NUM_RELAYS] = {0};

    struct gpiod_chip *chip = gpiod_chip_open(CHIP_PATH);
    if (!chip) return 1;

    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    struct gpiod_line_settings *settings = gpiod_line_settings_new();

    gpiod_request_config_set_consumer(req_cfg, "relay-wasd");
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, relay_off_value());

    gpiod_line_config_add_line_settings(line_cfg, offsets, NUM_RELAYS, settings);
    struct gpiod_line_request *req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    // --- TERMINAL CONFIGURATION ---
    // Change terminal mode so we can read single keys without hitting 'Enter'
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); // Disable buffering and echoing
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // Set stdin to non-blocking mode so the loop doesn't pause to wait for keys
    int oldflags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldflags | O_NONBLOCK);

    printf("WASD toggles GPIO0..GPIO3. Ctrl+C to exit.\n");

    while (keep_running) {
        char ch;
        // Read 1 byte from standard input
        int n = static_cast<int>(read(STDIN_FILENO, &ch, 1));

        if (n > 0) {
            int idx = -1;
            // Map keys to GPIO index
            if (ch == 'w' || ch == 'W') idx = 0;
            else if (ch == 'a' || ch == 'A') idx = 1;
            else if (ch == 's' || ch == 'S') idx = 2;
            else if (ch == 'd' || ch == 'D') idx = 3;

            if (idx >= 0) {
                state[idx] = !state[idx]; // Flip the state
                enum gpiod_line_value v = state[idx] ? relay_on_value() : relay_off_value();
                gpiod_line_request_set_value(req, offsets[idx], v);
                printf("Relay %d %s\n", idx, state[idx] ? "ON" : "OFF");
                fflush(stdout);
            }
        }
        usleep(5000); // 5ms sleep to prevent 100% CPU usage
    }

    // Restore original terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    // Cleanup and close
    gpiod_line_request_release(req);
    gpiod_chip_close(chip);
    return 0;
}