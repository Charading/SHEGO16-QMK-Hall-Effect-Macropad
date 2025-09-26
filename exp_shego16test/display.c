// Direct ST7735 display driver for QMK (ported from Arduino Adafruit_ST7735)
#include "config.h"
#include "display.h"

// Build-time switch: set to 0 to omit large GIF asset headers (helps avoid flash overflow)
#ifndef BUILD_WITH_GIFS
#define BUILD_WITH_GIFS 1
#endif

#if BUILD_WITH_GIFS
#include "gif/gif.h"  // Include the generated gif frames
#endif

// Animation speed multiplier (in hundredths)
// 100 = 1.0x (normal speed), 125 = 1.25x, 50 = 0.5x (half speed), 200 = 2.0x (double speed)
#define ANIMATION_SPEED_MULTIPLIER 75  // 0.75x speed (25% slower)

static bool display_initialized = false;
static bool animation_playing = false;
static uint16_t current_frame = 0;
static uint32_t last_frame_time = 0;

// Low-level SPI functions
static void spi_write_byte(uint8_t data) {
    // Bit-bang SPI for now since we removed quantum_painter
    for (int i = 7; i >= 0; i--) {
        writePin(TFT_SCLK, false);
        writePin(TFT_MOSI, (data >> i) & 1);
        writePin(TFT_SCLK, true);
    }
}

static void st7735_write_command(uint8_t cmd) {
    writePin(TFT_DC, false);  // Command mode
    writePin(TFT_CS, false);  // Select display
    spi_write_byte(cmd);
    writePin(TFT_CS, true);   // Deselect
}

static void st7735_write_data(uint8_t data) {
    writePin(TFT_DC, true);   // Data mode
    writePin(TFT_CS, false);  // Select display
    spi_write_byte(data);
    writePin(TFT_CS, true);   // Deselect
}

static void st7735_write_data_16(uint16_t data) {
    writePin(TFT_DC, true);   // Data mode
    writePin(TFT_CS, false);  // Select display
    spi_write_byte(data >> 8);    // High byte
    spi_write_byte(data & 0xFF);  // Low byte
    writePin(TFT_CS, true);   // Deselect
}

static void st7735_set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    st7735_write_command(ST7735_CASET); // Column addr set
    st7735_write_data_16(x0);
    st7735_write_data_16(x1);

    st7735_write_command(ST7735_RASET); // Row addr set  
    st7735_write_data_16(y0);
    st7735_write_data_16(y1);

    st7735_write_command(ST7735_RAMWR); // Write to RAM
}

// ST7735R initialization sequence (144 GREEN TAB)
static void st7735_init_r_144_green(void) {
    // Software reset
    st7735_write_command(ST7735_SWRESET);
    wait_ms(150);
    
    // Out of sleep mode
    st7735_write_command(ST7735_SLPOUT);
    wait_ms(500);
    
    // Frame rate control - normal mode
    st7735_write_command(ST7735_FRMCTR1);
    st7735_write_data(0x01);
    st7735_write_data(0x2C);
    st7735_write_data(0x2D);
    
    // Frame rate control - idle mode
    st7735_write_command(ST7735_FRMCTR2);
    st7735_write_data(0x01);
    st7735_write_data(0x2C);
    st7735_write_data(0x2D);
    
    // Frame rate control - partial mode
    st7735_write_command(ST7735_FRMCTR3);
    st7735_write_data(0x01); st7735_write_data(0x2C); st7735_write_data(0x2D);
    st7735_write_data(0x01); st7735_write_data(0x2C); st7735_write_data(0x2D);
    
    // Display inversion control
    st7735_write_command(ST7735_INVCTR);
    st7735_write_data(0x07);
    
    // Power control
    st7735_write_command(ST7735_PWCTR1);
    st7735_write_data(0xA2);
    st7735_write_data(0x02);
    st7735_write_data(0x84);
    
    st7735_write_command(ST7735_PWCTR2);
    st7735_write_data(0xC5);
    
    st7735_write_command(ST7735_PWCTR3);
    st7735_write_data(0x0A);
    st7735_write_data(0x00);
    
    st7735_write_command(ST7735_PWCTR4);
    st7735_write_data(0x8A);
    st7735_write_data(0x2A);
    
    st7735_write_command(ST7735_PWCTR5);
    st7735_write_data(0x8A);
    st7735_write_data(0xEE);
    
    // VCOM control
    st7735_write_command(ST7735_VMCTR1);
    st7735_write_data(0x0E);
    
    // Memory access control (rotation)
    st7735_write_command(ST7735_MADCTL);
    // MADCTL rotation options:
    // 0x00 -> 0° (normal)
    // 0x60 -> 90° clockwise (MV | MX)
    // 0xC0 -> 180° (MX | MY) 
    // 0xA0 -> 270° clockwise / 90° counter-clockwise (MV | MY)
    st7735_write_data(0xA0);  // Currently 270° clockwise

    // Color mode - 16-bit color
    st7735_write_command(ST7735_COLMOD);
    st7735_write_data(0x05);
    
    // Gamma correction
    st7735_write_command(ST7735_GMCTRP1);
    st7735_write_data(0x02); st7735_write_data(0x1c); st7735_write_data(0x07); st7735_write_data(0x12);
    st7735_write_data(0x37); st7735_write_data(0x32); st7735_write_data(0x29); st7735_write_data(0x2d);
    st7735_write_data(0x29); st7735_write_data(0x25); st7735_write_data(0x2B); st7735_write_data(0x39);
    st7735_write_data(0x00); st7735_write_data(0x01); st7735_write_data(0x03); st7735_write_data(0x10);
    
    st7735_write_command(ST7735_GMCTRN1);
    st7735_write_data(0x03); st7735_write_data(0x1d); st7735_write_data(0x07); st7735_write_data(0x06);
    st7735_write_data(0x2E); st7735_write_data(0x2C); st7735_write_data(0x29); st7735_write_data(0x2D);
    st7735_write_data(0x2E); st7735_write_data(0x2E); st7735_write_data(0x37); st7735_write_data(0x3F);
    st7735_write_data(0x00); st7735_write_data(0x00); st7735_write_data(0x02); st7735_write_data(0x10);
    
    // Normal display on
    st7735_write_command(ST7735_NORON);
    wait_ms(10);
    
    // Display on
    st7735_write_command(ST7735_DISPON);
    wait_ms(100);
    
    // Invert display (like your Arduino code)
    st7735_write_command(ST7735_INVON);
}

bool display_init(void) {
    if (display_initialized) return true;
    
    uprintf("ST7735 display_init: start\n");
    
    // Configure pins
    setPinOutput(TFT_CS);
    setPinOutput(TFT_DC);
    setPinOutput(TFT_RST);
    setPinOutput(TFT_SCLK);
    setPinOutput(TFT_MOSI);
    
    // Initial pin states
    writePin(TFT_CS, true);   // Deselect
    writePin(TFT_SCLK, false);
    writePin(TFT_MOSI, false);
    
    // Hardware reset
    writePin(TFT_RST, true);
    wait_ms(10);
    writePin(TFT_RST, false);
    wait_ms(10);
    writePin(TFT_RST, true);
    wait_ms(120);
    
    // Initialize with 144 GREEN TAB settings
    st7735_init_r_144_green();
    
    display_initialized = true;
    uprintf("ST7735 display_init: complete\n");
    
    return true;
}

void display_clear(void) {
    if (!display_initialized) return;
    
    st7735_set_addr_window(0, 0, TFT_WIDTH-1, TFT_HEIGHT-1);
    
    writePin(TFT_DC, true);   // Data mode
    writePin(TFT_CS, false);  // Select display
    
    for (uint16_t i = 0; i < TFT_WIDTH * TFT_HEIGHT; i++) {
        spi_write_byte(0x00); // Black - high byte
        spi_write_byte(0x00); // Black - low byte
    }
    
    writePin(TFT_CS, true);   // Deselect
}

void display_fill_rgb(uint8_t r, uint8_t g, uint8_t b) {
    if (!display_initialized) return;
    
    uint16_t color = RGB565(r, g, b);
    
    st7735_set_addr_window(0, 0, TFT_WIDTH-1, TFT_HEIGHT-1);
    
    writePin(TFT_DC, true);   // Data mode
    writePin(TFT_CS, false);  // Select display
    
    for (uint16_t i = 0; i < TFT_WIDTH * TFT_HEIGHT; i++) {
        spi_write_byte(color >> 8);     // High byte
        spi_write_byte(color & 0xFF);   // Low byte
    }
    
    writePin(TFT_CS, true);   // Deselect
}

// Function to swap red and blue channels in RGB565
static uint16_t swap_rb_rgb565(uint16_t color) {
    // Extract R, G, B components
    uint8_t r = (color >> 11) & 0x1F;  // 5 bits red
    uint8_t g = (color >> 5) & 0x3F;   // 6 bits green
    uint8_t b = color & 0x1F;          // 5 bits blue
    
    // Swap red and blue channels
    return (b << 11) | (g << 5) | r;   // BGR565 -> RGB565
}

void display_draw_rgb565_frame(const uint16_t *pixels, uint16_t w, uint16_t h) {
    if (!display_initialized || !pixels) return;
    
    st7735_set_addr_window(0, 0, w-1, h-1);
    
    writePin(TFT_DC, true);   // Data mode
    writePin(TFT_CS, false);  // Select display
    
    for (uint16_t i = 0; i < w * h; i++) {
        uint16_t pixel = pixels[i];
        spi_write_byte(pixel >> 8);     // High byte
        spi_write_byte(pixel & 0xFF);   // Low byte
    }
    
    writePin(TFT_CS, true);   // Deselect
}

void display_draw_rgb565_frame_rb_swapped(const uint16_t *pixels, uint16_t w, uint16_t h) {
    if (!display_initialized || !pixels) return;
    
    st7735_set_addr_window(0, 0, w-1, h-1);
    
    writePin(TFT_DC, true);   // Data mode
    writePin(TFT_CS, false);  // Select display
    
    for (uint16_t i = 0; i < w * h; i++) {
        uint16_t pixel = swap_rb_rgb565(pixels[i]);  // Swap red and blue channels
        spi_write_byte(pixel >> 8);     // High byte
        spi_write_byte(pixel & 0xFF);   // Low byte
    }
    
    writePin(TFT_CS, true);   // Deselect
}

void display_test_pattern(void) {
    if (!display_init()) return;
    
    // Fill red for test
    // display_fill_rgb(255, 0, 0); //this is actually blue because no inversion
    // wait_ms(500);
    
    // Start animation sequence: init gif (if exists) then main gif
    uprintf("ST7735: Starting animation test\n");
    display_start_animation();
}

// Optional init GIF support: check if init header exists
#if __has_include("gif/init.h")
#define HAVE_INIT_GIF_FILE 1
#include "gif/init.h"
#else
#define HAVE_INIT_GIF_FILE 0
#endif

// Simple container for a GIF dataset
typedef struct {
    const uint16_t *pixels;
    const uint32_t *offsets;
    const uint16_t *delays;
    uint32_t frames;
    uint16_t width;
    uint16_t height;
} gif_set_t;

// Main gif metadata (from gif/gif.h) - guarded so we can build without GIF assets
#if BUILD_WITH_GIFS
    #if defined(GIF_FRAMES)
    static const gif_set_t main_gif = {
        .pixels = gif_pixels,
        .offsets = gif_offsets,
        .delays = gif_delays,
        .frames = GIF_FRAMES,
        .width = GIF_W,
        .height = GIF_H,
    };
    #else
    static const gif_set_t main_gif = {0};
    #endif

    #if defined(INIT_GIF_FRAMES)
    static const gif_set_t init_gif = {
        .pixels = init_gif_pixels,
        .offsets = init_gif_offsets,
        .delays = init_gif_delays,
        .frames = INIT_GIF_FRAMES,
        .width = INIT_GIF_W,
        .height = INIT_GIF_H,
    };
    #define HAVE_INIT_GIF 1
    #else
    #define HAVE_INIT_GIF 0
    #endif
#else
    static const gif_set_t main_gif = {0};
    #define HAVE_INIT_GIF 0
#endif

// Current playback state
static const gif_set_t *cur_gif = NULL;
static bool cur_loop = false;
static bool init_phase = false; // true while playing init gif

// Helper: switch current gif and draw first frame
static void set_current_gif(const gif_set_t *gif, bool loop) {
#if BUILD_WITH_GIFS
    if (!gif || gif->frames == 0) {
        cur_gif = NULL;
        cur_loop = false;
        return;
    }
    cur_gif = gif;
    cur_loop = loop;
    current_frame = 0;
    last_frame_time = timer_read32();
    // Draw first frame immediately with appropriate color handling
    const uint16_t *pixels = &cur_gif->pixels[cur_gif->offsets[0]];
    
    if (init_phase) {
        // Init gif - normal colors
        display_draw_rgb565_frame(pixels, cur_gif->width, cur_gif->height);
    } else {
        // Main gif - swap red and blue channels
        display_draw_rgb565_frame_rb_swapped(pixels, cur_gif->width, cur_gif->height);
    }
#else
    // GIFs disabled: no-op
    (void)gif;
    (void)loop;
#endif
}

// Play init gif (if present) once, then switch to main loop
static void display_play_init_then_main(void) {
#if BUILD_WITH_GIFS
    #if HAVE_INIT_GIF
        init_phase = true;
        set_current_gif(&init_gif, false);
        animation_playing = true;
    #else
        init_phase = false;
        set_current_gif(&main_gif, true);
        animation_playing = true;
    #endif
#else
    // GIFs disabled: start no animation
    init_phase = false;
    cur_gif = NULL;
    animation_playing = false;
#endif
}

void display_start_animation(void) {
#if BUILD_WITH_GIFS
    // If an init GIF exists, play it once then main; otherwise start main loop
    if (HAVE_INIT_GIF) {
        display_play_init_then_main();
    } else {
        set_current_gif(&main_gif, true);
        animation_playing = true;
        uprintf("ST7735: animation started\n");
    }
#else
    uprintf("ST7735: GIFs disabled at build time; no animation started\n");
#endif
}

void display_stop_animation(void) {
    animation_playing = false;
    cur_gif = NULL;
    uprintf("ST7735: animation stopped\n");
}

void display_update_animation(void) {
#if !BUILD_WITH_GIFS
    // GIFs disabled at build time
    (void)animation_playing;
    (void)display_initialized;
    (void)cur_gif;
    return;
#endif
    if (!animation_playing || !display_initialized || !cur_gif) return;

    if (cur_gif->frames <= 1 && cur_loop) return; // nothing to do

    uint32_t now = timer_read32();
    uint16_t frame_delay = cur_gif->delays[current_frame];
    if (frame_delay == 0) frame_delay = 100; // Default 100ms
    
    // Apply speed multiplier (100 = normal speed, 50 = half speed, 200 = double speed)
    frame_delay = (frame_delay * 100) / ANIMATION_SPEED_MULTIPLIER;

    if (TIMER_DIFF_32(now, last_frame_time) >= frame_delay) {
        // advance frame
        current_frame++;
        if (current_frame >= cur_gif->frames) {
            if (cur_loop) {
                current_frame = 0;
            } else {
                // non-looping gif finished
                if (init_phase) {
                    // switch to main gif and loop
                    init_phase = false;
                    set_current_gif(&main_gif, true);
                    return;
                } else {
                    animation_playing = false;
                    return;
                }
            }
        }
        const uint16_t *pixels = &cur_gif->pixels[cur_gif->offsets[current_frame]];
        
        // Use red-blue channel swap for main gif, normal colors for init gif
        if (init_phase) {
            // Init gif - normal colors
            display_draw_rgb565_frame(pixels, cur_gif->width, cur_gif->height);
        } else {
            // Main gif - swap red and blue channels
            display_draw_rgb565_frame_rb_swapped(pixels, cur_gif->width, cur_gif->height);
        }
        
        last_frame_time = now;
    }
}
