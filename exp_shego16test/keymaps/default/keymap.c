// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

// Custom keycodes for UART commands
enum custom_keycodes {
    MENU_OPEN = SAFE_RANGE,
    MENU_UP,
    MENU_DOWN,
    MENU_SELECT,
};

// UART configuration
#define UART_TX_PIN GP16
#define UART_BAUD_RATE 115200
#define UART_BIT_DELAY_US (1000000 / UART_BAUD_RATE)

// Simple bit-banged UART transmit function
void uart_send_byte(uint8_t data) {
    // Start bit
    setPinOutput(UART_TX_PIN);
    writePinLow(UART_TX_PIN);
    wait_us(UART_BIT_DELAY_US);
    
    // Data bits (LSB first)
    for (int i = 0; i < 8; i++) {
        if (data & (1 << i)) {
            writePinHigh(UART_TX_PIN);
        } else {
            writePinLow(UART_TX_PIN);
        }
        wait_us(UART_BIT_DELAY_US);
    }
    
    // Stop bit
    writePinHigh(UART_TX_PIN);
    wait_us(UART_BIT_DELAY_US);
}

void uart_send_string(const char* str) {
    while (*str) {
        uart_send_byte(*str++);
    }
}

// Custom 4x4 ortho layout macro for cav16 (15 keys, encoder knob in top right)
#define LAYOUT_cav1( \
	k00, k01, k02, \
	k10, k11, k12, k13, \
	k20, k21, k22, k23, \
	k30, k31, k32, k33  \
) \
	{ \
		{ k00, k01, k02, KC_NO }, \
		{ k10, k11, k12, k13 }, \
		{ k20, k21, k22, k23 }, \
		{ k30, k31, k32, k33 }  \
	}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /*
     * Layer 0: Default layer
     * Top right has encoder (no switch wired to [0,3])
     * ┌───┬───┬───┬───┐
     * │ 7 │ 8 │ 9 │ENC│ <- Encoder physically here
     * ├───┼───┼───┼───┤
     * │ 4 │ 5 │ 6 │ * │
     * ├───┼───┼───┼───┤
     * │ 1 │ 2 │ 3 │ - │
     * ├───┼───┼───┼───┤
     * │ 0 │ . │ENT│ + │
     * └───┴───┴───┴───┘
     */
    [0] = LAYOUT_cav1(
        KC_1,    KC_2,    KC_3,       // [0,3] no switch - encoder here
        KC_Q,    KC_W,    KC_E,    KC_R,
        KC_A,    KC_S,    KC_D,    KC_F,
        KC_Z,    KC_X,    KC_C,    MO(3)  // Hold for layer 3 (UART commands)
    ),
    
    /*
     * Layer 1: Function keys
     */
    [1] = LAYOUT_cav1(
        KC_F7,   KC_F8,   KC_F9,       // [0,3] no switch - encoder here
        KC_F4,   KC_F5,   KC_F6,   KC_F11,
        KC_F1,   KC_F2,   KC_F3,   KC_F12,
        KC_ESC,  KC_TAB,  MO(2),   MO(3)  // Layer 2 (RGB) and Layer 3 (UART)
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
     * Layer 3: UART Commands for GIF control
     */
    [3] = LAYOUT_cav1(
        MENU_OPEN, MENU_UP,   MENU_DOWN,    // [0,3] no switch - encoder here
        MENU_SELECT, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    )
};

// Process custom keycodes for UART commands
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case MENU_OPEN:
            if (record->event.pressed) {
                uart_send_string("MENU_OPEN\n");
            }
            return false;
            
        case MENU_UP:
            if (record->event.pressed) {
                uart_send_string("MENU_UP\n");
            }
            return false;
            
        case MENU_DOWN:
            if (record->event.pressed) {
                uart_send_string("MENU_DOWN\n");
            }
            return false;
            
        case MENU_SELECT:
            if (record->event.pressed) {
                uart_send_string("MENU_SELECT\n");
            }
            return false;
    }
    return true;
}

// Encoder handling - Volume up/down/mute (inverted rotation)
bool encoder_update_user(uint8_t index, bool clockwise) {
    if (index == 0) {
        if (clockwise) {
            tap_code(KC_VOLD);  // Volume down (inverted)
        } else {
            tap_code(KC_VOLU);  // Volume up (inverted)
        }
    }
    return false;
}

// Custom key handling for encoder switch (mute)
void matrix_scan_user(void) {
    static bool encoder_switch_pressed = false;
    static uint16_t encoder_switch_timer = 0;
    
    // Read encoder switch pin
    bool switch_state = !readPin(GP21);  // Active low (assuming pullup)
    
    if (switch_state && !encoder_switch_pressed) {
        // Switch just pressed
        encoder_switch_pressed = true;
        encoder_switch_timer = timer_read();
    } else if (!switch_state && encoder_switch_pressed) {
        // Switch just released
        encoder_switch_pressed = false;
        if (timer_elapsed(encoder_switch_timer) > 50) {  // Debounce
            tap_code(KC_MUTE);  // Send mute
        }
    }
}

void keyboard_post_init_user(void) {
    // Set encoder switch pin as input with pullup
    setPinInputHigh(GP21);
    
    // Initialize UART TX pin (idle high for UART)
    setPinOutput(UART_TX_PIN);
    writePinHigh(UART_TX_PIN);
    
    // Small delay to ensure stable initialization
    wait_ms(10);
}