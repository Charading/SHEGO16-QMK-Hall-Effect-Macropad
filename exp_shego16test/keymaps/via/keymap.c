// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "color.h"
#include "lib/lib8tion/lib8tion.h"


extern led_config_t g_led_config;

//Custom keycodes for ambient section on the led strip and UART commands
enum custom_keycodes {
    AMB_TOG = SAFE_RANGE,
    AMB_HUI, AMB_HUD, AMB_SAI, AMB_SAD, AMB_VAI, AMB_VAD, AMB_NEXT,
    MENU_OPEN, MENU_UP, MENU_DOWN, MENU_SELECT
};

static bool ambient_enabled = false;
static HSV  amb = { .h = 0, .s = 255, .v = 80 };
static uint8_t preset = 0;

// Use hardware UART helper
#include "../../uart.h"

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;

    // First, handle VIA user slots (QK_USER_0..3) directly so VIA works without extra mapping.
    if (keycode == QK_USER_0) {
        if (record->event.pressed) {
            uart_send_string("MENU_OPEN\n");
            rgb_matrix_set_color_all(255, 0, 0);
            wait_ms(200);
        }
        return false;
    }
    if (keycode == QK_USER_1) {
        if (record->event.pressed) {
            uart_send_string("MENU_UP\n");
            rgb_matrix_set_color_all(0, 255, 0);
            wait_ms(200);
        }
        return false;
    }
    if (keycode == QK_USER_2) {
        if (record->event.pressed) {
            uart_send_string("MENU_DOWN\n");
            rgb_matrix_set_color_all(0, 0, 255);
            wait_ms(200);
        }
        return false;
    }
    if (keycode == QK_USER_3) {
        if (record->event.pressed) {
            uart_send_string("MENU_SELECT\n");
            rgb_matrix_set_color_all(255, 255, 0);
            wait_ms(200);
        }
        return false;
    }

    // Map VIA numeric custom keycodes to our enum-based custom keycodes.
    // VIA uses high-value codes (0xffff..0xfffc) to represent custom keys.
    uint16_t kc = keycode;
    if (keycode == 0xffff) kc = MENU_OPEN;
    else if (keycode == 0xfffe) kc = MENU_UP;
    else if (keycode == 0xfffd) kc = MENU_DOWN;
    else if (keycode == 0xfffc) kc = MENU_SELECT;

    switch (kc) {
        // Existing ambient LED controls
        case AMB_TOG: 
            ambient_enabled = !ambient_enabled; 
            // Also send UART test when toggling ambient
            uart_send_string("AMB_TOGGLE_TEST\n");
            rgb_matrix_set_color_all(255, 0, 255); // Magenta flash for test
            wait_ms(300);
            return false;
        case AMB_HUI: amb.h += 8;  return false;
        case AMB_HUD: amb.h -= 8;  return false;
        case AMB_SAI: amb.s = qadd8(amb.s, 16); return false;
        case AMB_SAD: amb.s = qsub8(amb.s, 16); return false;
        case AMB_VAI: amb.v = qadd8(amb.v, 16); return false;
        case AMB_VAD: amb.v = qsub8(amb.v, 16); return false;
        case AMB_NEXT: {
            const HSV presets[] = {{0,255,0},{85,255,0},{171,255,0},{43,255,0},{213,255,0},{0,0,0}};
            preset = (preset + 1) % (sizeof(presets)/sizeof(presets[0]));
            amb.h = presets[preset].h; amb.s = presets[preset].s;
            return false;
        }
        
        // New UART commands for menu control
        case MENU_OPEN: // MENU_OPEN (VIA)
            uart_send_string("MENU_OPEN\n");
            // Simple LED feedback - turn on RGB matrix briefly
            rgb_matrix_set_color_all(255, 0, 0); // Red flash
            wait_ms(200);
            return false;
        case MENU_UP: // MENU_UP (VIA)
            uart_send_string("MENU_UP\n");
            rgb_matrix_set_color_all(0, 255, 0); // Green flash
            wait_ms(200);
            return false;
        case MENU_DOWN: // MENU_DOWN (VIA)
            uart_send_string("MENU_DOWN\n");
            rgb_matrix_set_color_all(0, 0, 255); // Blue flash
            wait_ms(200);
            return false;
        case MENU_SELECT: // MENU_SELECT (VIA)
            uart_send_string("MENU_SELECT\n");
            rgb_matrix_set_color_all(255, 255, 0); // Yellow flash
            wait_ms(200);
            return false;
    }
    return true;
}


const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /*
     * Layer 0: Default layer
     * Top right has encoder (no switch wired to [0,3])
     * ┌───┬───┬───┬───┐
     * │ 1 │ 2 │ 3 │ENC│ <- Encoder physically here
     * ├───┼───┼───┼───┤
     * │ Q │ W │ E │ R │
     * ├───┼───┼───┼───┤
     * │ A │ S │ D │ F │
     * ├───┼───┼───┼───┤
     * │ Z │ X │ C │ V │
     * └───┴───┴───┴───┘
     */
    [0] = LAYOUT_cav1(
        KC_1,    KC_2,    KC_3,       // [0,3] no switch - encoder here
        KC_Q,    KC_W,    KC_E,    KC_R,
        KC_A,    KC_S,    KC_D,    KC_F,
        KC_Z,    KC_X,    KC_C,    MO(3)  // Hold for Layer 3 (UART commands)
    ),
    
    /*
     * Layer 1: Function keys
     */
    [1] = LAYOUT_cav1(
        0xffff,    KC_2,    KC_3,       // [0,3] no switch - encoder here
        0xfffe,    KC_W,    KC_E,    KC_R,
        0xfffd,    KC_S,    KC_D,    KC_F,
        0xfffc,    KC_X,    KC_C,    KC_V
    ),
    
    /*
     * Layer 2: RGB Controls
     */
    [2] = LAYOUT_cav1(
        RGB_TOG, RGB_MOD, RGB_HUI,     // [0,3] no switch - encoder here  
        RGB_VAI, RGB_VAD, RGB_SAI, RGB_SAD,
        RGB_SPI, RGB_SPD, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
    
    /*
     * Layer 3: UART Commands for MENU control
     * Encoder: CCW=MENU_DOWN, CW=MENU_UP, Press=MENU_SELECT
     */
    [3] = LAYOUT_cav1(
        MENU_OPEN, KC_TRNS, KC_TRNS,       // [0,3] no switch - encoder here
        KC_TRNS, AMB_TOG, AMB_HUI, AMB_SAI,
        AMB_VAI, AMB_NEXT, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
        
    )
};

const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },   // Handled in encoder_update_user
    [1] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },   // Handled in encoder_update_user
    [2] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },   // Handled in encoder_update_user
    [3] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS) }    // Handled in encoder_update_user
};

// Encoder handling - Layer-aware with UART commands
bool encoder_update_user(uint8_t index, bool clockwise) {
    if (index == 0) {
        uint8_t current_layer = get_highest_layer(layer_state);
        
        if (current_layer == 3) {
            // Layer 3: UART commands for GIF control
            if (clockwise) {
                // Clockwise = Menu UP
                uart_send_string("MENU_UP\n");
                rgb_matrix_set_color_all(0, 255, 0); // Flash green
                wait_ms(100);
            } else {
                // Counter-clockwise = Menu DOWN  
                uart_send_string("MENU_DOWN\n");
                rgb_matrix_set_color_all(0, 0, 255); // Flash blue
                wait_ms(100);
            }
        } else {
            // Other layers: Volume control (inverted rotation)
            if (clockwise) {
                tap_code(KC_VOLD);  // Volume down (inverted)
            } else {
                tap_code(KC_VOLU);  // Volume up (inverted)
            }
        }
    }
    return false;
}

// Encoder switch handling and mute key
void matrix_scan_user(void) {
    static bool encoder_switch_pressed = false;
    static uint16_t encoder_switch_timer = 0;
    bool switch_state = !readPin(GP21);  // Active low (assuming pullup)
    if (switch_state && !encoder_switch_pressed) {
        encoder_switch_pressed = true;
        encoder_switch_timer = timer_read();
    } else if (!switch_state && encoder_switch_pressed) {
        encoder_switch_pressed = false;
        if (timer_elapsed(encoder_switch_timer) > 50) {
            // Check current layer
            uint8_t current_layer = get_highest_layer(layer_state);
            if (current_layer == 3) {
                // On layer 3, send GIF_SELECT command
                uart_send_string("MENU_SELECT\n");
                rgb_matrix_set_color_all(255, 255, 0); // Flash yellow
                wait_ms(200);
            } else {
                // On other layers, send mute
                tap_code(KC_MUTE);
            }
        }
    }
}

void keyboard_post_init_user(void) {
    // Set encoder switch pin as input with pullup
    setPinInputHigh(GP21);
    
    // Initialize hardware UART and send welcome message
    uart_init_and_welcome();
    
    // Flash all LEDs white to indicate startup
    rgb_matrix_set_color_all(255, 255, 255);
    wait_ms(500);
    rgb_matrix_set_color_all(0, 0, 0);
}

bool rgb_matrix_indicators_user(void) {
    if (!ambient_enabled) return false;

    RGB rgb = hsv_to_rgb(amb);
    for (uint8_t i = 0; i < DRIVER_LED_TOTAL; i++) {
        if (g_led_config.flags[i] & LED_FLAG_UNDERGLOW) {
            rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
        }
    }
    return false;
}
