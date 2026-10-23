/*
 * Noctis IV Plus - Linux port.
 * SDL2 + OpenGL platform layer.
 *
 * The game renders exactly like the DOS original: into a 320x200 frame
 * buffer with a 256 colour VGA palette. This layer converts the frame
 * through the palette and hands it to the GPU, which scales it to the
 * window with one of several filters (nearest, sharp bilinear, bilinear,
 * CRT scanlines). Without OpenGL the SDL software renderer is used.
 */

#include "platform.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>

/* ------------------------------------------------------------------ */
/* State                                                               */
/* ------------------------------------------------------------------ */

static PlatConfig g_cfg;
static SDL_Window *g_window = nullptr;
static SDL_GLContext g_gl = nullptr;
static SDL_Renderer *g_sdlren = nullptr;
static SDL_Texture *g_sdltex = nullptr;

static const uint8_t *g_vram = nullptr;
static uint8_t g_dac[768];          /* 6-bit palette as set by the game */
static uint32_t g_pal32[256];       /* converted to RGBA8               */
static uint32_t g_rgba[320 * 200];  /* frame converted through palette  */
static int g_dirty = 1;
static uint64_t g_last_present = 0;
static int g_quit = 0;

static int g_grabbed = 0;
static float g_mouse_acc_x = 0, g_mouse_acc_y = 0;
static int g_mouse_buttons = 0;     /* currently held                  */
static int g_mouse_clicked = 0;     /* pressed since last poll         */

#define KEYQ_SIZE 256
static int g_keyq[KEYQ_SIZE];
static int g_keyq_head = 0, g_keyq_tail = 0;

/* ------------------------------------------------------------------ */
/* OpenGL                                                              */
/* ------------------------------------------------------------------ */

typedef GLuint (APIENTRY *PFN_glCreateShader)(GLenum);
typedef void (APIENTRY *PFN_glShaderSource)(GLuint, GLsizei, const GLchar *const *, const GLint *);
typedef void (APIENTRY *PFN_glCompileShader)(GLuint);
typedef void (APIENTRY *PFN_glGetShaderiv)(GLuint, GLenum, GLint *);
typedef void (APIENTRY *PFN_glGetShaderInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
typedef GLuint (APIENTRY *PFN_glCreateProgram)(void);
typedef void (APIENTRY *PFN_glAttachShader)(GLuint, GLuint);
typedef void (APIENTRY *PFN_glBindAttribLocation)(GLuint, GLuint, const GLchar *);
typedef void (APIENTRY *PFN_glLinkProgram)(GLuint);
typedef void (APIENTRY *PFN_glGetProgramiv)(GLuint, GLenum, GLint *);
typedef void (APIENTRY *PFN_glUseProgram)(GLuint);
typedef GLint (APIENTRY *PFN_glGetUniformLocation)(GLuint, const GLchar *);
typedef void (APIENTRY *PFN_glUniform1i)(GLint, GLint);
typedef void (APIENTRY *PFN_glUniform2f)(GLint, GLfloat, GLfloat);
typedef void (APIENTRY *PFN_glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *);
typedef void (APIENTRY *PFN_glEnableVertexAttribArray)(GLuint);

static PFN_glCreateShader p_glCreateShader;
static PFN_glShaderSource p_glShaderSource;
static PFN_glCompileShader p_glCompileShader;
static PFN_glGetShaderiv p_glGetShaderiv;
static PFN_glGetShaderInfoLog p_glGetShaderInfoLog;
static PFN_glCreateProgram p_glCreateProgram;
static PFN_glAttachShader p_glAttachShader;
static PFN_glBindAttribLocation p_glBindAttribLocation;
static PFN_glLinkProgram p_glLinkProgram;
static PFN_glGetProgramiv p_glGetProgramiv;
static PFN_glUseProgram p_glUseProgram;
static PFN_glGetUniformLocation p_glGetUniformLocation;
static PFN_glUniform1i p_glUniform1i;
static PFN_glUniform2f p_glUniform2f;
static PFN_glVertexAttribPointer p_glVertexAttribPointer;
static PFN_glEnableVertexAttribArray p_glEnableVertexAttribArray;

static GLuint g_tex = 0;
static GLuint g_prog = 0;
static GLint g_u_tex = -1, g_u_src = -1, g_u_dst = -1, g_u_mode = -1;

static const char *vs_src =
	"#version 120\n"
	"attribute vec2 a_pos;\n"
	"attribute vec2 a_uv;\n"
	"varying vec2 v_uv;\n"
	"void main() {\n"
	"	v_uv = a_uv;\n"
	"	gl_Position = vec4(a_pos, 0.0, 1.0);\n"
	"}\n";

static const char *fs_src =
	"#version 120\n"
	"uniform sampler2D u_tex;\n"
	"uniform vec2 u_src;\n"   /* source size in pixels (320,200)  */
	"uniform vec2 u_dst;\n"   /* destination size in pixels       */
	"uniform int u_mode;\n"   /* PlatFilter                       */
	"varying vec2 v_uv;\n"
	"vec4 sharp(vec2 uv) {\n"
	"	vec2 texel = uv * u_src;\n"
	"	vec2 scale = max(floor(u_dst / u_src), vec2(1.0));\n"
	"	vec2 range = 0.5 - 0.5 / scale;\n"
	"	vec2 cdist = fract(texel) - 0.5;\n"
	"	vec2 f = (cdist - clamp(cdist, -range, range)) * scale + 0.5;\n"
	"	return texture2D(u_tex, (floor(texel) + f) / u_src);\n"
	"}\n"
	"void main() {\n"
	"	vec4 c;\n"
	"	if (u_mode == 0) {\n"
	"		c = texture2D(u_tex, (floor(v_uv * u_src) + 0.5) / u_src);\n"
	"	} else if (u_mode == 2) {\n"
	"		c = texture2D(u_tex, v_uv);\n"
	"	} else if (u_mode == 3) {\n"
	"		vec4 b = sharp(v_uv);\n"
	"		vec4 glow = texture2D(u_tex, v_uv + vec2(0.0, 0.5 / u_src.y))\n"
	"		          + texture2D(u_tex, v_uv - vec2(0.0, 0.5 / u_src.y));\n"
	"		float y = fract(v_uv.y * u_src.y);\n"
	"		float scan = 0.62 + 0.38 * pow(sin(3.14159265 * y), 1.6);\n"
	"		float x = fract(v_uv.x * u_src.x * 3.0);\n"
	"		vec3 mask = vec3(0.92) + 0.08 * vec3(x < 0.333, x >= 0.333 && x < 0.666, x >= 0.666);\n"
	"		c = vec4((b.rgb * scan + glow.rgb * 0.09) * mask * 1.12, 1.0);\n"
	"	} else {\n"
	"		c = sharp(v_uv);\n"
	"	}\n"
	"	gl_FragColor = vec4(c.rgb, 1.0);\n"
	"}\n";

static void *gl_proc(const char *name)
{
	return SDL_GL_GetProcAddress(name);
}

static int gl_load_shader_functions(void)
{
#define LOAD(fn) p_##fn = (PFN_##fn)gl_proc(#fn); if (!p_##fn) return 0
	LOAD(glCreateShader);
	LOAD(glShaderSource);
	LOAD(glCompileShader);
	LOAD(glGetShaderiv);
	LOAD(glGetShaderInfoLog);
	LOAD(glCreateProgram);
	LOAD(glAttachShader);
	LOAD(glBindAttribLocation);
	LOAD(glLinkProgram);
	LOAD(glGetProgramiv);
	LOAD(glUseProgram);
	LOAD(glGetUniformLocation);
	LOAD(glUniform1i);
	LOAD(glUniform2f);
	LOAD(glVertexAttribPointer);
	LOAD(glEnableVertexAttribArray);
#undef LOAD
	return 1;
}

static GLuint gl_compile(GLenum type, const char *src)
{
	GLuint s = p_glCreateShader(type);
	p_glShaderSource(s, 1, &src, nullptr);
	p_glCompileShader(s);
	GLint ok = 0;
	p_glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[1024];
		p_glGetShaderInfoLog(s, sizeof(log), nullptr, log);
		fprintf(stderr, "noctis: shader compile error: %s\n", log);
		return 0;
	}
	return s;
}

static int gl_setup(void)
{
	glGenTextures(1, &g_tex);
	glBindTexture(GL_TEXTURE_2D, g_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 320, 200, 0, GL_RGBA,
		     GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (gl_load_shader_functions()) {
		GLuint vs = gl_compile(GL_VERTEX_SHADER, vs_src);
		GLuint fs = gl_compile(GL_FRAGMENT_SHADER, fs_src);
		if (vs && fs) {
			g_prog = p_glCreateProgram();
			p_glAttachShader(g_prog, vs);
			p_glAttachShader(g_prog, fs);
			p_glBindAttribLocation(g_prog, 0, "a_pos");
			p_glBindAttribLocation(g_prog, 1, "a_uv");
			p_glLinkProgram(g_prog);
			GLint ok = 0;
			p_glGetProgramiv(g_prog, GL_LINK_STATUS, &ok);
			if (!ok) {
				fprintf(stderr, "noctis: shader link failed, using fixed pipeline\n");
				g_prog = 0;
			}
			else {
				g_u_tex = p_glGetUniformLocation(g_prog, "u_tex");
				g_u_src = p_glGetUniformLocation(g_prog, "u_src");
				g_u_dst = p_glGetUniformLocation(g_prog, "u_dst");
				g_u_mode = p_glGetUniformLocation(g_prog, "u_mode");
			}
		}
	}
	return 1;
}

/* Destination rectangle with the requested aspect ratio, centred. */
static void dest_rect(int ww, int wh, int *x, int *y, int *w, int *h)
{
	double aspect = g_cfg.aspect43 ? 4.0 / 3.0 : 320.0 / 200.0;
	int dw = ww, dh = (int)(ww / aspect + 0.5);
	if (dh > wh) {
		dh = wh;
		dw = (int)(wh * aspect + 0.5);
	}
	*w = dw;
	*h = dh;
	*x = (ww - dw) / 2;
	*y = (wh - dh) / 2;
}

static void convert_frame(void)
{
	if (!g_vram) return;
	for (int i = 0; i < 320 * 200; i++)
		g_rgba[i] = g_pal32[g_vram[i]];
}

static void gl_present(void)
{
	int ww, wh, x, y, w, h;
	SDL_GL_GetDrawableSize(g_window, &ww, &wh);
	dest_rect(ww, wh, &x, &y, &w, &h);

	glViewport(0, 0, ww, wh);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(x, y, w, h);

	glBindTexture(GL_TEXTURE_2D, g_tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 320, 200, GL_RGBA,
			GL_UNSIGNED_BYTE, g_rgba);
	GLint flt = (g_cfg.filter == PLAT_FILTER_NEAREST) ? GL_NEAREST : GL_LINEAR;
	if (!g_prog && g_cfg.filter != PLAT_FILTER_LINEAR) flt = GL_NEAREST;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, flt);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, flt);

	static const GLfloat quad[] = {
		/* x, y, u, v (v=0 is the first scanline) */
		-1, -1, 0, 1,
		+1, -1, 1, 1,
		-1, +1, 0, 0,
		+1, +1, 1, 0,
	};

	if (g_prog) {
		p_glUseProgram(g_prog);
		p_glUniform1i(g_u_tex, 0);
		p_glUniform2f(g_u_src, 320.0f, 200.0f);
		p_glUniform2f(g_u_dst, (float)w, (float)h);
		p_glUniform1i(g_u_mode, g_cfg.filter);
		p_glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), quad);
		p_glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), quad + 2);
		p_glEnableVertexAttribArray(0);
		p_glEnableVertexAttribArray(1);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	else {
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_TRIANGLE_STRIP);
		for (int i = 0; i < 4; i++) {
			glTexCoord2f(quad[i * 4 + 2], quad[i * 4 + 3]);
			glVertex2f(quad[i * 4 + 0], quad[i * 4 + 1]);
		}
		glEnd();
	}
	SDL_GL_SwapWindow(g_window);
}

static void sdl_present(void)
{
	SDL_UpdateTexture(g_sdltex, nullptr, g_rgba, 320 * 4);
	int ww, wh, x, y, w, h;
	SDL_GetRendererOutputSize(g_sdlren, &ww, &wh);
	dest_rect(ww, wh, &x, &y, &w, &h);
	SDL_Rect dst = { x, y, w, h };
	SDL_SetRenderDrawColor(g_sdlren, 0, 0, 0, 255);
	SDL_RenderClear(g_sdlren);
	SDL_RenderCopy(g_sdlren, g_sdltex, nullptr, &dst);
	SDL_RenderPresent(g_sdlren);
}

/* ------------------------------------------------------------------ */
/* Configuration                                                       */
/* ------------------------------------------------------------------ */

void plat_default_config(PlatConfig *cfg)
{
	cfg->fullscreen = 0;
	cfg->window_scale = 3;
	cfg->filter = PLAT_FILTER_SHARP;
	cfg->vsync = 1;
	cfg->software = 0;
	cfg->aspect43 = 1;
	cfg->mouse_sens = 1.0f;
	cfg->grab_mouse = 1;
}

static const char *filter_names[PLAT_FILTER_COUNT] = { "nearest", "sharp", "linear", "crt" };

static void usage(void)
{
	printf("Noctis IV Plus - Linux port\n\n"
	       "Usage: noctis [options]\n\n"
	       "  --fullscreen         start in fullscreen (Alt+Enter toggles)\n"
	       "  --window             start in a window\n"
	       "  --scale N            window size multiplier (default 3 = 960x720)\n"
	       "  --filter MODE        nearest | sharp | linear | crt (F11 cycles)\n"
	       "  --no-vsync           do not wait for the display refresh\n"
	       "  --software           do not use OpenGL\n"
	       "  --square-pixels      do not stretch 320x200 to 4:3\n"
	       "  --mouse-sens X       mouse sensitivity multiplier (default 1.0)\n"
	       "  --no-grab            do not capture the mouse (F12 toggles)\n"
	       "  --help               show this text\n\n"
	       "Environment: NOCTIS_DATA=<dir> selects the game directory.\n");
}

int plat_parse_args(PlatConfig *cfg, int argc, char **argv)
{
	for (int i = 1; i < argc; i++) {
		const char *a = argv[i];
		if (!strcmp(a, "--fullscreen")) cfg->fullscreen = 1;
		else if (!strcmp(a, "--window")) cfg->fullscreen = 0;
		else if (!strcmp(a, "--scale") && i + 1 < argc) {
			cfg->window_scale = atoi(argv[++i]);
			if (cfg->window_scale < 1) cfg->window_scale = 1;
			if (cfg->window_scale > 12) cfg->window_scale = 12;
		}
		else if (!strcmp(a, "--filter") && i + 1 < argc) {
			const char *m = argv[++i];
			for (int f = 0; f < PLAT_FILTER_COUNT; f++)
				if (!strcmp(m, filter_names[f])) cfg->filter = f;
		}
		else if (!strcmp(a, "--no-vsync")) cfg->vsync = 0;
		else if (!strcmp(a, "--software")) cfg->software = 1;
		else if (!strcmp(a, "--square-pixels")) cfg->aspect43 = 0;
		else if (!strcmp(a, "--mouse-sens") && i + 1 < argc) {
			cfg->mouse_sens = (float)atof(argv[++i]);
			if (cfg->mouse_sens <= 0) cfg->mouse_sens = 1;
		}
		else if (!strcmp(a, "--no-grab")) cfg->grab_mouse = 0;
		else if (!strcmp(a, "--help") || !strcmp(a, "-h")) {
			usage();
			return 0;
		}
		else {
			fprintf(stderr, "noctis: unknown option '%s' (try --help)\n", a);
		}
	}
	return 1;
}

/* ------------------------------------------------------------------ */
/* Init / shutdown                                                     */
/* ------------------------------------------------------------------ */

static void set_grab(int on)
{
	g_grabbed = on;
	SDL_SetRelativeMouseMode(on ? SDL_TRUE : SDL_FALSE);
	g_mouse_acc_x = g_mouse_acc_y = 0;
}

static void update_title(void)
{
	char t[160];
	snprintf(t, sizeof(t), "Noctis IV Plus  [%s%s]%s", filter_names[g_cfg.filter],
		 g_sdlren ? ", software" : ", OpenGL",
		 g_grabbed ? "" : "  - click to capture the mouse");
	SDL_SetWindowTitle(g_window, t);
}

int plat_init(const PlatConfig *cfg)
{
	g_cfg = *cfg;

	SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "0");
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
		fprintf(stderr, "noctis: SDL_Init failed: %s\n", SDL_GetError());
		return 0;
	}

	int w = 320 * g_cfg.window_scale;
	int h = (g_cfg.aspect43 ? 240 : 200) * g_cfg.window_scale;
	Uint32 flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
	if (g_cfg.fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	if (!g_cfg.software) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		g_window = SDL_CreateWindow("Noctis IV Plus", SDL_WINDOWPOS_CENTERED,
					    SDL_WINDOWPOS_CENTERED, w, h, flags | SDL_WINDOW_OPENGL);
		if (g_window) {
			g_gl = SDL_GL_CreateContext(g_window);
			if (!g_gl) {
				fprintf(stderr, "noctis: no OpenGL context (%s), using software rendering\n",
					SDL_GetError());
				SDL_DestroyWindow(g_window);
				g_window = nullptr;
			}
			else {
				if (g_cfg.vsync) {
					if (SDL_GL_SetSwapInterval(-1) != 0)
						SDL_GL_SetSwapInterval(1);
				}
				else
					SDL_GL_SetSwapInterval(0);
				gl_setup();
			}
		}
	}

	if (!g_window) {
		g_window = SDL_CreateWindow("Noctis IV Plus", SDL_WINDOWPOS_CENTERED,
					    SDL_WINDOWPOS_CENTERED, w, h, flags);
		if (!g_window) {
			fprintf(stderr, "noctis: cannot create window: %s\n", SDL_GetError());
			return 0;
		}
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,
			    g_cfg.filter == PLAT_FILTER_NEAREST ? "nearest" : "linear");
		g_sdlren = SDL_CreateRenderer(g_window, -1, g_cfg.vsync ? SDL_RENDERER_PRESENTVSYNC : 0);
		if (!g_sdlren) g_sdlren = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
		if (!g_sdlren) {
			fprintf(stderr, "noctis: cannot create renderer: %s\n", SDL_GetError());
			return 0;
		}
		g_sdltex = SDL_CreateTexture(g_sdlren, SDL_PIXELFORMAT_ABGR8888,
					     SDL_TEXTUREACCESS_STREAMING, 320, 200);
	}

	memset(g_dac, 0, sizeof(g_dac));
	for (int i = 0; i < 256; i++) g_pal32[i] = 0xFF000000u;

	if (g_cfg.grab_mouse) set_grab(1);
	update_title();
	return 1;
}

void plat_shutdown(void)
{
	if (g_gl) SDL_GL_DeleteContext(g_gl);
	if (g_sdltex) SDL_DestroyTexture(g_sdltex);
	if (g_sdlren) SDL_DestroyRenderer(g_sdlren);
	if (g_window) SDL_DestroyWindow(g_window);
	g_gl = nullptr;
	g_sdltex = nullptr;
	g_sdlren = nullptr;
	g_window = nullptr;
	SDL_Quit();
}

/* ------------------------------------------------------------------ */
/* Video                                                               */
/* ------------------------------------------------------------------ */

void plat_set_vram(const uint8_t *vram)
{
	g_vram = vram;
}

void plat_set_palette(const uint8_t *rgb6, int first, int count)
{
	for (int i = 0; i < count; i++) {
		int c = first + i;
		if (c < 0 || c > 255) continue;
		for (int k = 0; k < 3; k++)
			g_dac[c * 3 + k] = rgb6[i * 3 + k] & 0x3F;
		/* 6-bit DAC to 8-bit: replicate the high bits, like VGA monitors. */
		uint32_t r = (g_dac[c * 3 + 0] << 2) | (g_dac[c * 3 + 0] >> 4);
		uint32_t g = (g_dac[c * 3 + 1] << 2) | (g_dac[c * 3 + 1] >> 4);
		uint32_t b = (g_dac[c * 3 + 2] << 2) | (g_dac[c * 3 + 2] >> 4);
		g_pal32[c] = 0xFF000000u | (b << 16) | (g << 8) | r;
	}
	g_dirty = 1;
}

/* Testing aids (environment variables):
     NOCTIS_FRAMEDUMP=<dir>   write every 18th presented frame as PPM
     NOCTIS_FRAMEDUMP_EVERY=n dump every n-th frame instead
     NOCTIS_SCRIPT=<list>     inject input: "ms:key;ms:key" where key is a
                              character, ESC, ENTER, F1..F10, UP/DOWN/LEFT/RIGHT,
                              or QUIT */
static void frame_dump(void)
{
	static const char *dir = getenv("NOCTIS_FRAMEDUMP");
	static int counter = 0;
	static int every = getenv("NOCTIS_FRAMEDUMP_EVERY") ? atoi(getenv("NOCTIS_FRAMEDUMP_EVERY")) : 18;
	if (!dir) return;
	if (every < 1) every = 1;
	if (counter++ % every) return;
	char name[512];
	snprintf(name, sizeof(name), "%s/frame%05d.ppm", dir, counter / every);
	FILE *f = fopen(name, "wb");
	if (!f) return;
	fprintf(f, "P6\n320 200\n255\n");
	for (int i = 0; i < 320 * 200; i++) {
		uint32_t c = g_rgba[i];
		unsigned char px[3] = { (unsigned char)c, (unsigned char)(c >> 8), (unsigned char)(c >> 16) };
		fwrite(px, 1, 3, f);
	}
	fclose(f);
}

void plat_present(const uint8_t *vram)
{
	if (vram) g_vram = vram;
	if (!g_window) return;
	convert_frame();
	frame_dump();
	if (g_sdlren) sdl_present();
	else gl_present();
	g_dirty = 0;
	g_last_present = SDL_GetTicks64();
}

void plat_mark_dirty(void)
{
	g_dirty = 1;
}

/* ------------------------------------------------------------------ */
/* Keyboard translation to DOS/BIOS codes                              */
/* ------------------------------------------------------------------ */

static void keyq_push(int k)
{
	int next = (g_keyq_head + 1) % KEYQ_SIZE;
	if (next == g_keyq_tail) return; /* full, like the BIOS buffer */
	g_keyq[g_keyq_head] = k;
	g_keyq_head = next;
}

static void key_ext(int scan)
{
	keyq_push(0);
	keyq_push(scan);
}

struct KeyDef {
	SDL_Scancode sc;
	int bios;       /* BIOS scan code (used for Alt combinations) */
	char normal;
	char shifted;
};

/* US keyboard layout: the game was designed for it and its commands are
   letters, so keys are mapped by position regardless of the OS layout. */
static const KeyDef printable_keys[] = {
	{ SDL_SCANCODE_A, 0x1E, 'a', 'A' }, { SDL_SCANCODE_B, 0x30, 'b', 'B' },
	{ SDL_SCANCODE_C, 0x2E, 'c', 'C' }, { SDL_SCANCODE_D, 0x20, 'd', 'D' },
	{ SDL_SCANCODE_E, 0x12, 'e', 'E' }, { SDL_SCANCODE_F, 0x21, 'f', 'F' },
	{ SDL_SCANCODE_G, 0x22, 'g', 'G' }, { SDL_SCANCODE_H, 0x23, 'h', 'H' },
	{ SDL_SCANCODE_I, 0x17, 'i', 'I' }, { SDL_SCANCODE_J, 0x24, 'j', 'J' },
	{ SDL_SCANCODE_K, 0x25, 'k', 'K' }, { SDL_SCANCODE_L, 0x26, 'l', 'L' },
	{ SDL_SCANCODE_M, 0x32, 'm', 'M' }, { SDL_SCANCODE_N, 0x31, 'n', 'N' },
	{ SDL_SCANCODE_O, 0x18, 'o', 'O' }, { SDL_SCANCODE_P, 0x19, 'p', 'P' },
	{ SDL_SCANCODE_Q, 0x10, 'q', 'Q' }, { SDL_SCANCODE_R, 0x13, 'r', 'R' },
	{ SDL_SCANCODE_S, 0x1F, 's', 'S' }, { SDL_SCANCODE_T, 0x14, 't', 'T' },
	{ SDL_SCANCODE_U, 0x16, 'u', 'U' }, { SDL_SCANCODE_V, 0x2F, 'v', 'V' },
	{ SDL_SCANCODE_W, 0x11, 'w', 'W' }, { SDL_SCANCODE_X, 0x2D, 'x', 'X' },
	{ SDL_SCANCODE_Y, 0x15, 'y', 'Y' }, { SDL_SCANCODE_Z, 0x2C, 'z', 'Z' },
	{ SDL_SCANCODE_1, 0x78, '1', '!' }, { SDL_SCANCODE_2, 0x79, '2', '@' },
	{ SDL_SCANCODE_3, 0x7A, '3', '#' }, { SDL_SCANCODE_4, 0x7B, '4', '$' },
	{ SDL_SCANCODE_5, 0x7C, '5', '%' }, { SDL_SCANCODE_6, 0x7D, '6', '^' },
	{ SDL_SCANCODE_7, 0x7E, '7', '&' }, { SDL_SCANCODE_8, 0x7F, '8', '*' },
	{ SDL_SCANCODE_9, 0x80, '9', '(' }, { SDL_SCANCODE_0, 0x81, '0', ')' },
	{ SDL_SCANCODE_MINUS, 0x82, '-', '_' }, { SDL_SCANCODE_EQUALS, 0x83, '=', '+' },
	{ SDL_SCANCODE_LEFTBRACKET, 0x1A, '[', '{' }, { SDL_SCANCODE_RIGHTBRACKET, 0x1B, ']', '}' },
	{ SDL_SCANCODE_SEMICOLON, 0x27, ';', ':' }, { SDL_SCANCODE_APOSTROPHE, 0x28, '\'', '"' },
	{ SDL_SCANCODE_GRAVE, 0x29, '`', '~' }, { SDL_SCANCODE_BACKSLASH, 0x2B, '\\', '|' },
	{ SDL_SCANCODE_COMMA, 0x33, ',', '<' }, { SDL_SCANCODE_PERIOD, 0x34, '.', '>' },
	{ SDL_SCANCODE_SLASH, 0x35, '/', '?' }, { SDL_SCANCODE_SPACE, 0x39, ' ', ' ' },
};

static void toggle_fullscreen(void)
{
	Uint32 f = SDL_GetWindowFlags(g_window);
	if (f & SDL_WINDOW_FULLSCREEN_DESKTOP)
		SDL_SetWindowFullscreen(g_window, 0);
	else
		SDL_SetWindowFullscreen(g_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	g_dirty = 1;
}

static void translate_key(const SDL_Keysym *ks)
{
	SDL_Scancode sc = ks->scancode;
	Uint16 mod = ks->mod;
	int shift = (mod & KMOD_SHIFT) != 0;
	int ctrl = (mod & KMOD_CTRL) != 0;
	int alt = (mod & KMOD_ALT) != 0;
	int num = (mod & KMOD_NUM) != 0;
	int caps = (mod & KMOD_CAPS) != 0;

	/* Port specific shortcuts. */
	if (alt && (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_KP_ENTER)) {
		toggle_fullscreen();
		return;
	}
	if (sc == SDL_SCANCODE_F11) {
		g_cfg.filter = (g_cfg.filter + 1) % PLAT_FILTER_COUNT;
		if (g_sdlren)
			SDL_SetTextureScaleMode(g_sdltex, g_cfg.filter == PLAT_FILTER_NEAREST
						? SDL_ScaleModeNearest : SDL_ScaleModeLinear);
		update_title();
		g_dirty = 1;
		return;
	}
	if (sc == SDL_SCANCODE_F12) {
		set_grab(!g_grabbed);
		update_title();
		return;
	}

	/* Function keys F1..F10. */
	if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F10) {
		int n = sc - SDL_SCANCODE_F1;
		if (alt) key_ext(0x68 + n);
		else if (ctrl) key_ext(0x5E + n);
		else if (shift) key_ext(0x54 + n);
		else key_ext(0x3B + n);
		return;
	}

	/* Keypad. */
	switch (sc) {
	case SDL_SCANCODE_KP_PLUS:
		if (ctrl) key_ext(0x90); else keyq_push('+');
		return;
	case SDL_SCANCODE_KP_MINUS:
		if (ctrl) key_ext(0x8E); else keyq_push('-');
		return;
	case SDL_SCANCODE_KP_MULTIPLY:
		if (ctrl) key_ext(0x96); else keyq_push('*');
		return;
	case SDL_SCANCODE_KP_DIVIDE:
		if (ctrl) key_ext(0x95); else keyq_push('/');
		return;
	case SDL_SCANCODE_KP_ENTER:
		keyq_push(ctrl ? 10 : 13);
		return;
	default:
		break;
	}

	int kp_digit = -1, nav = -1;
	switch (sc) {
	case SDL_SCANCODE_KP_7: kp_digit = 7; nav = 0x47; break;
	case SDL_SCANCODE_KP_8: kp_digit = 8; nav = 0x48; break;
	case SDL_SCANCODE_KP_9: kp_digit = 9; nav = 0x49; break;
	case SDL_SCANCODE_KP_4: kp_digit = 4; nav = 0x4B; break;
	case SDL_SCANCODE_KP_5: kp_digit = 5; nav = 0x4C; break;
	case SDL_SCANCODE_KP_6: kp_digit = 6; nav = 0x4D; break;
	case SDL_SCANCODE_KP_1: kp_digit = 1; nav = 0x4F; break;
	case SDL_SCANCODE_KP_2: kp_digit = 2; nav = 0x50; break;
	case SDL_SCANCODE_KP_3: kp_digit = 3; nav = 0x51; break;
	case SDL_SCANCODE_KP_0: kp_digit = 0; nav = 0x52; break;
	case SDL_SCANCODE_KP_PERIOD: kp_digit = -2; nav = 0x53; break;
	case SDL_SCANCODE_HOME: nav = 0x47; break;
	case SDL_SCANCODE_UP: nav = 0x48; break;
	case SDL_SCANCODE_PAGEUP: nav = 0x49; break;
	case SDL_SCANCODE_LEFT: nav = 0x4B; break;
	case SDL_SCANCODE_RIGHT: nav = 0x4D; break;
	case SDL_SCANCODE_END: nav = 0x4F; break;
	case SDL_SCANCODE_DOWN: nav = 0x50; break;
	case SDL_SCANCODE_PAGEDOWN: nav = 0x51; break;
	case SDL_SCANCODE_INSERT: nav = 0x52; break;
	case SDL_SCANCODE_DELETE: nav = 0x53; break;
	default: break;
	}
	if (nav != -1) {
		if (kp_digit != -1 && (num != shift) && !ctrl && !alt) {
			keyq_push(kp_digit == -2 ? '.' : '0' + kp_digit);
			return;
		}
		if (ctrl) {
			static const struct { int n, c; } ctrl_nav[] = {
				{ 0x47, 0x77 }, { 0x48, 0x8D }, { 0x49, 0x84 }, { 0x4B, 0x73 },
				{ 0x4C, 0x8F }, { 0x4D, 0x74 }, { 0x4F, 0x75 }, { 0x50, 0x91 },
				{ 0x51, 0x76 }, { 0x52, 0x92 }, { 0x53, 0x93 },
			};
			for (auto &e : ctrl_nav)
				if (e.n == nav) { key_ext(e.c); return; }
		}
		if (nav == 0x4C) return;
		key_ext(nav);
		return;
	}

	switch (sc) {
	case SDL_SCANCODE_RETURN: keyq_push(ctrl ? 10 : 13); return;
	case SDL_SCANCODE_ESCAPE: keyq_push(27); return;
	case SDL_SCANCODE_BACKSPACE: keyq_push(ctrl ? 127 : 8); return;
	case SDL_SCANCODE_TAB:
		if (ctrl) key_ext(0x94);
		else if (shift) key_ext(0x0F);
		else keyq_push(9);
		return;
	default: break;
	}

	for (const KeyDef &k : printable_keys) {
		if (k.sc != sc) continue;
		int letter = (k.normal >= 'a' && k.normal <= 'z');
		if (alt) {
			if (letter || (k.normal >= '0' && k.normal <= '9') || k.normal == '-' || k.normal == '=')
				key_ext(k.bios);
			return;
		}
		if (ctrl) {
			if (letter) keyq_push(k.normal - 'a' + 1);
			else if (k.normal == '[') keyq_push(27);
			else if (k.normal == '\\') keyq_push(28);
			else if (k.normal == ']') keyq_push(29);
			else if (k.normal == '-') keyq_push(31);
			else if (k.normal == '=') key_ext(0x90); /* convenience: Ctrl+= acts as Ctrl+Gray+ */
			else if (k.normal == ' ') keyq_push(' ');
			return;
		}
		int up = shift;
		if (letter && caps) up = !up;
		keyq_push(up ? k.shifted : k.normal);
		return;
	}
}

/* ------------------------------------------------------------------ */
/* Events                                                              */
/* ------------------------------------------------------------------ */

static void key_ext(int scan);
static void keyq_push(int k);

static void run_script(void)
{
	static const char *script = getenv("NOCTIS_SCRIPT");
	static const char *pos = script;
	if (!pos || !*pos) return;
	long when = strtol(pos, nullptr, 10);
	if ((long)SDL_GetTicks64() < when) return;
	const char *colon = strchr(pos, ':');
	if (!colon) { pos = nullptr; return; }
	const char *end = strchr(colon, ';');
	std::string key(colon + 1, end ? end - colon - 1 : strlen(colon + 1));
	if (key == "ESC") keyq_push(27);
	else if (key == "ENTER") keyq_push(13);
	else if (key == "QUIT") g_quit = 1;
	else if (key == "UP") key_ext(0x48);
	else if (key == "DOWN") key_ext(0x50);
	else if (key == "LEFT") key_ext(0x4B);
	else if (key == "RIGHT") key_ext(0x4D);
	else if (key == "LCLICK") g_mouse_clicked |= 1;
	else if (key == "RCLICK") g_mouse_clicked |= 2;
	else if (key.rfind("MX", 0) == 0) g_mouse_acc_x += atoi(key.c_str() + 2);
	else if (key.rfind("MY", 0) == 0) g_mouse_acc_y += atoi(key.c_str() + 2);
	else if (key[0] == 'F' && key.size() > 1) key_ext(0x3B + atoi(key.c_str() + 1) - 1);
	else for (char c : key) keyq_push((unsigned char)c);
	pos = end ? end + 1 : nullptr;
}

void plat_pump(void)
{
	run_script();
	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		switch (ev.type) {
		case SDL_QUIT:
			g_quit = 1;
			break;
		case SDL_KEYDOWN:
			translate_key(&ev.key.keysym);
			break;
		case SDL_MOUSEMOTION:
			if (g_grabbed) {
				g_mouse_acc_x += ev.motion.xrel * g_cfg.mouse_sens;
				g_mouse_acc_y += ev.motion.yrel * g_cfg.mouse_sens;
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (!g_grabbed) {
				set_grab(1);
				update_title();
				break;
			}
			if (ev.button.button == SDL_BUTTON_LEFT) { g_mouse_buttons |= 1; g_mouse_clicked |= 1; }
			if (ev.button.button == SDL_BUTTON_RIGHT) { g_mouse_buttons |= 2; g_mouse_clicked |= 2; }
			if (ev.button.button == SDL_BUTTON_MIDDLE) { g_mouse_buttons |= 4; g_mouse_clicked |= 4; }
			break;
		case SDL_MOUSEBUTTONUP:
			if (ev.button.button == SDL_BUTTON_LEFT) g_mouse_buttons &= ~1;
			if (ev.button.button == SDL_BUTTON_RIGHT) g_mouse_buttons &= ~2;
			if (ev.button.button == SDL_BUTTON_MIDDLE) g_mouse_buttons &= ~4;
			break;
		case SDL_WINDOWEVENT:
			if (ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
				if (g_grabbed) {
					set_grab(0);
					update_title();
				}
				g_mouse_buttons = 0;
			}
			g_dirty = 1;
			break;
		default:
			break;
		}
	}

	/* Keep the window responsive while the game draws into video
	   memory directly or fades the palette. */
	uint64_t now = SDL_GetTicks64();
	if (g_dirty && now - g_last_present >= 15)
		plat_present(nullptr);
}

uint64_t plat_ticks_ms(void)
{
	return SDL_GetTicks64();
}

void plat_sleep_ms(int ms)
{
	SDL_Delay(ms);
}

int plat_key_available(void)
{
	return g_keyq_head != g_keyq_tail;
}

int plat_key_get(void)
{
	if (g_keyq_head == g_keyq_tail) return -1;
	int k = g_keyq[g_keyq_tail];
	g_keyq_tail = (g_keyq_tail + 1) % KEYQ_SIZE;
	return k;
}

void plat_mouse(int16_t *dx, int16_t *dy, int16_t *buttons)
{
	int ix = (int)g_mouse_acc_x;
	int iy = (int)g_mouse_acc_y;
	g_mouse_acc_x -= ix;
	g_mouse_acc_y -= iy;
	if (ix > 32767) ix = 32767;
	if (ix < -32768) ix = -32768;
	if (iy > 32767) iy = 32767;
	if (iy < -32768) iy = -32768;
	*dx = (int16_t)ix;
	*dy = (int16_t)iy;
	*buttons = (int16_t)(g_mouse_buttons | g_mouse_clicked);
	g_mouse_clicked = 0;
}

int plat_quit_requested(void)
{
	return g_quit;
}
