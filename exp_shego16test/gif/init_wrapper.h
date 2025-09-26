// Wrapper for init.h to avoid symbol conflicts
#pragma once

// Save current definitions if they exist
#ifdef gif_pixels
#define SAVED_gif_pixels gif_pixels
#undef gif_pixels
#endif
#ifdef gif_offsets  
#define SAVED_gif_offsets gif_offsets
#undef gif_offsets
#endif
#ifdef gif_delays
#define SAVED_gif_delays gif_delays  
#undef gif_delays
#endif
#ifdef GIF_W
#define SAVED_GIF_W GIF_W
#undef GIF_W
#endif
#ifdef GIF_H
#define SAVED_GIF_H GIF_H
#undef GIF_H
#endif
#ifdef GIF_FRAMES
#define SAVED_GIF_FRAMES GIF_FRAMES
#undef GIF_FRAMES
#endif

// Include the init header which uses generic names
#include "init.h"

// Rename to init-specific names
static const uint16_t * const init_gif_pixels = gif_pixels;
static const uint32_t * const init_gif_offsets = gif_offsets;
static const uint16_t * const init_gif_delays = gif_delays;
#define INIT_GIF_W GIF_W
#define INIT_GIF_H GIF_H  
#define INIT_GIF_FRAMES GIF_FRAMES

// Clean up the generic names
#undef gif_pixels
#undef gif_offsets
#undef gif_delays
#undef GIF_W
#undef GIF_H
#undef GIF_FRAMES

// Restore original definitions if they existed
#ifdef SAVED_gif_pixels
#define gif_pixels SAVED_gif_pixels
#undef SAVED_gif_pixels
#endif
#ifdef SAVED_gif_offsets
#define gif_offsets SAVED_gif_offsets
#undef SAVED_gif_offsets  
#endif
#ifdef SAVED_gif_delays
#define gif_delays SAVED_gif_delays
#undef SAVED_gif_delays
#endif
#ifdef SAVED_GIF_W
#define GIF_W SAVED_GIF_W
#undef SAVED_GIF_W
#endif
#ifdef SAVED_GIF_H
#define GIF_H SAVED_GIF_H
#undef SAVED_GIF_H
#endif
#ifdef SAVED_GIF_FRAMES
#define GIF_FRAMES SAVED_GIF_FRAMES
#undef SAVED_GIF_FRAMES
#endif
