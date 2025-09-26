# UART Commands Implementation for Shego16test

This document explains the custom UART functionality added to the Shego16test keyboard for communicating with an ESP32 to control GIF playback.

## Overview

The keyboard now has custom keycodes on Layer 3 that send UART commands to an ESP32. The ESP32 can respond to these commands to control a GIF player interface.

## Hardware Configuration

### Wiring
- **QMK Side (RP2040)**: 
  - TX: GP1 → ESP32 RX pin
  - GND: Connect GND between boards

**Note**: Using GP1 for UART TX to avoid conflicts with RGB matrix pins and other peripherals.

### UART Settings
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None

## Layer Layout

### Layer 3 - UART Commands
```
┌─────────┬─────────┬─────────┬───┐
│GIF_MENU │ GIF_UP  │GIF_DOWN │ENC│
├─────────┼─────────┼─────────┼───┤
│GIF_SELECT│  ---   │   ---   │---│
├─────────┼─────────┼─────────┼───┤
│   ---   │   ---   │   ---   │---│
├─────────┼─────────┼─────────┼───┤
│   ---   │   ---   │   ---   │---│
└─────────┴─────────┴─────────┴───┘
```

## Custom Keycodes

| Keycode | UART Command Sent | Description |
|---------|-------------------|-------------|
| `GIF_MENU` | `MENU_OPEN\n` | Opens/toggles the GIF selection menu |
| `GIF_UP` | `MENU_UP\n` | Navigate up in menu |
| `GIF_DOWN` | `MENU_DOWN\n` | Navigate down in menu |
| `GIF_SELECT` | `MENU_SELECT\n` | Select current GIF |

## Layer Access

To access Layer 3 (UART commands):
- **From Layer 0**: Hold the bottom-right key (was KC_V, now MO(3))
- **From Layer 1**: Hold the bottom-right key (MO(3))

## ESP32 Expected Responses

According to the documentation provided, the ESP32 should send back these responses:

### Status Messages
- `MENU_OPENED` / `MENU_CLOSED` - Menu state changes
- `MENU_POS:3` - Current menu position (number)
- `GIF_SELECTED:filename.gif` - Selected GIF file
- `STATUS:MENU=OPEN,GIF=vyse.gif,POS=2,COUNT=4` - Full status
- `UNKNOWN_COMMAND` - For invalid commands

### How to Test

1. **Build and flash** the firmware to your RP2040
2. **Connect UART wires** between RP2040 and ESP32 as specified
3. **Access Layer 3** by holding the bottom-right key
4. **Press the UART command keys** and monitor the ESP32's serial output
5. **Verify** that the ESP32 receives the expected commands

## Code Files Modified

- `config.h` - Added UART pin definitions
- `rules.mk` - Enabled UART feature
- `keymaps/default/keymap.c` - Added custom keycodes and UART command handling

## Troubleshooting

### No UART Communication
1. Check wiring (TX/RX crossed correctly)
2. Verify baud rate matches on both sides (115200)
3. Ensure GND is connected between boards

### Commands Not Recognized
1. Check that ESP32 code is properly handling the command strings
2. Verify newline characters are being sent (`\n`)
3. Monitor ESP32 serial output for debugging

### Layer 3 Not Accessible
1. Confirm you're holding (not tapping) the MO(3) key
2. Check that the keymap compiled correctly
3. Verify layer switching is working with other layers first

## Future Enhancements

Consider adding:
- Status LED indicators based on ESP32 responses
- Additional UART commands (volume, play/pause, etc.)
- Two-way communication to display ESP32 status on keyboard
- Error handling for failed UART communication