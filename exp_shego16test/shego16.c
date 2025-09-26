#include "raw_hid.h"
#include "rgb_matrix.h"
#include "display.h"

void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    // This is the keyboard-level hook (safe with VIA)
    for (int i = 0; i < DRIVER_LED_TOTAL && (i*3+2) < length; i++) {
        uint8_t g = data[i*3 + 0];
        uint8_t r = data[i*3 + 1];
        uint8_t b = data[i*3 + 2];
        rgb_matrix_set_color(i, r, g, b);
    }
}

void keyboard_post_init_kb(void) {
    // Inform console that post-init is running and trigger display init/test
    uprintf("Hello from shego16 keyboard\n");
    // Temporarily disable RGB to lower current draw while initializing the display
    rgb_matrix_disable_noeeprom();
    // Initialize and test the ST7735 display
    display_test_pattern();
    wait_ms(1000); // Show test pattern for 1 second
    // Start the gif animation
    display_start_animation();
    // Re-enable RGB afterwards
    rgb_matrix_enable_noeeprom();
}

void housekeeping_task_kb(void) {
    // Update display animation
    display_update_animation();
}
