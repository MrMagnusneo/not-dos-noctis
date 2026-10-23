/*
 * Noctis IV Plus - Linux port.
 * DOS / Borland C++ 3.1 compatibility layer.
 *
 * The original game was a 16-bit real-mode DOS program built with
 * Borland C++ 3.1. This header provides replacements for everything the
 * game took from DOS, the BIOS and the Borland runtime:
 *
 *   - a "conventional memory" arena that reproduces the layout of the
 *     Borland far heap (the renderer relies on segment:offset details);
 *   - low level file I/O (_open/_read/...) with DOS-style paths
 *     ("..\\DATA\\StarMap.BIN") resolved case-insensitively on Linux;
 *   - Borland's rand()/srand()/random() and float->int conversion rules,
 *     so that the procedural universe is identical to the DOS version;
 *   - the 18.2 Hz BIOS timer used by clock();
 *   - keyboard (INT 21h AH=08h/0Bh) and mouse (INT 33h) services.
 */

#ifndef NOCTIS_DOS_H
#define NOCTIS_DOS_H

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <climits>

/* ------------------------------------------------------------------ */
/* Memory                                                              */
/* ------------------------------------------------------------------ */

/* Initialise the emulated conventional memory. Must be called first. */
void dos_init_memory(void);

/* Borland farmalloc()/farfree() replacements. Blocks are laid out exactly
   like the Borland far heap: every block starts at a paragraph boundary
   plus 4 bytes (segment:0004), and consecutive allocations are adjacent. */
void *farmalloc(uint32_t size);
void farfree(void *block);

/* The emulated VGA frame buffer (segment A000h). 64000 bytes are visible,
   the rest is slack that absorbs out-of-range writes like real hardware. */
extern uint8_t *dos_vga_memory;

/* Returns the start of the 64K "segment" a huge (normalised) pointer
   belongs to, i.e. the pointer rounded down to a paragraph boundary
   relative to the arena. Used where the original code loaded a
   normalised segment register (e.g. FS for texture fetches). */
uint8_t *dos_paragraph_base(const void *p);

/* ------------------------------------------------------------------ */
/* Borland arithmetic helpers                                          */
/* ------------------------------------------------------------------ */

/* Borland converted floating point values to integers through FISTP into
   a 64-bit temporary (truncating), then took the low bits. */
static inline int64_t bc_ftol(double d)
{
	if (d != d) return INT64_MIN;
	if (d >= 9.2233720368547758e18 || d <= -9.2233720368547758e18)
		return INT64_MIN;
	return (int64_t)d;
}

/* x87 FISTP with the default rounding mode (round to nearest even). */
static inline int32_t fistp32(double d)
{
	if (d != d) return INT32_MIN;
	if (d >= 2147483647.5 || d < -2147483648.5) return INT32_MIN;
	return (int32_t)nearbyint(d);
}

static inline int16_t fistp16(double d)
{
	if (d != d) return INT16_MIN;
	if (d >= 32767.5 || d < -32768.5) return INT16_MIN;
	return (int16_t)nearbyint(d);
}

template <typename T> static inline int16_t to_i16(T v) { return (int16_t)v; }
template <> inline int16_t to_i16<float>(float v) { return (int16_t)bc_ftol(v); }
template <> inline int16_t to_i16<double>(double v) { return (int16_t)bc_ftol(v); }
template <typename T> static inline uint16_t to_u16(T v) { return (uint16_t)v; }
template <> inline uint16_t to_u16<float>(float v) { return (uint16_t)bc_ftol(v); }
template <> inline uint16_t to_u16<double>(double v) { return (uint16_t)bc_ftol(v); }
template <typename T> static inline int32_t to_i32(T v) { return (int32_t)v; }
template <> inline int32_t to_i32<float>(float v) { return (int32_t)bc_ftol(v); }
template <> inline int32_t to_i32<double>(double v) { return (int32_t)bc_ftol(v); }

/* Borland C++ 3.1 runtime random generator (LCG 22695477 * s + 1). */
void bc_srand(uint16_t seed);
int16_t bc_rand(void);
static inline int16_t bc_random(int16_t num)
{
	return (int16_t)(((int32_t)bc_rand() * (int32_t)num) / 32768);
}

/* random() and srand() accept any arithmetic type and convert it the way
   Borland did when passing it to an "int" / "unsigned" parameter. */
template <typename T> static inline int16_t random(T num) { return bc_random(to_i16(num)); }
template <typename T> static inline void bc_srand_any(T seed) { bc_srand(to_u16(seed)); }

/* ------------------------------------------------------------------ */
/* Files                                                               */
/* ------------------------------------------------------------------ */

/* Root directory of the game installation (contains data/, gallery/...). */
const char *dos_game_root(void);
/* Locate the game root, make it the base for DOS paths and chdir into
   <root>/modules like the original executable expected. */
int dos_setup_paths(const char *argv0);

/* Translates a DOS path (relative to the "modules" directory, with
   backslashes, any letter case) to an existing Linux path when possible.
   The result is written to 'out'. */
void dos_resolve_path(const char *dospath, char *out, size_t outlen);

int dos_open(const char *path, int mode);
int dos_creat(const char *path, int attrib);
int dos_read(int fh, void *buf, uint32_t len);
int dos_write(int fh, const void *buf, uint32_t len);
int dos_close(int fh);
int32_t dos_lseek(int fh, int32_t offset, int whence);
int32_t dos_tell(int fh);
int32_t dos_filelength(int fh);
int dos_eof(int fh);
int dos_remove(const char *path);
int dos_mkdir(const char *path);
FILE *dos_fopen(const char *path, const char *mode);

/* ------------------------------------------------------------------ */
/* Strings (Borland extensions)                                        */
/* ------------------------------------------------------------------ */

char *dos_strupr(char *s);
int dos_memicmp(const void *a, const void *b, size_t n);
char *dos_gcvt(double value, int digits, char *buf);

/* ------------------------------------------------------------------ */
/* Timer, keyboard, mouse                                              */
/* ------------------------------------------------------------------ */

/* BIOS tick counter, 18.2065 ticks per second (Borland clock()). */
int32_t dos_clock(void);
/* Blocks until the BIOS tick counter changes (keeps the window alive). */
void dos_wait_tick(void);
/* Local date and time, as read from the BIOS RTC. */
void dos_get_datetime(int *year, int *month, int *day,
		      int *hour, int *minute, int *second);

/* INT 21h AH=0Bh: 1 if a key is waiting. */
int dos_kbhit(void);
/* INT 21h AH=08h: read a key; extended keys return 0, then the scancode. */
int dos_getkey(void);

/* INT 33h AX=0Bh (motion counters) and AX=05h (button status). */
void dos_mouse_read(int16_t *dx, int16_t *dy, int16_t *buttons);

#endif
