/*

	Inclusioni del c++ / Common definitions for NOCTIS.

	Linux port: this header merges the original NOCTIS-D.H and DEFS.H.
	The original was a 16-bit program, so the integer types used by the
	game keep their DOS sizes:

		Word   = int            (16 bit, signed)
		Uword  = unsigned       (16 bit)
		Dword  = long           (32 bit, signed)
		Udword = unsigned long  (32 bit)

*/

#ifndef NOCTIS_D_H
#define NOCTIS_D_H

#include "../dos/dos.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>

typedef int16_t  Word;
typedef uint16_t Uword;
typedef int32_t  Dword;
typedef uint32_t Udword;
typedef uint8_t  Uchar;
typedef unsigned char Bool;
typedef int FastBool;
#define True  1
#define False 0

#define WIDTH 320
#define HEIGHT 200
#define DISPLAYBYTES (WIDTH*HEIGHT)
#define SQRT(x) sqrt(x)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

/* Borland runtime and DOS services. */
#define _open        dos_open
#define _creat       dos_creat
#define _read        dos_read
#define _write       dos_write
#define _close       dos_close
#define lseek        dos_lseek
#define tell         dos_tell
#define filelength   dos_filelength
#define eof          dos_eof
#define chsize       dos_chsize
#define strupr       dos_strupr
#define memicmp      dos_memicmp
#define gcvt         dos_gcvt
#define _fmemmove    memmove
#define _fmemcpy     memcpy
#define _fmemset     memset
#define clock()      dos_clock()
#define CLOCK()      dos_clock()
#define srand(x)     bc_srand_any(x)
#define remove(x)    dos_remove(x)
#define mkdir(x)     dos_mkdir(x)
#define fopen(a, b)  dos_fopen(a, b)
#define OPENFILE(f, m)       _open (f, m)
#define READFILE(fh, p, l)   _read (fh, p, l)
#define WRITEFILE(fh, p, l)  _write (fh, p, l)
#define CLOSEFILE(fh)        _close (fh)
#define NOFILE               -1
#define OPEN_RB              0
#define OPEN_RPLUS           4

// The engine's roll angle would clash with gamma() from <cmath>.
#define gamma noctis_gamma

typedef int32_t clock_t_dos;
#define clock_t clock_t_dos

/*

	Definizioni per NOCTIS.
	Tante.

*/

// area dati: 	lunghezza:	puntatore:	descrizione:

#define om_bytes      7340 // 	n_offsets_map	contiene "offsets.map",
			   //			(sfera concava in QT-VR).
#define gl_bytes     22586 //   n_globes_map	contiene "globes.map"
			   //			(sfera convessa in QT-VR).
			   //			texture superficie del mare.
#define gl_brest     10182 //   (indicativo)	resto dei bytes allocati per
			   //			n_globes_map (mare 32K, font 32x36).
#define st_bytes     64800 //	s_background    mappa superficie stelle;
			   //			mappa superficie satelliti;
			   //			mappa del cielo in superficie;
			   //			buffer di shading superficie.
#define pl_bytes     65552 // 	p_background	mappa planetaria dall'orbita;
			   // 			texture per il suolo.
#define ps_bytes     40000 //   p_surfacemap	piantina altimetrica superf.;
			   //			buffer temporaneo di lettura;
			   //			buffer aggiornamento schermi.
#define oc_bytes     40000 //   objectschart	mappa oggetti in superficie;
			   //			overlay per l'atmosfera.
#define sc_bytes     65540 //	adapted	      	la pagina video nascosta (64Kb+4).
#define pv_bytes     20480 //   pvfile		data bank dinamica dei files
			   //			di grafica poligonale.

// Mappa del file SUPPORTS.NCT: offsets relativi alla sua fine.

#define off_digimap2	-60776
#define dm2_bytes	  9360

#define path(folder) "..\\" folder

#define bull_ncc     path("data\\critter1.ncc")
#define supports_nct path("data\\supports.nct")
#define x_txt        path("data\\x.txt")
#define xbuff_txt    path("data\\xbuff.txt")
#define restart_txt  path("data\\restart.txt")
#define errors_txt   path("data\\errors.txt")

#define mammal_ncc	-51416

#define os_voidscrn	-48664
#define os_goescomm	-44704
#define os_logogoes	-40744

#define birdy_ncc	-36784
#define vehicle_ncc	-35782

#define header_bmp	-29980
#define offsets_map	-29926
#define globes_map	-22586

// Limiti, relativi al centro dello schermo, per i bastoncini 3D.

#define stk_lbx       -150
#define stk_lby        -90
#define stk_ubx        160
#define stk_uby         90

// Parametri di lavoro per "tdpolygs.h", la libreria 3D poligonale.

#define VERTICI_PER_POLIGONO 4

#define larghezza 306
#define altezza   180
#define x_centro  158
#define y_centro  100

#define lbx (-larghezza/2 + x_centro)
#define ubx (larghezza/2 + x_centro)
#define lby (-altezza/2 + y_centro)
#define uby (altezza/2 + y_centro)

// Quattro terzi di pigreco.

#define qt_M_PI   (4*M_PI/3)

// Tipi di pianeta e numero di classi stellari.

#define star_classes    12
#define planet_types    10
#define avgmoons        4
#define log2avgmoons	2
#define maxbodies	(20 * avgmoons)

// Codici identificativi degli oggetti sulla superficie dei pianeti.

#define ROCKS	0
#define VEGET	1
#define TREES	2
#define NOTHING 3

// Codici identificativi delle rovine (in tre stili di texture).

#define AF1     0x40
#define AF2     0x80
#define AF3     0xC0

// Assegnazione handles per i files PV.

#define vehicle_handle	0

#define bird_base	0
#define bird_result	1

#define mamm_base	2
#define mamm_result	3

// Struttura di un byte nella mappa oggetti di superficie (objectschart).
// (Borland packed these bit fields in a single byte.)

struct __attribute__((packed)) quadrant {
	uint8_t nr_of_objects : 2;
	uint8_t object0_class : 2;
	uint8_t object1_class : 2;
	uint8_t object2_class : 2;
};

// Struttura che identifica un poligono e i suoi vertici per le funzioni PV.

struct pvlist {
	uint16_t polygon_id : 12;
	uint16_t vtxflag_0  : 1;
	uint16_t vtxflag_1  : 1;
	uint16_t vtxflag_2  : 1;
	uint16_t vtxflag_3  : 1;
};

#endif
