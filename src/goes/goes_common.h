/*
 * Noctis IV Plus - Linux port.
 * Support code shared by the GOES Net modules.
 *
 * Each module lives in its own namespace and keeps the code (and the
 * globals) of the original stand-alone DOS program.
 */

#ifndef NOCTIS_GOES_COMMON_H
#define NOCTIS_GOES_COMMON_H

#include "../game/noctis-d.h"
#include <cstdarg>

/* Game objects used by the modules (the VGA memory and its helpers). */
extern unsigned char *adaptor;
extern void areaclear (unsigned char *dest, Word x, Word y,
		       Word x2, Word y2, Word l, Word h, unsigned char pattern);
extern char star_in_sector (Dword sect_x, Dword sect_y, Dword sect_z,
			    Dword *sx, Dword *sy, Dword *sz);
extern void port_present_now (void);

/* Standard output of the running module (redirected to GOESFILE.TXT). */
extern FILE *goes_out;
int goes_printf (const char *fmt, ...);
#define printf goes_printf
#define kbhit  dos_kbhit
#define getch  dos_getkey

/* The PRI module printed on PRN: here the printout goes to a text file
   (GALLERY/PRINTOUT.TXT). */
FILE *goes_printer (void);
#define stdprn goes_printer ()

/* Shows what the module drew into the video memory. */
static inline void goes_refresh (void) { port_present_now (); }

/* Reads an 84-byte guide record (8-byte subject id + 76 bytes of text):
   the DOS modules read it straight into two adjacent globals. */
template <size_t N>
static inline int goes_read84 (int fh, double *subject, char (&message)[N])
{
	unsigned char rec[84];
	int n = _read (fh, rec, 84);
	if (n == 84) {
		memcpy (subject, rec, 8);
		memcpy (message, rec + 8, N < 76 ? N : 76);
	}
	return n;
}

/* Reads the first 244 bytes of CURRENT.BIN into the module's copy of the
   game variables (which have the same names, types and order). */
#define GOES_F(v) memcpy (&v, b_ + o_, sizeof (v)); o_ += sizeof (v);
#define GOES_READ_SITUATION(fh) do { \
	unsigned char b_[245]; memset (b_, 0, sizeof (b_)); \
	_read (fh, b_, 244); unsigned o_ = 0; \
	GOES_F(nsync) GOES_F(anti_rad) GOES_F(pl_search) GOES_F(field_amplificator) \
	GOES_F(ilight) GOES_F(ilightv) GOES_F(charge) GOES_F(revcontrols) \
	GOES_F(ap_targetting) GOES_F(ap_targetted) GOES_F(ip_targetting) GOES_F(ip_targetted) \
	GOES_F(ip_reaching) GOES_F(ip_reached) GOES_F(ap_target_spin) GOES_F(ap_target_r) \
	GOES_F(ap_target_g) GOES_F(ap_target_b) GOES_F(nearstar_spin) GOES_F(nearstar_r) \
	GOES_F(nearstar_g) GOES_F(nearstar_b) GOES_F(gburst) GOES_F(menusalwayson) \
	GOES_F(depolarize) GOES_F(sys) GOES_F(pwr) GOES_F(dev_page) GOES_F(ap_target_class) \
	GOES_F(f_ray_elapsed) GOES_F(nearstar_class) GOES_F(nearstar_nop) \
	GOES_F(pos_x) GOES_F(pos_y) GOES_F(pos_z) GOES_F(user_alfa) GOES_F(user_beta) \
	GOES_F(navigation_beta) GOES_F(ap_target_ray) GOES_F(nearstar_ray) \
	GOES_F(dzat_x) GOES_F(dzat_y) GOES_F(dzat_z) GOES_F(ap_target_x) GOES_F(ap_target_y) \
	GOES_F(ap_target_z) GOES_F(nearstar_x) GOES_F(nearstar_y) GOES_F(nearstar_z) \
	GOES_F(helptime) GOES_F(ip_target_initial_d) GOES_F(requested_approach_coefficient) \
	GOES_F(current_approach_coefficient) GOES_F(reaction_time) GOES_F(fcs_status) \
	GOES_F(fcs_status_delay) GOES_F(psys) GOES_F(ap_target_initial_d) \
	GOES_F(requested_vimana_coefficient) GOES_F(current_vimana_coefficient) \
	GOES_F(vimana_reaction_time) GOES_F(lithium_collector) GOES_F(autoscreenoff) \
	GOES_F(ap_reached) GOES_F(lifter) GOES_F(secs) \
	(void)o_; } while (0)

/* Scans the cube of sectors around the stardrifter looking for the star
   with the given identification code (the same hash used by the game).
   'slice' is called after every completed X slice but the last one and
   may abort the scan by returning non-zero.
   Returns 1 if found, 0 if not, or the value returned by 'slice'. */
template <typename Slice>
char goes_scan (double star_id, double idscale, Uword range, char skip_zero,
		double dzat_x, double dzat_y, double dzat_z,
		double *laststar_x, double *laststar_y, double *laststar_z,
		Slice slice)
{
	double sidlow = star_id - idscale;
	double sidhigh = star_id + idscale;
	Dword advance = 100000;
	Dword sect_x, sect_y, sect_z;
	Dword align_m = 100000 * (Dword)range;
	Dword align_e = 100000 * (Dword)range;

	sect_x = to_i32 ((dzat_x - (Dword)range*50000) / 100000); sect_x *= 100000;
	sect_y = to_i32 ((dzat_y - (Dword)range*50000) / 100000); sect_y *= 100000;
	sect_z = to_i32 ((dzat_z - (Dword)range*50000) / 100000); sect_z *= 100000;

	for (Uword sx = range; sx; ) {
		for (Uword sy = range; sy; sy--) {
			for (Uword sz = range; sz; sz--) {
				Dword tx, ty, tz;
				star_in_sector (sect_x, sect_y, sect_z, &tx, &ty, &tz);
				if (skip_zero && (!tx || !ty || !tz)) {
					if (tx) *laststar_x = tx;
					if (tx && ty) *laststar_y = ty;
					sect_z += advance;
					continue;
				}
				*laststar_x = tx;
				*laststar_y = ty;
				*laststar_z = tz;
				long double id = ((long double)tx * idscale)
					* (((long double)ty * idscale) * ((long double)tz * idscale));
				if (!(id < sidlow) && (double)id < sidhigh) return 1;
				sect_z += advance;
			}
			sect_z -= align_m;
			sect_y += advance;
		}
		sect_y -= align_e;
		sect_x += advance;
		sx--;
		if (!sx) break;
		char r = slice ((Uword)(range - sx), range);
		if (r) return r;
	}
	return 0;
}

#endif
