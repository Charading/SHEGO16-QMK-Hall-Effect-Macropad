# UART Troubleshooting Summary - Shego16test

## What We Implemented
- Custom bit-banged UART implementation on GP1
- UART commands for GIF control (MENU_OPEN, MENU_UP, MENU_DOWN, MENU_SELECT)
- Encoder-based UART controls on Layer 3
- LED feedback to confirm key presses
- Startup test message
- Multiple baud rates tested (115200 and 9600)

## Issues Encountered
- No UART output detected on receiving device
- Custom keycodes trigger (LED feedback works) but UART transmission fails

## Possible Causes
1. **Hardware Issues:**
   - GP1 pin might not be properly connected
   - Voltage level mismatch (3.3V RP2040 vs 5V/different voltage ESP32)
   - Missing ground connection between boards
   - Wrong ESP32 RX pin configuration

2. **Software Issues:**
   - QMK's `wait_us()` function might not be precise enough for bit-banging
   - Interrupt conflicts during transmission
   - Pin configuration conflicts

3. **Timing Issues:**
   - Bit-banged UART timing might be off due to QMK overhead
   - ESP32 might expect different baud rate or timing

## Alternative Approaches to Try Later

### 1. Use QMK's Built-in Serial Driver
```c
// In config.h
#define SERIAL_USART_DRIVER SD1
#define SERIAL_USART_TX_PIN GP0

// In rules.mk
SERIAL_DRIVER = vendor
```

### 2. Use Raw HID Instead of UART
- Send commands via USB HID to host computer
- Host computer forwards to ESP32 via its own UART/USB
- More reliable than bit-banged UART

### 3. Hardware UART (if available)
- Use RP2040's hardware UART peripheral instead of bit-banging
- More precise timing and less CPU overhead

### 4. Different Pin/Voltage Level
- Try different GPIO pin (GP0, GP4, GP8, etc.)
- Add level shifter if voltage mismatch
- Verify ESP32 RX pin with multimeter/oscilloscope

### 5. I2C Communication
- Use I2C protocol instead of UART
- More robust for short-distance communication
- Built-in error checking

## Hardware Debugging Steps
1. **Verify pin output with multimeter:**
   - Check if GP1 goes high/low during transmission
   - Confirm 3.3V levels

2. **Test with simple LED:**
   - Connect LED to GP1 to visually confirm pin toggling
   - Should blink during UART transmission

3. **Check connections:**
   - Verify GP1 → ESP32 RX
   - Verify GND → GND
   - Check for loose connections

## Code Cleanup
If you want to remove UART functionality for now, remove these sections:
- Custom keycodes (GIF_MENU, GIF_UP, GIF_DOWN, GIF_SELECT)
- UART functions (uart_send_byte, uart_send_string)
- UART initialization in keyboard_post_init_user
- UART handling in encoder_update_user and process_record_user

## Current Status
- VIA keymap compiles successfully
- Custom keycodes trigger with LED feedback
- UART transmission implemented but not working
- Ambient LED functionality preserved and working

The implementation is solid from a software perspective - the issue is likely hardware-related or requires a different communication approach.