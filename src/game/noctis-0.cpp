/*

	Noctis.
	-------
	Primo programma piu' serioso del solito, che rovina la sana (ehm...)
	manifestazione informatica delle mie follie, pur senza tradirla
	del tutto. Oh yes.

	Noctis: il popolo dei gatti.
	----------------------------
	Spazio: ultima frontiera.
	Questi sono i viaggi delle zattere cosmiche del popolo felino.

	Modulo delle funzioni di base di Noctis.
	----------------------------------------
	Il progetto e' costituito da NOCTIS-0.CPP, NOCTIS-1.CPP, NOCTIS.CPP,
	piu' un file includente definizioni comuni ai moduli.

	Linux port: the 16-bit inline assembly has been translated to C++.
	Where the original relied on real-mode details (segment wrap-around,
	the 4-byte offset of Borland far heap blocks, Borland's random number
	generator, 16-bit integer overflow) the behaviour is reproduced so that
	the generated universe is the same as in the DOS version.

*/

#include "noctis-d.h"
#include "noctis-0.h"
#include "../platform/platform.h"

/*

	Dati e funzioni specifiche importate da ASSEMBLY.H

*/

Word QUADWORDS = 16000;

unsigned char * adaptor = 0; // A000:0000 (emulated)
unsigned char * adapted = 0; // pagina video nascosta

unsigned char	tmppal[768];
unsigned char	return_palette[768];
unsigned char	surface_palette[768];

static inline void touch (const unsigned char *dest)
{
	if (adaptor && dest >= adaptor && dest < adaptor + 65536)
		plat_mark_dirty ();
}

void port_present_now ()
{
	plat_present (adaptor);
}

int port_quit_requested ()
{
	return plat_quit_requested ();
}

void reach_your_dir ()
{
	// Linux port: the working directory is set up by dos_setup_paths().
}

void _320_200_256 () // inizializza grafica a 320x200x256 colori.
{
	memset (adaptor, 0, 64000);
	plat_set_vram (adaptor);
	plat_mark_dirty ();
}

void _80_25_C () // modo grafico 80x25 testo a colori.
{
}

Word attendi_pressione_tasto () // aspetta un tasto e da' qual'e'.
{
	return (Word)(unsigned char)dos_getkey ();
}

Word tasto_premuto () // torna 1 se c'e' un tasto premuto da estrarre.
{
	return dos_kbhit () ? 1 : 0;
}

unsigned char range8088[64*3] = {
 0,  0,  0,  1,  1,  1,  2,  2,  2,  3,  3,  3,  4,  4,  4,  5,  5,  5,
 6,  6,  6,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10, 10, 10, 11, 11, 11,
12, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17, 17,
18, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 21, 22, 22, 22, 23, 23, 23,
24, 24, 24, 25, 25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28, 29, 29, 29,
30, 30, 30, 31, 31, 31, 32, 32, 32, 33, 33, 33, 34, 34, 34, 35, 35, 35,
36, 36, 36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 40, 40, 40, 41, 41, 41,
42, 42, 42, 43, 43, 43, 44, 44, 44, 45, 45, 45, 46, 46, 46, 47, 47, 47,
48, 48, 48, 49, 49, 49, 50, 50, 50, 51, 51, 51, 52, 52, 52, 53, 53, 53,
54, 54, 54, 55, 55, 55, 56, 56, 56, 57, 57, 57, 58, 58, 58, 59, 59, 59,
60, 60, 60, 61, 61, 61, 62, 62, 62, 63, 63, 63 };

void tavola_colori (unsigned char *nuova_tavolozza,
		    Uword colore_di_partenza, Uword nr_colori,
		    char filtro_rosso, char filtro_verde, char filtro_blu)
{
	Uword c, cc = 0;
	Uword temp;
	nr_colori *= 3;
	colore_di_partenza *= 3;

	c = colore_di_partenza;
	while (cc<nr_colori) {
		tmppal[c] = nuova_tavolozza[cc];
		cc++;
		c++;
	}

	c = colore_di_partenza;
	while (c<nr_colori+colore_di_partenza) {
		temp = tmppal[c];
		temp = (Uword)(temp * filtro_rosso);
		temp /= 63;
		if (temp > 63) temp = 63;
		tmppal[c] = temp;
		c++;
		temp = tmppal[c];
		temp = (Uword)(temp * filtro_verde);
		temp /= 63;
		if (temp > 63) temp = 63;
		tmppal[c] = temp;
		c++;
		temp = tmppal[c];
		temp = (Uword)(temp * filtro_blu);
		temp /= 63;
		if (temp > 63) temp = 63;
		tmppal[c] = temp;
		c++;
	}

	// Il DAC viene caricato dal colore zero fino all'ultimo modificato.
	plat_set_palette (tmppal, 0, (colore_di_partenza + nr_colori) / 3);
}

/* Lettura del mouse e ritorno nelle variabili indicate. */

Word mdltx = 0, mdlty = 0, mx = 0, my = 0, mpul = 0;

void mouse_input ()
{
	Word dx, dy, b;
	dos_mouse_read (&dx, &dy, &b);
	mdltx = dx;
	mdlty = dy;
	mx += dx;
	my += dy;
	if (b) mpul = b;
}

// Verifica della presenza del mouse (o del supporto per esso).

char test_and_init_mouse ()
{
	Word dx, dy, b;
	dos_mouse_read (&dx, &dy, &b);
	return (1);
}

// Ultraveloce copia di pagina grafica.

void pcopy (unsigned char *dest, unsigned char *sorg)
{
	Uword n = (Uword)QUADWORDS;
	if (dest != sorg) memmove (dest, sorg, (Udword)n * 4);
	if (dest == adaptor) port_present_now ();
	else touch (dest);
}

// Ultraveloce cancella pagina grafica.

void pclear (unsigned char *target, unsigned char pattern)
{
	Uword n = (Uword)QUADWORDS;
	memset (target, pattern, (Udword)n * 4);
	touch (target);
}

// Copia un'area rettangolare.

void areacopy (unsigned char *dest, unsigned char *sorg,
	       Word x, Word y, Word l, Word h)
{
	Uword p = 320 * y + x;
	do {
		memmove (dest + p, sorg + p, (Uword)l);
		p += 320;
		h--;
	} while (h);
	touch (dest);
}

// Cancella un'area rettangolare.
// Esegue tutti i clipping necessari per l'MCGA.
// O si specificano X2 ed Y2, o si specificano L ed H:
// in ogni modo, i valori non usati vanno lasciati a zero.

void areaclear (unsigned char *dest, Word x, Word y,
		Word x2, Word y2, Word l, Word h, unsigned char pattern)
{
	Uword p;

	if (x<0) x = 0;
	if (y<0) y = 0;

	if (x2>=320) x2 = 319;
	if (y2>=200) y2 = 199;

	if (x2>0) l = x2 - x;
	if (y2>0) h = y2 - y;

	if (x+l>=320) l = 320 - x;
	if (y+h>=200) h = 200 - y;

	if (l<1 || h<1) return;

	p = 320 * y + x;

	do {
		memset (dest + p, pattern, l);
		p += 320;
		h--;
	} while (h);
	touch (dest);
}

static inline Udword rd32 (const unsigned char *p)
{
	Udword v;
	memcpy (&v, p, 4);
	return v;
}

/*

	Altro gioiellino: smussa lo schermo, attenuando il contrasto dei
	bordi con un procedimento di media su 4 x 4 pixels.
	Col trucco bastano quattro addizioni a 32 bit, altre quattro a 8 bit,
	piu' un paio di shifts ed un and logico su edx.

	La tavola dei colori dev'essere una sfumatura unica e compatta,
	da far rientrare nei primi 64 registri del video DAC.

*/

static void smooth4x4 (unsigned char *target, Uword stride, Uword count)
{
	unsigned char *di = target + stride;
	while (count) {
		Udword edx = rd32 (di - stride) + rd32 (di) + rd32 (di + stride) + rd32 (di + 2*stride);
		edx &= 0xFCFCFCFCu;
		edx >>= 2;
		unsigned char al = (unsigned char)(edx & 0xFF);
		al += (unsigned char)((edx >> 8) & 0xFF);
		edx >>= 16;
		al += (unsigned char)(edx & 0xFF);
		al += (unsigned char)((edx >> 8) & 0xFF);
		al >>= 2;
		*di = al;
		di++;
		count--;
	}
}

void psmooth_grays (unsigned char *target)
{
	Uword cx = (Uword)((Uword)QUADWORDS << 2);
	cx -= 320 << 2;
	smooth4x4 (target, 320, cx);
}

/* Produce un effetto di dissolvenza al nero, molto rapido. */

void pfade (unsigned char *target, Uword segshift, unsigned char speed)
{
	unsigned char *ds = dos_paragraph_base (target) + (Udword)segshift * 16;
	Uword cx = (Uword)(((Uword)QUADWORDS - 80) << 2);
	Uword di = 0;
	while (cx) {
		unsigned char al = ds[di] & 0x3F;
		if (al < speed) al = 0;
		else al -= speed;
		ds[di] = al;
		di++;
		cx--;
	}
}

/* Versione a colori: 4 sfumature di 64 intensita' ciascuna. */

void psmooth_64 (unsigned char *target, Uword segshift)
{
	unsigned char *ds = dos_paragraph_base (target) + (Udword)segshift * 16;
	Uword si = (Uword)(((Uword)QUADWORDS - 80) << 2);
	unsigned char *di = ds;
	while (si) {
		unsigned char al = (di[320] & 0x3F) + (di[640] & 0x3F);
		unsigned char ah = (di[321] & 0x3F) + (di[641] & 0x3F);
		unsigned char cl = di[320] & 0xC0;
		al += ah;
		al >>= 2;
		*di = al | cl;
		di++;
		si--;
	}
}

/* Versione circolare del procedimento di smoothing */

void smootharound_64 (unsigned char *target, Dword cx, Dword cy, Dword r, char diffuse)
{
	Dword x1 = cx - r, y1 = cy - r;
	Dword x2 = cx + r, y2 = cy + r;
	Dword px, py, rs = r*r;
	Uword cp;

	if (r <= 0) return;

	if (x1 > 318) return;
	if (y1 > 198) return;
	if (y1 < 0) y1 = 0;

	if (x2 < 0) return;
	if (x2 > 318) x2 = 318;
	if (y2 < 0) return;
	if (y2 > 198) y2 = 198;

	py = -r;
	while (y1 <= y2) {
		px = -r;
		x1 = cx - r;
		if (x1 < 0) {
			px -= x1;
			x1 = 0;
		}
		cp = (320 * y1) + x1;
		if (diffuse) {
			while (x1 <= x2) {
				if (px*px + py*py < rs) {
					unsigned char *p = target + cp;
					unsigned char a0 = p[0], a1 = p[1];
					unsigned char b0 = p[320], b1 = p[321];
					unsigned char al = (a0 & 0x3F) + (b0 & 0x3F);
					unsigned char ah = (a1 & 0x3F) + (b1 & 0x3F);
					al += ah;
					al >>= 2;
					p[0] = al | (a0 & 0xC0);
					p[1] = al | (a1 & 0xC0);
					p[320] = al | (b0 & 0xC0);
					p[321] = al | (b1 & 0xC0);
				}
				cp++;
				px++;
				x1++;
			}
		}
		else {
			while (x1 <= x2) {
				if (px*px + py*py < rs) {
					unsigned char *p = target + cp;
					unsigned char cl = p[0] & 0xC0;
					unsigned char al = (p[0] & 0x3F) + (p[320] & 0x3F);
					unsigned char ah = (p[1] & 0x3F) + (p[321] & 0x3F);
					al += ah;
					al >>= 2;
					p[0] = al | cl;
				}
				cp++;
				px++;
				x1++;
			}
		}
		py++;
		y1++;
	}
}

// Usando 64 livelli in 4 sfumature, porta lo schermo ad una sola sfumatura.

void mask_pixels (unsigned char *target, unsigned char mask)
{
	Uword cx = (Uword)QUADWORDS;
	Udword n = (Udword)cx * 4;
	for (Udword i = 0; i < n; i++)
		target[i] = (unsigned char)((target[i] & 0x3F) + mask);
}

/*

	Inclusioni HSP.

*/

#include "tdpolygs.h" // 3d-Engine.

/*

	Catalogo files di supporto.

*/

const char   *situation_file  = "..\\DATA\\Current.BIN";
const char   *starmap_file    = "..\\DATA\\StarMap.BIN";
const char   *goesoutputfile	= "..\\DATA\\GOESfile.TXT";
const char   *surface_file	= "..\\DATA\\Surface.BIN";
Word 	sfh; // handle del file della situazione di superficie.

/*

	Dati globali di continuita' (salvati e ripristinati).

*/

char   nsync              = 1;		// 0
char   anti_rad           = 1;          // 1
char   pl_search          = 0;          // 2
char   field_amplificator = 0;          // 3
char   ilight             = 63;         // 4
char   ilightv            = 1;          // 5
char   charge             = 3;          // 6
char   revcontrols        = 0;          // 7
char   ap_targetting      = 0;          // 8
char   ap_targetted       = 0;          // 9
char   ip_targetting      = 0;          // 10
char   ip_targetted       = -1;		// 11
char   ip_reaching        = 0;          // 12
char   ip_reached         = 0;          // 13
char   ap_target_spin     = 0;          // 14
char   ap_target_r        = 0;          // 15
char   ap_target_g        = 0;          // 16
char   ap_target_b        = 0;          // 17
char   nearstar_spin      = 0;          // 18
char   nearstar_r         = 0;          // 19
char   nearstar_g         = 0;          // 20
char   nearstar_b         = 0;          // 21
char   gburst	          = 0;          // 22
char   menusalwayson      = 1;		// 23
char   depolarize  	  = 0;          // 24
Word   sys                = 4;		// 25
Word   pwr                = 20000;	// 27
Word   dev_page           = 0;		// 29
Word   ap_target_class    = 0;		// 31
Word   f_ray_elapsed	  = 0;		// 33
Word   nearstar_class     = 0;		// 35
Word   nearstar_nop       = 0;		// 37
float  pos_x              = 0;		// 39
float  pos_y              = 0;		// 43
float  pos_z              = -500;	// 47
float  user_alfa          = 0;		// 51
float  user_beta          = 0;		// 55
float  navigation_beta    = 0;		// 59
float  ap_target_ray      = 1000;	// 63
float  nearstar_ray       = 1000;	// 67
double dzat_x             = +3797120;	// 71
double dzat_y             = -4352112;	// 79
double dzat_z             = -925018;	// 87
double ap_target_x        = 0;		// 95
double ap_target_y        = 1E8;	// 103
double ap_target_z        = 0;		// 111
double nearstar_x         = 0;		// 119
double nearstar_y         = 1E8;	// 127
double nearstar_z         = 0;		// 135
double helptime           = 0;		// 143
double ip_target_initial_d= 1E8;	// 151
double requested_approach_coefficient=1;// 159
double current_approach_coefficient = 1;// 167
double reaction_time      = 0.01;	// 175
char   fcs_status[11]     = "STANDBY";	// 183
Word   fcs_status_delay   = 0;		// 194
Word   psys               = 4;		// 196
double ap_target_initial_d= 1E8;	// 198
double requested_vimana_coefficient = 1;// 206
double current_vimana_coefficient = 1;	// 214
double vimana_reaction_time = 0.01;	// 222
char   lithium_collector  = 0;		// 230
char   autoscreenoff	  = 0;		// 231
char   ap_reached         = 0;		// 232
Word   lifter		  = 0;		// 233
double secs		  = 0;		// 235
char   data		  = 0;		// 243
char   surlight		  = 16;		// 244
Word old_currentbin_length = 245;

//Additional variables: (SL and Mega)
Dword lastSnapshot = -1;			//0
char option_mouseLook = 0;			//4
char option_antialias_in_stardrifter = 1;

Word new_currentbin_length = 5;
char 	fcs_status_extended[42] = "STANDBY";

/* The DOS program saved the block of globals above with a single write
   relying on their layout in the data segment; the port serialises the
   same 245 bytes field by field (little endian, packed). */

struct SitField { void *p; Uword size; };

static const SitField situation_fields[] = {
	{ &nsync, 1 }, { &anti_rad, 1 }, { &pl_search, 1 }, { &field_amplificator, 1 },
	{ &ilight, 1 }, { &ilightv, 1 }, { &charge, 1 }, { &revcontrols, 1 },
	{ &ap_targetting, 1 }, { &ap_targetted, 1 }, { &ip_targetting, 1 }, { &ip_targetted, 1 },
	{ &ip_reaching, 1 }, { &ip_reached, 1 }, { &ap_target_spin, 1 }, { &ap_target_r, 1 },
	{ &ap_target_g, 1 }, { &ap_target_b, 1 }, { &nearstar_spin, 1 }, { &nearstar_r, 1 },
	{ &nearstar_g, 1 }, { &nearstar_b, 1 }, { &gburst, 1 }, { &menusalwayson, 1 },
	{ &depolarize, 1 },
	{ &sys, 2 }, { &pwr, 2 }, { &dev_page, 2 }, { &ap_target_class, 2 },
	{ &f_ray_elapsed, 2 }, { &nearstar_class, 2 }, { &nearstar_nop, 2 },
	{ &pos_x, 4 }, { &pos_y, 4 }, { &pos_z, 4 }, { &user_alfa, 4 }, { &user_beta, 4 },
	{ &navigation_beta, 4 }, { &ap_target_ray, 4 }, { &nearstar_ray, 4 },
	{ &dzat_x, 8 }, { &dzat_y, 8 }, { &dzat_z, 8 },
	{ &ap_target_x, 8 }, { &ap_target_y, 8 }, { &ap_target_z, 8 },
	{ &nearstar_x, 8 }, { &nearstar_y, 8 }, { &nearstar_z, 8 },
	{ &helptime, 8 }, { &ip_target_initial_d, 8 },
	{ &requested_approach_coefficient, 8 }, { &current_approach_coefficient, 8 },
	{ &reaction_time, 8 },
	{ fcs_status, 11 }, { &fcs_status_delay, 2 }, { &psys, 2 },
	{ &ap_target_initial_d, 8 }, { &requested_vimana_coefficient, 8 },
	{ &current_vimana_coefficient, 8 }, { &vimana_reaction_time, 8 },
	{ &lithium_collector, 1 }, { &autoscreenoff, 1 }, { &ap_reached, 1 },
	{ &lifter, 2 }, { &secs, 8 }, { &data, 1 }, { &surlight, 1 },
};

void situation_to_bytes (unsigned char *out)
{
	Uword o = 0;
	for (const SitField &f : situation_fields) {
		memcpy (out + o, f.p, f.size);
		o += f.size;
	}
}

void situation_from_bytes (const unsigned char *in)
{
	Uword o = 0;
	for (const SitField &f : situation_fields) {
		memcpy (f.p, in + o, f.size);
		o += f.size;
	}
}

/*

	Dati di controllo per lo sbarco sulla superficie.

*/

char   land_now		  = 0;
char   landing_point	  = 0;
Word   landing_pt_lon	  = 0;
Word   landing_pt_lat	  = 60;

Word 	crepzone;
Word 	nightzone;
Word	sun_x_factor;

/*

	Dati globali non salvati (deducibili da quelli salvati).

*/

char seconds[3], minutes[3], hours[3], day[3], month[3], year[5];
Word  epoc = 6011;

char          ctb[512];
char	      dec[64];
char 	      triadFormat[16];

char	      _delay	     = 12;
char          stspeed        = 0;
char          bright;
char          elight	     = 0;
Uword         gl_start 	     = 0;
Uword         point;
Uword         vptr;
Word          infoarea       = 0;
Word          s_control      = 1;
Word          s_command      = 0;
Word          isecs, p_isecs;
double	      fsecs;
Word	      gl_fps         = 1;
Word	      fps            = 1;
float         dlt_alfa       = 0;
float	      dlt_beta       = 0;
float	      dlt_nav_beta   = 0;
float	      step           = 0;
float	      shift          = 0;
double        s_m = 1000;
double	      plx, ply, plz;
double        pxx, pyy;
double        delta_x, delta_y;
double 	      nearstar_identity;
Word	      nearstar_nob = 0, nearstar_labeled;
Word	      npcs, resident_map1, resident_map2;
char   	      ontheroof;
Word	      datasheetscroll = 0;
Word	      datasheetdelta = 0;

extern Word movieexists;
extern char movie;
float moviefps;

/*

	Dati costanti nel segmento globale.

*/

/* Alcuni ordinali (da 0 a 20) per determinate rappresentazioni. */

const char *ord[21] = { "zeroth", "first", "second", "third", "fourth", "fifth",
		  "sixth", "seventh", "eight", "nineth", "tenth", "eleventh",
		  "twelveth", "thiteenth", "fourteenth",
		  "fifteenth", "sixteenth", "seventeenth",
		  "eighteenth", "nineteenth", "twentyth" };

const char *star_description[star_classes] = {
	"medium size, yellow star, suitable for planets having indigenous lifeforms.",
	"very large, blue giant star, high energy radiations around.",
	"white dwarf star, possible harmful radiations.",
	"very large, ancient, red giant star.",
	"large and glowing, orange giant star, high nuclear mass.",
	"small, weak, cold, brown dwarf substellar object.",
	"large, very weak, very cold, gray giant dead star.",
	"very small, blue dwarf star, strong gravity well around.",
	"possible MULTIPLE system - planets spread over wide ranges.",
	"medium size, surrounded by gas clouds, young star.",
	"very large and ancient runaway star, unsuitable for planets.",
	"tiny pulsar object, unsafe, high radiation, strong gravity." };

char class_rgb[3*star_classes] = {
	63, 58, 40,
	30, 50, 63,
	63, 63, 63,
	63, 30, 20,
	63, 55, 32,
	32, 16, 10,
	32, 28, 24,
	10, 20, 63,
	63, 32, 16,
	48, 32, 63,
	40, 10, 10,
	00, 63, 63
};

Word class_ray[star_classes] = { 5000, 15000, 300, 20000, 15000, 1000, 3000,
				2000, 4000, 1500, 30000, 250 };

Word class_rayvar[star_classes] = { 2000, 10000, 200, 15000, 5000, 1000, 3000,
				   500, 5000, 10000, 1000, 10 };

Word class_act[star_classes] = { 2, 4, 1, 6, 5, 10, 100, 1, 2, 1, 10, 1 };

char class_planets[star_classes] = { 12, 18, 8, 15, 20, 3, 0, 1, 7, 20, 2, 5 };

char   nearstar_p_type       [maxbodies];
Word   nearstar_p_owner	     [maxbodies];
char   nearstar_p_moonid     [maxbodies];
double nearstar_p_ring       [maxbodies];
double nearstar_p_tilt       [maxbodies];
double nearstar_p_ray        [maxbodies];
double nearstar_p_orb_ray    [maxbodies];
double nearstar_p_orb_seed   [maxbodies];
double nearstar_p_orb_tilt   [maxbodies];
double nearstar_p_orb_orient [maxbodies];
double nearstar_p_orb_ecc    [maxbodies];

Word   nearstar_p_rtperiod   [maxbodies];
Word   nearstar_p_rotation   [maxbodies];
Word   nearstar_p_term_start [maxbodies];
Word   nearstar_p_term_end   [maxbodies];

Word   nearstar_p_qsortindex [maxbodies];
float  nearstar_p_qsortdist  [maxbodies];

const char *planet_description[11] = { "medium size, internally hot, unstable surface, no atmosphere.",
			       "small, solid, dusty, craterized, no atmosphere.",
			       "medium size, solid, thick atmosphere, fully covered by clouds.",
			       "medium size, felisian, breathable atmosphere, suitable for life.",
			       "medium size, rocky, creased, no atmosphere.",
			       "small, solid, thin atmosphere.",
			       "large, not consistent, covered with dense clouds.",
			       "small, solid, icy surface, no atmosphere.",
			       "medium size, surface is mainly native quartz, oxygen atmosphere.",
			       "very large, substellar object, not consistent.",
			       "companion star - not a planet" };

unsigned char planet_rgb_and_var[44] = { 60, 30, 15,      20,
				       40, 50, 40,	25,
				       32, 32, 32, 	32,
				       16, 32, 48,      40,
				       32, 40, 32,	20,
				       32, 32, 32,      32,
				       32, 32, 32,      32,
				       32, 40, 48,      24,
				       40, 40, 40,      30,
				       50, 25, 10,      20,
				       40, 40, 40,	40 };

Word planet_possiblemoons[11] = { 1, 1, 2, 3, 2, 2, 18, 2, 3, 20, 20 };

const double planet_orb_scaling=  5.0;
const double avg_planet_sizing =  2.4;
const double moon_orb_scaling  = 12.8;
const double avg_moon_sizing   =  1.8;

double avg_planet_ray[] = { 0.007, 0.003, 0.010, 0.011, 0.010,
			    0.008, 0.064, 0.009, 0.012, 0.125,
			    5.000 };

float mindiff = 0.01;

/*

	Matrici video fisiche e logiche, cartografie,
	ed altri blocchi di memoria...

*/

unsigned char  *s_background;
unsigned char  *p_background;
unsigned char  *p_surfacemap;
quadrant       *objectschart;
unsigned char  *ruinschart; // come objectschart, ma dichiarato in bytes
unsigned char  *pvfile;

unsigned char  *n_offsets_map;
unsigned char  *n_globes_map;

/*

	Dati di regolazione delle superfici planetarie.

*/

char sky_red_filter = 63; // Filtri per il cielo.
char sky_grn_filter = 63;
char sky_blu_filter = 63;
char gnd_red_filter = 63; // Filtri per il suolo.
char gnd_grn_filter = 63;
char gnd_blu_filter = 63;

float planet_grav  = 150; // corrisponde alla gravita' terrestre.
float rainy         = 0; // 0..5 a seconda dell'albedo delle nubi.
Word  albedo        = 0; // Albedo media della superficie di sbarco (0..62).

unsigned char sky_brightness = 32; // Luminosita' del cielo (0-48).
unsigned char horiz_brt      = 20; // Luminosita' minima dell'orizzonte (0-48).

Uword m200[200]; // numeri da 0 a 199, moltiplicati per 200 (ottimizzaz.)

float	 rwp = 15; // Potenza richiesta del vento (per regolarlo, in NODI).
float    iwp =  0; // Potenza ideale del vento (da 0 a 1).
float    wp  =  0; // Potenza attuale del vento.
float    wdir=  0; // Direzione di provenienza del vento.
float	 wdirsin=0;// Valore di ottimizzazione.
float	 wdircos=1;// Valore di ottimizzazione.

char landed; // Flag settato al momento dell'atterraggio.
Dword atl_x, atl_z, atl_x2, atl_z2; // Posizione codificata quoziente:resto
				    // del bozzolo di sbarco.

double qid = 1.0 / 16384; // Costante per trovare l'id x/z di un quadrante.

/*

	Aree per grafica poligonale (personaggi, navicelle, diciture, etc...)

*/

#define handles	16 // 16 handles in questo caso...

Udword pvfile_datatop = 0; // cima dei dati.

// nota: dataptr e' un puntatore relativo che puo' andare da zero a "pvbytes".
Uword pvfile_dataptr[handles] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
Uword pvfile_datalen[handles] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
Uword pvfile_npolygs[handles] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

char  *pv_n_vtx[handles]; // numero vertici per ogni poligono (3 o 4).
float *pvfile_x[handles]; // coord. X (quattro vertici) di ogni poligono.
float *pvfile_y[handles]; // coord. Y (quattro vertici) di ogni poligono.
float *pvfile_z[handles]; // coord. Z (quattro vertici) di ogni poligono.
char  *pvfile_c[handles]; // intensita' del colore di ogni poligono (0..63).
float *pv_mid_x[handles]; // coordinate X dei punti medi di ogni poligono.
float *pv_mid_y[handles]; // coordinate Y dei punti medi di ogni poligono.
float *pv_mid_z[handles]; // coordinate Z dei punti medi di ogni poligono.
float *pv_mid_d[handles]; // buffer distanza di punti medi dall'osservatore.
Word  *pv_dep_i[handles]; // indice distanza (ordinamento poligoni).

/* Procedimenti d'estrazione di numeri pseudo-casuali. */

Dword flat_rnd_seed;

void fast_srand (Dword seed) // selezione tabella pseudo.
{
	flat_rnd_seed = seed | 3;
}

Dword fast_random (Dword mask) // estrazione di un numero: "mask" attiva i bits.
{
	uint64_t prod = (uint64_t)(Udword)flat_rnd_seed * (uint64_t)(Udword)flat_rnd_seed;
	Udword eax = (Udword)prod;
	Udword edx = (Udword)(prod >> 32);
	eax = (eax & 0xFFFFFF00u) | ((eax + edx) & 0xFF);
	flat_rnd_seed = (Dword)((Udword)flat_rnd_seed + eax);
	return (Dword)(eax & (Udword)mask);
}

Word ranged_fast_random (Word range)
{
	if (range<=0) range = 1;
	return (Word)(fast_random(0x7FFF) % range);
}

float flandom ()
{ return ((float)random(32767) * 0.000030518); }

float fast_flandom ()
{ return ((float)fast_random(32767) * 0.000030518); }

/* Effettua l'apertura di un file "virtuale", contenuto in SUPPORTS.NCT. */

Word sa_open (Dword offset_of_virtual_file)
{
	Word fh;

	fh = _open (supports_nct, 0);
	if (fh==-1) return (-1);
	Dword ret = lseek (fh, offset_of_virtual_file, SEEK_END);
	if (ret > -1) {
		return (fh);
	} else {
		_close (fh);
		return (-1);
	}
}

/* Definisce una parte della tavola colori in modo che una sfumatura graduale
   vada da una tinta ad un'altra, entro un certo numero di tinte intermedie */

void shade (unsigned char *palette_buffer,
	    Word first_color, Word number_of_colors,
	    float start_r,  float start_g,  float start_b,
	    float finish_r, float finish_g, float finish_b)
{
	Word  count = number_of_colors;

	float k = 1.00 / (float)number_of_colors;
	float delta_r = (finish_r - start_r) * k;
	float delta_g = (finish_g - start_g) * k;
	float delta_b = (finish_b - start_b) * k;

	first_color *= 3;

	while (count) {
		if (start_r >= 0 && start_r < 64)
			palette_buffer[first_color + 0] = (unsigned char)to_i16 (start_r);
		else {
			if (start_r > 0)
				palette_buffer[first_color + 0] = 63;
			else
				palette_buffer[first_color + 0] = 00;
		}
		//
		if (start_g >= 0 && start_g < 64)
			palette_buffer[first_color + 1] = (unsigned char)to_i16 (start_g);
		else {
			if (start_g > 0)
				palette_buffer[first_color + 1] = 63;
			else
				palette_buffer[first_color + 1] = 00;
		}
		//
		if (start_b >= 0 && start_b < 64)
			palette_buffer[first_color + 2] = (unsigned char)to_i16 (start_b);
		else {
			if (start_b > 0)
				palette_buffer[first_color + 2] = 63;
			else
				palette_buffer[first_color + 2] = 00;
		}
		//
		start_r += delta_r;
		start_g += delta_g;
		start_b += delta_b;
		//
		first_color += 3;
		count--;
	}
}

/* Copia aree video piu' grandi di 4 per 1 pixels. */

void copia (unsigned char *dest, unsigned char *sorg, Uword x, Uword y, Word l, Word a)
{
	if (l<4) return;
	if (a<1) return;

	x = 320*y + x;
	while (a) {
		memmove (dest + x, sorg + x, l);
		x += 320;
		a--;
	}
	touch (dest);
}

/*

	Controllo visualizzazione.

*/

/* Imposta le proiezioni 3d dal punto di vista dell'astrozattera. */

void from_vehicle ()
{
	cam_x = dzat_x; cam_y = dzat_y; cam_z = dzat_z;
	alfa = user_alfa; beta = user_beta + navigation_beta + 180;
	if (beta>=360) beta -= 360;
	change_angle_of_view ();
}

/* Imposta le proiezioni 3d dal punto di vista del protagonista. */

void from_user ()
{
	cam_x = pos_x; cam_y = pos_y; cam_z = pos_z;
	alfa = user_alfa; beta = user_beta;
	change_angle_of_view ();
}

/* Orienta la videocamera virtuale verso un certo punto dato da see_x;y;z,
   frattalizzando l'angolo giro per fare piu' in fretta. */

void watch (double cam_x, double cam_y, double cam_z,
	    double see_x, double see_y, double see_z)
{
	double rx, ry, rz, w, ww, k, q, f1, f2, f3;
	double sinbeta, cosbeta, cosalfa;

	rx     = cam_x - see_x;
	ry     = cam_y - see_y;
	rz     = cam_z - see_z;
	w      = sqrt (rx*rx+ry*ry+rz*rz);

	see_x -= cam_x;
	see_y -= cam_y;
	see_z -= cam_z;

	f1     = 0;
	f2     = 0;

	f3     = 2 * M_PI;

	while (f3 > 0.02) {

		q       = 1e+111;
		ww      = f3 / 10;

		sinbeta = sin(beta);
		cosbeta = cos(beta);

		for (alfa=f2-f3; alfa<f2+f3; alfa+=ww) {
			rx  = - w * sinbeta * cos(alfa) - see_x;
			ry  = + w * sin(alfa) 	        - see_y;
			rz  = + w * cosbeta * cos(alfa) - see_z;
			k   = rx*rx + ry*ry + rz*rz;
			if (k<q) {
				q = k;
				f2 = alfa;
			}
		}

		alfa    = f2;

		ry      = w * sin(alfa) - see_y;
		ry     *= ry;

		cosalfa = cos(alfa);

		q       = 1e+111;
		ww      = f3 / 10;

		for (beta=f1-f3; beta<f1+f3; beta+=ww) {
			rx  = - w * sin(beta) * cosalfa - see_x;
			rz  = + w * cos(beta) * cosalfa - see_z;
			k   = rx*rx + ry + rz*rz;
			if (k<q) {
				q = k;
				f1 = beta;
			}
		}

		beta = f1;

		f3 /= 5;

	}

	alfa /= deg;
	beta /= deg;
}

/* Calcola lo scostamento x-y dal centro del video di un certo punto
   dato da point_x;y;z. Restituisce i valori in delta_x;y. */

char xy (double cam_x, double cam_y, double cam_z,
	 double point_x, double point_y, double point_z)
{
	double xx, yy, zz, z2, rz;

	xx = point_x - cam_x;
	yy = point_y - cam_y;
	zz = point_z - cam_z;

	delta_x = xx * opt_pcosbeta + zz * opt_psinbeta;
	z2      = zz * opt_tcosbeta - xx * opt_tsinbeta;
	rz      = z2 * opt_tcosalfa + yy * opt_tsinalfa;
	delta_y = yy * opt_pcosalfa - z2 * opt_psinalfa;

	if (rz>0.001) {
		delta_x /= rz;
		delta_y /= rz;
		return (1);
	}

	return (0);
}

/* Sposta il protagonista all'interno dell'astrozattera. */

void p_Forward (float delta)
{
	pos_x -= delta * opt_tsinbeta * opt_tcosalfa;
	pos_z += delta * opt_tcosbeta * opt_tcosalfa;
}

/* Calcola la posizione di un certo pianeta in orbita attorno ad una certa
   stella (quella vicina), sulla base dei parametri orbitali, e ritorna i
   valori in plx, ply, plz. */

double mox, moy, moz;
void   moonorigin (Word n)
{
	double xx;
	double p_riv, ors;
	double alfa, beta;

	ors = nearstar_p_orb_ray[n] * nearstar_p_orb_ray[n];
	p_riv = sqrt (s_m / ors);

	beta = (secs * p_riv * M_PI) / 180;
	alfa = nearstar_p_orb_tilt[n] * deg;

	xx  = -nearstar_p_orb_ray[n] * sin(beta) * cos(alfa);
	moz =  nearstar_p_orb_ray[n] * cos(beta) * cos(alfa);
	moy =  nearstar_p_orb_ray[n] * sin(alfa);

	moz *= nearstar_p_orb_ecc[n];

	beta = nearstar_p_orb_orient[n];

	mox = xx * cos(beta) + moz * sin(beta);
	moz = moz * cos(beta) - xx * sin(beta);
}

void planet_xyz (Word n)
{
	double xx;
	double alfa, beta;
	double p_m, p_riv, ors;

	if (n < 0 || n >= maxbodies) n = 0;

	ors = nearstar_p_orb_ray[n] * nearstar_p_orb_ray[n];

	if (nearstar_p_owner[n] > -1) {
		xx = nearstar_p_ray[nearstar_p_owner[n]];
		p_m = qt_M_PI * xx * xx * xx * 0.44e-4;
		p_riv = sqrt (p_m / ors);
	}
	else
		p_riv = sqrt (s_m / ors);

	beta = (secs * p_riv * M_PI) / 180;
	alfa = nearstar_p_orb_tilt[n] * deg;

	xx  = -nearstar_p_orb_ray[n] * sin(beta) * cos(alfa);
	plz =  nearstar_p_orb_ray[n] * cos(beta) * cos(alfa);
	ply =  nearstar_p_orb_ray[n] * sin(alfa);

	plz *= nearstar_p_orb_ecc[n];

	beta = nearstar_p_orb_orient[n];

	plx = xx * cos(beta) + plz * sin(beta);
	plz = plz * cos(beta) - xx * sin(beta);

	plx += nearstar_x;
	ply += nearstar_y;
	plz += nearstar_z;

	if (nearstar_p_owner[n] > -1) {
		moonorigin (nearstar_p_owner[n]);
		plx += mox;
		ply += moy;
		plz += moz;
	}
}

/* Calcola il periodo di rivoluzione di un corpo, in secondi. */

float rtp (Word n)
{
	double p_m, p_riv, ors, xx;

	ors = nearstar_p_orb_ray[n] * nearstar_p_orb_ray[n];

	if (nearstar_p_owner[n] > -1) {
		xx = nearstar_p_ray[nearstar_p_owner[n]];
		p_m = qt_M_PI * xx * xx * xx * 0.44e-4;
		p_riv = sqrt (p_m / ors);
	}
	else
		p_riv = sqrt (s_m / ors);

	return (360 / p_riv);
}

/* Calcola la longitudine d'incidenza dello sguardo sulla superficie
   di un certo pianeta dato da (plx, ply, plz). */

Word planet_viewpoint (double obs_x, double obs_z)
{
	Uword a;
	Word plwp = 0;

	double xx;
	double zz;

	double min = 1E99;

	for (a=0; a<360; a++) {
		xx = plx + cos(deg*(double)a) - obs_x;
		zz = plz + sin(deg*(double)a) - obs_z;
		xx = xx*xx + zz*zz;
		if (xx<min) {
			plwp = a;
			min = xx;
		}
	}

	return (plwp);
}

Word cplx_planet_viewpoint (Word logical_id)
{
	Word owner;

	double ownerplx;
	double ownerplz;

	if (nearstar_p_owner[logical_id] == -1)
		return (planet_viewpoint (nearstar_x, nearstar_z));

	owner = nearstar_p_owner[logical_id];
	if (nearstar_p_type[owner] == 10) {
		planet_xyz (owner);
		ownerplx = plx;
		ownerplz = plz;
		planet_xyz (logical_id);
		return (planet_viewpoint (ownerplx, ownerplz));
	}
	else
		return (planet_viewpoint (nearstar_x, nearstar_z));
}

/* Selezione effetti tramite variabile di controllo "flares". */

char previous_flares_value = 0;
void setfx (char fx) { previous_flares_value = flares; flares = fx; }
void chgfx (char fx) { flares = fx; }
void resetfx (void)  { flares = previous_flares_value; }

/* Tracciamento bastoncini (parte 2d). */

void Stick (Udword sxp, Udword syp, Udword sxa, Udword sya)
{
	Udword a, b;
	Uword pi, pf;

	if (sxp==sxa) {
		if (sya>=syp) {
			pi = riga[syp & 255] + sxp;
			pf = riga[(sya+1) & 255];
		}
		else {
			pi = riga[sya & 255] + sxp;
			pf = riga[(syp+1) & 255];
		}
		switch (flares) {
			case 0:
				while (pi < pf) {
					adapted[pi] = 0x00;
					adapted[(Uword)(pi+1)] = 0x3E;
					pi += 320;
				}
				break;
			case 1:
				while (pi < pf) {
					unsigned char cl = adapted[pi] & 0x3F;
					adapted[pi] &= 0xC0;
					cl += 0x08;
					if (!(cl < 0x3E)) cl = 0x3E;
					adapted[pi] += cl;
					pi += 320;
				}
				break;
			case 2:
				while (pi < pf) {
					unsigned char cl = adapted[pi] & 0x3F;
					adapted[pi] &= 0xC0;
					cl >>= 1;
					adapted[pi] += cl;
					pi += 320;
				}
				break;
			case 3:
				while (pi < pf) {
					adapted[pi] = 0x0E;
					adapted[(Uword)(pi+1)] = 0x13;
					adapted[(Uword)(pi+2)] = 0x1E;
					adapted[(Uword)(pi+3)] = 0x2E;
					pi += 320;
				}
				break;
		}
		return;
	}

	xp = sxp; yp = syp; xa = sxa; ya = sya;
	dda_setup (a, b);

	Udword cx = xa;

	switch (flares) {

		case 0: // bastoncini solidi che "riflettono" luce.
			do {
				Uword bx = (Uword)(global_y >> 16);
				Uword di = (Uword)(global_x >> 16);
				global_x += a;
				di += riga[bx & 255];
				global_y += b;
				adapted[di] = 0x00;
				adapted[(Uword)(di+1)] = 0x3E;
			} while (global_x < cx);
			break;

		case 1: // bastoncini intrinsecamente luminosi.
			a <<= 1; // un punto si', uno no.
			b <<= 1;
			do {
				Uword bx = (Uword)(global_y >> 16);
				Uword di = (Uword)(global_x >> 16);
				global_x += a;
				di += riga[bx & 255];
				global_y += b;
				unsigned char p = adapted[di];
				unsigned char lev = p & 0x3F;
				unsigned char nl = (lev < 56) ? lev + 8 : 0x3E;
				adapted[di] = (p & 0xC0) | nl;
			} while (global_x < cx);
			break;

		case 2: // bastoncini che assorbono luce ("affumicati").
			do {
				Uword bx = (Uword)(global_y >> 16);
				Uword di = (Uword)(global_x >> 16);
				global_x += a;
				di += riga[bx & 255];
				global_y += b;
				unsigned char cl = adapted[di] & 0x3F;
				adapted[di] &= 0xC0;
				cl >>= 1;
				adapted[di] += cl;
			} while (global_x < cx);
			break;

		case 3: // come tipo 0, ma piu' larghi.
			do {
				Uword bx = (Uword)(global_y >> 16);
				Uword di = (Uword)(global_x >> 16);
				global_x += a;
				di += riga[bx & 255];
				global_y += b;
				adapted[di] = 0xCE;
				adapted[(Uword)(di+1)] = 0xD3;
				adapted[(Uword)(di+2)] = 0xDE;
				adapted[(Uword)(di+3)] = 0xEE;
			} while (global_x < cx);
			break;
	}
}

/* Tracciamento bastoncini (parte 3d). */

Dword fpx = -1; // first-point-x
Dword fpy;      // first-point-y

float p_rx, p_ry, p_rz;
float stick_uneg = 200;

void stick3d (float p_x, float p_y, float p_z,
	      float x, float y, float z)
{
	Dword  lx, ly;

	float p_z2, diff;

	float rx, ry, rz, z2, kk;

	fpx = -1;

	p_x -= cam_x;
	p_z -= cam_z;
	p_z2 = p_z * opt_tcosbeta - p_x * opt_tsinbeta;
	p_y -= cam_y;
	p_rz = p_z2 * opt_tcosalfa + p_y * opt_tsinalfa;

	x -= cam_x;
	z -= cam_z;
	z2 = z * opt_tcosbeta - x * opt_tsinbeta;
	y -= cam_y;
	rz = z2 * opt_tcosalfa + y * opt_tsinalfa;

	if (rz<stick_uneg&&p_rz<stick_uneg) return;

	p_rx = p_x * opt_pcosbeta + p_z * opt_psinbeta;
	p_ry = p_y * opt_pcosalfa - p_z2 * opt_psinalfa;

	rx = x * opt_pcosbeta + z * opt_psinbeta;
	ry = y * opt_pcosalfa - z2 * opt_psinalfa;

	/* Conversione punti alle spalle dell'osservatore rispetto al piano
	   dello schermo. */

	if (rz<stick_uneg) {
		diff = rz-p_rz;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stick_uneg-p_rz) / diff;
			rx = kk * (rx-p_rx) + p_rx;
			ry = kk * (ry-p_ry) + p_ry;
		}
		rz = stick_uneg;
	}
	else {
		if (p_rz<stick_uneg) {
			diff = p_rz-rz;
			if (diff<-mindiff || diff>mindiff) {
				kk = (stick_uneg-rz) / diff;
				p_rx = kk * (p_rx-rx) + rx;
				p_ry = kk * (p_ry-ry) + ry;
			}
			p_rz = stick_uneg;
		}
	}

	// Prospettiva.

	float k = uno / rz;
	lx = fistp32 (k * rx);
	ly = fistp32 (k * ry);
	k = uno / p_rz;
	fpx = fistp32 (k * p_rx);
	fpy = fistp32 (k * p_ry);

	if (fpy<stk_lby&&ly<stk_lby) return; // Linee del tutto fuori campo.
	if (fpy>stk_uby&&ly>stk_uby) return;
	if (fpx<stk_lbx&&lx<stk_lbx) return;
	if (fpx>stk_ubx&&lx>stk_ubx) return;

	/* Ottimizza il tracciamento del segmento, tagliando via le parti
	   che di sicuro non si vedono. */

	if (fpx<stk_lbx) {
		diff = fpx-lx;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stk_lbx-lx) / diff;
			fpy = to_i32 (kk * (fpy-ly) + ly);
			fpx = stk_lbx;
		}
	}
	if (lx<stk_lbx) {
		diff = lx-fpx;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stk_lbx-fpx) / diff;
			ly = to_i32 (kk * (ly-fpy) + fpy);
			lx = stk_lbx;
		}
	}
	if (fpy<stk_lby) {
		diff = fpy-ly;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stk_lby-ly) / diff;
			fpx = to_i32 (kk * (fpx-lx) + lx);
			fpy = stk_lby;
		}
	}
	if (ly<stk_lby) {
		diff = ly-fpy;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stk_lby-fpy) / diff;
			lx = to_i32 (kk * (lx-fpx) + fpx);
			ly = stk_lby;
		}
	}
	if (fpx>stk_ubx) {
		diff = fpx-lx;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stk_ubx-lx) / diff;
			fpy = to_i32 (kk * (fpy-ly) + ly);
			fpx = stk_ubx;
		}
	}
	if (lx>stk_ubx) {
		diff = lx-fpx;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stk_ubx-fpx) / diff;
			ly = to_i32 (kk * (ly-fpy) + fpy);
			lx = stk_ubx;
		}
	}
	if (fpy>stk_uby) {
		diff = fpy-ly;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stk_uby-ly) / diff;
			fpx = to_i32 (kk * (fpx-lx) + lx);
			fpy = stk_uby;
		}
	}
	if (ly>stk_uby) {
		diff = ly-fpy;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stk_uby-fpy) / diff;
			lx = to_i32 (kk * (lx-fpx) + fpx);
			ly = stk_uby;
		}
	}

	if (fpx==lx&&fpy==ly) return; // Esclude le linee costituite da un punto solo.

	Stick (fpx+x_centro, fpy+y_centro, lx+x_centro, ly+y_centro);
}

/* Un "link" e' un "ponte" tra il punto iniziale dell'ultimo stick
   tracciato ed il punto passato a questa funzione. */

void link3d (float x, float y, float z)
{
	Dword lx, ly;

	float rx, ry, rz, z2;

	if (fpx == -1) return;

	if (fpy<=stk_lby) return;
	if (fpy>=stk_uby) return;

	if (fpx<=stk_lbx) return;
	if (fpx>=stk_ubx) return;

	x -= cam_x;
	z -= cam_z;
	z2 = z * opt_tcosbeta - x * opt_tsinbeta;
	y -= cam_y;
	rz = z2 * opt_tcosalfa + y * opt_tsinalfa;

	if (rz<stick_uneg) return;

	rx = x * opt_pcosbeta + z * opt_psinbeta;
	ry = y * opt_pcosalfa - z2 * opt_psinalfa;

	// Prospettiva.

	float k = uno / rz;
	lx = fistp32 (k * rx);
	ly = fistp32 (k * ry);

	// Chopping.

	if (ly<=stk_lby) return;
	if (ly>=stk_uby) return;

	if (lx<=stk_lbx) return;
	if (lx>=stk_ubx) return;

	if (fpx==lx&&fpy==ly) return;

	Stick (fpx+x_centro, fpy+y_centro, lx+x_centro, ly+y_centro);
}

/* Tracciamento bastoncini luminosi (in 2d, per i bagliori,
   generalmente usata con il flag flares = 1). */

void fline (Dword fx, Dword fy,
	    Dword lx, Dword ly)
{
	float kk, diff;

	if (fx<stk_lbx) {
		diff = fx-lx;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stk_lbx-lx) / diff;
			fy = to_i32 (kk * (fy-ly) + ly);
			fx = stk_lbx;
		}
	}
	if (lx<stk_lbx) {
		diff = lx-fx;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stk_lbx-fx) / diff;
			ly = to_i32 (kk * (ly-fy) + fy);
			lx = stk_lbx;
		}
	}
	if (fy<stk_lby) {
		diff = fy-ly;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stk_lby-ly) / diff;
			fx = to_i32 (kk * (fx-lx) + lx);
			fy = stk_lby;
		}
	}
	if (ly<stk_lby) {
		diff = ly-fy;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stk_lby-fy) / diff;
			lx = to_i32 (kk * (lx-fx) + fx);
			ly = stk_lby;
		}
	}
	if (fx>stk_ubx) {
		diff = fx-lx;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stk_ubx-lx) / diff;
			fy = to_i32 (kk * (fy-ly) + ly);
			fx = stk_ubx;
		}
	}
	if (lx>stk_ubx) {
		diff = lx-fx;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stk_ubx-fx) / diff;
			ly = to_i32 (kk * (ly-fy) + fy);
			lx = stk_ubx;
		}
	}
	if (fy>stk_uby) {
		diff = fy-ly;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stk_uby-ly) / diff;
			fx = to_i32 (kk * (fx-lx) + lx);
			fy = stk_uby;
		}
	}
	if (ly>stk_uby) {
		diff = ly-fy;
		if (diff<-mindiff || diff>mindiff) {
			kk = (stk_uby-fy) / diff;
			lx = to_i32 (kk * (lx-fx) + fx);
			ly = stk_uby;
		}
	}

	if (fx==lx&&fy==ly) return; // Esclude le linee costituite da un punto solo.

	if (fy<stk_lby||ly<stk_lby) return; // Esclude le linee che mai e poi mai si vedranno.
	if (fy>stk_uby||ly>stk_uby) return;
	if (fx<stk_lbx||lx<stk_lbx) return;
	if (fx>stk_ubx||lx>stk_ubx) return;

	Stick (fx+x_centro, fy+y_centro, lx+x_centro, ly+y_centro);
}

/* Funzione ricorsiva che suddivide poligoni triangolari in quattro parti
   ugualmente triangolari, casualizzandone leggermente la colorazione. */

unsigned char map_color_a = 30;
unsigned char map_color_b = 31;
unsigned char map_color_c = 32;
unsigned char map_color_d = 33;

void randomic_mapper (float x0,
		      float y0,
		      float z0,
		      float x1,
		      float y1,
		      float z1,
		      float x2,
		      float y2,
		      float z2,
		      char divisions)
{
	float vx[3], vy[3], vz[3];

	float e0, f0, g0;
	float e1, f1, g1;
	float e2, f2, g2;

	divisions--;

	if (divisions) {
		e0 = (x0 + x1) * 0.5; f0 = (y0 + y1) * 0.5; g0 = (z0 + z1) * 0.5;
		e1 = (x1 + x2) * 0.5; f1 = (y1 + y2) * 0.5; g1 = (z1 + z2) * 0.5;
		e2 = (x0 + x2) * 0.5; f2 = (y0 + y2) * 0.5; g2 = (z0 + z2) * 0.5;
		if (divisions==1) {
			vx[0] = x0; vy[0] = y0; vz[0] = z0;
			vx[1] = e0; vy[1] = f0; vz[1] = g0;
			vx[2] = e2; vy[2] = f2; vz[2] = g2;
			poly3d (vx, vy, vz, 3, map_color_a);
			randomic_mapper (x0, y0, z0, e0, f0, g0, e2, f2, g2, divisions);
			vx[0] = e1; vy[0] = f1; vz[0] = g1;
			poly3d (vx, vy, vz, 3, map_color_b);
			randomic_mapper (e1, f1, g1, e0, f0, g0, e2, f2, g2, divisions);
			vx[1] = x2; vy[1] = y2; vz[1] = z2;
			poly3d (vx, vy, vz, 3, map_color_c);
			randomic_mapper (e1, f1, g1, x2, y2, z2, e2, f2, g2, divisions);
			vx[2] = x1; vy[2] = y1; vz[2] = z1;
			vx[1] = e0; vy[1] = f0; vz[1] = g0;
			poly3d (vx, vy, vz, 3, map_color_d);
			randomic_mapper (e1, f1, g1, e0, f0, g0, x1, y1, z1, divisions);
		}
		else {
			randomic_mapper (x0, y0, z0, e0, f0, g0, e2, f2, g2, divisions);
			randomic_mapper (e1, f1, g1, e0, f0, g0, e2, f2, g2, divisions);
			randomic_mapper (e1, f1, g1, x2, y2, z2, e2, f2, g2, divisions);
			randomic_mapper (e1, f1, g1, e0, f0, g0, x1, y1, z1, divisions);
		}
	}
}

/* Libera un handle in cui era stato caricato un file di grafica poligonale:
   se l'handle e' gia' libero, non succede nulla. */

void unloadpv (Word handle)
{
	Word     h;
	Uword    eod;

	if (handle >= handles) return;
	if (!pvfile_datalen[handle]) return;

	// aggiorna i puntatori di tutti gli handle
	// che sono memorizzati oltre quello specificato.
	for (h = 0; h < handles; h++)
		if (pvfile_dataptr[h] > pvfile_dataptr[handle]) {
			Uword len = pvfile_datalen[handle];
			pv_n_vtx[h] = (char *)((char *)pv_n_vtx[h] - len);
			pvfile_x[h] = (float *)((char *)pvfile_x[h] - len);
			pvfile_y[h] = (float *)((char *)pvfile_y[h] - len);
			pvfile_z[h] = (float *)((char *)pvfile_z[h] - len);
			pvfile_c[h] = (char *)((char *)pvfile_c[h] - len);
			if (pv_mid_x[h]) {
				pv_mid_x[h] = (float *)((char *)pv_mid_x[h] - len);
				pv_mid_y[h] = (float *)((char *)pv_mid_y[h] - len);
				pv_mid_z[h] = (float *)((char *)pv_mid_z[h] - len);
				pv_mid_d[h] = (float *)((char *)pv_mid_d[h] - len);
				pv_dep_i[h] = (Word *)((char *)pv_dep_i[h] - len);
			}
			pvfile_dataptr[h] -= len;
		}

	// sposta indietro i dati per liberare memoria nell'area poligonale.
	eod = pvfile_dataptr[handle] + pvfile_datalen[handle];
	if (eod < pvfile_datatop)
		_fmemmove (pv_n_vtx[handle],
			   pv_n_vtx[handle] + pvfile_datalen[handle],
			   pvfile_datatop - eod);

	// aggiorna la cima dei dati dell'area poligonale.
	pvfile_datatop -= pvfile_datalen[handle];

	// l'handle e' ora considerato libero.
	pvfile_datalen[handle] = 0;
}

/* Libera tutti gli handles. */

void unloadallpv ()
{
	Word h;

	pvfile_datatop = 0;

	for (h = 0; h < handles; h++)
		pvfile_datalen[h] = 0;
}

/* Carica un file di dati per la grafica poligonale.
      * Ritorno: -1 se non e' possibile accedere al file;
		  0 se l'handle non e' assegnabile, o manca memoria;
		 +1 se tutto e' andato bene. */

char loadpv (Word  handle, Dword virtual_file_position,
	     float xscale, float yscale, float zscale,
	     float xmove,  float ymove,  float zmove,
	     unsigned char base_color,   char depth_sort)
{
	Word fh, c, p;

	// verifica disponibilita' del file e dell'handle.
	if (handle >= handles) return (0);
	fh = sa_open (virtual_file_position);
	if (fh == -1) return (-1);

	// verifica se l'handle e' gia' occupato. se lo e', lo libera.
	if (pvfile_datalen[handle]) unloadpv (handle);

	// aggiornamento handle interno.
	pvfile_datalen[handle] = 0;
	pvfile_dataptr[handle] = pvfile_datatop;

	// lettura numero poligoni.
	_read (fh, &pvfile_npolygs[handle], 2);

	// preparazione puntatori.
	pv_n_vtx[handle] = (char *)  (pvfile + pvfile_datatop);
	pvfile_datatop  +=  1 * pvfile_npolygs[handle];
	pvfile_x[handle] = (float *) (pvfile + pvfile_datatop);
	pvfile_datatop  += 16 * pvfile_npolygs[handle];
	pvfile_y[handle] = (float *) (pvfile + pvfile_datatop);
	pvfile_datatop  += 16 * pvfile_npolygs[handle];
	pvfile_z[handle] = (float *) (pvfile + pvfile_datatop);
	pvfile_datatop  += 16 * pvfile_npolygs[handle];
	pvfile_c[handle] = (char *)  (pvfile + pvfile_datatop);
	pvfile_datatop  +=  1 * pvfile_npolygs[handle];

	// azzera il primo puntatore dei dati per il depth sort.
	pv_mid_x[handle] = 0;

	// verifica disponibilita' memoria prima di leggere i dati.
	if (pvfile_datatop > pv_bytes) {
		pvfile_datatop = pvfile_dataptr[handle];
		_close (fh);
		return (0);
	}

	// lettura di tutti i dati sui poligoni, in un unico blocco.
	_read (fh, pvfile + pvfile_dataptr[handle],
	       pvfile_datatop - pvfile_dataptr[handle]);

	_close (fh);

	// azzeramento dati sui vertici non usati (per i triangoli)
	for (p=0; p<(Word)pvfile_npolygs[handle]; p++)
		if (pv_n_vtx[handle][p] == 3) {
			pvfile_x[handle][4*p+3] = 0;
			pvfile_y[handle][4*p+3] = 0;
			pvfile_z[handle][4*p+3] = 0;
		}

	// preparazione puntatori per la gestione del depth sorting.
	if (depth_sort) {
		pv_mid_x[handle] = (float *) (pvfile + pvfile_datatop);
		pvfile_datatop  += 4 * pvfile_npolygs[handle];
		pv_mid_y[handle] = (float *) (pvfile + pvfile_datatop);
		pvfile_datatop  += 4 * pvfile_npolygs[handle];
		pv_mid_z[handle] = (float *) (pvfile + pvfile_datatop);
		pvfile_datatop  += 4 * pvfile_npolygs[handle];
		pv_mid_d[handle] = (float *) (pvfile + pvfile_datatop);
		pvfile_datatop  += 4 * pvfile_npolygs[handle];
		pv_dep_i[handle] = (Word *)  (pvfile + pvfile_datatop);
		pvfile_datatop  += 2 * pvfile_npolygs[handle];
		if (pvfile_datatop > pv_bytes) {
			pvfile_datatop = pvfile_dataptr[handle];
			return (0);
		}
	}

	// adattamento scala, colore e traslazione.
	// (il colore viene aggiunto, come nell'originale, a 4 * npolygs bytes)
	for (c=0; c<4*(Word)pvfile_npolygs[handle]; c++) {
		pvfile_x[handle][c] *= xscale;
		pvfile_x[handle][c] += xmove;
		pvfile_y[handle][c] *= yscale;
		pvfile_y[handle][c] += ymove;
		pvfile_z[handle][c] *= zscale;
		pvfile_z[handle][c] += zmove;
		pvfile_c[handle][c] += base_color;
	}

	// calcolo punti medi e preparazione indici per il depth-sorting.
	if (depth_sort) {
		for (p=0; p<(Word)pvfile_npolygs[handle]; p++) {
			pv_dep_i[handle][p] = p;
			pv_mid_d[handle][p] = 0;
			pv_mid_x[handle][p] = 0;
			pv_mid_y[handle][p] = 0;
			pv_mid_z[handle][p] = 0;
			if (pv_n_vtx[handle][p]) {
				for (c=0; c<pv_n_vtx[handle][p]; c++) {
					pv_mid_x[handle][p] += pvfile_x[handle][4*p+c];
					pv_mid_y[handle][p] += pvfile_y[handle][4*p+c];
					pv_mid_z[handle][p] += pvfile_z[handle][4*p+c];
				}
				pv_mid_x[handle][p] /= c;
				pv_mid_y[handle][p] /= c;
				pv_mid_z[handle][p] /= c;
			}
		}
	}

	// tutto fatto: computo memoria utilizzata da questo handle.
	pvfile_datalen[handle] = pvfile_datatop - pvfile_dataptr[handle];
	return (1);
}

/* Ordinamento a ricorsione. Per mettere in ordine in fretta i poligoni,
   e i pianeti e le lune per distanza. */

void QuickSort (Word *index, float *mdist, Word start, Word end)
{
	Word  tq;

	Word  jq = end;
	Word  iq = start;

	float xq = mdist[index[(start+end)/2]];

	while (iq<=jq) {
		while (mdist[index[iq]] > xq) iq++;
		while (mdist[index[jq]] < xq) jq--;
		if (iq<=jq) {
			tq = index[iq];
			index[iq] = index[jq];
			index[jq] = tq;
			iq++;
			jq--;
		}
	}

	if (start < jq) QuickSort (index, mdist, start, jq);
	if (iq < end)   QuickSort (index, mdist, iq, end);
}

/* Traccia una figura poligonale.
   mode: 0 = poligoni in tinta unita; 1 = texture mapping;
	 2 = rimappatura randomica ricorsiva dei poligoni. */

void drawpv (Word handle, Word mode, Word rm_iterations,
	     float center_x, float center_y, float center_z,
	     char use_depth_sort)
{
	float dx, dy, dz;
	Uword p, c, i, k;

	if (handle >= handles) return;
	if (!pvfile_datalen[handle]) return;

	// traslazione intero spazio all'origine dell'oggetto.
	cam_x -= center_x;
	cam_y -= center_y;
	cam_z -= center_z;

	if (use_depth_sort && pv_mid_x[handle]) {
		// fase 1: calcolo distanza punti medi.
		for (p=0; p<pvfile_npolygs[handle]; p++) {
			dx                  = pv_mid_x[handle][p] - cam_x;
			dy                  = pv_mid_y[handle][p] - cam_y;
			dz                  = pv_mid_z[handle][p] - cam_z;
			pv_mid_d[handle][p] = dx*dx + dy*dy + dz*dz;
		}
		// fase 2: ordinamento poligoni in base alla distanza.
		QuickSort (pv_dep_i[handle], pv_mid_d[handle],
			   0, pvfile_npolygs[handle] - 1);
		// fase 3: tracciamento, nell'ordine specificato sopra.
		for (p=0; p<pvfile_npolygs[handle]; p++) {
			c = pv_dep_i[handle][p];
			i = c * 4;
			switch (mode) {
				case 0:	poly3d (pvfile_x[handle] + i,
						pvfile_y[handle] + i,
						pvfile_z[handle] + i,
						pv_n_vtx[handle][c],
						pvfile_c[handle][c]);
					break;
				case 1: k = (Uword)(Word)pvfile_c[handle][c];
					k = (k & 0xC0) | ((k & 0x3F) >> 1);
					polymap (pvfile_x[handle] + i,
						 pvfile_y[handle] + i,
						 pvfile_z[handle] + i,
						 pv_n_vtx[handle][c], k);
					break;
				case 2:	map_color_a = pvfile_c[handle][c];
					map_color_b = map_color_a - 2;
					map_color_c = map_color_a - 1;
					map_color_d = map_color_a + 1;
					randomic_mapper (pvfile_x[handle][i+0], pvfile_y[handle][i+0], pvfile_z[handle][i+0],
							 pvfile_x[handle][i+1], pvfile_y[handle][i+1], pvfile_z[handle][i+1],
							 pvfile_x[handle][i+2], pvfile_y[handle][i+2], pvfile_z[handle][i+2], rm_iterations);
					if (pv_n_vtx[handle][p] == 4)
						randomic_mapper (pvfile_x[handle][i+2], pvfile_y[handle][i+2], pvfile_z[handle][i+2],
								 pvfile_x[handle][i+3], pvfile_y[handle][i+3], pvfile_z[handle][i+3],
								 pvfile_x[handle][i+0], pvfile_y[handle][i+0], pvfile_z[handle][i+0], rm_iterations);
			}
		}
	}
	else {
		// tracciamento senza depth sorting, nell'ordine dei dati.
		for (p=0, i=0; p<pvfile_npolygs[handle]; p++, i+=4)
			switch (mode) {
				case 0:	poly3d (pvfile_x[handle] + i,
						pvfile_y[handle] + i,
						pvfile_z[handle] + i,
						pv_n_vtx[handle][p],
						pvfile_c[handle][p]);
					break;
				case 1: k = (Uword)(Word)pvfile_c[handle][p];
					k = (k & 0xC0) | ((k & 0x3F) >> 1);
					polymap (pvfile_x[handle] + i,
						 pvfile_y[handle] + i,
						 pvfile_z[handle] + i,
						 pv_n_vtx[handle][p], k);
					break;
				case 2:	map_color_a = pvfile_c[handle][p];
					map_color_b = map_color_a - 2;
					map_color_c = map_color_a - 1;
					map_color_d = map_color_a + 1;
					randomic_mapper (pvfile_x[handle][i+0], pvfile_y[handle][i+0], pvfile_z[handle][i+0],
							 pvfile_x[handle][i+1], pvfile_y[handle][i+1], pvfile_z[handle][i+1],
							 pvfile_x[handle][i+2], pvfile_y[handle][i+2], pvfile_z[handle][i+2], rm_iterations);
					if (pv_n_vtx[handle][p] == 4)
						randomic_mapper (pvfile_x[handle][i+2], pvfile_y[handle][i+2], pvfile_z[handle][i+2],
								 pvfile_x[handle][i+3], pvfile_y[handle][i+3], pvfile_z[handle][i+3],
								 pvfile_x[handle][i+0], pvfile_y[handle][i+0], pvfile_z[handle][i+0], rm_iterations);
			}
	}

	// traslazione intero spazio all'origine precedente.
	cam_x += center_x;
	cam_y += center_y;
	cam_z += center_z;
}

/* Replica una forma poligonale, copiandola da un'handle gia' definito
   a uno di uguali dimensioni. In caso d'errore, non succede nulla. */

void copypv (Word dest_handle, Word src_handle)
{
	if (src_handle >= handles) return;
	if (dest_handle >= handles) return;
	if (!pvfile_datalen[src_handle]) return;
	if (pvfile_datalen[dest_handle] != pvfile_datalen[src_handle]) return;
	_fmemmove (pv_n_vtx[dest_handle], pv_n_vtx[src_handle], pvfile_datalen[src_handle]);
}

/* Ruota una forma poligonale rispetto a uno dei suoi vertici, applicando
   anche un fattore di scalatura. Gli angoli sono espressi in gradi. */

void modpv (Word handle, Word polygon_id, Word vertex_id,
	    float x_scale, float y_scale, float z_scale,
	    float x_angle, float y_angle, float z_angle,
	    pvlist *vertexs_to_affect)
{
	if (handle >= handles) return;
	if (!pvfile_datalen[handle]) return;

	float sin_x = sin (deg * x_angle);
	float cos_x = cos (deg * x_angle);
	float sin_y = sin (deg * y_angle);
	float cos_y = cos (deg * y_angle);
	float sin_z = sin (deg * z_angle);
	float cos_z = cos (deg * z_angle);

	Word  c, p, v, i, j = 0;

	float x1, y1, z1;
	float cx, cy, cz;

	if (polygon_id > -1 && vertex_id > -1) {
		i = 4 * polygon_id + vertex_id;
		cx = pvfile_x[handle][i];
		cy = pvfile_y[handle][i];
		cz = pvfile_z[handle][i];
	}
	else {
		cx = 0;
		cy = 0;
		cz = 0;
	}

	if (!vertexs_to_affect) {
		for (p = 0; p < (Word)pvfile_npolygs[handle]; p++) {
			i = 4 * p;
			for (v = 0; v < pv_n_vtx[handle][p]; v++) {
				x1 = (pvfile_x[handle][i]-cx) * cos_y + (pvfile_z[handle][i]-cz) * sin_y;
				z1 = (pvfile_z[handle][i]-cz) * cos_y - (pvfile_x[handle][i]-cx) * sin_y;
				pvfile_z[handle][i] = z_scale * (z1 * cos_x + (pvfile_y[handle][i]-cy) * sin_x) + cz;
				y1 = (pvfile_y[handle][i]-cy) * cos_x - z1 * sin_x;
				pvfile_x[handle][i] = x_scale * (x1 * cos_z + y1 * sin_z) + cx;
				pvfile_y[handle][i] = y_scale * (y1 * cos_z - x1 * sin_z) + cy;
				i++;
			}
		}
	}
	else {
		p = 0;
		while (vertexs_to_affect[p].polygon_id != 0xFFF) {
			c = vertexs_to_affect[p].polygon_id;
			i = 4 * c;
			v = 0;
			do {	if (v==0 && vertexs_to_affect[p].vtxflag_0) { j = i;     goto perform; }
				if (v==1 && vertexs_to_affect[p].vtxflag_1) { j = i + 1; goto perform; }
				if (v==2 && vertexs_to_affect[p].vtxflag_2) { j = i + 2; goto perform; }
				if (v==3 && vertexs_to_affect[p].vtxflag_3) { j = i + 3; goto perform; }
				goto next;
		       perform: x1 = (pvfile_x[handle][j]-cx) * cos_y + (pvfile_z[handle][j]-cz) * sin_y;
				z1 = (pvfile_z[handle][j]-cz) * cos_y - (pvfile_x[handle][j]-cx) * sin_y;
				pvfile_z[handle][j] = z_scale * (z1 * cos_x + (pvfile_y[handle][j]-cy) * sin_x) + cz;
				y1 = (pvfile_y[handle][j]-cy) * cos_x - z1 * sin_x;
				pvfile_x[handle][j] = x_scale * (x1 * cos_z + y1 * sin_z) + cx;
				pvfile_y[handle][j] = y_scale * (y1 * cos_z - x1 * sin_z) + cy;
			  next: v ++;
			} while (v < pv_n_vtx[handle][c]);
			p ++;
		}
	}

	if (pv_mid_x[handle]) {
		for (p = 0; p < (Word)pvfile_npolygs[handle]; p++) {
			i = 4 * p;
			cx = 0; cy = 0; cz = 0;
			for (v = 0; v < pv_n_vtx[handle][p]; v++) {
				cx += pvfile_x[handle][i];
				cy += pvfile_y[handle][i];
				cz += pvfile_z[handle][i];
				i++;
			}
			pv_mid_x[handle][p] = cx / v;
			pv_mid_y[handle][p] = cy / v;
			pv_mid_z[handle][p] = cz / v;
		}
	}
}

/* Ritorna il corrispondente alfabetico di numeri interi e/o reali. */

char *alphavalue (double value)
{
	gcvt (value, 15, dec);
	return (dec);
}

/* Formats triad dexter/medius/sinister properly */

char *formatTriad (double value)
{
	sprintf (triadFormat, "%03d", (int)(Word)to_i16 (value));
	return triadFormat;
}

/* Background, con la mappa offsets.map, in quicktime vr. */

void background (Uword start,
		 unsigned char *target,
		 unsigned char *background,
		 unsigned char *offsetsmap,
		 Uword total_map_bytes,
		 Uword screenshift)
{
	unsigned char *es = dos_paragraph_base (target);
	unsigned char *fs = dos_paragraph_base (background);
	Uword dx = screenshift + (Uword)(target - es);
	Uword cx = total_map_bytes >> 1;
	Uword bp = start + 4;
	const unsigned char *si = offsetsmap;

	while (cx) {
		Uword w = si[0] | (si[1] << 8);
		if (w >= 64000) {
			bp += w - 64000;
		}
		else {
			Uword di = w + dx;
			unsigned char al = fs[bp];
			for (Uword r = 0; r <= 1280; r += 320)
				for (Uword k = 0; k < 5; k++)
					es[(Uword)(di + r + k)] = al;
			bp += 1;
		}
		si += 2;
		cx--;
	}
	touch (target);
}

/* Cielo stellato, tre-di vero. A campo amplificato, annovera 2744 stelle,
   con magnitudine da 0 a +13. */

/* The pseudo-random star field: each sector of 100000 units contains at
   most one star, whose position derives from the sector coordinates
   through this hash. It must match the original bit for bit. */
char star_in_sector (Dword sect_x, Dword sect_y, Dword sect_z,
				   Dword *sx, Dword *sy, Dword *sz)
{
	Udword eax = (Udword)sect_x + (Udword)sect_z;
	Udword ecx = eax;
	Udword edx = eax & 0x0001FFFF;
	edx += (Udword)sect_x;
	edx -= 50000;
	*sx = (Dword)edx;
	int64_t m = (int64_t)(Dword)eax * (int64_t)(Dword)edx;
	eax = (Udword)m;
	edx = (Udword)(m >> 32);
	edx += eax;
	ecx += edx;
	edx &= 0x0001FFFF;
	edx += (Udword)sect_y;
	edx -= 50000;
	*sy = (Dword)edx;
	eax = ecx;
	m = (int64_t)(Dword)eax * (int64_t)(Dword)edx;
	eax = (Udword)m;
	edx = (Udword)(m >> 32);
	edx += eax;
	edx &= 0x0001FFFF;
	edx += (Udword)sect_z;
	edx -= 50000;
	*sz = (Dword)edx;
	return 1;
}

void sky (Uword limits)
{
	Udword min_xy = 1000000000;

	char visible_sectors = 9;
	if (field_amplificator) visible_sectors = 14;

	unsigned char sx, sy, sz;
	float xx, yy, zz, z2, rz, inv_rz, starneg;

	if (!ap_targetting)
		starneg = 10000;
	else
		starneg = 1;

	Dword sect_x, sect_y, sect_z, rx, ry;
	Dword advance = 100000, k = 100000*visible_sectors;

	Dword temp_x, temp_y, temp_z, temp;

	/* Rarefazione delle stelle all'aumentare della distanza dal centro
	   galattico (la scala sull'asse Y e' amplificata di 30 volte). */

	Word rarity_factor;
	double distance_from_home;

	distance_from_home = sqrt (dzat_x*dzat_x + dzat_z*dzat_z);
	distance_from_home += 30 * fabs(dzat_y);
	rarity_factor = to_i16 (distance_from_home * 0.25e-8);
	rarity_factor = (Word)((rarity_factor >= 0 && rarity_factor < 16) ? (1 << rarity_factor) : 0);
	rarity_factor--;

	sect_x = to_i32 ((dzat_x - visible_sectors*50000) / 100000); sect_x *= 100000;
	sect_y = to_i32 ((dzat_y - visible_sectors*50000) / 100000); sect_y *= 100000;
	sect_z = to_i32 ((dzat_z - visible_sectors*50000) / 100000); sect_z *= 100000;

	for (sx = visible_sectors; sx; sx--) {
		for (sy = visible_sectors; sy; sy--) {
			for (sz = visible_sectors; sz; sz--) {
				star_in_sector (sect_x, sect_y, sect_z, &temp_x, &temp_y, &temp_z);
				if (!temp_x || !temp_y || !temp_z) goto next;
				if ((Uword)(temp_x + temp_y + temp_z) & (Uword)rarity_factor) goto next;

				zz = temp_z - dzat_z;
				xx = temp_x - dzat_x;
				z2 = zz * opt_tcosbeta - xx * opt_tsinbeta;
				yy = temp_y - dzat_y;
				rz = z2 * opt_tcosalfa + yy * opt_tsinalfa;
				if (rz < starneg) goto next;
				{
					inv_rz = uno / rz;
					rx = fistp32 ((xx * opt_pcosbeta + zz * opt_psinbeta) * inv_rz);
					Uword di = (Uword)rx + x_centro;
					if (!(di > 10)) goto next;
					if (!(di < 310)) goto next;
					ry = fistp32 ((yy * opt_pcosalfa - z2 * opt_psinalfa) * inv_rz);
					ry = (Dword)(((Udword)ry & 0xFFFF0000u) | (Uword)((Uword)ry - 2));
					Uword bx = (Uword)ry + y_centro;
					if (!(bx > 10)) goto next;
					if (!(bx < 190)) goto next;
					di += riga[bx];
					if (ap_targetting != 1) {
						unsigned char p = adapted[di];
						if (p == 68) goto next;
						if (p < (limits >> 8)) goto next;
						if (p > (limits & 0xFF)) goto next;
					}
					temp = fistp32 (rz);
					unsigned char cl = 13 + field_amplificator;
					Udword edx = (Udword)temp >> (cl & 31);
					unsigned char dl = (unsigned char)edx;
					if (dl <= 63) {
						unsigned char al = 63 - dl;
						unsigned char pix = adapted[di];
						adapted[di] &= 0xC0;
						al += pix & 0x3F;
						if (al > 63) al = 63;
						adapted[di] |= al;
					}
					if (ap_targetting == 1) {
						double d = (double)rx * (double)rx + (double)ry * (double)ry;
						temp = fistp32 (d);
						if ((Udword)temp < min_xy) {
							min_xy = (Udword)temp;
							ap_target_x = temp_x;
							ap_target_y = temp_y;
							ap_target_z = temp_z;
						}
					}
				}
			next:	sect_z += advance;
			}
			sect_z -= k;
			sect_y += advance;
		}
		sect_y -= k;
		sect_x += advance;
	}
}

/* Una sfera che puo' essere un pianeta o una stella, in quicktime-vr. */

unsigned char glass_bubble = 1; // Se settato, disegna una specie di bolla
				// trasparente attorno ai globi tracciati con
				// la funzione "globe" (atmosfera).

void globe (Uword start,
	    unsigned char *target,
	    unsigned char *tapestry,
	    unsigned char *offsetsmap,
	    Uword total_map_bytes,
	    double x, double y, double z,
	    float mag_factor, char colormask, char globe_saturation)
{
	Word	gman;
	Word	center_x, center_y, temp;
	double	xx, yy, zz, z2, rx, ry, rz;

	xx = x - dzat_x;
	yy = y - dzat_y;
	zz = z - dzat_z;

	rx = xx * (double)opt_pcosbeta + zz * (double)opt_psinbeta;
	z2 = zz * (double)opt_tcosbeta - xx * (double)opt_tsinbeta;
	rz = z2 * (double)opt_tcosalfa + yy * (double)opt_tsinalfa;
	ry = yy * (double)opt_pcosalfa - z2 * (double)opt_psinalfa;

	if (rz<0.001) return;

	mag_factor /= rz;
	gman = 1;

	if (mag_factor<0.01) mag_factor = 0.001;
	if (mag_factor>0.33) gman = 2;
	if (mag_factor>0.66) gman = 3;
	if (mag_factor>0.99) gman = 4;
	if (mag_factor>1.32) mag_factor = 1.32;

	rx /= rz; ry /= rz;

	if (rx<-292||rx>292) return; // 292 = (320 / 2) + (100 * 1.32)
	if (ry<-232||ry>232) return; // 232 = (200 / 2) + (100 * 1.32)

	center_x = to_i16 (rx + x_centro_f);
	center_y = to_i16 (ry + y_centro_f);

	unsigned char *es = dos_paragraph_base (target);
	unsigned char *fs = dos_paragraph_base (tapestry);
	Uword bx = start + (Uword)(tapestry - fs);
	Uword cx = total_map_bytes >> 1;
	const unsigned char *si = offsetsmap;

	while (cx) {
		if (si[0] == 100) {
			bx += si[1];
		}
		else {
			Word t = fistp16 ((float)(signed char)si[0] * mag_factor);
			Uword di = t + center_y;
			if (di >= 6 && di < 191) {
				di = riga[di];
				t = fistp16 ((float)(signed char)si[1] * mag_factor);
				Uword ax = t + center_x;
				if (ax >= 6 && ax < 311) {
					di += ax;
					unsigned char dl = fs[bx];
					if (dl < (unsigned char)globe_saturation) dl = globe_saturation;
					dl |= colormask;
					di += 4;
					for (Word r = 0; r < gman; r++)
						for (Word c = 0; c < gman; c++)
							es[(Uword)(di + 320 * r + c)] = dl;
				}
			}
			bx += 1;
		}
		si += 2;
		cx--;
	}

	if (!glass_bubble) return;

	temp = to_i16 (mag_factor * 7.25);

	if (!temp) return;

	rx = mag_factor * 110;
	ry =(1.2 * deg) / mag_factor;
	rz = 0.5 * ry;
	z2 = 0.833 * rx;
	while (rz < 2*M_PI) {
		smootharound_64 (target,
				 to_i32 (center_x + rx*cos(rz)),
				 to_i32 (center_y + z2*sin(rz)), temp, 1);
		rz += ry;
	}
}

/* Come precedente, modificata per fare globi luminosi, senza dettagli
   ma con un evidente demarcazione fra emisfero illuminato e buio.
   Viene usata per i pianeti in media distanza. */

void glowinglobe (Word start,
		  unsigned char *target,
		  unsigned char *offsetsmap,
		  Uword total_map_bytes,
		  double x, double y, double z, float mag_factor,
		  Word terminator_start, Word terminator_arc,
		  unsigned char color)
{
	Uword center_x, center_y;
	double xx, yy, zz, z2, rx, ry, rz;

	xx = x - dzat_x;
	yy = y - dzat_y;
	zz = z - dzat_z;

	rx = xx * (double)opt_pcosbeta + zz * (double)opt_psinbeta;
	z2 = zz * (double)opt_tcosbeta - xx * (double)opt_tsinbeta;
	rz = z2 * (double)opt_tcosalfa + yy * (double)opt_tsinalfa;
	ry = yy * (double)opt_pcosalfa - z2 * (double)opt_psinalfa;

	if (rz<0.001) return;

	mag_factor /= rz;

	if (mag_factor>0.66) mag_factor = 0.66;
	if (mag_factor<0.01) mag_factor = 0.001;

	rx /= rz; ry /= rz;

	if (rx<-226||rx>226) return;
	if (ry<-166||ry>166) return;

	center_x = to_u16 (rx + x_centro_f);
	center_y = to_u16 (ry + y_centro_f);

	start -= terminator_start;
	while (start < 0) start += 360;

	unsigned char *es = dos_paragraph_base (target);
	unsigned char bl = color;
	unsigned char bh = (unsigned char)((((color & 0x3F) >> 2)) | (color & 0xC0));
	Uword dx = start;
	Uword cx = total_map_bytes >> 1;
	const unsigned char *si = offsetsmap;

	while (cx) {
		if (si[0] == 100) {
			dx += si[1];
			while (dx >= 360) dx -= 360;
		}
		else {
			if (!(dx & 3)) {
				Word t = fistp16 ((float)(signed char)si[0] * mag_factor);
				Uword di = t + center_y;
				// (the original vertical check is ineffective; rows
				// outside the table only produced stray pixels)
				if (di < 200) {
					di = riga[di];
					t = fistp16 ((float)(signed char)si[1] * mag_factor);
					Uword ax = t + center_x;
					if (ax >= 9 && ax < 310) {
						di += ax;
						if (dx < (Uword)terminator_arc)
							es[(Uword)(di + 4)] = bh;
						else
							es[(Uword)(di + 4)] = bl;
					}
				}
			}
			dx += 1;
			if (dx >= 360) dx = 0;
		}
		si += 2;
		cx--;
	}
}

/* Modificata per fare globi bianchi.
   Viene usata per le stelle viste da lontano, e per la corona stellare. */

void whiteglobe (unsigned char *target,
		 double x, double y, double z,
		 float mag_factor, float fgm_factor)
{
	double center_x, center_y, mag, fgm, shade_ext, ise;
	double xx, yy, zz, z2, rx, ry, rz, xa, ya, xb, yb;
	double magsq, fgmsq;

	Uword pixptr;
	signed char pix;

	xx = x - dzat_x;
	yy = y - dzat_y;
	zz = z - dzat_z;

	rx = xx * (double)opt_pcosbeta + zz * (double)opt_psinbeta;
	z2 = zz * (double)opt_tcosbeta - xx * (double)opt_tsinbeta;
	rz = z2 * (double)opt_tcosalfa + yy * (double)opt_tsinalfa;
	ry = yy * (double)opt_pcosalfa - z2 * (double)opt_psinalfa;

	if (rz<0.001) return;

	mag_factor /= rz;

	if (mag_factor>2.99) mag_factor = 2.99;
	if (mag_factor<0.01) mag_factor = 0.01;

	rx /= rz; ry /= rz;

	if (rx<-460||rx>460) return;
	if (ry<-400||ry>400) return;

	center_x = rx + x_centro_f + 0.5;
	center_y = ry + y_centro_f + 0.5;

	mag = mag_factor * 100 + 1.5;
	fgm = fgm_factor * mag; // full globe magnitude
	shade_ext = mag - fgm;
	if (shade_ext < 1) shade_ext = 1;
	ise = 0x3F / shade_ext;

	magsq = mag * mag;
	fgmsq = fgm * fgm;

	ya = - mag * 1.2;
	yb = center_y + mag;
	yy = center_y - mag;
	while (yy < yb) {
		xa = - mag;
		xb = center_x + mag;
		xx = center_x - mag;
		while (xx < xb) {
			if (xx > 9 && xx < 313 && yy > 9 && yy < 190) {
				zz = xa*xa + ya*ya;
				if (zz < magsq) {
					if (zz > fgmsq)
						pix = (signed char)to_i16 (0x3F - (sqrt(zz) - fgm) * ise);
					else
						pix = 0x3F;
					pixptr = riga[(Word)yy] + (Word)xx;
					pix += target[pixptr];
					unsigned char v = (pix > 0x3F) ? 0x3F : (unsigned char)pix;
					target[pixptr] = v;
					target[(Uword)(pixptr+1)] = v;
					target[(Uword)(pixptr+320)] = v;
					target[(Uword)(pixptr+321)] = v;
				}
			}
			xa += 2;
			xx += 2;
		}
		ya += 2.4;
		yy += 2;
	}
}

void CalculatemovieFPS (Word moviefscap, Word moviestime) {
	float moviesecs;
	moviesecs = (float(gtime) - float(moviestime)) / 18;
	if (moviesecs != 0) {
		moviefps = float(moviefscap) / float(moviesecs);
	}
}

void ShowMovieSetup (Word moviefsec, char movieflashoff, Word moviedeck) {
	char tempsnapfile[64];

	if (movieexists == 0) {	//let's check if the moviedeck exists already.
		sprintf (tempsnapfile, "..\\MOVIES\\%03i\\00000001.BMP", moviedeck);
		FILE * tmpFile = fopen(tempsnapfile, "r");
		if (tmpFile == NULL) {
			movieexists = 1;
		} else {
			movieexists = 2;
			fclose(tmpFile);
		}
	}

	areaclear (adapted, 13, 131, 175, 180, 0, 0, 72);
	areaclear (adapted, 13, 138, 175, 139, 0, 0, 90);

	sprintf (outhudbuffer, "MOVIEDECK %3i                 (CTRL +/-)" , moviedeck);
	wrouthud (14, 141, 0, outhudbuffer);

	sprintf (outhudbuffer, "CAPTURE EVERY %3i FRAMES           (+/-)" , moviefsec);
	wrouthud (14, 148, 0, outhudbuffer);
	if (movieflashoff == 1) { wrouthud (14, 155, 0, "NO FLASH WHEN SCREENCAPTURE          (F)"); } else {wrouthud (14, 155, 0, "BLACK FLASH WHEN SCREENCAPTURE       (F)"); }

	//warning signals
	if ((moviefsec == 1 && movieflashoff == 0) || movieexists == 2) {
		areaclear (adapted, 14, 161, 174, 168, 0, 0, 85);
		wrouthud (15, 162, 0, "WARNING:");
	}
	if (moviefsec == 1 && movieflashoff == 0) wrouthud (51, 162, 0, "BLACK SCREEN!");
	if (movieexists == 2) wrouthud (106,162, 0, "MOVIEDECK EXISTS!");

	if (!movie)	{
		wrouthud (14,169, 0, "START RECORDING                  (ENTER)");
	}
	if (movie) {
		areaclear(adapted, 15, 132, 130, 137, 0, 0, 82);
		wrouthud (14,169, 0, "STOP RECORDING                   (ENTER)");
		areaclear(adapted, 131, 131, 132, 138, 0, 0, 90);
		sprintf (outhudbuffer, "FPS: %2.2f" , moviefps);
		wrouthud (133, 132, 0, outhudbuffer);
	}

	wrouthud (14,132, 0, "  NOCTIS IV+ MOVIEMAKER"); // The title, at the end to allow colorings.
}

void FitOutHudBuffer (Word min, Word max) {
	Word len = strlen(outhudbuffer);
	Word a;
	if (len<max) {
		movetextforward:
		Word move = max-len;
		for (a=len; a>=min; a--) {
			outhudbuffer[a+move]=outhudbuffer[a];
			outhudbuffer[a]=' ';
		}
		for (a=len+1; a<max-len; a++) {
			outhudbuffer[a]=' ';
		}
	} else if (len>max) {
		for (a=len-1; a>=min; a--) {
			if (outhudbuffer[a]=='.') {
				if (a==max-1) {
					outhudbuffer[max]=0;
					len=max-1;
					goto movetextforward;
				} else if (a>=max) {	//34567.23 where '5' is chr 27. a is 30.
					trimtext:
					{
						Word missingdigits = a-(max-2);
						Word writeEAt = (max-2);
						while (missingdigits>9) {
							missingdigits++;
							writeEAt--;
						}
						if (writeEAt<min) {
							sprintf(outhudbuffer+min, "     !!!!!!");
						} else {
							sprintf(outhudbuffer+writeEAt, "E%i", (int)missingdigits);
						}
						outhudbuffer[max]=0;
					}
					break;
				} else {
					outhudbuffer[max]=0;
					break;
				}
			}
		}
		if (a==min-1) {	//we finished the for loop instead of running break
			a=len;
			goto trimtext;
		}
	}
}

/* Come sopra, ma traccia in 1x1: applicabile solo alla visualizzazione
   delle stelle viste dai pianeti. */

double xsun_onscreen;

void whitesun (unsigned char *target,
	       double x, double y, double z,
	       float mag_factor, float fgm_factor)
{
	double center_x, center_y, mag, fgm, shade_ext, ise;
	double xx, yy, zz, z2, rx, ry, rz, xa, ya, xb, yb;
	double magsq, fgmsq;

	Uword pixptr;
	signed char pix;

	xx = x - dzat_x;
	yy = y - dzat_y;
	zz = z - dzat_z;

	rx = xx * (double)opt_pcosbeta + zz * (double)opt_psinbeta;
	z2 = zz * (double)opt_tcosbeta - xx * (double)opt_tsinbeta;
	rz = z2 * (double)opt_tcosalfa + yy * (double)opt_tsinalfa;
	ry = yy * (double)opt_pcosalfa - z2 * (double)opt_psinalfa;

	if (rz<0.001) return;

	mag_factor /= rz;

	if (mag_factor>2.99) mag_factor = 2.99;
	if (mag_factor<0.01) mag_factor = 0.01;

	rx /= rz; ry /= rz;
	xsun_onscreen = rx + x_centro_f;

	if (rx<-460||rx>460) return;
	if (ry<-400||ry>400) return;

	center_x = rx + x_centro_f + 0.5;
	center_y = ry + y_centro_f + 0.5;

	mag = mag_factor * 100 + 1.5;
	fgm = fgm_factor * mag; // full globe magnitude
	shade_ext = mag - fgm;
	if (shade_ext < 1) shade_ext = 1;
	ise = 0x3F / shade_ext;

	magsq = mag * mag;
	fgmsq = fgm * fgm;

	ya = - mag * 1.2;
	yb = center_y + mag;
	yy = center_y - mag;
	while (yy < yb) {
		xa = - mag;
		xb = center_x + mag;
		xx = center_x - mag;
		while (xx < xb) {
			if (xx > 9 && xx < 313 && yy > 9 && yy < 190) {
				zz = xa*xa + ya*ya;
				if (zz < magsq) {
					if (zz > fgmsq)
						pix = (signed char)to_i16 (0x3F - (sqrt(zz) - fgm) * ise);
					else
						pix = 0x3F;
					pixptr = riga[(Word)yy] + (Word)xx;
					pix += target[pixptr];
					if (pix > 0x3F)
						target[pixptr] = 0x3F;
					else
						target[pixptr] = pix;
				}
			}
			xa ++;
			xx ++;
		}
		ya += 1.2;
		yy ++;
	}
}

/* Bagliori attorno alle luci piu' intense. */

float lft_sin[361];
float lft_cos[361];

char lens_flares_init ()
{
	Word c;

	double a = 0, step = M_PI / 180;

	for (c = 0; c <= 360; c++) {
		lft_cos[c] = cos (a);
		lft_sin[c] = sin (a);
		a += step;
	}

	return (1);
}

void lens_flares_for (double cam_x, double cam_y, double cam_z,
		      double xlight, double ylight, double zlight,
		      double step, Word added, char on_hud, char condition,
		      Word xshift, Word yshift)
{
	double k = 10 / step, l = 1, u = 1.5;

	double xx, yy, zz, z2, rx, ry, rz;

	Dword xs, ys, dx, dy;
	float xr, yr;

	unsigned char temp;

	Word c, r;

	setfx (1);

	xx = xlight - cam_x;
	yy = ylight - cam_y;
	zz = zlight - cam_z;

	rx = xx * (double)opt_pcosbeta + zz * (double)opt_psinbeta;
	z2 = zz * (double)opt_tcosbeta - xx * (double)opt_tsinbeta;
	rz = z2 * (double)opt_tcosalfa + yy * (double)opt_tsinalfa;
	ry = yy * (double)opt_pcosalfa - z2 * (double)opt_psinalfa;

	if (rz>1) {
		if (step<0)
			k = -step / rz;
		else
			k = 10 / step;
		xs = to_i32 (rx / rz + xshift);
		ys = to_i32 (ry / rz + yshift);
		if (xs>-150&&ys>-90&&xs<160&&ys<90) {
			switch (condition) {
				case 1: temp = adapted[(Uword)(xs+x_centro+riga[ys+y_centro])];
					if (temp<64) goto exit_local;
					break;
				case 2: temp = adapted[(Uword)(xs+x_centro+riga[ys+y_centro])];
					if (temp<64||temp>127) goto exit_local;
					break;
			}
			for (c=0; c<180; c+=added) {
				dx = to_i32 (lft_cos[c] * k * l);
				dy = to_i32 (lft_sin[c] * k * l);
				fline (xs-dx, ys-dy, xs+dx, ys+dy);
				if (on_hud && !(c%8)) {
					dx /= 10; dy /= 10;
					xr = (float)xs * -0.1;
					yr = (float)ys * -0.1;
					for (r = 0; r < 3; r++)  {
						fline (to_i32 (xr-dx), to_i32 (yr-dy), to_i32 (xr+dx), to_i32 (yr+dy));
						dx *= 4; dy *= 4;
						xr *= 3; yr *= 3;
					}
				}
				l *= u;
				if (l>3||l<1) u = 1 / u;
			}
		}
	}

	exit_local:
	resetfx ();
}

/* Puntini lontani, per esempio pianeti e lune. */

const double  pix_dst_scale = 0.384;
const double  pix_rad_scale = 1228.8;
#define	      LIGHT_EMITTING	0
#define	      LIGHT_ABSORBING	1
#define	      MULTICOLOUR	2
char 	      pixilating_effect = LIGHT_EMITTING;
char	      pixel_spreads = 1;
unsigned char multicolourmask = 0xC0;

void single_pixel_at_ptr (Uword ptr, unsigned char pixel_color)
{
	unsigned char p = adapted[ptr];
	if (p < 64) return;
	unsigned char al = (p & 0x3F) + pixel_color;
	switch (pixilating_effect) {
		case LIGHT_EMITTING:
			if (al > 0x3E) al = 0x3E;
			adapted[ptr] = al | (p & 0xC0);
			break;
		case LIGHT_ABSORBING:
			al >>= 1;
			adapted[ptr] = al | (p & 0xC0);
			break;
		case MULTICOLOUR:
			al >>= 1;
			adapted[ptr] = al | (multicolourmask & 0xC0);
			break;
	}
}

char far_pixel_at (double xlight, double ylight, double zlight,
		   double radii, unsigned char unconditioned_color)
{
	double	xx, yy, zz, z2, rz;
	Dword	pixel_color;

	unsigned char edge_color_1;
	unsigned char edge_color_2;
	unsigned char edge_color_3;
	unsigned char edge_color_4;

	xx = xlight - dzat_x;
	yy = ylight - dzat_y;
	zz = zlight - dzat_z;

	pxx = xx * opt_pcosbeta + zz * opt_psinbeta;
	z2  = zz * opt_tcosbeta - xx * opt_tsinbeta;
	rz  = z2 * opt_tcosalfa + yy * opt_tsinalfa;
	pyy = yy * opt_pcosalfa - z2 * opt_psinalfa;

	if (rz>0.001) {
		if (!unconditioned_color) {
			pixel_color = to_i32 (64 - (rz * pix_dst_scale) + (radii * pix_rad_scale));
			if (pixel_color > 63) pixel_color = 63;
			if (pixel_color < 0) pixel_color = 0;
		}
		else
			pixel_color = unconditioned_color;
		pxx /= rz; pxx += x_centro;
		pyy /= rz; pyy += y_centro;
		if (pxx>10&&pyy>10&&pxx<310&&pyy<190) {
			vptr = to_u16 (320*(Word)pyy + pxx);
			if (pixel_spreads) {
				edge_color_1 = pixel_color >> 1;
				edge_color_2 = pixel_color >> 2;
				edge_color_3 = pixel_color >> 3;
				edge_color_4 = pixel_color >> 4;
				if (edge_color_1 > 7) {
					single_pixel_at_ptr (vptr - 320, edge_color_1);
					single_pixel_at_ptr (vptr + 320, edge_color_1);
					single_pixel_at_ptr (vptr -   1, edge_color_1);
					single_pixel_at_ptr (vptr +   1, edge_color_1);
				}
				if (edge_color_2 > 7) {
					single_pixel_at_ptr (vptr - 321, edge_color_2);
					single_pixel_at_ptr (vptr - 319, edge_color_2);
					single_pixel_at_ptr (vptr + 321, edge_color_2);
					single_pixel_at_ptr (vptr + 319, edge_color_2);
				}
				if (edge_color_3 > 7) {
					single_pixel_at_ptr (vptr - 640, edge_color_3);
					single_pixel_at_ptr (vptr + 640, edge_color_3);
					single_pixel_at_ptr (vptr -   2, edge_color_3);
					single_pixel_at_ptr (vptr +   2, edge_color_3);
				}
				if (edge_color_4 > 7) {
					single_pixel_at_ptr (vptr - 641, edge_color_4);
					single_pixel_at_ptr (vptr - 639, edge_color_4);
					single_pixel_at_ptr (vptr + 641, edge_color_4);
					single_pixel_at_ptr (vptr + 639, edge_color_4);
					single_pixel_at_ptr (vptr - 322, edge_color_4);
					single_pixel_at_ptr (vptr - 318, edge_color_4);
					single_pixel_at_ptr (vptr + 322, edge_color_4);
					single_pixel_at_ptr (vptr + 318, edge_color_4);
				}
				if (pixel_color > 7)
					single_pixel_at_ptr (vptr, pixel_color);
			}
			else {
				if (pixel_color)
					single_pixel_at_ptr (vptr, pixel_color);
			}
			return (1);
		}
	}

	return (0);
}

/*

	Resto delle funzioni che operano su dati locali.

*/

/* Misura il tempo trascorso dal 1-1-1984 ad oggi, con la precisione di
   un fotogramma. */

static void two_digits (char *dst, int v)
{
	dst[0] = '0' + (v / 10) % 10;
	dst[1] = '0' + v % 10;
}

void getsecs ()
{
	Word m;

	char dfm[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	int ye, mo, da, ho, mi, se;
	dos_get_datetime (&ye, &mo, &da, &ho, &mi, &se);
	two_digits (year, ye / 100);
	two_digits (year + 2, ye % 100);
	two_digits (month, mo);
	two_digits (day, da);
	two_digits (hours, ho);
	two_digits (minutes, mi);
	two_digits (seconds, se);

	secs = atol (year) - 1984;		      // anni dal 1984
	secs = secs * 365 + (Dword)(secs / 4);	      // anni * 365 + anni / 4

	for (m=1; m<atoi(month); m++)
		secs += dfm[m];			      // giorni per mese

	if (atoi(month)>2 && !(atol(year)%4)) secs++; // se l'anno e' bisestile

	secs += atoi(day) - 1;			      // + giorni mese attuale
	secs *= 86400;                                // secondi in un giorno

	secs += 3600 * atol(hours);		      // ora corrente in sec.
	secs += 60 * atoi(minutes);		      // minuti corr. in sec.

	isecs = atoi(seconds);			      // secondi correnti.
	secs += isecs;

	if (p_isecs != isecs) {			      // timing dei fotogrammi
		if (_delay>=10) _delay--;
		p_isecs = isecs;
		gl_fps = fps;
		fps = 1;
	}
	else {					      // frazioni di secondo
		if (gl_fps) secs += (double)fps / (double)gl_fps;
		fsecs = (double)fps / (double)gl_fps;
		fps++;
	}

	epoc = to_i16 (6011 + secs / 1e9);
}

/* Estrae dalla tabella pseudo le informazioni sulla stella che si e' scelta. */

void extract_ap_target_infos ()
{
	srand (ap_target_x/100000*ap_target_y/100000*ap_target_z/100000);

	ap_target_class = random (star_classes);
	ap_target_ray = ((float)class_ray[ap_target_class] + (float)random(class_rayvar[ap_target_class])) * 0.001;

	ap_target_r = class_rgb[3*ap_target_class+0];
	ap_target_g = class_rgb[3*ap_target_class+1];
	ap_target_b = class_rgb[3*ap_target_class+2];

	ap_target_spin = 0;
	if (ap_target_class==11) ap_target_spin = random (30) + 1;
	if (ap_target_class==7) ap_target_spin = random (12) + 1;
	if (ap_target_class==2) ap_target_spin = random (4) + 1;
}

/* Estrae un numero pseudo-casuale di tipo intero convertendolo a f-p. */

float zrandom (Word range)
{
	// (left operand first, like the Borland code generator)
	Word first = random (range);
	Word second = random (range);
	return (first - second);
}
template <typename T> static inline float zrandom_any (T range) { return zrandom (to_i16 (range)); }

/* Cerca un codice d'identificazione (per un pianeta o per una stella)
   nel file di cartografia stellare, e riporta la posizione del record.
   Se il risultato e' -1, il codice non esiste. Type: P = Pianeta, S = Stella.
   * usa come buffer di lettura "p_surfacemap". */

Word	smh;
double	idscale = 0.00001;

Dword search_id_code (double id_code, char type)
{
	Dword    	pos = 4;
	char     	found = 0;
	Uword    	n, ptr, index;

	char 	 	*buffer_ascii = (char *)p_surfacemap;

	double		id_low = id_code - idscale;
	double		id_high = id_code + idscale;

	smh = _open (starmap_file, 0);
	if (smh > -1) {
		lseek (smh, 4, SEEK_SET);
		while ((n = _read (smh, buffer_ascii, ps_bytes)) > 0) {
			ptr = 0;
			index = 0;
			while (ptr < n) {
				if (buffer_ascii[ptr + 29] == type) {
					double id;
					memcpy (&id, buffer_ascii + 8 * index, 8);
					if (id > id_low && id < id_high) {
						found = 1;
						goto stop;
					}
				}
				pos   += 32;
				ptr   += 32;
				index += 4;
			}
		}
		stop:
		_close (smh);
	}

	if (found)
		return (pos);
	else
		return (-1);
}

/* Prepara le informazioni sulla stella vicina, quella attorno alla quale
   ci si trovera': tra l'altro, prepara i pianeti estraendoli dalla
   tabella pseudo. */

Word starnop (double star_x, double star_y, double star_z)
// stima il numero di pianeti maggiori associato alle coord. di una stella
{
	Word r;
	srand ((Dword)((Dword)((Dword)(to_i32 (star_x) % 10000) * to_i32 (star_y)) % 10000 * to_i32 (star_z)) % 10000);
	r = random (class_planets[ap_target_class] + 1);
	r += random (2);
	r -= random (2);
	if (r < 0) r = 0;
	return (r);
}

void prepare_nearstar ()
{
	Word   n, c, q, r, s, t;
	double key_radius;

	if (!_delay) {
		nearstar_class = ap_target_class;
		nearstar_x = ap_target_x;
		nearstar_y = ap_target_y;
		nearstar_z = ap_target_z;
		nearstar_ray = ap_target_ray;
		nearstar_spin = ap_target_spin;
		nearstar_r = ap_target_r;
		nearstar_g = ap_target_g;
		nearstar_b = ap_target_b;
	}

	s_m = qt_M_PI * nearstar_ray * nearstar_ray * nearstar_ray * 0.01e-7;

	nearstar_identity = nearstar_x/100000*nearstar_y/100000*nearstar_z/100000;

	srand ((Dword)((Dword)((Dword)(to_i32 (nearstar_x) % 10000) * to_i32 (nearstar_y)) % 10000 * to_i32 (nearstar_z)) % 10000);

	nearstar_nop = random (class_planets[nearstar_class] + 1);

	/* Prima estrazione (pressoche' casuale, non realistica). */

	for (n=0; n<nearstar_nop; n++) {
		nearstar_p_owner[n]	 = -1;
		nearstar_p_orb_orient[n] = (double) deg * (double) random (360);
		nearstar_p_orb_seed[n]   = 3 * (Word)(n*n+1) * nearstar_ray + (float) random (300 * nearstar_ray) / 100;
		nearstar_p_tilt[n]       = zrandom_any (10*nearstar_p_orb_seed[n]) / 500;
		nearstar_p_orb_tilt[n]   = zrandom_any (10*nearstar_p_orb_seed[n]) / 5000;
		nearstar_p_orb_ecc[n]    = 1 - (double) random (nearstar_p_orb_seed[n] + 10*fabs(nearstar_p_orb_tilt[n])) / 2000;
		nearstar_p_ray[n]        = (double) random (nearstar_p_orb_seed[n]) * 0.001 + 0.01;
		nearstar_p_ring[n]	 = zrandom_any (nearstar_p_ray[n]) * (1 + (double) random (1000) / 100);
		if (nearstar_class != 8)
			nearstar_p_type[n] = random (planet_types);
		else {
			if (random(2)) {
				nearstar_p_type[n] = 10;
				nearstar_p_orb_tilt[n] *= 100;
			}
			else
				nearstar_p_type[n] = random (planet_types);
		}
		if (nearstar_class==2||nearstar_class==7||nearstar_class==15)
			nearstar_p_orb_seed[n] *= 10;
	}

	/* Aumento delle probabilita' di pianeti abitabili su classe zero. */

	if (!nearstar_class) {
		if (random(4)==2) nearstar_p_type[2] = 3;
		if (random(4)==2) nearstar_p_type[3] = 3;
		if (random(4)==2) nearstar_p_type[4] = 3;
	}

	/* Eliminazione di pianeti impossibili attorno a certe stelle.
	   Fase 1: solo quelli impossibili per tipo di stella. */

	for (n=0; n<nearstar_nop; n++) {
		switch (nearstar_class) {
			case 2: while (nearstar_p_type[n]==3)
					nearstar_p_type[n] = random (10);
				break;
			case 5: while (nearstar_p_type[n]==6||
				       nearstar_p_type[n]==9)
					nearstar_p_type[n] = random (10);
				break;
			case 7:	nearstar_p_type[n] = 9;
				break;
			case 9: while (nearstar_p_type[n]!=0&&
				       nearstar_p_type[n]!=6&&
				       nearstar_p_type[n]!=9)
					nearstar_p_type[n] = random (10);
				break;
			case 11:while (nearstar_p_type[n]!=1&&
				       nearstar_p_type[n]!=7)
					nearstar_p_type[n] = random (10);
		}
	}

	/* Eliminazione di pianeti impossibili attorno a certe stelle.
	   Fase 2: solo quelli impossibili per distanza dalla stella. */

	for (n=0; n<nearstar_nop; n++) {
		switch (nearstar_p_type[n]) {
			case 0:
				if (random(8))
					nearstar_p_type[n] ++;
				break;
			case 3:
				if ((n<2)||(n>6)||(nearstar_class&&random(4))) {
					if (random(2))
						nearstar_p_type[n]++;
					else
						nearstar_p_type[n]--;
				}
				break;
			case 7:
				if (n<7) {
					if (random(2))
						nearstar_p_type[n] --;
					else
						nearstar_p_type[n] -= 2;
				}
				break;
		}
	}

	/* Estrazione dei satelliti naturali (lune). */

	nearstar_nob = nearstar_nop;

	if (nearstar_class==2||nearstar_class==7||nearstar_class==15)
		goto no_moons;

	for (n=0; n<nearstar_nop; n++) {
		// (t=) Numero di satelliti per pianeta.
		s = nearstar_p_type[n];
		if (n < 2) {
			t = 0;
			if (s == 10)
				t = random (3);
		}
		else
			t = random (planet_possiblemoons[s] + 1);
		if (nearstar_nob + t > maxbodies)
			t = maxbodies - nearstar_nob;
		// Caratteristiche dei satelliti.
		for (c=0; c<t; c++) {
			q 			 = nearstar_nob + c;
			nearstar_p_owner[q]	 = n;
			nearstar_p_moonid[q]	 = c;
			nearstar_p_orb_orient[q] = (double) deg * (double) random (360);
			nearstar_p_orb_seed[q]   = (Word)(c*c+4) * nearstar_p_ray[n] + (float) zrandom_any (300 * nearstar_p_ray[n]) / 100;
			nearstar_p_tilt[q]       = zrandom_any (10*nearstar_p_orb_seed[q]) / 50;
			nearstar_p_orb_tilt[q]   = zrandom_any (10*nearstar_p_orb_seed[q]) / 500;
			nearstar_p_orb_ecc[q]    = 1 - (double) random (nearstar_p_orb_seed[q] + 10*fabs(nearstar_p_orb_tilt[q])) / 2000;
			nearstar_p_ray[q]        = (double) random (nearstar_p_orb_seed[n]) * 0.05 + 0.1;
			nearstar_p_ring[q]	 = 0;
			nearstar_p_type[q]       = random (planet_types);
			// Estrazione tipologia di satellite:
			r = nearstar_p_type[q];
			// Un oggetto substellare come luna? Solo una stella compagna.
			if (r==9 && s != 10) r = 2;
			// Un gigante gassoso come luna? Solo un oggetto substellare,
			// o una stella compagna in un sistema multiplo.
			if (r==6 && s < 9) r = 5;
			// "Raffreddamento" satelliti esterni.
			if (n > 7 && random(c)) r = 7;
			if (n > 9 && random(c)) r = 7;
			// Lune relativamente grandi solo attorno a pianeti gassosi
			// ed oggetti substellari.
			if (r==2 || r==3 || r==4 || r==8) {
				if (s != 6 && s < 9)
					r = 1;
			}
			// Lune abitabili attorno ai giganti gassosi.
			if (r==3 && s < 9) {
				if (n > 7)
					r = 7;
				if (nearstar_class && random(4))
					r = 5;
				if (nearstar_class == 2 ||
				    nearstar_class == 7 ||
				    nearstar_class == 11)
					r = 8;
			}
			// Una luna ghiacciata e' esclusa prima della sesta orbita.
			if (r==7 && n <= 5) r = 1;
			// Lune ghiacciate piu' frequenti attorno a stelle fredde.
			if ((nearstar_class==2||nearstar_class==5||
			     nearstar_class==7||nearstar_class==11)
			     && random(n)) r = 7;
			// Fine estrazione tipologia di satellite.
			nearstar_p_type[q] = r;
		}
		nearstar_nob += t;
	}

	/* Ri-Normalizzazione delle dimensioni dei pianeti,
	   normalizzazione delle orbite in base al principio di Keplero
	   (rinormalizzato oltre l'ottava orbita al 22%). */

no_moons:
	key_radius = nearstar_ray * planet_orb_scaling;
	if (nearstar_class == 8) key_radius *= 2;
	if (nearstar_class == 2) key_radius *= 16;
	if (nearstar_class == 7) key_radius *= 18;
	if (nearstar_class == 11) key_radius *= 20;
	for (n=0; n<nearstar_nop; n++) {
		nearstar_p_ray[n] = avg_planet_ray[(Word)nearstar_p_type[n]]
				  + avg_planet_ray[(Word)nearstar_p_type[n]] * zrandom (100) / 200;
		nearstar_p_ray[n] *= avg_planet_sizing;
		nearstar_p_orb_ray[n] = key_radius + key_radius * zrandom (100) / 500;
		nearstar_p_orb_ray[n] += key_radius * avg_planet_ray[(Word)nearstar_p_type[n]];
		if (n < 8)
			key_radius += nearstar_p_orb_ray[n];
		else
			key_radius += 0.22 * nearstar_p_orb_ray[n];
	}

	/* Ri-Normalizzazione delle dimensioni delle lune e delle loro orbite. */

	n = nearstar_nop;
	while (n < nearstar_nob) {
		q = 0;
		c = nearstar_p_owner[n];
		key_radius = nearstar_p_ray[c] * moon_orb_scaling;
		while (n<nearstar_nob && nearstar_p_owner[n] == c) {
			nearstar_p_ray[n] = avg_planet_ray[(Word)nearstar_p_type[n]]
					  + avg_planet_ray[(Word)nearstar_p_type[n]] * zrandom (100) / 200;
			nearstar_p_ray[n] *= avg_moon_sizing;
			nearstar_p_orb_ray[n] = key_radius + key_radius * zrandom (100) / 250;
			nearstar_p_orb_ray[n] += key_radius * avg_planet_ray[(Word)nearstar_p_type[n]];
			if (q < 2) key_radius += nearstar_p_orb_ray[n];
			if (q >= 2 && q < 8) key_radius += 0.12 * nearstar_p_orb_ray[n];
			if (q >= 8) key_radius += 0.025 * nearstar_p_orb_ray[n];
			q++;
			n++;
		}
	}

	/* Eliminazione di anelli improbabili. */

	for (n = 0; n < nearstar_nop; n++) {
		nearstar_p_ring[n] = 0.75 * nearstar_p_ray[n] * (2 + random(3));
		s = nearstar_p_type[n];
		if (s != 6 && s != 9) {
			if (random(5))
				nearstar_p_ring[n] = 0;
		}
		else {
			if (random(2))
				nearstar_p_ring[n] = 0;
		}
	}

	/* Conteggio degli oggetti che hanno un nome (suggerimento di Ryan) */

	nearstar_labeled = 0;
	for (n = 1; n <= nearstar_nob; n++) {
		if (search_id_code (nearstar_identity + n, 'P') != -1)
			nearstar_labeled++;
	}

	/* Reset dei periodi di rotazione (vengono calcolati con la superficie) */

	for (n = 0; n < nearstar_nob; n++)
		nearstar_p_rtperiod[n] = 0;
}

/* Smussa la superficie di un pianeta: media 4x4 rapida. */

void ssmooth (unsigned char *target)
{
	Uword cx = (Uword)((Uword)QUADWORDS << 2);
	cx -= 360 << 2;
	smooth4x4 (target, 360, cx);
}

/* Smussa leggermente la superficie di un pianeta: media 2x2. */

void lssmooth (unsigned char *target)
{
	Uword cx = (Uword)(((Uword)QUADWORDS - 80) << 2);
	unsigned char *di = target;
	while (cx) {
		unsigned char al = di[0] & 0xC0;
		unsigned char dl = (di[0] & 0x3F) + (di[1] & 0x3F);
		dl += di[360] & 0x3F;
		dl += di[361] & 0x3F;
		dl >>= 2;
		di[0] = al | dl;
		di++;
		cx--;
	}
}

Word     c, gr, r, g, b, cr, cx, cy;
float    a, kfract = 2;
char     lave, crays;
Uword    px, py;

/*

	Funzioni modulari per particolareggiare le superfici.
	Chiamate solo da surface. I parametri vengono passati nelle
	variabili all'uopo adibite: c, gr, r, g, b, cr, cx, cy, lave,
	crays, px, py, ed a.

	Lavorano tutte su p_background (che durante la definizione della
	superficie di una luna viene scambiato con s_background).

*/

/* Byte of p_background addressed the way the assembly did it: the 16-bit
   offset (including the 4 bytes of the far heap header) wraps around. */
static inline unsigned char &pbg (Uword offset)
{
	unsigned char *base = dos_paragraph_base (p_background);
	return base[(Uword)((Uword)(p_background - base) + offset)];
}

void spot () // una piccola macchia chiara sulla superficie.
{
	unsigned char &p = pbg ((Uword)(py + px));
	unsigned char al = p + (unsigned char)gr;
	if (!(al < 0x3E)) al = 0x3E;
	p = al;
}

void permanent_storm () // tempesta permanente (una macchia colossale).
{
	for (g=1; g<cr; g++) {
		for (a=0; a<2*M_PI; a+=4*deg) {
			px = to_u16 (cx + g * cos (a));
			py = to_u16 (cy + g * sin (a));
			py *= 360;
			spot ();
		}
	}
}

void crater () // un cratere.
{
	for (a=0; a<2*M_PI; a+=4*deg) {
		for (gr=0; gr<cr; gr++) {
			px = to_u16 (cx + cos (a) * gr);
			py = to_u16 (cy + sin (a) * gr);
			vptr = px + 360*py;
			unsigned char &p = pbg (vptr);
			unsigned char ah = (unsigned char)gr;
			ah >>= (lave & 31);
			if (p < ah) p = 0;
			else p = p - ah;
		}
		pbg (vptr) = 0x3E;
		pbg ((Uword)(vptr + 1)) = 0x01;
		if (crays&&!random(crays)) {
			b = (2+random(2)) * cr;
			if (cy-b>0&&cy+b<179) {
				for (gr=cr+1; gr<b; gr++) {
					px = to_u16 (cx + cos (a) * gr);
					py = to_u16 (cy + sin (a) * gr);
					vptr = px + 360*py;
					unsigned char &p = pbg (vptr);
					unsigned char al = p + (unsigned char)cr;
					if (!(al < 0x3E)) al = 0x3E;
					p = al;
				}
			}
		}
	}
}

void band () /* banda scura orizzontale: puo' essere portata al chiaro
		negando la superficie sulla base del fondo scala 0x3E */
{
	Uword di = py;
	Uword cnt = cr;
	unsigned char ah = (unsigned char)g;
	do {
		unsigned char &p = pbg (di);
		if (p < ah) p = 0;
		else p = p - ah;
		di++;
		cnt--;
	} while (cnt);
}

void wave () // Una banda come sopra, pero' ondulata.
{
	px = 360;
	Uword bx = cy;
	do {
		py = fistp16 (sin ((double)(Word)px * a) * cr);
		py += bx;
		Uword ax = (Uword)(py * 360);
		ax += 4;
		Uword di = ax + px;
		dos_paragraph_base (p_background)[di] = 0;
		px--;
	} while (px);
}

void fracture (unsigned char *target, float max_latitude)
{ // solco scuro: tipo le linee su Europa.
  // viene usata anche per simulare i fulmini quando piove.
	a = random (360) * deg;
	gr ++;

	float px = cx;
	float py = cy;

	do {
		a += (random (g) - random (g)) * deg;
		px += kfract * cos(a);
		if (px>359) px -= 360;
		if (px<0) px += 360;
		py += kfract * sin(a);
		if (py>max_latitude-1) py -= max_latitude;
		if (py<0) py += max_latitude;
		vptr = to_u16 (px + 360 * (Uword)to_u16 (py));
		target[vptr] >>= (unsigned char) b;
	gr--; } while (gr);
}

void volcano () // un krakatoa volcano.
{
	for (a=0; a<2*M_PI; a+=4*deg) {
		b = gr;
		for (g=cr/2; g<cr; g++) {
			px = to_u16 (cx + cos (a) * g);
			py = to_u16 (cy + sin (a) * g);
			py *= 360;
			spot ();
			gr--;
			if (gr<0) gr = 0;
		}
		gr = b;
	}
}

void contrast (float kt, float kq, float thrshld)
{
	Uword c;

	for (c=0; c<64800; c++) {
		a = p_background[c];
		a -= thrshld;
		if (a>0)
			a *= kt;
		else
			a *= kq;
		a += thrshld;
		if (a<0) a = 0;
		if (a>63) a = 63;
		p_background[c] = to_i16 (a);
	}
}

void randoface (Word range, Word upon)
{
	Uword c;

	for (c=0; c<64800; c++) {
		gr = p_background[c];
		if ((upon>0&&gr>=upon)||(upon<0&&gr<=-upon)) {
			gr += random (range);
			gr -= random (range);
			if (gr>63) gr = 63;
			if (gr<0) gr = 0;
			p_background[c] = gr;
		}
	}
}

void negate ()
{
	for (Uword i = 0; i < 64800; i++)
		p_background[i] = (unsigned char)(0x3E - p_background[i]);
}

void crater_juice ()
{
	lave = random (3);
	crays = random (3) * 2;
	for (c=0; c<r; c++) {
		cx = random (360);
		cr = 2 + random (1+r-c);
		while (cr>20) cr -= 10;
		cy = random (178 - 2*cr) + cr;
		crater ();
		if (cr>15) lssmooth (p_background);
	}
}

/* Funzioni di mappatura dell'atmosfera. Lavorano su "objectschart",
   a risoluzione dimezzata, con albedo delle nubi da 0 a 0x1F. */

void cirrus () // una piccola macchia chiara (nube brillante).
{
	unsigned char *ov = (unsigned char *)objectschart;
	Uword bx = (Uword)(py + px);
	bx >>= 1;
	unsigned char al = ov[bx] + (unsigned char)gr;
	if (!(al < 0x1F)) al = 0x1F;
	ov[bx] = al;
}

void atm_cyclon () // ciclone atmosferico: un'ammasso di nubi a spirale.
{
	b = 0;
	while (cr>0) {
		px = to_u16 (cx + cr * cos (a));
		py = to_u16 (cy + cr * sin (a));
		py *= 360; cirrus ();
		px += random(4); cirrus ();
		py += 359; cirrus ();
		px -= random(4); cirrus ();
		py += 361; cirrus ();
		px += random(4); cirrus ();
		b++; b %= g;
		if (!b) cr--;
		a += 6 * deg;
	}
}

void storm () // tempesta (una grande macchia chiara sull'atmosfera).
{
	for (g=1; g<cr; g++) {
		for (a=0; a<2*M_PI; a+=4*deg) {
			px = to_u16 (cx + g * cos (a));
			py = to_u16 (cy + g * sin (a));
			py *= 360;
			cirrus ();
		}
	}
}

/* Random pattern generator used for planetary and stellar surfaces. */
void random_pattern (unsigned char *dest, Uword count, Uword seed, unsigned char mask)
{
	Uword ax = seed;
	for (Uword cnt = count; cnt; cnt--) {
		ax += cnt;
		Dword prod = (Dword)(Word)ax * (Dword)(Word)ax;
		ax = (Uword)((Uword)prod + (Uword)((Udword)prod >> 16));
		*dest++ = (unsigned char)(ax & mask);
	}
}

/* Calcola la superficie estrapolandola dai dati sul pianeta e dalla
   tabella pseudo-casuale assegnatagli. Include il terminatore giorno-notte.
   "colorbase" viene assegnato a 192 per i pianeti, a 128 per le lune. */

void surface (Word logical_id, Word type,
	      double seedval, unsigned char colorbase)
{
	Word			plwp;
	Uword 			seed;
	char 			knot1 = 0, brt;
	Word 			QW = QUADWORDS;
	float 			r1, r2, r3, g1, g2, g3, b1, b2, b3;
	unsigned char 		*overlay = (unsigned char *)objectschart;

	if (type == 10) return; // stella compagna: ha superficie stellare...

	/* impostazione del periodo di rotazione */

	fast_srand (seedval + 4112);

	nearstar_p_rtperiod[logical_id] = 10 * (ranged_fast_random(50) + 1)
					+ 10 * ranged_fast_random(25)
					+ ranged_fast_random (250)
					+ 41;

	nearstar_p_rotation[logical_id] = to_i16 (secs / nearstar_p_rtperiod[logical_id]);
	nearstar_p_rotation[logical_id] %= 360;

	/* orientamento del pianeta per il successivo oscuramento del lato buio */

	plwp = 89 - cplx_planet_viewpoint (logical_id);
	plwp += nearstar_p_rotation[logical_id];
	plwp %= 360; if (plwp<0) plwp += 360;

	/* selezione della tabella pseudo relativa a questo pianeta */

	fast_srand (seedval * 10);
	seed = fast_random (0xFFFF);

	/* preparazione di una superficie standard (random pattern 0..62) */

	srand (seed);
	random_pattern (p_background, 64800, seed, 0x3E);

	/* preparazione dell'overlay per la mappatura dell'atmosfera */

	_fmemset (overlay, 0, 32400);

	/* elaborazione della superficie specifica. */

	srand (seed);
	QUADWORDS = 16200;

	switch (type) {
		case 0: r = ranged_fast_random (3) + 5;
			for (c=0; c<r; c++) ssmooth (p_background);
			for (Uword i = 0; i < 64800; i++)
				if (p_background[i] >= 28) p_background[i] = 62;
			r = ranged_fast_random (5) + 5;
			for (c=0; c<r; c++) ssmooth (p_background);
			r = 5 + ranged_fast_random (26);
			for (c=0; c<r; c++) {
				cr = 5 + ranged_fast_random (20);
				cx = ranged_fast_random (360);
				cy = ranged_fast_random (130) + 25;
				gr = ranged_fast_random (cr/2) + cr/2 + 2;
				volcano ();
			}
			r = 100 + ranged_fast_random (100);
			b = ranged_fast_random (3) + 1;
			g = 360;
			for (c=0; c<r; c++) {
				cx = ranged_fast_random (360);
				cy = ranged_fast_random (180);
				gr = ranged_fast_random (100);
				fracture (p_background, 180);
			}
			lssmooth (p_background);
			break;
		case 1: if (ranged_fast_random(2)) ssmooth (p_background);
			r = 10 + ranged_fast_random (41);
			crater_juice ();
			lssmooth (p_background);
			if (!ranged_fast_random(5)) negate ();
			break;
		case 2:	r = 5 + ranged_fast_random (25);
			for (c=0; c<r; c++) {
				cr = ranged_fast_random (20) + 1;
				cy = ranged_fast_random (178 - 2*cr) + cr;
				switch (random(2)) {
					case 0: cx = (Word)(((Dword)to_i32 (10 * secs) / (Dword)(ranged_fast_random (3600) + 180)) % 360);
						gr = ranged_fast_random (12) + 2;
						storm ();
						break;
					case 1:	gr = ranged_fast_random (15) + 3;
						py = cy * 360; cr *= 360;
						g = 1 + ranged_fast_random (gr);
						band ();
				}
			}
			if (!ranged_fast_random(3)) negate ();
			break;
		case 3: {
			r = ranged_fast_random(3) + 4;
			g = 26 + ranged_fast_random(3) - ranged_fast_random(5);
			for (c=0; c<r; c++) ssmooth (p_background);
			Uword ax = seed;
			Uword cnt = 64000;
			for (Uword i = 0; i < 64000; i++, cnt--) {
				if (p_background[i] < (unsigned char)g) {
					p_background[i] = 16;
					continue;
				}
				ax += cnt;
				Dword prod = (Dword)(Word)ax * (Dword)(Word)ax;
				ax = (Uword)((Uword)prod + (Uword)((Udword)prod >> 16));
				p_background[i] += (unsigned char)(ax & 0x3E);
				if (!(p_background[i] < 0x3E)) {
					p_background[i] = 0x3E;
					p_background[i+1] = 0x00;
				}
			}
			r = 20 + ranged_fast_random (40);
			for (c=0; c<r; c++) {
				gr = ranged_fast_random (5) + 1;
				cr = ranged_fast_random (10) + 10;
				if (ranged_fast_random(3))
					cy = ranged_fast_random (172 - 2*cr) + cr + 2;
				else
					cy = 60 + ranged_fast_random (10) - ranged_fast_random (10);
				cx = (Word)(((Dword)to_i32 (secs) / (Dword)(ranged_fast_random (360) + 180)) % 360);
				g  = ranged_fast_random (5) + 7;
				a  = ranged_fast_random (360) * deg;
				atm_cyclon ();
			}
			break;
		}
		case 4: ssmooth (p_background);
			if (ranged_fast_random(2)) ssmooth (p_background);
			for (Uword i = 0; i < 64000; i++) {
				if (p_background[i] == 32) {
					p_background[i] = 0x01;
					p_background[i+1] = 0x3E;
					p_background[i+360] = 0x01;
				}
			}
			r = ranged_fast_random (30);
			if (r>20) r *= 10;
			b = ranged_fast_random (3) + 1;
			g = 200 + ranged_fast_random (300);
			for (c=0; c<r; c++) {
				cx = ranged_fast_random (360);
				cy = ranged_fast_random (180);
				gr = 50 + ranged_fast_random (100);
				fracture (p_background, 180);
			}
			r = ranged_fast_random (25) + 1; crater_juice ();
			lssmooth (p_background);
			if (ranged_fast_random(2)) lssmooth (p_background);
			break;
		case 5: r = ranged_fast_random (3) + 4;
			for (c=0; c<r; c++) ssmooth (p_background);
			{ // (arguments evaluated right to left, as in Borland C++)
				auto _seq2 = 25 + ranged_fast_random(3);
				auto _seq1 = (float)ranged_fast_random(350) / 100 + 4.0;
				auto _seq0 = (float)ranged_fast_random(200) / 900 + 0.6;
				contrast (_seq0, _seq1, _seq2);
			}
			{ // (arguments evaluated right to left, as in Borland C++)
				auto _seq1 = -20 * (ranged_fast_random(3) + 1);
				auto _seq0 = 5 + ranged_fast_random(3);
				randoface (_seq0, _seq1);
			}
			r = 5 + ranged_fast_random (5);
			for (c=0; c<r; c++) {
				cr = 5 + ranged_fast_random (10);
				cx = ranged_fast_random (360);
				cy = ranged_fast_random (145) + 15;
				gr = ranged_fast_random (cr/2) + 2;
				volcano ();
			}
			r = 5 + ranged_fast_random (5);
			for (c=0; c<r; c++) {
				cr = ranged_fast_random (30) + 1;
				cy = ranged_fast_random (178 - 2*cr) + cr;
				cx = (Word)(((Dword)to_i32 (60*secs) / (Dword)(ranged_fast_random (3600) + 360)) % 360);
				gr = ranged_fast_random (2) + 1;
				permanent_storm ();
			}
			for (c=0; c<10000; c++) {
				gr = ranged_fast_random (10) + 10;
				px = ranged_fast_random (360);
				py = ranged_fast_random (10);
				py *= 360; spot ();
				px = ranged_fast_random (360);
				py = 125 - ranged_fast_random (10);
				py *= 360; spot ();
			}
			if (ranged_fast_random(2))
				ssmooth (p_background);
			else
				lssmooth (p_background);
			break;
		case 6:	r = 3 + ranged_fast_random (5);
			for (c=0; c<r; c++) ssmooth (p_background);
			r = 50 + ranged_fast_random (100);
			for (c=0; c<r; c++) {
				cr = ranged_fast_random (10) + 1;
				cy = ranged_fast_random (178 - 2*cr) + cr;
				if (ranged_fast_random(8)) {
					gr = ranged_fast_random (5) + 2;
					g = 1 + ranged_fast_random (gr);
					py = cy * 360;
					cr *= 360;
					band ();
				}
				else {
					a = (float) (5 + ranged_fast_random(10)) / 30;
					cr = cr / 4 + 1;
					wave ();
				}
			}
			r = 50 + ranged_fast_random (100);
			for (c=0; c<r; c++) {
				cr = ranged_fast_random (15) + 1;
				cy = ranged_fast_random (178 - 2*cr) + cr;
				cx = (Word)(((Dword)to_i32 (60*secs) / (Dword)(ranged_fast_random (8000) + 360)) % 360);
				gr = ranged_fast_random (2) + 1;
				if (ranged_fast_random(10))
					cr = cr / 2 + 1;
				else
					gr *= 3;
				storm ();
			}
			lssmooth (p_background);
			if (!ranged_fast_random(3)) negate ();
			break;
		case 7: r = 5 + ranged_fast_random (5);
			for (c=0; c<r; c++) ssmooth (p_background);
			r = 10 + ranged_fast_random (50);
			g = 5 + ranged_fast_random (20);
			b = ranged_fast_random (2) + 1;
			for (c=0; c<r; c++) {
				cx = ranged_fast_random (360);
				cy = ranged_fast_random (180);
				gr = ranged_fast_random (300);
				fracture (p_background, 180);
			}
			if (ranged_fast_random(2)) lssmooth (p_background);
			randoface (1 + ranged_fast_random(10), 1);
			if (ranged_fast_random(2)) negate ();
			break;
		case 8:	r = ranged_fast_random (10) + 1;
			for (c=0; c<r; c++) lssmooth (p_background);
			r = 100 + ranged_fast_random (50);
			for (c=0; c<r; c++) {
				cr = ranged_fast_random (5) + 1;
				gr = ranged_fast_random (5) + 1;
				cx = ranged_fast_random (360);
				cy = ranged_fast_random (178 - 2*cr) + cr;
				permanent_storm ();
			}
			if (ranged_fast_random(2)) negate ();
			break;
		case 9: pclear (p_background, 0x1F);
			for (px = 0; px < 32400; px++) overlay[px] = 0x1F;
	}

	/* rinormalizzazione superficie a intervallo 00-1F per pianeti
	   felisiani e per quelli simili a marte. */

	if (type==3||type==5) {
		for (px = 0; px < 64800; px++)
			p_background[px] >>= 1;
	}

	/* ritocchi specifici finali alla superficie - pianeti felisiani */

	if (type==3) {
		if (ranged_fast_random(2))
			lssmooth (p_background);
		else
			ssmooth (p_background);
	}

	/* fusione mappa terreno + overlay atmosfera. */

	for (px = 0, py = 0; px < 32400; py += 2, px++) {
		p_background[py] += overlay[px];
		if (p_background[py] > 0x3E) p_background[py] = 0x3E;
		p_background[py+1] += overlay[px];
		if (p_background[py+1] > 0x3E) p_background[py+1] = 0x3E;
	}

	/* ritocchi specifici finali alla superficie - pianeti venusiani */

	if (type==2) {
		if (!random(3)) {
			psmooth_grays (p_background);
			knot1 = 1;
		}
	}

	/* applicazione terminatore giorno-notte */

	nearstar_p_term_start[logical_id] = plwp + 35;
	if (nearstar_p_term_start[logical_id] >= 360) nearstar_p_term_start[logical_id] -= 360;
	nearstar_p_term_end[logical_id] = nearstar_p_term_start[logical_id] + 130;
	if (nearstar_p_term_end[logical_id] >= 360) nearstar_p_term_end[logical_id] -= 360;

	{
		Uword di = plwp + 35;
		for (Word row = 0; row < 179; row++) {
			for (Word k = 0; k < 130; k++) {
				pbg (di) >>= 2;
				di++;
			}
			di += 230;
		}
	}

	/* ritocchi specifici finali alla superficie - altri pianeti */

	if (type==2) {
		if (knot1)
			ssmooth (p_background);
		else {
			r = 3 + ranged_fast_random (5);
			for (c=0; c<r; c++)
				ssmooth (p_background);
		}
	}

	if (type==6) {
		for (c=0; c<3; c++)
			if (ranged_fast_random(2))
				ssmooth (p_background);
	}

	if (type==9) for (c=0; c<6; c++) ssmooth (p_background);

	/* Elaborazione tabella dei colori (ridefinisce da 192 a 255). */

	if (colorbase == 255) {
		QUADWORDS = QW;
		return;
	}

	type <<= 2;

	r = planet_rgb_and_var[type+0];
	g = planet_rgb_and_var[type+1];
	b = planet_rgb_and_var[type+2];
	c = planet_rgb_and_var[type+3];

	r <<= 1; r += nearstar_r; r >>= 1;
	g <<= 1; g += nearstar_g; g >>= 1;
	b <<= 1; b += nearstar_b; b >>= 1;

	r1 = r + random(c) - random(c);
	g1 = g + random(c) - random(c);
	b1 = b + random(c) - random(c);
	r2 = r + random(c) - random(c);
	g2 = g + random(c) - random(c);
	b2 = b + random(c) - random(c);
	r3 = r + random(c) - random(c);
	g3 = g + random(c) - random(c);
	b3 = b + random(c) - random(c);

	r1 *= 0.25; g1 *= 0.25; b1 *= 0.25;
	r2 *= 0.75; g2 *= 0.75; b2 *= 0.75;
	r3 *= 1.25; g3 *= 1.25; b3 *= 1.25;

	type >>= 2;

	shade (tmppal, colorbase + 00, 16, 00, 00, 00, r1, g1, b1);
	shade (tmppal, colorbase + 16, 16, r1, g1, b1, r2, g2, b2);
	shade (tmppal, colorbase + 32, 16, r2, g2, b2, r3, g3, b3);
	shade (tmppal, colorbase + 48, 16, r3, g3, b3, 64, 64, 64);

	brt = nearstar_p_owner[logical_id];
	if (brt == -1) brt = logical_id;

	if (brt <= 4)
		brt = 64;
	else
		brt = 64 - (4 * (brt - 4));

	tavola_colori (tmppal + 3*colorbase, colorbase, 64, brt, brt, brt);

	QUADWORDS = QW;
}

/* Tracciamento degli anelli (eventuali). */

void ring (Word planet_id, double ox, double oy, double oz, Word start, Word layers)
{
	Word 	a, b, c, n, m, partn, partcls;
	double	sx, sy, sz;

	double 	ringray = nearstar_p_ring[planet_id];
	double	ringtilt = 0.1 * ringray * nearstar_p_tilt[planet_id];
	double 	step = 0.0075 * ringray;

	fast_srand (10000 * ringray + planet_id);
	b = 1 + fast_random (0x1F) - layers;
	partcls = 1 + fast_random (3);
	while (b > 0) {
		if (!fast_random(7)) {
			c = 1 + fast_random (3);
			pixel_spreads = 0;
		}
		else {
			c = 1 + fast_random (7);
			pixel_spreads = 1;
		}
		a = start - (start % c);
		pixilating_effect = fast_random(1) + fast_random(1);
		if (a < 0) a += 360;
		n = c; while (n + c < 180) {
			m = c + fast_random (1);
			n += m; a += m; if (a > 360) a -= 360;
			sy = oy - ringtilt * lft_sin[a];
			sx = ox + ringray  * lft_sin[a];
			sz = oz + ringray  * lft_cos[a];
			partn = partcls;
			while (partn) {
				sz += step - (fast_flandom() * step);
				sx += step - (fast_flandom() * step);
				far_pixel_at (sx, sy, sz, -0.042, 0);
				partn--;
			}
		}
		ringray += step;
		if (!fast_random(7)) ringray += 5 * step;
		ringtilt = 0.1 * ringray * nearstar_p_tilt[planet_id];
		b--;
	}

	pixilating_effect = LIGHT_EMITTING;
	pixel_spreads = 1;
}

/* Visualizza appopriatamente i pianeti, come punti, barlumi di luce
   o globi ben visibili, a seconda di distanza e raggio. */

void planets ()
{
	char *atmosphere = (char *) objectschart;
	unsigned char *surface_backup = (unsigned char *)p_background;

	char is_moon;
	Dword poffs;
	Dword test;

	Word i1, i2, n1, n2, c1, c2, pnpcs;
	Word c, n, t, ts, te, ll, plwp = 0, riwp;

	Word te_ll_distance, te_ll_distance_1, te_ll_distance_2;
	Word ts_ll_distance, ts_ll_distance_1, ts_ll_distance_2;

	double xx, yy, zz;
	double d3, d2, md2 = 1E9;

	unsigned char colorbase, showdisc, showrings, surfacemap;
	Word ringlayers = 0;

	if (ip_targetting) ip_targetted = -1;
	if (!nearstar_nop) return;

	for (n=0; n<nearstar_nob; n++) {
		planet_xyz (n);
		xx = plx - dzat_x;
		yy = ply - dzat_y;
		zz = plz - dzat_z;
		nearstar_p_qsortindex[n] = n;
		nearstar_p_qsortdist[n]  = sqrt (xx*xx+yy*yy+zz*zz);
	}

	QuickSort (nearstar_p_qsortindex,
		   nearstar_p_qsortdist,
		   0, nearstar_nob - 1);

	if (nearstar_nob == 1) {
		pnpcs = npcs;
		npcs  = 0;
		if (npcs != pnpcs) {
			resident_map1 = 0;
			resident_map2 = -1;
		}
	}
	else {
		pnpcs = npcs;
		n1 = nearstar_nob - 1;
		i1 = nearstar_p_qsortindex[n1];
		c1 = nearstar_p_owner[i1];
		n2 = nearstar_nob - 2;
		i2 = nearstar_p_qsortindex[n2];
		c2 = nearstar_p_owner[i2];
		if (c1 > -1) {
			while (c2 > -1 && n2 > 0) {
				n2 --;
				i2 = nearstar_p_qsortindex[n2];
				c2 = nearstar_p_owner[i2];
			}
		}
		npcs = i1 * maxbodies - i2;
		if (npcs != pnpcs) {
			resident_map1 = i1;
			resident_map2 = i2;
		}
	}

	for (c=0; c<nearstar_nob; c++) {
		n  = nearstar_p_qsortindex[c];
		//
		d3 = nearstar_p_qsortdist[n];
		planet_xyz (n);
		if (nearstar_p_owner[n] > -1) {
			p_background = s_background;
			colorbase = 128;
			is_moon = 1;
		}
		else {
			p_background = surface_backup;
			colorbase = 192;
			is_moon = 0;
		}
		showdisc = 0;
		showrings = 0;
		surfacemap = 0;
		if (d3 < 250*nearstar_p_ray[n]) {
			showrings = 1;
			ringlayers = to_i16 (0.05 * (d3 / nearstar_p_ray[n]));
			if (d3 < 100*nearstar_p_ray[n]) {
				showdisc = 1;
				if (d3 < 25*nearstar_p_ray[n])
					surfacemap = 1;
			}
		}
		//
		if (ip_targetting || !showdisc) {
			far_pixel_at (plx, ply, plz, nearstar_p_ray[n], 0);
			if (ip_targetting) {
				pxx -= x_centro;
				pyy -= y_centro;
				d2 = pxx*pxx + pyy*pyy;
				if (d2<md2) {
					md2 = d2;
					ip_targetted = n;
				}
			}
		}
		//
		if (nearstar_p_type[n] == 10) goto notaplanet;
		//
		if (showrings) {
			if (surfacemap)
				multicolourmask = 0xC0;
			else
				multicolourmask = 0x40;
			plwp = 359 - planet_viewpoint (dzat_x, dzat_z);
			if (nearstar_p_ring[n]) {
				riwp = plwp + 180;
				if (riwp > 359) riwp -= 360;
				ring (n, plx, ply, plz, riwp, ringlayers);
			}
		}
		if (showdisc) {
		    if (surfacemap) {
			if (n==resident_map1 || n==resident_map2) {
				if (is_moon) {
					if (nearstar_p_type[n]) {
						surface (n, nearstar_p_type[n],
							 1000000		*
							 nearstar_ray		*
							 nearstar_p_type[n]     *
							 nearstar_p_orb_orient[n],
							 128);
					}
					else {
						surface (n, nearstar_p_type[n],
							 2000000 * n		*
							 nearstar_ray		*
							 nearstar_p_orb_orient[n],
							 128);
					}
				}
				else {
					if (nearstar_p_type[n]) {
						surface (n, nearstar_p_type[n],
							 1000000		*
							 nearstar_p_type[n]     *
							 nearstar_p_orb_seed[n] *
							 nearstar_p_orb_tilt[n] *
							 nearstar_p_orb_ecc[n]  *
							 nearstar_p_orb_orient[n],
							 192);
					}
					else {
						surface (n, nearstar_p_type[n],
							 2000000 * n		*
							 nearstar_p_orb_seed[n] *
							 nearstar_p_orb_tilt[n] *
							 nearstar_p_orb_ecc[n]  *
							 nearstar_p_orb_orient[n],
							 192);
					}
				}
				if (n==resident_map1) resident_map1 = -1;
				if (n==resident_map2) resident_map2 = -1;
			}
			if (n == ip_targetted && landing_point) {
				nightzone = 0;
				//
				ts = nearstar_p_term_start[n];
				te = nearstar_p_term_end[n];
				ll = landing_pt_lon;
				//
				if (ts > te) {
					if (ll >= ts
						|| ll < te)
							nightzone = 1;
				}
				else {
					if (ll >= ts
						&& ll < te)
							nightzone = 1;
				}
				//
				te_ll_distance_1 = 0;
				ll = landing_pt_lon;
				while (ll != te) {
					te_ll_distance_1++;
					ll++; if (ll >= 360) ll = 0;
				}
				te_ll_distance_2 = 0;
				ll = landing_pt_lon;
				while (ll != te) {
					te_ll_distance_2++;
					ll--; if (ll <= -1) ll = 359;
				}
				if (te_ll_distance_1 < te_ll_distance_2)
					te_ll_distance = te_ll_distance_1;
				else
					te_ll_distance = te_ll_distance_2;
				ts_ll_distance_1 = 0;
				ll = landing_pt_lon;
				while (ll != ts) {
					ts_ll_distance_1++;
					ll++; if (ll >= 360) ll = 0;
				}
				ts_ll_distance_2 = 0;
				ll = landing_pt_lon;
				while (ll != ts) {
					ts_ll_distance_2++;
					ll--; if (ll <= -1) ll = 359;
				}
				if (ts_ll_distance_1 < ts_ll_distance_2)
					ts_ll_distance = ts_ll_distance_1;
				else
					ts_ll_distance = ts_ll_distance_2;
				if (ts_ll_distance <= te_ll_distance) {
					sun_x_factor = +1;
					crepzone = ts_ll_distance;
				}
				else {
					sun_x_factor = -1;
					crepzone = te_ll_distance;
				}
				//
				fast_srand (nearstar_p_orb_seed[n] * nearstar_p_orb_ecc[n] * 12345);
				ptr = 360 * landing_pt_lat + landing_pt_lon;
				sky_red_filter = fast_random (31) + 32;
				sky_grn_filter = fast_random (15) + 48;
				sky_blu_filter = fast_random (15) + 48;
				if (is_moon) {
					gnd_red_filter = tmppal[(3 * p_background[ptr]) + 128*3 + 0];
					gnd_grn_filter = tmppal[(3 * p_background[ptr]) + 128*3 + 1];
					gnd_blu_filter = tmppal[(3 * p_background[ptr]) + 128*3 + 2];
				}
				else {
					gnd_red_filter = tmppal[(3 * p_background[ptr]) + 192*3 + 0];
					gnd_grn_filter = tmppal[(3 * p_background[ptr]) + 192*3 + 1];
					gnd_blu_filter = tmppal[(3 * p_background[ptr]) + 192*3 + 2];
				}
				gnd_red_filter += fast_random (15);
				gnd_grn_filter += fast_random (15);
				gnd_blu_filter += fast_random (15);
				test = nearstar_p_type[n];
				if (nightzone) {
					albedo = p_background[ptr];
					albedo <<= 2;
					if (test==3||test==5)
						albedo -= atmosphere[ptr>>1];
					albedo >>= 2;
					albedo <<= 2;
				}
				else {
					albedo = p_background[ptr];
					if (test==3||test==5)
						albedo -= atmosphere[ptr>>1];
					albedo >>= 2;
					albedo <<= 2;
				}
				if (test==3||test==5) albedo *= 2; // da 0 a 1F --> da 0 a 3F
				rainy = (float)atmosphere[ptr>>1] * 0.25;
				if (rainy > 5) rainy = 5;
				if (nightzone) {
					sky_grn_filter /= 2;
					sky_blu_filter /= 2;
					if (crepzone > 5) {
						sky_red_filter /= 2;
						sky_brightness = 8;
						horiz_brt = 10;
					}
					else {
						sky_brightness = 32;
						horiz_brt = 16;
					}
				}
				else {
					if (crepzone > 5) {
						sky_brightness = 48;
						horiz_brt = 20;
					}
					else {
						sky_grn_filter /= 2;
						sky_blu_filter /= 3;
						sky_brightness = 40;
						horiz_brt = 13;
					}
				}
				for (poffs = -180; poffs < 180; poffs++) {
					test = poffs + ptr;
					if (test > 0 && test < 64800)
						p_background[test] ^= 0x1E;
				}
				for (poffs = -60; poffs < 60; poffs++) {
					if (poffs) {
						test = 360 * poffs + ptr;
						if (test > 0 && test < 64800)
							p_background[test] ^= 0x1E;
					}
				}
			}
			t = nearstar_p_type[n];
			if (t == 2 || t == 3 || t == 6 || t == 8 || t == 9)
				glass_bubble = 1;
			else
				glass_bubble = 0;
			globe (plwp + nearstar_p_rotation[n],
			       adapted, p_background, n_globes_map, gl_bytes,
			       plx, ply, plz, nearstar_p_ray[n], colorbase, 0);
			if (n == ip_targetted && landing_point) {
				for (poffs = -180; poffs < 180; poffs++) {
					test = poffs + ptr;
					if (test > 0 && test < 64800)
						p_background[test] ^= 0x1E;
				}
				for (poffs = -60; poffs < 60; poffs++) {
					if (poffs) {
						test = 360 * poffs + ptr;
						if (test > 0 && test < 64800)
							p_background[test] ^= 0x1E;
					}
				}
			}
		    }
		    else {
			    ts = (89+35) - cplx_planet_viewpoint (n);
			    if (ts < 0) ts += 360;
			    if (ts > 359) ts -= 360;
			    glowinglobe (plwp, adapted, n_globes_map, gl_bytes, plx,
					 ply, plz, nearstar_p_ray[n], ts, 130, 127);
		    }
		}
		if (showrings) {
			if (nearstar_p_ring[n]) {
				ring (n, plx, ply, plz, plwp, ringlayers);
				if (!showdisc) far_pixel_at (plx, ply, plz, nearstar_p_ray[n], 0);
			}
		}
		notaplanet:;
	}

	p_background = surface_backup;
}

// Necessaria all'inizializzazione (ottimizzazione caricamento segmenti).

void init_FP_segments ()
{
}

/*

	Gestione cartografia galattica.

	Ad ogni stella e' assegnato un codice d'identificazione univoco,
	derivante dalle sue coordinate. I codici identificativi dei pianeti
	vengono calcolati a partire da quello della loro stella, piu' il
	numero (progressivo) del pianeta in ordine di distanza.

*/

double 	laststar_x, laststar_y, laststar_z;

/* Determina se una certa stella, di cui si specifica il codice
   d'identificazione univoco, e' attualmente nel range dei sensori. */

char isthere (double star_id)
{
	char 		visible_sectors = 9;

	Dword  		sect_x, sect_y, sect_z;
	Dword  		k, advance = 100000;

	double 		sidlow = star_id - idscale;
	double 		sidhigh = star_id + idscale;

	unsigned char 	sx, sy, sz;
	double 		laststar_id;

	sect_x = to_i32 ((dzat_x - visible_sectors*50000) / 100000); sect_x *= 100000;
	sect_y = to_i32 ((dzat_y - visible_sectors*50000) / 100000); sect_y *= 100000;
	sect_z = to_i32 ((dzat_z - visible_sectors*50000) / 100000); sect_z *= 100000;

	if (field_amplificator) visible_sectors = 14;
	k = 100000 * visible_sectors;

	for (sx = visible_sectors; sx; sx--) {
		for (sy = visible_sectors; sy; sy--) {
			for (sz = visible_sectors; sz; sz--) {
				Dword tx, ty, tz;
				star_in_sector (sect_x, sect_y, sect_z, &tx, &ty, &tz);
				laststar_x = tx;
				laststar_y = ty;
				laststar_z = tz;
				// (computed on the x87 stack in extended precision)
				long double id = ((long double)tx * idscale)
					* (((long double)ty * idscale) * ((long double)tz * idscale));
				laststar_id = (double)id;
				if (!(id < sidlow) && laststar_id < sidhigh)
					return (1);
				sect_z += advance;
			}
			sect_z -= k;
			sect_y += advance;
		}
		sect_y -= k;
		sect_x += advance;
	}

	return (0);
}

/* Ricerca tutte le stelle note visibili, fino a 50 contemporaneamente. */

const Word 	tgt_bytes_per_scan = 5 * 32;
Dword 		tgt_collect_lastpos = 0;

Word   		targets = 0;
Word   		topmost_target = 0;
Word   		target_line = 0;
char   		update_targets = 0;
char   		collecting_targets = 0;

double 		targets_table_id[50];
double 		targets_table_px[50];
double 		targets_table_py[50];
double 		targets_table_pz[50];

void collect_targets ()
{
	Word		local_smh;
	Uword 		n, ptr, index, toread;

	char 		*buffer_ascii = (char *)p_surfacemap;

	local_smh = _open (starmap_file, 0);
	if (local_smh > -1) {
		toread = tgt_bytes_per_scan;
		while (toread) {
			lseek (local_smh, tgt_collect_lastpos, SEEK_SET);
			n = _read (local_smh, buffer_ascii, toread);
			if (!n || n == (Uword)-1) {
				collecting_targets = 0;
				goto stop;
			}
			ptr = 0;
			index = 0;
			toread = 0;
			while (ptr < n) {
				if (buffer_ascii[ptr + 29] == 'S') {
					double id;
					memcpy (&id, buffer_ascii + 8 * index, 8);
					if (isthere (id)) {
						targets_table_id[targets] = id;
						targets_table_px[targets] = laststar_x;
						targets_table_py[targets] = laststar_y;
						targets_table_pz[targets] = laststar_z;
						if (targets < 3)
							target_line = targets;
						else
							target_line = 2;
						if (targets > 2)
							topmost_target = targets - 2;
						update_targets = 1;
						targets++;
						if (targets == 50) {
							collecting_targets = 0;
							goto stop;
						}
					}
				}
				else
					toread += 32;
				ptr += 32;
				index += 4;
				tgt_collect_lastpos += 32;
			}
		}
	  stop: _close (local_smh);
	}
}

/* Cambia lo stato visualizzato dall'FCS sull'Head-Up-Display. */

void status (const char *status_description, Word message_delay)
{
	if (message_delay >= fcs_status_delay) {
		Word len = strlen(status_description);
		if (len<10) {
			memcpy(fcs_status, status_description, len);
			fcs_status[len]=0;
			memcpy(fcs_status_extended, fcs_status, len+1);
		} else if (len<41) {
			memcpy(fcs_status, status_description, 10);
			fcs_status[10]=0;
			memcpy(fcs_status_extended, status_description, len);
			fcs_status_extended[len]=0;
		} else {
			memcpy(fcs_status, status_description, 10);
			fcs_status[10]=0;
			memcpy(fcs_status_extended, status_description, 41);
			fcs_status_extended[41]=0;
		}
		fcs_status_delay = message_delay;
	}
}

/* Mappa caratteri per l'HUD: font iper-compatto (3 x 5 pixels). */

char digimap[65*5] = {
	 0, 0, 0, 0, 0, // 32. spazio vuoto
	 2, 2, 2, 0, 2, // 33. punto esclamativo
	 5, 0, 0, 0, 0, // 34. virgolette
	 0, 0, 3, 5, 5, // 35. cancelletto (rappresentato da una piccola n.)
	 2, 2, 6, 2, 2, // 36. sistro
	 1, 4, 2, 1, 4, // 37. percento
	 0, 0, 2, 0, 0, // 38. e commericale (non e' possibile visualizzarla)
	 0, 2, 2, 0, 0, // 39. apice
	 4, 2, 2, 2, 4, // 40. parentesi tonda aperta
	 1, 2, 2, 2, 1, // 41. parentesi tonda chiusa
	 0, 0, 7, 2, 2, // 42. asterisco
	 0, 2, 7, 2, 0, // 43. segno piu'
	 0, 0, 0, 2, 1, // 44. virgola
	 0, 0, 7, 0, 0, // 45. segno meno
	 0, 0, 0, 0, 2, // 46. punto
	 0, 4, 2, 1, 0, // 47. barra destrorsa
	 7, 5, 5, 5, 7, // 48. 0
	 3, 2, 2, 2, 7, // 49. 1
	 7, 4, 7, 1, 7, // 50. 2
	 7, 4, 6, 4, 7, // 51. 3
	 4, 6, 5, 7, 4, // 52. 4
	 7, 1, 7, 4, 7, // 53. 5
	 7, 1, 7, 5, 7, // 54. 6
	 7, 4, 4, 4, 4, // 55. 7
	 7, 5, 7, 5, 7, // 56. 8
	 7, 5, 7, 4, 4, // 57. 9
	 0, 2, 0, 2, 0, // 58. duepunti
	 0, 2, 0, 2, 1, // 59. punto e virgola
	 4, 2, 1, 2, 4, // 60. minore
	 0, 7, 0, 7, 0, // 61. uguale
	 1, 2, 4, 2, 1, // 62. maggiore
	 7, 4, 6, 0, 2, // 63. punto interrogativo
	 0, 2, 0, 0, 0, // 64. a commerciale (non visualizzabile)
	 7, 5, 7, 5, 5, // 65. A
	 7, 5, 3, 5, 7, // 66. B
	 7, 1, 1, 1, 7, // 67. C
	 3, 5, 5, 5, 3, // 68. D
	 7, 1, 3, 1, 7, // 69. E
	 7, 1, 3, 1, 1, // 70. F
	 7, 1, 5, 5, 7, // 71. G
	 5, 5, 7, 5, 5, // 72. H
	 2, 2, 2, 2, 2, // 73. I
	 4, 4, 4, 5, 7, // 74. J
	 5, 5, 3, 5, 5, // 75. K
	 1, 1, 1, 1, 7, // 76. L
	 7, 7, 5, 5, 5, // 77. M
	 5, 7, 7, 5, 5, // 78. N
	 7, 5, 5, 5, 7, // 79. O
	 7, 5, 7, 1, 1, // 80. P
	 7, 5, 5, 1, 5, // 81. Q
	 7, 5, 3, 5, 5, // 82. R
	 7, 1, 7, 4, 7, // 83. S
	 7, 2, 2, 2, 2, // 84. T
	 5, 5, 5, 5, 7, // 85. U
	 5, 5, 5, 5, 2, // 86. V
	 5, 5, 7, 7, 5, // 87. W
	 5, 5, 2, 5, 5, // 88. X
	 5, 5, 7, 2, 2, // 89. Y
	 7, 4, 2, 1, 7, // 90. Z
	 0, 0, 6, 2, 2, // 91. parentesi quadra aperta
	 1, 3, 7, 3, 1, // 92. barra sinistrorsa
	 2, 2, 6, 0, 0, // 93. parentesi quadra chiusa
	 2, 2, 2, 2, 2, // 94. ordinale femminile
	 0, 0, 0, 0, 7, // 95. sottolinea
	 1, 2, 0, 0, 0  // 96. accento
};

/* Mappa caratteri per il computer ed il sistema operativo:
   font a piu' alta risoluzione (32 x 36 pixels), caricato in
   n_globes_map + gl_bytes (occupa 9360 bytes). */

Udword *digimap2; // verra' assegnato a n_globes_map + gl_bytes

/* Cupola panoramica. */

const float cupsize = 1800;
const float cupheight = 1667;

void cupola (float y_or, float brk)
{
	float xx, yy, zz;
	float lat, lon, dlat, dlon, dlon_2, k, clon, slon, ck, sk;

	dlat = M_PI / 20;
	dlon = M_PI / 10;

	dlon_2 = dlon / 2;

	for (lon = 0; lon < 2*M_PI - dlon_2; lon += dlon) {
		k = lon + dlon;
		ck = cos (k); sk = sin (k);
		clon = cos (lon); slon = sin (lon);
		for (lat = dlat; lat < brk * dlat; lat += dlat) {
			xx = cupsize * sin(lat+dlat);
			yy = - cupheight * cos(lat) * y_or;
			zz = cupsize * sin(lat);
			stick3d (zz*clon, yy, zz*slon,
				 xx*clon, -cupheight*cos(lat+dlat)*y_or, xx*slon);
			stick3d (zz*clon, yy, zz*slon,
				 zz*ck, yy, zz*sk);
		}
		if (gburst > 1) {
			lat = (M_PI / 20) * 8 * ((float)gburst / 63);
			lens_flares_for (cam_x, cam_y, cam_z,
					 +cupsize * clon * sin(lat),
					 -cupheight * 	   cos(lat),
					 +cupsize * slon * sin(lat),
					 -50000, 10, 1, 0, 1, 1);
			flares = 0;
		}
	}
}

void polycupola (float y_or, char textured)
{
	float d1, d2, d3, dd;
	float x[4], y[4], z[4];
	float lat, lon, dlat, dlon, dlon_2, k, clon, slon, ck, sk;

	dlat = M_PI / 20;
	dlon = M_PI / 10;

	dlon_2 = dlon / 2;

	if (textured)
		setfx (4);
	else
		setfx (2);

	for (lon = 0; lon < 2*M_PI - dlon_2; lon += dlon) {
		k = lon + dlon;
		ck = cos (k); sk = sin (k);
		clon = cos (lon); slon = sin (lon);
		for (lat = dlat; lat < 8 * dlat; lat += dlat) {
			xx = cupsize * sin(lat+dlat);
			yy = - cupheight * cos(lat) * y_or;
			zz = cupsize * sin(lat);
			x[0] = zz * clon;
			y[0] = yy;
			z[0] = zz * slon;
			x[1] = zz * ck;
			y[1] = yy;
			z[1] = zz * sk;
			x[2] = xx * ck;
			y[2] = - cupheight * cos(lat+dlat) * y_or;
			z[2] = xx * sk;
			x[3] = xx * clon;
			y[3] = - cupheight * cos(lat+dlat) * y_or;
			z[3] = xx * slon;
			if (ontheroof && y_or == 1) {
				d1 = 0.5 * (x[0] + x[1]) - cam_x;
				d2 = 0.5 * (z[0] + z[1]) - cam_z;
				dd = 1000 - sqrt (d1*d1 + d2*d2);
				if (dd > 600) dd = 600;
				if (dd < 0) dd = 0;
				cam_y += dd;
				poly3d (x, y, z, 4, 64);
				cam_y -= dd;
			}
			else {
				if (textured) {
					d1 = 0.5 * (x[0] + x[1]) - cam_x;
					d2 = 0.5 * (y[0] + y[2]) - cam_y;
					d3 = 0.5 * (z[0] + z[1]) - cam_z;
					dd = 500 - sqrt (d1*d1 + d2*d2 + d3*d3);
					if (dd > 500) dd = 500;
					if (dd < 0) dd = 0;
					cam_y += 4 * dd * y_or;
					xx = x[3]; yy = y[3]; zz = z[3];
					x[3] = x[2]; y[3] = y[2]; z[3] = z[2];
					x[2] = x[1]; y[2] = y[1]; z[2] = z[1];
					x[1] = x[0]; y[1] = y[0]; z[1] = z[0];
					x[0] = xx; y[0] = yy; z[0] = zz;
					polymap (x, y, z, 4, 0);
					cam_y -= 4 * dd * y_or;
				}
				else
					poly3d (x, y, z, 4, 64);
			}
		}
	}

	resetfx ();
}

/* Sincronizzatore: su qualsiasi PC un massimo di 18 fotogrammi al secondo. */

clock_t gtime;

void sync_start ()
{ gtime = CLOCK(); }

void sync_stop ()
{
	while (CLOCK() == gtime) {
		if (port_quit_requested ()) return;
		plat_sleep_ms (1);
	}
}

Dword  global_surface_seed;
float  ppos_x, ppos_y, ppos_z;
double dsd;			// per misurare distanze

/* Carica la bitmap per la superficie stellare. */

void load_starface ()
{
	Uword seed = to_u16 (nearstar_identity * 12345);
	random_pattern (s_background, 64800, seed, 0x3E);

	Word smoothcount;

	fast_srand ((Dword)seed);
	smoothcount = fast_random(3);

	if (nearstar_class == 11 || nearstar_class == 7 || nearstar_class == 2)
		smoothcount += fast_random(3) + 2;

	while (smoothcount) {
		ssmooth (s_background);
		smoothcount--;
	}
}

void load_QVRmaps ()
{
	Word fh;

	fh = sa_open (offsets_map);
	if (fh>-1) {
		_read (fh, n_offsets_map, om_bytes);
		_close (fh);
	}

	fh = sa_open (globes_map);
	if (fh>-1) {
		_read (fh, n_globes_map, gl_bytes);
		_close (fh);
	}
}

void load_digimap2 ()
{
	Word fh;

	fh = sa_open (off_digimap2);
	if (fh>-1) {
		_read (fh, digimap2, dm2_bytes);
		_close (fh);
	}
}

char	outhudbuffer[96];
const char	*compass = "N.........E.........S.........W.........N.........E.........S.......";

float	tp_gravity = 1, pp_gravity = 1;
float	tp_temp = 22, pp_temp = 22;
float	tp_pressure = 1, pp_pressure = 1;
float	tp_pulse = 118, pp_pulse = 118;

void wrouthud (Uword x, Uword y, Uword l, const char *text)
{
	Word j, i, n;
	Uword spot;

	n = 0; if (!l) l = 32767;
	spot = y * 320 + x;

	while (text[n] && (Uword)n < l) {
		unsigned char ch = (unsigned char)text[n];
		if (ch < 32 || ch > 96) ch = 32; // (glyphs outside the font)
		j = (ch - 32) * 5;
		for (i = 0; i < 5; i++) {
			if (digimap[j + i] & 1) adapted[spot+0] = 191 - adapted[spot+0];
			if (digimap[j + i] & 2) adapted[spot+1] = 191 - adapted[spot+1];
			if (digimap[j + i] & 4) adapted[spot+2] = 191 - adapted[spot+2];
			spot += 320;
		}
		spot -= 320 * 5;
		spot += 4;
		n++;
	}
}

char blinkhudlights;
char blinkhudlights_stay;
char blinkhudlights_flash;
Word blnr;

void surrounding (char compass_on, Word openhudcount)
{
	Word	cpos, crem;
	Dword	lsecs, lptr;
	float	pp_delta, ccom;

	for (lptr=0; lptr<04; lptr++) areaclear (adapted, 10, openhudcount + 9 - lptr, 0, 0, 300, 1, 54 + surlight + 3*lptr);
	for (lptr=0; lptr<10; lptr++) areaclear (adapted, 0, 9 - lptr, 0, 0, 320, 1, 64 + surlight - lptr);
	for (lptr=0; lptr<10; lptr++) areaclear (adapted, 0, 190 + lptr, 0, 0, 320, 1, 64 + surlight - lptr);
	for (lptr=0; lptr<10; lptr++) areaclear (adapted, 9 - lptr, 10, 0, 0, 1, 180, 64 + surlight - lptr);
	for (lptr=0; lptr<10; lptr++) areaclear (adapted, 310 + lptr, 10, 0, 0, 1, 180, 64 + surlight - lptr);

	lptr = 64 + 3 * surlight;
	if (lptr > 127) lptr = 127;

	// Turn on (and make a bit larger) those lights. Blinkhudlights turns them on, and
	// Blinkhudlights_stay makes them either turn off again, or stay on.
	if (blinkhudlights == 1) {
		if (lptr <= 111)
		lptr = 127;
		else lptr = 96;
		if (blinkhudlights_stay == 0) blinkhudlights = 0;

		blnr = blnr + 1;
		if (blnr >= 10) {
			if (blinkhudlights_flash == 1) {blinkhudlights_flash = 0; goto bla;}
			if (blinkhudlights_flash == 0) blinkhudlights_flash = 1;

			bla:
			blnr = 0;
		}

		if ((blinkhudlights_stay == 1 && blinkhudlights_flash == 1) || blinkhudlights_stay == 0) {
			areaclear (adapted, 9, 9, 0, 0, 5, 5, lptr);
			smootharound_64 (adapted, 9, 9, 5, 1);
			areaclear (adapted, 307, 9, 0, 0, 5, 5, lptr);
			smootharound_64 (adapted, 308, 9, 5, 1);
			areaclear (adapted, 9, 187, 0, 0, 5, 5, lptr);
			smootharound_64 (adapted, 9, 188, 5, 1);
			areaclear (adapted, 307, 187, 0, 0, 5, 5, lptr);
			smootharound_64 (adapted, 308, 188, 5, 1);
		} else goto bla2;
	} else {
		bla2:
		areaclear (adapted, 9, 9, 0, 0, 4, 4, lptr);
		smootharound_64 (adapted, 9, 9, 5, 1);
		areaclear (adapted, 308, 9, 0, 0, 4, 4, lptr);
		smootharound_64 (adapted, 308, 9, 5, 1);
		areaclear (adapted, 9, 188, 0, 0, 4, 4, lptr);
		smootharound_64 (adapted, 9, 188, 5, 1);
		areaclear (adapted, 308, 188, 0, 0, 4, 4, lptr);
		smootharound_64 (adapted, 308, 188, 5, 1);
	}

	sprintf (outhudbuffer, "EPOC %d & ", epoc);
	lsecs = to_i32 (secs); lsecs -= lsecs % 1000000L; lsecs /= 1000000L; lsecs%=1000;
	strcat (outhudbuffer, formatTriad(lsecs)); strcat (outhudbuffer, ".");
	lsecs = to_i32 (secs); lsecs %= 1000000L; lsecs -= lsecs % 1000; lsecs /= 1000;
	strcat (outhudbuffer, formatTriad(lsecs)); strcat (outhudbuffer, ".");
	lsecs = to_i32 (secs); lsecs %= 1000;
	strcat (outhudbuffer, formatTriad(lsecs));
	if (compass_on) {
		strcat (outhudbuffer, " & SQC ");
		strcat (outhudbuffer, alphavalue(landing_pt_lon));
		strcat (outhudbuffer, ".");
		strcat (outhudbuffer, alphavalue(landing_pt_lat));
		strcat (outhudbuffer, ":");
		strcat (outhudbuffer, alphavalue(((to_i32 (pos_x)) >> 14) - 100));
		strcat (outhudbuffer, ".");
		strcat (outhudbuffer, alphavalue(((to_i32 (pos_z)) >> 14) - 100));
		areaclear (adapted, 254, 1, 0, 0, 5, 7, 64 + 0);
		areaclear (adapted, 256, 8, 0, 0, 1, 1, 64 + 63);
		ccom = 360 - user_beta;
		if (ccom > 359) ccom -= 360;
		cpos = to_i16 (ccom / 9); crem = to_i16 (ccom * 0.44444);
		if (cpos < 0) cpos = 0;
		if (cpos > 40) cpos = 40;
		wrouthud (200 - (crem % 4), 2, 28, compass + cpos);
	}
	else {
		if (!ontheroof) {
			strcat (outhudbuffer, " & ");
			if (sys==4)
				strcat (outhudbuffer, "5\\FLIGHTCTR R\\DEVICES    D\\PREFS      X\\SCREEN OFF");
			else {
				cpos = strlen (outhudbuffer);
				outhudbuffer[cpos + 00] = '6';
				outhudbuffer[cpos + 01] = '\\';
				memcpy (outhudbuffer + cpos + 02, ctb + 20 + 27 * 0, 10);
				outhudbuffer[cpos + 12] = ' ';
				outhudbuffer[cpos + 13] = '7';
				outhudbuffer[cpos + 14] = '\\';
				memcpy (outhudbuffer + cpos + 15, ctb + 20 + 27 * 1, 10);
				outhudbuffer[cpos + 25] = ' ';
				outhudbuffer[cpos + 26] = '8';
				outhudbuffer[cpos + 27] = '\\';
				memcpy (outhudbuffer + cpos + 28, ctb + 20 + 27 * 2, 10);
				outhudbuffer[cpos + 38] = ' ';
				outhudbuffer[cpos + 39] = '9';
				outhudbuffer[cpos + 40] = '\\';
				memcpy (outhudbuffer + cpos + 41, ctb + 20 + 27 * 3, 10);
				outhudbuffer[cpos + 51] = 0;
				strupr (outhudbuffer);
			}
		}
	}
	wrouthud (2, 2, 0, outhudbuffer);

	pp_delta = (pp_gravity - tp_gravity) * 0.25; tp_gravity += pp_delta;
	pp_delta = (pp_temp - tp_temp) * 0.05; tp_temp += pp_delta;
	pp_delta = (pp_pressure - tp_pressure) * 0.02; tp_pressure += pp_delta;
	pp_delta = (pp_pulse - tp_pulse) * 0.01; tp_pulse += pp_delta;
	snprintf (outhudbuffer, sizeof (outhudbuffer), "GRAVITY %2.3f FG & TEMPERATURE %+3.1f@C & PRESSURE %2.3f ATM & PULSE %3.0f PPS", tp_gravity, tp_temp, tp_pressure, tp_pulse);
	wrouthud (2, 192, 0, outhudbuffer);
}

/* Salva una fotografia dello schermo in formato BMP
   (numerazione NICE-style: 00000001.BMP). */

extern Dword star_label_pos;
extern char star_label[25];
extern Dword planet_label_pos;
extern char planet_label[25];
Word movienr;
char movieflashoff = 1;
char movie;
Word moviefscap;
Word moviestime;
Word moviedeck = 1;
Word movieexists;
char snapfilename[64];

void snapshot (Word forcenumber, char showdata)
{
	Dword prog;
	Uword pqw;
	double parsis_x, parsis_y, parsis_z;

	Uword ptr, c;
	char a, b;
	unsigned char t[54];

	Word ih = sa_open (header_bmp);
	if (ih==-1) return;
	_read (ih, t, 54);
	_close (ih);

	pqw = QUADWORDS;
	QUADWORDS = 16000;
	if (movie == 0 || (movieflashoff == 0 && movie == 1)) {
		pclear(adaptor, 0);
	}
	if (movie == 1){
		if (movieexists == 1) {				//movie does NOT exist yet
			sprintf (snapfilename, "..\\MOVIES\\%03i", moviedeck);
			mkdir("..\\MOVIES");
			mkdir(snapfilename);			//the folder for the movie to record to.
		}
		if (movieflashoff == 1) {
			if (surlight <= 38) areaclear(adaptor, 1, 198, 319, 199, 0, 1, 127);	//bright line when surroundinglight <= 38
				else areaclear(adaptor, 1, 198, 319, 199, 0, 1, 1);		//dark line when surroundinglight > 38
		}
		moviefscap ++;
	}
	QUADWORDS = pqw;

	if (!forcenumber) {
		prog = lastSnapshot;
		char recycled=0;
		Word ihx;
		do {
			if (!movie) {prog++; if (prog == 100000000) {	//Image limit increased to 99,999,999 (SL)
				if (recycled) {
					status("TOO MANY SNAPSHOTS", 300);
					return;
				} else {
					recycled=1;
					prog=0;
				}
				} }
			if (movie == 1) {	//Recording a movie instead of doing a screenshot? Do moviestuffs...
				movienr++;
				sprintf (snapfilename, "..\\MOVIES\\%03i\\%08d.BMP", moviedeck, movienr);
				remove(snapfilename);
			} else sprintf (snapfilename, "..\\GALLERY\\%08d.BMP", prog);
			ihx = _open (snapfilename, 0);
			if (ihx != -1) _close (ihx);
		} while (ihx != -1);
		lastSnapshot = prog;
	}
	else
		sprintf (snapfilename, "..\\GALLERY\\WIDE%04d.BMP", forcenumber);

    if (showdata) {
	areaclear (adapted, 2, 191, 0, 0, 316, 7, 64 + 63);

	parsis_x = nearbyint (dzat_x);
	parsis_y = nearbyint (dzat_y);
	parsis_z = nearbyint (dzat_z);

	strcpy (outhudbuffer, "LOCATION PARSIS: ");
	strcat (outhudbuffer, alphavalue(parsis_x));
	strcat (outhudbuffer, ";");
	strcat (outhudbuffer, alphavalue(-parsis_y));
	strcat (outhudbuffer, ";");
	strcat (outhudbuffer, alphavalue(parsis_z));
	if (ip_targetted > -1) {
		if (nearstar_p_owner[(Word)ip_targetted] > -1) {
			strcat (outhudbuffer, " & TGT: MOON N@");
			strcat (outhudbuffer, alphavalue(nearstar_p_moonid[(Word)ip_targetted]+1));
			strcat (outhudbuffer, " OF PLANET N@");
			strcat (outhudbuffer, alphavalue(nearstar_p_owner[(Word)ip_targetted]+1));
		}
		else {
			strcat (outhudbuffer, " & TGT: PLANET N@");
			strcat (outhudbuffer, alphavalue(ip_targetted+1));
		}
	}
	wrouthud (3, 192, 0, outhudbuffer);

	if (ap_targetted == 1 && star_label_pos != -1) {
		areaclear (adapted, 14, 14, 0, 0, 102, 7, 64 + 63);
		wrouthud (15, 15, 20, star_label);
	}

	if (ip_targetted != -1 && planet_label_pos != -1) {
		areaclear (adapted, 14, 23, 0, 0, 102, 7, 64 + 63);
		wrouthud (15, 24, 20, planet_label);
	}
    }

	ih = _creat (snapfilename, 0);
	if (ih>-1) {
		a = 0;
		_write (ih, t, 54);
		for (c=0; c<768; c+=3) {
			b = tmppal[c+2]*4; _write (ih, &b, 1);
			b = tmppal[c+1]*4; _write (ih, &b, 1);
			b = tmppal[c+0]*4; _write (ih, &b, 1);
			_write (ih, &a, 1);
		}
		for (ptr=63680; ptr<64000; ptr-=320) _write (ih, adapted+ptr, 320);
		_close (ih);
	}
}

/*
    Consumi supplementari di litio, dal piu' dispendioso al piu' economico:
	- orbita vimana:			1 KD ogni 7 secondi.
	- inseguimento a punto lontano: 	1 KD ogni 18 secondi.
	- inseguimento a punto fisso:		1 KD ogni 29 secondi.
	- inseguimento a punto vicino:		1 KD ogni 33 secondi.
	- amplificatore di campo stellare:	1 KD ogni 41 secondi.
	- orbita geosincrona:			1 KD ogni 58 secondi.
	- lampada interna:			1 KD ogni 84 secondi.
	- cercapianeti:				1 KD ogni 155 secondi.
*/

Dword iqsecs = 0;
void additional_consumes()
{
	if (iqsecs < to_i32 (secs))
		iqsecs = to_i32 (secs);
	//
	if (ip_targetted > -1 && pwr > 15000) {
		if (ip_reached && nsync) {
			if (nsync==1) // fixed-point chase
				if (!(iqsecs % 29)) { pwr--; iqsecs++; }
			if (nsync==2) // far chase
				if (!(iqsecs % 18)) { pwr--; iqsecs++; }
			if (nsync==3) // syncrone orbit
				if (!(iqsecs % 58)) { pwr--; iqsecs++; }
			if (nsync==4) // vimana orbit
				if (!(iqsecs %  7)) { pwr--; iqsecs++; }
			if (nsync==5) // near chase
				if (!(iqsecs % 33)) { pwr--; iqsecs++; }
		}
	}
	//
	if (pl_search		&& !(iqsecs % 155)) { pwr--; iqsecs++; }
	if (ilightv == 1	&& !(iqsecs %  84)) { pwr--; iqsecs++; }
	if (field_amplificator	&& !(iqsecs %  41)) { pwr--; iqsecs++; }
	//
	if (pwr <= 15000) {
		if (charge>0) {
			charge--;
			pwr = 20000;
			status ("FCS: READY", 100);
		} else if (charge<0) {	//HACK: If you set your Li+ negative, you have infinite fuel. (SL)
			pwr = 20000;
			status ("OMEGA", 100);
		} else {
			stspeed 	= 0;
			ip_reaching 	= 0;
			ip_reached 	= 1;
			ip_targetted 	= -1;
			if (pwr != 15000) {
				status ("POWER LOSS", 100);
				pwr = 15000;
			}
		}
	}
}
