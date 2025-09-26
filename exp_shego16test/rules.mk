# MCU and bootloader
MCU = RP2040
BOOTLOADER = rp2040

# Features
ENCODER_ENABLE = yes
ENCODER_MAP_ENABLE = yes

# Optimizations
LTO_ENABLE = yes

VIA_ENABLE = yes
RAW_ENABLE = yes

# Use extended matrix scanning (not complete custom)
CUSTOM_MATRIX = lite
SRC += shego_adc.c display.c uart.c

# Enable analog for RP2040
ANALOG_DRIVER_REQUIRED = yes
ANALOG_DRIVER = rp2040_adc

# Dual WS2812 support
WS2812_DRIVER = vendor
RGB_MATRIX_ENABLE = yes
# RGBLIGHT_ENABLE = yes

# ST7735R Display support (direct SPI)
# Using custom ST7735 driver instead of quantum_painter

# Use the QMK analog driver
ANALOG_DRIVER = rp2040_adc

# Ignore warnings and errors
# ALLOW_WARNINGS = yes

# Debug features
# Enable console so display init can print diagnostic messages at runtime
CONSOLE_ENABLE = yes
# COMMAND_ENABLE = yes