/* uart.h - simple UART helper for shego16test
 * Provides a small wrapper around the RP2040 UART peripheral
 * to send strings from the keyboard firmware to an attached ESP32.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void uart_init_and_welcome(void);
void uart_send_string(const char* str);

#ifdef __cplusplus
}
#endif
