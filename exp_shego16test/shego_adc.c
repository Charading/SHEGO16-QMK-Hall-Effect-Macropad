//shego_adc.c
#include QMK_KEYBOARD_H
#include "quantum.h"
#include "matrix.h"
#include "analog.h"
#include "wait.h"
#include "debug.h"
#include "timer.h"
#include "print.h"

// MUX control pins
#define MUX_S0 GP10
#define MUX_S1 GP11
#define MUX_S2 GP12
#define MUX_S3 GP13
#define MUX1_EN GP14
#define MUX2_EN GP15
#define MUX1_ADC_PIN GP29
#define MUX2_ADC_PIN GP28

// Hall effect thresholds - temporarily lowered for testing
static const uint16_t key_thresholds[32] = {
    // MUX1 thresholds (channels 0-15) - lowered to 400 for testing
    540, 540, 0, 0, 0, 0, 540, 540, 540, 540, 0, 0, 0, 0, 0, 540,
    // MUX2 thresholds (channels 0-15) - lowered to 400 for testing
    540, 540, 0, 0, 0, 0, 540, 540, 540, 540, 0, 0, 0, 0, 540, 540
};

// Map MUX channels to matrix positions
typedef struct {
    uint8_t row;
    uint8_t col;
    uint16_t keycode;
} key_mapping_t;

// MUX1 channel mappings
static const key_mapping_t mux1_keys[16] = {
    {0, 1, KC_2},   // CH0 -> he2 -> "2"
    {0, 0, KC_1},   // CH1 -> he3 -> "1" 
    {255, 255, KC_NO},  // CH2 -> unused
    {255, 255, KC_NO},  // CH3 -> unused
    {255, 255, KC_NO},  // CH4 -> unused
    {255, 255, KC_NO},  // CH5 -> unused
    {1, 0, KC_Q},   // CH6 -> he7 -> "Q"
    {1, 1, KC_W},   // CH7 -> he6 -> "W"
    {1, 2, KC_E},   // CH8 -> he5 -> "E"
    {1, 3, KC_R},   // CH9 -> he4 -> "R"
    {255, 255, KC_NO},  // CH10 -> unused
    {255, 255, KC_NO},  // CH11 -> unused
    {255, 255, KC_NO},  // CH12 -> unused
    {255, 255, KC_NO},  // CH13 -> unused
    {255, 255, KC_NO},  // CH14 -> unused
    {0, 2, KC_3}    // CH15 -> he1 -> "3"
};

// MUX2 channel mappings
static const key_mapping_t mux2_keys[16] = {
    {2, 1, KC_S},   // CH0 -> he10 -> "S"
    {2, 0, KC_A},   // CH1 -> he11 -> "A"
    {255, 255, KC_NO},  // CH2 -> unused
    {255, 255, KC_NO},  // CH3 -> unused
    {255, 255, KC_NO},  // CH4 -> unused
    {255, 255, KC_NO},  // CH5 -> unused
    {3, 0, KC_Z},   // CH6 -> he15 -> "Z"
    {3, 1, KC_X},   // CH7 -> he14 -> "X"
    {3, 2, KC_C},   // CH8 -> he13 -> "C"
    {3, 3, KC_V},   // CH9 -> he12 -> "V"
    {255, 255, KC_NO},  // CH10 -> unused
    {255, 255, KC_NO},  // CH11 -> unused
    {255, 255, KC_NO},  // CH12 -> unused
    {255, 255, KC_NO},  // CH13 -> unused
    {2, 3, KC_F},   // CH14 -> he8 -> "F"
    {2, 2, KC_D}    // CH15 -> he9 -> "D"
};

// Key state tracking
static bool key_pressed[32];
static uint32_t key_timer[32];
static matrix_row_t matrix_state[MATRIX_ROWS];

// SOCD state tracking for A and D keys
static bool a_pressed = false;
static bool d_pressed = false;
static bool a_was_last = false; // Track which was pressed last

// Debug counter to limit output
static uint32_t debug_counter = 0;

#define DEBOUNCE_MS 5
#define HYSTERESIS 20  // Hysteresis to prevent bouncing

// Debug output control
static bool debug_enabled = false;

// Helper for debug printing
#define debug_print(...) do { if (debug_enabled) uprintf(__VA_ARGS__); } while(0)

// Real ADC reading
static uint16_t read_adc_pin(pin_t pin) {
    return analogReadPin(pin);
}

static void select_mux_channel(uint8_t channel) {
    writePin(MUX_S0, (channel & 0x01) ? 1 : 0);
    writePin(MUX_S1, (channel & 0x02) ? 1 : 0);
    writePin(MUX_S2, (channel & 0x04) ? 1 : 0);
    writePin(MUX_S3, (channel & 0x08) ? 1 : 0);
    wait_us(50);
}

void matrix_init_custom(void) {
    // Setup MUX control pins
    setPinOutput(MUX_S0);
    setPinOutput(MUX_S1);
    setPinOutput(MUX_S2);
    setPinOutput(MUX_S3);
    setPinOutput(MUX1_EN);
    setPinOutput(MUX2_EN);
    
    // Disable both MUXes (active low)
    writePinHigh(MUX1_EN);
    writePinHigh(MUX2_EN);
    
    // Initialize ADC pins
    setPinInputHigh(MUX1_ADC_PIN);
    setPinInputHigh(MUX2_ADC_PIN);
    
    // Initialize state
    for (uint8_t i = 0; i < 32; i++) {
        key_pressed[i] = false;
        key_timer[i] = 0;
    }
    
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        matrix_state[row] = 0;
    }
    
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool changed = false;
    uint32_t now = timer_read32();
    
    // Clear matrix
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        current_matrix[row] = 0;
    }
    
    // Track raw A and D presses before SOCD cleaning
    bool raw_a_pressed = false;
    bool raw_d_pressed = false;
    
    // Debug output every 2000 scans to reduce spam
    bool debug_this_scan = (debug_enabled && (debug_counter % 2000 == 0));
    if (debug_this_scan) {
        debug_print("\n=== DEBUG SCAN %lu ===\n", debug_counter);
    }

    for (uint8_t ch = 0; ch < 16; ch++) {
        select_mux_channel(ch);
        
        // Read MUX1
        writePinLow(MUX1_EN);   // Enable MUX1
        writePinHigh(MUX2_EN);  // Disable MUX2
        wait_us(100);
        
        uint16_t adc1 = read_adc_pin(MUX1_ADC_PIN);
        uint8_t key_idx1 = ch;
        uint16_t threshold1 = key_thresholds[key_idx1];
        
        // Debug output for active channels
        if (debug_this_scan && threshold1 > 0 && mux1_keys[ch].keycode != KC_NO) {
            debug_print("MUX1 CH%d: ADC=%d, Thresh=%d, Key=%d\n", 
                        ch, adc1, threshold1, mux1_keys[ch].keycode);
        }

        // Process MUX1 key
        if (threshold1 > 0 && mux1_keys[ch].keycode != KC_NO && mux1_keys[ch].row != 255) {
            bool should_press = (adc1 > threshold1);

            // Detect raw A/D
            if (mux1_keys[ch].keycode == KC_A && should_press) raw_a_pressed = true;
            if (mux1_keys[ch].keycode == KC_D && should_press) raw_d_pressed = true;

            // Debug key state changes
            if (debug_this_scan && should_press != key_pressed[key_idx1]) {
                debug_print("MUX1 CH%d Key %d: %s (ADC=%d vs Thresh=%d)\n", 
                            ch, mux1_keys[ch].keycode, 
                            should_press ? "PRESS" : "RELEASE", adc1, threshold1);
            }

            // Debounce check
            if (timer_elapsed32(key_timer[key_idx1]) > DEBOUNCE_MS) {
                if (should_press != key_pressed[key_idx1]) {
                    key_pressed[key_idx1] = should_press;
                    key_timer[key_idx1] = now;
                    changed = true;
                }
            }
            
            // Only set non-A/D keys in the matrix here
            if (key_pressed[key_idx1] && mux1_keys[ch].keycode != KC_A && mux1_keys[ch].keycode != KC_D) {
                current_matrix[mux1_keys[ch].row] |= (1 << mux1_keys[ch].col);
            }
        }
        
        // Read MUX2
        writePinHigh(MUX1_EN);  // Disable MUX1
        writePinLow(MUX2_EN);   // Enable MUX2
        wait_us(100);
        
        uint16_t adc2 = read_adc_pin(MUX2_ADC_PIN);
        uint8_t key_idx2 = 16 + ch;
        uint16_t threshold2 = key_thresholds[key_idx2];
        
        // Debug output for active channels
        if (debug_this_scan && threshold2 > 0 && mux2_keys[ch].keycode != KC_NO) {
            debug_print("MUX2 CH%d: ADC=%d, Thresh=%d, Key=%d\n", 
                        ch, adc2, threshold2, mux2_keys[ch].keycode);
        }

        // Process MUX2 key
        if (threshold2 > 0 && mux2_keys[ch].keycode != KC_NO && mux2_keys[ch].row != 255) {
            bool should_press = (adc2 > threshold2);

            // Detect raw A/D
            if (mux2_keys[ch].keycode == KC_A && should_press) raw_a_pressed = true;
            if (mux2_keys[ch].keycode == KC_D && should_press) raw_d_pressed = true;

            // Debug key state changes
            if (debug_this_scan && should_press != key_pressed[key_idx2]) {
                debug_print("MUX2 CH%d Key %d: %s (ADC=%d vs Thresh=%d)\n", 
                            ch, mux2_keys[ch].keycode, 
                            should_press ? "PRESS" : "RELEASE", adc2, threshold2);
            }

            // Debounce check
            if (timer_elapsed32(key_timer[key_idx2]) > DEBOUNCE_MS) {
                if (should_press != key_pressed[key_idx2]) {
                    key_pressed[key_idx2] = should_press;
                    key_timer[key_idx2] = now;
                    changed = true;
                }
            }
            
            // Only set non-A/D keys in the matrix here
            if (key_pressed[key_idx2] && mux2_keys[ch].keycode != KC_A && mux2_keys[ch].keycode != KC_D) {
                current_matrix[mux2_keys[ch].row] |= (1 << mux2_keys[ch].col);
            }
        }
        
        // Disable both MUXes
        writePinHigh(MUX1_EN);
        writePinHigh(MUX2_EN);
    }
    
    // SOCD cleaning for A and D - Last Input Priority
    if (raw_a_pressed && raw_d_pressed) {
        // Both pressed - keep only the last one pressed
        if (a_was_last) {
            // A was pressed last, so only output A
            for (uint8_t ch = 0; ch < 16; ch++) {
                if (mux1_keys[ch].keycode == KC_A && key_pressed[ch]) {
                    current_matrix[mux1_keys[ch].row] |= (1 << mux1_keys[ch].col);
                }
                if (mux2_keys[ch].keycode == KC_A && key_pressed[16 + ch]) {
                    current_matrix[mux2_keys[ch].row] |= (1 << mux2_keys[ch].col);
                }
            }
        } else {
            // D was pressed last, so only output D
            for (uint8_t ch = 0; ch < 16; ch++) {
                if (mux1_keys[ch].keycode == KC_D && key_pressed[ch]) {
                    current_matrix[mux1_keys[ch].row] |= (1 << mux1_keys[ch].col);
                }
                if (mux2_keys[ch].keycode == KC_D && key_pressed[16 + ch]) {
                    current_matrix[mux2_keys[ch].row] |= (1 << mux2_keys[ch].col);
                }
            }
        }
    } else if (raw_a_pressed) {
        // Only A pressed
        for (uint8_t ch = 0; ch < 16; ch++) {
            if (mux1_keys[ch].keycode == KC_A && key_pressed[ch]) {
                current_matrix[mux1_keys[ch].row] |= (1 << mux1_keys[ch].col);
            }
            if (mux2_keys[ch].keycode == KC_A && key_pressed[16 + ch]) {
                current_matrix[mux2_keys[ch].row] |= (1 << mux2_keys[ch].col);
            }
        }
    } else if (raw_d_pressed) {
        // Only D pressed  
        for (uint8_t ch = 0; ch < 16; ch++) {
            if (mux1_keys[ch].keycode == KC_D && key_pressed[ch]) {
                current_matrix[mux1_keys[ch].row] |= (1 << mux1_keys[ch].col);
            }
            if (mux2_keys[ch].keycode == KC_D && key_pressed[16 + ch]) {
                current_matrix[mux2_keys[ch].row] |= (1 << mux2_keys[ch].col);
            }
        }
    }
    
    // Update SOCD state tracking
    if (raw_a_pressed && !a_pressed) {
        a_was_last = true; // A was just pressed
    }
    if (raw_d_pressed && !d_pressed) {
        a_was_last = false; // D was just pressed
    }
    
    a_pressed = raw_a_pressed;
    d_pressed = raw_d_pressed;
    
    return changed;
}