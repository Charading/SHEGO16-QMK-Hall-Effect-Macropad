#pragma once

// Rotary encoder - CLK19, DT20, SW21

// RGB Matrix - 15 LEDs (no LED at encoder position [0,3], 10 Ambient LEDs)
#define WS2812_DI_PIN GP22
#define RGB_MATRIX_LED_COUNT 25

// Underglow - 10 ambient LEDs on GP9  
// #define RGBLIGHT_LED_COUNT 10
// #define WS2812_DI_PIN_RGBLIGHT GP9

#define DRIVER_LED_TOTAL 25

// Split configuration
#define WS2812_LED_COUNT_1 15  // Per-key LEDs on GP8
#define WS2812_LED_COUNT_2 10  // Ambient LEDs on GP9

#define MATRIX_ROWS 4
#define MATRIX_COLS 4

// RGB Matrix configuration (for per-key LEDs)

#define RGB_MATRIX_KEYPRESSES
#define RGB_MATRIX_KEYRELEASES
#define RGB_MATRIX_FRAMEBUFFER_EFFECTS

// Underglow configuration
#define RGBLIGHT_EFFECT_BREATHING
#define RGBLIGHT_EFFECT_RAINBOW_MOOD
#define RGBLIGHT_EFFECT_RAINBOW_SWIRL
#define RGBLIGHT_EFFECT_SNAKE
#define RGBLIGHT_EFFECT_KNIGHT
#define RGBLIGHT_EFFECT_CHRISTMAS
#define RGBLIGHT_EFFECT_STATIC_GRADIENT
#define RGBLIGHT_EFFECT_RGB_TEST
#define RGBLIGHT_EFFECT_ALTERNATING
#define RGBLIGHT_EFFECT_TWINKLE

// ST7735 SPI pin configuration (ensure these match your wiring)
#ifndef SPI_SCK
#define SPI_SCK GP2
#endif
#ifndef SPI_MOSI
#define SPI_MOSI GP3
#endif
#ifndef SPI_RES
#define SPI_RES GP4
#endif
#ifndef SPI_DC
#define SPI_DC GP5
#endif
#ifndef SPI_CS
#define SPI_CS GP24
#endif

// Display dimensions (ST7735R 128x128 panel)
#ifndef SPI_WIDTH
#define SPI_WIDTH 128
#endif
#ifndef SPI_HEIGHT
#define SPI_HEIGHT 128
#endif

// Debounce and performance
#define DEBOUNCE 5
#define USB_POLLING_INTERVAL_MS 1
