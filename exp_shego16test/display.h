#pragma once

#include "quantum.h"

// ST7735 SPI control pins (matching your Arduino wiring)
#define TFT_CS    GP24  // SPI_CS
#define TFT_DC    GP5   // SPI_DC  
#define TFT_RST   GP4   // SPI_RES
#define TFT_SCLK  GP2   // SPI_SCK
#define TFT_MOSI  GP3   // SPI_MOSI

#define TFT_WIDTH  128
#define TFT_HEIGHT 128

// ST7735 commands
#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09
#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13
#define ST7735_INVOFF  0x20
#define ST7735_INVON   0x21
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_RAMRD   0x2E
#define ST7735_PTLAR   0x30
#define ST7735_COLMOD  0x3A
#define ST7735_MADCTL  0x36
#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6
#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5
#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC
#define ST7735_RDID4   0xDD
#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

// Color conversion
#define RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

// Display API
bool display_init(void);
void display_clear(void);
void display_fill_rgb(uint8_t r, uint8_t g, uint8_t b);
void display_draw_rgb565_frame(const uint16_t *pixels, uint16_t w, uint16_t h);
void display_test_pattern(void);

// Animation functions
void display_start_animation(void);
void display_stop_animation(void);
void display_update_animation(void);


