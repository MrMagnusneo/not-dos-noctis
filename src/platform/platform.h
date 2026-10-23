/*
 * Noctis IV Plus - Linux port.
 * Platform layer: window, OpenGL presentation, input and timing (SDL2).
 */

#ifndef NOCTIS_PLATFORM_H
#define NOCTIS_PLATFORM_H

#include <cstdint>

enum PlatFilter {
	PLAT_FILTER_NEAREST = 0, /* blocky pixels, integer-ish scaling          */
	PLAT_FILTER_SHARP,       /* sharp bilinear: crisp, no uneven pixels       */
	PLAT_FILTER_LINEAR,      /* plain bilinear (soft)                         */
	PLAT_FILTER_CRT,         /* scanlines + slight glow, like a VGA monitor  */
	PLAT_FILTER_COUNT
};

struct PlatConfig {
	int fullscreen;        /* 1 = desktop fullscreen                      */
	int window_scale;      /* window size = 320*scale x 240*scale (4:3)   */
	int filter;            /* PlatFilter                                  */
	int vsync;             /* 1 = synchronise presentation to the display */
	int software;          /* 1 = do not use OpenGL (SDL renderer)        */
	int aspect43;          /* 1 = stretch 320x200 to 4:3 like a CRT       */
	float mouse_sens;      /* multiplier applied to mouse motion          */
	int grab_mouse;        /* capture the mouse while the window is focused */
};

void plat_default_config(PlatConfig *cfg);
/* Parses command line options (--fullscreen, --scale N, --filter X, ...).
   Returns 0 if the program should exit (e.g. after --help). */
int plat_parse_args(PlatConfig *cfg, int argc, char **argv);

int plat_init(const PlatConfig *cfg);
void plat_shutdown(void);

/* Emulated VGA DAC: 6-bit RGB components, like port 3C9h. */
void plat_set_palette(const uint8_t *rgb6, int first, int count);
/* Shows the 320x200 8-bit frame buffer now. */
void plat_present(const uint8_t *vram);
/* Marks the frame buffer as modified (it will be shown by plat_pump). */
void plat_mark_dirty(void);
/* Processes window events and refreshes the screen when needed. */
void plat_pump(void);

uint64_t plat_ticks_ms(void);
void plat_sleep_ms(int ms);

/* Keyboard queue in DOS format (extended keys are 0 followed by a scan code). */
int plat_key_available(void);
int plat_key_get(void);

/* Mouse: accumulated motion since the last call and button state. */
void plat_mouse(int16_t *dx, int16_t *dy, int16_t *buttons);

/* Set when the user closes the window. */
int plat_quit_requested(void);

/* Pointer to the frame buffer the platform presents (set by the DOS layer). */
void plat_set_vram(const uint8_t *vram);

#endif
