/*
 * Noctis IV Plus - Linux port.
 * DOS / Borland C++ 3.1 compatibility layer (implementation).
 */

#include "dos.h"
#include "../platform/platform.h"

#include <cerrno>
#include <cctype>
#include <ctime>
#include <string>
#include <vector>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

/* ------------------------------------------------------------------ */
/* Memory arena                                                        */
/* ------------------------------------------------------------------ */

static uint8_t *arena_raw = nullptr;
static uint8_t *arena = nullptr;      /* paragraph aligned base */
static const uint32_t ARENA_SIZE = 1024 * 1024;
static uint32_t arena_top = 0;        /* next free paragraph offset */
uint8_t *dos_vga_memory = nullptr;

void dos_init_memory(void)
{
	if (arena) return;
	arena_raw = (uint8_t *)calloc(ARENA_SIZE + 64, 1);
	if (!arena_raw) {
		fprintf(stderr, "noctis: out of memory\n");
		exit(1);
	}
	arena = (uint8_t *)(((uintptr_t)arena_raw + 15) & ~(uintptr_t)15);
	/* Leave a guard area before the first block: some routines read a
	   few bytes before the start of their buffers. */
	arena_top = 4096;
	/* Video memory (A000:0000) lives at a paragraph boundary with
	   offset 0, exactly like the real thing. */
	dos_vga_memory = arena + arena_top;
	arena_top += 65536 + 4096;
}

void *farmalloc(uint32_t size)
{
	dos_init_memory();
	uint32_t need = (size + 4 + 15) & ~15u;
	if (arena_top + need + 65536 > ARENA_SIZE) return nullptr;
	uint8_t *p = arena + arena_top + 4;
	arena_top += need;
	memset(p, 0, size);
	return p;
}

void farfree(void *block)
{
	/* Blocks are never reused: the game allocates everything once. */
	(void)block;
}

uint8_t *dos_paragraph_base(const void *p)
{
	const uint8_t *b = (const uint8_t *)p;
	if (arena && b >= arena && b < arena + ARENA_SIZE)
		return (uint8_t *)(b - ((b - arena) & 15));
	return (uint8_t *)(b - ((uintptr_t)b & 15));
}

/* ------------------------------------------------------------------ */
/* Borland random generator                                            */
/* ------------------------------------------------------------------ */

static uint32_t bc_seed = 1;

void bc_srand(uint16_t seed)
{
	bc_seed = seed;
}

int16_t bc_rand(void)
{
	bc_seed = 0x015A4E35u * bc_seed + 1u;
	return (int16_t)((bc_seed >> 16) & 0x7FFF);
}

/* ------------------------------------------------------------------ */
/* Paths                                                               */
/* ------------------------------------------------------------------ */

static std::string g_root;

const char *dos_game_root(void)
{
	return g_root.c_str();
}

static bool is_dir(const std::string &p)
{
	struct stat st;
	return stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

static bool is_file(const std::string &p)
{
	struct stat st;
	return stat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

/* Finds 'name' inside 'dir' ignoring case. Returns the real name, or
   the requested name if nothing matches. */
static std::string match_ci(const std::string &dir, const std::string &name)
{
	std::string probe = dir.empty() ? name : dir + "/" + name;
	struct stat st;
	if (lstat(probe.c_str(), &st) == 0) return name;
	DIR *d = opendir(dir.empty() ? "." : dir.c_str());
	if (!d) return name;
	std::string found = name;
	while (struct dirent *e = readdir(d)) {
		if (strcasecmp(e->d_name, name.c_str()) == 0) {
			found = e->d_name;
			break;
		}
	}
	closedir(d);
	return found;
}

void dos_resolve_path(const char *dospath, char *out, size_t outlen)
{
	std::string path(dospath);
	for (char &c : path)
		if (c == '\\') c = '/';
	/* Drive letters are meaningless here. */
	if (path.size() >= 2 && path[1] == ':') path = path.substr(2);

	std::string result;
	bool absolute = !path.empty() && path[0] == '/';
	if (absolute) result = "";
	size_t i = absolute ? 1 : 0;
	std::string dir = absolute ? "/" : "";
	while (i <= path.size()) {
		size_t j = path.find('/', i);
		if (j == std::string::npos) j = path.size();
		std::string comp = path.substr(i, j - i);
		if (!comp.empty()) {
			std::string real = (comp == "." || comp == "..") ? comp : match_ci(dir, comp);
			if (dir.empty()) dir = real;
			else if (dir == "/") dir = "/" + real;
			else dir += "/" + real;
		}
		i = j + 1;
	}
	if (dir.empty()) dir = ".";
	snprintf(out, outlen, "%s", dir.c_str());
}

static bool dir_writable(const std::string &d)
{
	return access(d.c_str(), W_OK) == 0;
}

static bool valid_root(const std::string &r)
{
	if (!is_dir(r)) return false;
	std::string data = r + "/" + match_ci(r, "data");
	if (!is_dir(data)) return false;
	return is_file(data + "/" + match_ci(data, "supports.nct"));
}

static bool copy_file(const std::string &src, const std::string &dst)
{
	FILE *in = fopen(src.c_str(), "rb");
	if (!in) return false;
	FILE *out = fopen(dst.c_str(), "wb");
	if (!out) {
		fclose(in);
		return false;
	}
	char buf[65536];
	size_t n;
	bool ok = true;
	while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
		if (fwrite(buf, 1, n, out) != n) ok = false;
	fclose(in);
	if (fclose(out) != 0) ok = false;
	return ok;
}

static void mkdirs(const std::string &p)
{
	std::string cur;
	for (size_t i = 0; i < p.size(); i++) {
		cur += p[i];
		if (p[i] == '/' && i > 0) mkdir(cur.c_str(), 0755);
	}
	mkdir(p.c_str(), 0755);
}

/* Copies the pristine game data into a writable per-user directory. */
static std::string make_user_root(const std::string &src_root)
{
	const char *xdg = getenv("XDG_DATA_HOME");
	const char *home = getenv("HOME");
	std::string base;
	if (xdg && *xdg) base = xdg;
	else if (home && *home) base = std::string(home) + "/.local/share";
	else return src_root;
	std::string root = base + "/noctis-iv-plus";
	std::string data = root + "/data";
	mkdirs(data);
	std::string src_data = src_root + "/" + match_ci(src_root, "data");
	DIR *d = opendir(src_data.c_str());
	if (d) {
		while (struct dirent *e = readdir(d)) {
			if (e->d_name[0] == '.') continue;
			std::string from = src_data + "/" + e->d_name;
			if (!is_file(from)) continue;
			std::string lower = e->d_name;
			for (char &c : lower) c = (char)tolower((unsigned char)c);
			std::string to = data + "/" + match_ci(data, lower);
			if (!is_file(to)) copy_file(from, to);
		}
		closedir(d);
	}
	return root;
}

int dos_setup_paths(const char *argv0)
{
	std::vector<std::string> candidates;
	const char *env = getenv("NOCTIS_DATA");
	if (env && *env) candidates.push_back(env);

	char exe[PATH_MAX];
	ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
	std::string exedir;
	if (n > 0) {
		exe[n] = 0;
		exedir = exe;
	}
	else if (argv0) {
		exedir = argv0;
	}
	size_t slash = exedir.rfind('/');
	exedir = (slash == std::string::npos) ? "." : exedir.substr(0, slash);

	candidates.push_back(exedir);
	candidates.push_back(exedir + "/..");
	candidates.push_back(exedir + "/../share/noctis-iv-plus");
#ifdef NOCTIS_DATADIR
	candidates.push_back(NOCTIS_DATADIR);
#endif
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd))) {
		candidates.push_back(cwd);
		candidates.push_back(std::string(cwd) + "/..");
	}

	std::string root;
	for (const std::string &c : candidates) {
		if (valid_root(c)) {
			root = c;
			break;
		}
	}
	if (root.empty()) {
		fprintf(stderr,
			"noctis: game data not found.\n"
			"        Expected a directory containing data/SUPPORTS.NCT,\n"
			"        data/STARMAP.BIN and data/GUIDE.BIN next to the executable,\n"
			"        or set NOCTIS_DATA=<directory>.\n");
		return 0;
	}

	char real[PATH_MAX];
	if (realpath(root.c_str(), real)) root = real;

	std::string data = root + "/" + match_ci(root, "data");
	if (!dir_writable(root) || !dir_writable(data))
		root = make_user_root(root);

	g_root = root;
	mkdir((root + "/" + match_ci(root, "modules")).c_str(), 0755);
	mkdir((root + "/" + match_ci(root, "gallery")).c_str(), 0755);
	mkdir((root + "/" + match_ci(root, "movies")).c_str(), 0755);
	std::string modules = root + "/" + match_ci(root, "modules");
	if (chdir(modules.c_str()) != 0) {
		fprintf(stderr, "noctis: cannot enter %s: %s\n", modules.c_str(), strerror(errno));
		return 0;
	}
	return 1;
}

/* ------------------------------------------------------------------ */
/* Files                                                               */
/* ------------------------------------------------------------------ */

#define MAX_HANDLES 64
static FILE *handles[MAX_HANDLES];

static int alloc_handle(FILE *f)
{
	for (int i = 5; i < MAX_HANDLES; i++) {
		if (!handles[i]) {
			handles[i] = f;
			return i;
		}
	}
	fclose(f);
	return -1;
}

static FILE *get_handle(int fh)
{
	if (fh < 0 || fh >= MAX_HANDLES) return nullptr;
	return handles[fh];
}

int dos_open(const char *path, int mode)
{
	char real[PATH_MAX];
	dos_resolve_path(path, real, sizeof(real));
	if (is_dir(real)) return -1;
	const char *m = "rb";
	/* Borland: O_RDONLY = 1, O_WRONLY = 2, O_RDWR = 4 (0 = read). */
	if (mode & 4 || mode & 2) m = "r+b";
	FILE *f = fopen(real, m);
	if (!f) return -1;
	return alloc_handle(f);
}

int dos_creat(const char *path, int attrib)
{
	(void)attrib;
	char real[PATH_MAX];
	dos_resolve_path(path, real, sizeof(real));
	FILE *f = fopen(real, "w+b");
	if (!f) return -1;
	return alloc_handle(f);
}

int dos_read(int fh, void *buf, uint32_t len)
{
	FILE *f = get_handle(fh);
	if (!f) return -1;
	fseek(f, 0, SEEK_CUR); /* allow switching between read and write */
	return (int)fread(buf, 1, len, f);
}

int dos_write(int fh, const void *buf, uint32_t len)
{
	FILE *f = get_handle(fh);
	if (!f) return -1;
	fseek(f, 0, SEEK_CUR);
	return (int)fwrite(buf, 1, len, f);
}

int dos_close(int fh)
{
	FILE *f = get_handle(fh);
	if (!f) return -1;
	fclose(f);
	handles[fh] = nullptr;
	return 0;
}

int32_t dos_lseek(int fh, int32_t offset, int whence)
{
	FILE *f = get_handle(fh);
	if (!f) return -1;
	if (fseek(f, offset, whence) != 0) return -1;
	return (int32_t)ftell(f);
}

int32_t dos_tell(int fh)
{
	FILE *f = get_handle(fh);
	if (!f) return -1;
	return (int32_t)ftell(f);
}

int32_t dos_filelength(int fh)
{
	FILE *f = get_handle(fh);
	if (!f) return -1;
	long cur = ftell(f);
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, cur, SEEK_SET);
	return (int32_t)len;
}

int dos_eof(int fh)
{
	FILE *f = get_handle(fh);
	if (!f) return 1;
	long cur = ftell(f);
	return cur >= dos_filelength(fh);
}

int dos_remove(const char *path)
{
	char real[PATH_MAX];
	dos_resolve_path(path, real, sizeof(real));
	return remove(real);
}

int dos_mkdir(const char *path)
{
	char real[PATH_MAX];
	dos_resolve_path(path, real, sizeof(real));
	return mkdir(real, 0755);
}

FILE *dos_fopen(const char *path, const char *mode)
{
	char real[PATH_MAX];
	dos_resolve_path(path, real, sizeof(real));
	return fopen(real, mode);
}

/* ------------------------------------------------------------------ */
/* Strings                                                             */
/* ------------------------------------------------------------------ */

char *dos_strupr(char *s)
{
	for (char *p = s; *p; p++)
		if (*p >= 'a' && *p <= 'z') *p -= 32;
	return s;
}

int dos_memicmp(const void *a, const void *b, size_t n)
{
	const unsigned char *x = (const unsigned char *)a;
	const unsigned char *y = (const unsigned char *)b;
	for (size_t i = 0; i < n; i++) {
		int cx = tolower(x[i]), cy = tolower(y[i]);
		if (cx != cy) return cx - cy;
	}
	return 0;
}

char *dos_gcvt(double value, int digits, char *buf)
{
	sprintf(buf, "%.*g", digits, value);
	return buf;
}

/* ------------------------------------------------------------------ */
/* Timer                                                               */
/* ------------------------------------------------------------------ */

static uint64_t last_pump = 0;

static void pump_throttled(void)
{
	uint64_t now = plat_ticks_ms();
	if (now - last_pump >= 2) {
		last_pump = now;
		plat_pump();
	}
}

int32_t dos_clock(void)
{
	pump_throttled();
	/* 1193182 Hz PIT / 65536 = 18.2065 ticks per second. */
	return (int32_t)((plat_ticks_ms() * 1193182ull) / (65536ull * 1000ull));
}

void dos_wait_tick(void)
{
	int32_t t = dos_clock();
	while (dos_clock() == t) {
		if (plat_quit_requested()) return;
		plat_sleep_ms(1);
	}
}

void dos_get_datetime(int *year, int *month, int *day,
		      int *hour, int *minute, int *second)
{
	time_t t = time(nullptr);
	struct tm lt;
	localtime_r(&t, &lt);
	*year = lt.tm_year + 1900;
	*month = lt.tm_mon + 1;
	*day = lt.tm_mday;
	*hour = lt.tm_hour;
	*minute = lt.tm_min;
	*second = lt.tm_sec;
}

/* ------------------------------------------------------------------ */
/* Keyboard and mouse                                                  */
/* ------------------------------------------------------------------ */

int dos_kbhit(void)
{
	pump_throttled();
	return plat_key_available() || plat_quit_requested();
}

int dos_getkey(void)
{
	for (;;) {
		plat_pump();
		if (plat_key_available()) return plat_key_get();
		if (plat_quit_requested()) return 27;
		plat_sleep_ms(2);
	}
}

void dos_mouse_read(int16_t *dx, int16_t *dy, int16_t *buttons)
{
	pump_throttled();
	plat_mouse(dx, dy, buttons);
}
