/*

	The Third Flare.
	----------------
	Full-3D Graphic Engine.

	Funzioni per disegnare poligoni 3d riempiti di colore
	in grafica 320x200x256.

	- Non si deve mai usare il colore nr. 255 per
	  tracciare qualcosa nell'area di lavoro, quando si usa questa
	  routine, perche' lo utilizza solo lei per delimitare i poligoni.

	Chiamare initscanlines prima di tracciare alcunche'.

	Nato (incredibile a dirsi) per Alpha Dream.
	Ultima revisione 21 Dicembre 1997.

	Linux port: the original was "al 99% puro assembly per coprocessori
	da 387 in su e codice 32-bit". It has been rewritten in C++ keeping
	the exact behaviour of the assembly: vertex rototranslation, 3D and 2D
	clipping, the edge tracer that marks polygon borders with colour 255,
	the scan-line filler (with all its "flares" effects) and the
	perspective-correct texture mapper working on 8.8 fixed point
	coordinates in 16-pixel spans.

	This file is included by noctis-0.cpp.

*/

/* Controllo visualizzazione. */

float uneg = 100; /* distanza minima alla quale un punto non viene
		     considerato alle spalle dell'osservatore. */

float alfa = 0, beta = 0, gamma = 0;
float cam_x = 0, cam_y = 0, cam_z = 0;

const double deg = M_PI / 180;

// Distanza dal piano di proiezione.

float dpp = 200;
float inv_dpp = 1 / dpp;

/* Costanti e variabili di controllo del texture mapping.
   H_MATRIXS e V_MATRIXS specificano le ripetizioni della stessa
   texture sui poligoni, in sedicesimi di texture. */

const Word MPIX = 319; // Massimo PIXEL su X.
const Word MPIY = 199; // Massimo PIXEL su Y.

float   EMU_K	      =		                 16; // Cost. di emulaz. FPU
Dword   H_MATRIXS     =		         	 16; // Nr. ripetizioni. 16-128
Dword   V_MATRIXS     =		         	 16; // Nr. ripetizioni. 16-128
Dword   XSIZE         =	  TEXTURE_XSIZE * H_MATRIXS; // Calibraz. dimensioni.
Dword   YSIZE         =	  TEXTURE_YSIZE * V_MATRIXS; // Calibraz. dimensioni.
float   XCOEFF        =	     	        EMU_K / dpp; // Coefficente di comodo.
float   YCOEFF        =	       	        EMU_K / dpp; // Coefficente di comodo.

// Inizializza indirizzi scanlines.
// (The table is longer than 200 entries: a few routines index it slightly
// out of range, which on DOS read whatever followed it in memory.)

Uword riga[256];

void initscanlines ()
{
	Uword c;

	for (c=0; c<256; c++)
		riga[c] = 320 * c;
}

/* Traccia un segmento (con colore 255) senza controlli sui limiti. */

Uword ptr;

Udword xp, yp, xa, ya;
Udword global_x, global_y;

/* Common set-up of the DDA used by Segmento and Stick: orders the end
   points left to right and computes the 16.16 increments exactly like
   the original 32-bit code (including the 16-bit DIV). */
static void dda_setup (Udword &a, Udword &b)
{
	Udword L;
	char neg = 0;

	Udword dx = xa - xp;
	if (xa < xp) {
		Udword t;
		t = xa; xa = xp; xp = t;
		t = ya; ya = yp; yp = t;
		dx = (Udword)0 - dx;
	}
	a = dx;
	L = dx;
	Udword dy = ya - yp;
	if (ya < yp) {
		neg = 1;
		dy = (Udword)0 - dy;
	}
	b = dy;
	if (!(dy < L)) L = dy;
	L++;
	xa <<= 16;
	global_x = xp << 16;
	global_y = yp << 16;
	a <<= 16;
	b <<= 16;
	Uword l16 = (Uword)L;
	if (!l16) l16 = 1;
	a = (Uword)(a / l16);
	b = (Uword)(b / l16);
	if (neg) b = (Udword)0 - b;
}

void Segmento ()
{
	Udword a, b;
	Uword pi, pf;

	if (xp==xa) {
		if (ya>=yp) {
			pi = riga[yp & 255] + xp;
			pf = riga[(ya+1) & 255];
		}
		else {
			pi = riga[ya & 255] + xp;
			pf = riga[(yp+1) & 255];
		}
		do {
			adapted[pi] = 255;
			pi += 320;
		} while (pi < pf);
		return;
	}

	dda_setup (a, b);

	Udword cx = xa;
	do {
		Uword di = (Uword)(global_x >> 16);
		Uword bx = (Uword)(global_y >> 16);
		global_x += a;
		di += riga[bx & 255];
		global_y += b;
		adapted[di] = 255;
	} while (global_x < cx);
}

// Flag "flares": effetti speciali di riempimento dei poligoni.

char flares = 0;

Dword mp[6*VERTICI_PER_POLIGONO];

Uword max_x, max_y, min_x, min_y;

// Dati globali per conversioni, rototraslazioni e clipping.

char doflag;
Word pvert, nvert, vvert;
Word vr, vr2, vr3, vr4, vr5, vr6;

float zk;
float x2, y2, z2;
float xx, yy, zz;

char rwf[VERTICI_PER_POLIGONO];

float rxf[VERTICI_PER_POLIGONO];
float ryf[VERTICI_PER_POLIGONO];
float rzf[VERTICI_PER_POLIGONO];

// Limiti dell'area video rielaborati.

Dword lbxl = lbx;
Dword ubxl = ubx;
Dword lbyl = lby;
Dword ubyl = uby;

float lbxf = lbx;
float ubxf = ubx;
float lbyf = lby;
float ubyf = uby;

float x_centro_f = x_centro;
float y_centro_f = y_centro;

// Chiamare dopo aver cambiato l'angolo visuale.

float opt_pcosbeta = dpp;
float opt_psinbeta = 0;
float opt_tcosbeta = 1;
float opt_tsinbeta = 0;
float opt_pcosalfa = dpp;
float opt_psinalfa = 0;
float opt_tcosalfa = 1;
float opt_tsinalfa = 0;

float opt_tcosgamma = 1;
float opt_tsingamma = 0;

void change_angle_of_view ()
{
	opt_pcosbeta = cos (beta*deg) * dpp;
	opt_psinbeta = sin (beta*deg) * dpp;
	opt_tcosbeta = cos (beta*deg);
	opt_tsinbeta = sin (beta*deg);
	opt_pcosalfa = cos (alfa*deg) * dpp;
	opt_psinalfa = sin (alfa*deg) * dpp;
	opt_tcosalfa = cos (alfa*deg);
	opt_tsinalfa = sin (alfa*deg);

	opt_tcosgamma = cos (gamma*deg);
	opt_tsingamma = sin (gamma*deg);
}

// Chiamare dopo aver cambiato dpp.

void change_camera_lens ()
{
	inv_dpp = 1 / dpp;

	XCOEFF = EMU_K / dpp;
	YCOEFF = EMU_K / dpp;

	change_angle_of_view ();
}

// Chiamare dopo aver cambiato V_MATRIXS o H_MATRIXS.

void change_txm_repeating_mode ()
{
	XSIZE = TEXTURE_XSIZE * H_MATRIXS;
	YSIZE = TEXTURE_YSIZE * V_MATRIXS;
}

float uno = 1; // sempre uno: e' una costante di comodo.

unsigned char entity = 1; /* controlla quantita' generiche nel riempimento
			     dei poligoni con alcuni effetti speciali. */

/* Clipping of the vertices behind the observer (rz < uneg). Every
   invisible vertex is replaced by its intersections with the plane
   z = uneg along the edges that lead to visible neighbours. Returns the
   number of resulting vertices. */
static Word clip_3d (Word nrv, float *ox, float *oy, float *oz)
{
	Word o = 0;
	Word last = nrv - 1;

	for (Word v = 0; v < nrv; v++) {
		if (rwf[v]) {
			ox[o] = rxf[v];
			oy[o] = ryf[v];
			oz[o] = rzf[v];
			o++;
			continue;
		}
		Word p = (v == 0) ? last : v - 1;
		Word n = (v + 1 > last) ? 0 : v + 1;
		if (!rwf[p] && !rwf[n]) continue;
		if (rwf[p] && rwf[n]) {
			if (rzf[v] != rzf[p]) {
				zk = (uneg - rzf[p]) / (rzf[v] - rzf[p]);
				ox[o] = zk * (rxf[v] - rxf[p]) + rxf[p];
				oy[o] = zk * (ryf[v] - ryf[p]) + ryf[p];
			}
			else {
				ox[o] = rxf[v];
				oy[o] = ryf[v];
			}
			oz[o] = uneg;
			if (rzf[v] != rzf[n]) {
				zk = (uneg - rzf[n]) / (rzf[v] - rzf[n]);
				ox[o+1] = zk * (rxf[v] - rxf[n]) + rxf[n];
				oy[o+1] = zk * (ryf[v] - ryf[n]) + ryf[n];
			}
			else {
				ox[o+1] = rxf[v];
				oy[o+1] = ryf[v];
			}
			oz[o+1] = uneg;
			o += 2;
		}
		else {
			Word q = rwf[p] ? p : n;
			if (rzf[v] != rzf[q]) {
				zk = (uneg - rzf[q]) / (rzf[v] - rzf[q]);
				ox[o] = zk * (rxf[v] - rxf[q]) + rxf[q];
				oy[o] = zk * (ryf[v] - ryf[q]) + ryf[q];
			}
			else {
				ox[o] = rxf[v];
				oy[o] = ryf[v];
			}
			oz[o] = uneg;
			o++;
		}
	}

	return o;
}

/* One stage of the 2D clipping against a screen edge. 'a' is the clipped
   coordinate, 'b' the other one; 'lower' selects a lower bound (a < lim
   is outside) or an upper bound (a > lim is outside). */
static Word clip_2d (Word n, const float *ia, const float *ib, float *oa, float *ob,
		     float lim, char lower)
{
	Word o = 0;
	Word last = n - 1;

	for (Word v = 0; v < n; v++) {
		char out_v = lower ? (ia[v] < lim) : (ia[v] > lim);
		if (!out_v) {
			oa[o] = ia[v];
			ob[o] = ib[v];
			o++;
			continue;
		}
		Word p = (v == 0) ? last : v - 1;
		Word nn = (v + 1 > last) ? 0 : v + 1;
		char in_p = lower ? !(ia[p] < lim) : !(ia[p] > lim);
		char in_n = lower ? !(ia[nn] < lim) : !(ia[nn] > lim);
		if (!in_p && !in_n) continue;
		if (in_p && in_n) {
			if (ia[v] != ia[p])
				ob[o] = (lim - ia[p]) / (ia[v] - ia[p]) * (ib[v] - ib[p]) + ib[p];
			else
				ob[o] = ib[v];
			oa[o] = lim;
			if (ia[v] != ia[nn])
				ob[o+1] = (lim - ia[nn]) / (ia[v] - ia[nn]) * (ib[v] - ib[nn]) + ib[nn];
			else
				ob[o+1] = ib[v];
			oa[o+1] = lim;
			o += 2;
		}
		else {
			Word q = in_p ? p : nn;
			if (ia[v] != ia[q])
				ob[o] = (lim - ia[q]) / (ia[v] - ia[q]) * (ib[v] - ib[q]) + ib[q];
			else
				ob[o] = ib[v];
			oa[o] = lim;
			o++;
		}
	}

	return o;
}

/* Scan string emulation for the polygon filler (REPNE/REPE SCASB looking
   for the border colour 255). 'zf' keeps the zero flag between scans,
   exactly like the processor did. */
struct Scanner {
	Uword p;
	Uword cx;
	char zf;
	void repne () { while (cx) { cx--; zf = (adapted[p++] == 255); if (zf) break; } }
	void repe ()  { while (cx) { cx--; zf = (adapted[p++] == 255); if (!zf) break; } }
};

/* Locates the span to fill on one scan line. Returns 0 if the line has no
   border pixel, 1 for a normal span [*s, *e), 2 for the "single border run"
   case where only the border itself is filled. */
static char scan_row (Uword start, Uword bytes, Uword *s, Uword *e, Uword *count)
{
	Scanner sc;
	sc.p = start;
	sc.cx = bytes;
	sc.zf = 0;
	sc.repne ();
	if (!sc.zf) return 0;
	Uword si = sc.p;
	sc.repe ();
	Uword bx = sc.p;
	sc.repne ();
	if (!sc.zf) {
		*s = si - 1;
		*e = bx - 1;
		*count = sc.cx;
		return 2;
	}
	sc.repe ();
	*s = si - 1;
	*e = sc.p - 1;
	*count = sc.cx;
	return 1;
}

/* Questa e' la funzione da invocare per ogni poligono da tracciare. */

void poly3d (float *x, float *y, float *z,
	     Uword nrv, unsigned char colore)
{
	Uword _8n;
	unsigned char ent = entity;

	// Matrici 3d.

	float ultima_x[2*VERTICI_PER_POLIGONO];
	float ultima_y[2*VERTICI_PER_POLIGONO];
	float ultima_z[2*VERTICI_PER_POLIGONO];

	// Matrici 2d.

	float video_x0[2*VERTICI_PER_POLIGONO], video_y0[2*VERTICI_PER_POLIGONO];
	float video_x1[3*VERTICI_PER_POLIGONO], video_y1[3*VERTICI_PER_POLIGONO];
	float video_x2[4*VERTICI_PER_POLIGONO], video_y2[4*VERTICI_PER_POLIGONO];
	float video_x3[6*VERTICI_PER_POLIGONO], video_y3[6*VERTICI_PER_POLIGONO];

	if (nrv == 0 || nrv > VERTICI_PER_POLIGONO) return;

	// Rototraslazione dei vertici; i dati rimangono ancora 3d.

	doflag = 0;
	for (Uword i = 0; i < nrv; i++) {
		float zr = z[i] - cam_z;
		float xr = x[i] - cam_x;
		zz = zr;
		xx = xr;
		rxf[i] = zr * opt_psinbeta + xr * opt_pcosbeta;
		z2 = zz * opt_tcosbeta - xx * opt_tsinbeta;
		float yr = y[i] - cam_y;
		yy = yr;
		rzf[i] = z2 * opt_tcosalfa + yr * opt_tsinalfa;
		ryf[i] = yy * opt_pcosalfa - z2 * opt_psinalfa;
		if (rzf[i] < uneg)
			rwf[i] = 0;
		else {
			doflag++;
			rwf[i] = 1;
		}
	}

	if (!doflag) return;

	// Fast-load per poligoni completamente visibili,
	// altrimenti conversione dei punti alle spalle dell'osservatore.

	if (doflag == nrv) {
		for (Uword i = 0; i < nrv; i++) {
			ultima_x[i] = rxf[i];
			ultima_y[i] = ryf[i];
			ultima_z[i] = rzf[i];
		}
		vr2 = nrv;
	}
	else {
		vr2 = clip_3d (nrv, ultima_x, ultima_y, ultima_z);
		if (vr2 < 3) return;
	}

	// Proiezioni prospettiche da 3d a 2d, con ricerca di minimi e massimi.

	Dword ax = lbxl, bx = lbyl, cx = ubxl, dx = ubyl;
	for (Word i = vr2 - 1; i >= 0; i--) {
		float k = uno / ultima_z[i];
		float vx = k * ultima_x[i] + x_centro_f;
		video_x0[i] = vx;
		mp[2*i] = fistp32 (vx);
		if (mp[2*i] < cx) cx = mp[2*i];
		if (mp[2*i] > ax) ax = mp[2*i];
		float vy = k * ultima_y[i] + y_centro_f;
		video_y0[i] = vy;
		mp[2*i+1] = fistp32 (vy);
		if (mp[2*i+1] < dx) dx = mp[2*i+1];
		if (mp[2*i+1] > bx) bx = mp[2*i+1];
	}
	_8n = (vr2 - 1) * 8;

	max_x = (Uword)ax;
	max_y = (Uword)bx;
	min_x = (Uword)cx;
	min_y = (Uword)dx;
	Word clip = 0;
	if (ax >= ubxl) { clip++; max_x = ubx; }
	if (bx >= ubyl) { clip++; max_y = uby; }
	if (cx < lbxl)  { clip++; min_x = lbx; }
	if (dx < lbyl)  { clip++; min_y = lby; }

	if (clip) {
		// Lato superiore, inferiore, sinistro e destro del video.
		vr3 = clip_2d (vr2, video_y0, video_x0, video_y1, video_x1, lbyf, 1);
		if (vr3 < 3) return;
		vr4 = clip_2d (vr3, video_y1, video_x1, video_y2, video_x2, ubyf, 0);
		if (vr4 < 3) return;
		vr5 = clip_2d (vr4, video_x2, video_y2, video_x3, video_y3, lbxf, 1);
		if (vr5 < 3) return;
		float fx[6*VERTICI_PER_POLIGONO], fy[6*VERTICI_PER_POLIGONO];
		vr6 = clip_2d (vr5, video_x3, video_y3, fx, fy, ubxf, 0);
		if (vr6 < 3) return;
		for (Word i = 0; i < vr6; i++) {
			if (fx[i] == ubxf) mp[2*i] = ubx;
			else mp[2*i] = fistp32 (fx[i]);
			mp[2*i+1] = fistp32 (fy[i]);
		}
		_8n = (vr6 - 1) * 8;
	}

	/* Riempimento del poligono risultante. */

	// Tracciamento rapido (per i poligoni piccoli).

	if (!flares) {
		if (min_y==max_y) {
			if (min_x==max_x)
				adapted[min_x + riga[min_y & 255]] = colore;
			else {
				ptr = max_x + riga[min_y & 255];
				while (max_x>=min_x) {
					adapted[ptr] = colore;
					max_x--;
					ptr--;
				}
			}
			return;
		}
	}

	// Disegna i bordi del poligono, con la funzione Segmento.

	Uword si;
	for (si = 0; si < _8n; si += 8) {
		xp = mp[si/4];
		yp = mp[si/4+1];
		xa = mp[si/4+2];
		ya = mp[si/4+3];
		Segmento ();
	}
	xp = mp[si/4];
	yp = mp[si/4+1];
	xa = mp[0];
	ya = mp[1];
	Segmento ();

	Uword segmptr = riga[min_y & 255] + min_x;
	Uword lim_y = riga[max_y & 255] + min_x;
	Uword lim_x = segmptr + max_x - min_x;
	Uword bytes = lim_x - segmptr + 2;

	Uword di, s, e, cnt;

	switch (flares) {
		case 0:
			for (di = segmptr; di <= lim_y; di += 320) {
				char r = scan_row (di, bytes, &s, &e, &cnt);
				if (r) memset (adapted + s, colore, (Uword)(e - s));
			}
			break;
		case 1:
			colore &= 0x3F;
			for (di = segmptr; di <= lim_y; di += 320) {
				char r = scan_row (di, bytes, &s, &e, &cnt);
				if (!r) continue;
				unsigned char add = (r == 1) ? colore : (unsigned char)(colore >> 1);
				for (Uword p = s; p != e; p++) {
					unsigned char al = (adapted[p-1] & 0x3F) + add;
					if (al >= 62) al = 62;
					adapted[p] = al;
				}
			}
			break;
		case 2:
			for (di = segmptr; di <= lim_y; di += 320) {
				char r = scan_row (di, bytes, &s, &e, &cnt);
				if (!r) continue;
				Uword c = (Uword)(e - s);
				Uword p = s;
				if (r == 1) {
					while (c) {
						if (adapted[p] == 0xFF)
							adapted[p] = (adapted[(Uword)(p-321)] & 0x3F) | 0x40;
						else {
							Uword a = (adapted[p] & 0x3F) | 0x40;
							a += c;
							adapted[p] = (a >= 128) ? 127 : (unsigned char)a;
						}
						p++;
						c--;
					}
				}
				else {
					while (c) {
						unsigned char ah = adapted[(Uword)(p-321)];
						if (ah == 0xFF) ah = adapted[(Uword)(p-642)];
						adapted[p] = (ah & 0x3F) | 0x40;
						p++;
						c--;
					}
				}
			}
			break;
		// effetto flares = 3 spostato a "polymap"
		case 4:
			for (di = segmptr; di <= lim_y; di += 320) {
				char r = scan_row (di, bytes, &s, &e, &cnt);
				if (r) memset (adapted + s, colore, (Uword)(e - s));
				unsigned char al = colore & 0x3F;
				unsigned char ah = colore & 0xC0;
				al += ent;
				if (al > 0x3F) {
					al = 0x3F;
					if (ent & 0x80) al = 0;
				}
				colore = al | ah;
			}
			break;
	}
}

/*

	Sezione per il texture mapping.

*/

#define OK					   1 // Ritorno positivo.
#define NOT_OK					   0 // Ritorno negativo.

unsigned char *txtr; /* Area della texture (64 livelli per pixel). */

float	pnx, pny, pnz; // valori di ritorno della funzione successiva.

void pnorm (float *x, float *y, float *z)
	// Calcola i coefficenti x/y/z della normale ad un poligono.
	// Valori calcolati in pnx, pny, pnz. Richiesti almeno tre vertici.
{
	float x1, y1, z1;
	float x2, y2, z2;
	float xr, yr, zr;

	float ln;

	x1 = x[0] - x[2];
	y1 = y[0] - y[2];
	z1 = z[0] - z[2];
	x2 = x[1] - x[2];
	y2 = y[1] - y[2];
	z2 = z[1] - z[2];

	xr = y1 * z2 - y2 * z1;
	yr = z1 * x2 - z2 * x1;
	zr = x1 * y2 - x2 * y1;

	ln = sqrt (xr * xr + yr * yr + zr * zr);
	if (ln>0) {
		ln  =  1 / ln;
		pnx = xr * ln;
		pny = yr * ln;
		pnz = zr * ln;
	}
	else {
		pnx = 0;
		pny = 0;
		pnz = 0;
	}
}

float	_0 = 0.0;
float	_16 = 16.0;

float	x_antialias = 1.125;
float	y_antialias = 1.125;
float	z_antialias = 1.125;

char	culling_needed = 0;  // flag: traccia due punti per volta.
char	halfscan_needed = 0; // flag: traccia due linee per volta.

unsigned char escrescenze = 0xE0; // primo colore dei bumps (escrescenze)

/* Rototranslation shared by polymap and getcoords: unlike poly3d it does
   not include the projection distance, and it supports the gamma axis. */
static inline char rototranslate (float x, float y, float z,
				  float *rx, float *ry, float *rz, char t_axis)
{
	float zr = z - cam_z;
	float xr = x - cam_x;
	zz = zr;
	xx = xr;
	float rxv = zr * opt_tsinbeta + xr * opt_tcosbeta;
	z2 = zz * opt_tcosbeta - xx * opt_tsinbeta;
	float yr = y - cam_y;
	yy = yr;
	float rzv = z2 * opt_tcosalfa + yr * opt_tsinalfa;
	float ryv = yy * opt_tcosalfa - z2 * opt_tsinalfa;
	if (t_axis) {
		float my = ryv;
		ryv = my * opt_tcosgamma - rxv * opt_tsingamma;
		rxv = my * opt_tsingamma + rxv * opt_tcosgamma;
	}
	*rx = rxv;
	*ry = ryv;
	*rz = rzv;
	return !(rzv < uneg);
}

static inline char gamma_is_set ()
{
	Udword a, b;
	memcpy (&a, &_0, 4);
	memcpy (&b, &gamma, 4);
	return a != b;
}

/* Se si desidera una texture sui poligoni, invocare quest'altra funzione.
   Il parametro "tinta" specifica il colore meno luminoso di una sequenza
   di sue copie piu' intense. Il poligono deve avere sempre 4 vertici,
   anche se il numero di vertici effettivi e' specificato in "nv". */

void polymap (float *x, float *y, float *z, char nv, unsigned char tinta)
{
	float ultima_x[2*VERTICI_PER_POLIGONO];
	float ultima_y[2*VERTICI_PER_POLIGONO];
	float ultima_z[2*VERTICI_PER_POLIGONO];

	Word  fpart[256];
	Word  ipart[256];

	float tempXsize = XSIZE;
	float tempYsize = YSIZE;

	char  _flares = flares;

	char  culling = culling_needed;
	char  halfscan = halfscan_needed;

	Word  vr22;

	Dword i_low_lim = -10000;
	Dword i_hig_lim = +10000;

	Dword u, v, min_y, max_y;
	Dword x1, y1, x2, y2, ity, jty;

	float k1, k2, k3, k4;
	float hx, vx, ox, hy, vy, oy, hz, vz, oz, _x, _y, _z;
	float kx, rx, ry, rz, mx, my, mz, nx, ny, nz;

	float midx, midy, midz;
	float trxf[4], tryf[4], trzf[4];

	// Polymap ha routines fatte per 4 vertici.
	// Se sono 3, l'ultimo va duplicato.

	if (nv==3) {
		x[3] = x[2];
		y[3] = y[2];
		z[3] = z[2];
	}

	// Rototraslazione dei vertici; i dati rimangono ancora 3d.

	doflag = 0;
	char t_axis = gamma_is_set ();
	for (Word i = 0; i < 4; i++) {
		if (rototranslate (x[i], y[i], z[i], &rxf[i], &ryf[i], &rzf[i], t_axis)) {
			doflag++;
			rwf[i] = 1;
		}
		else
			rwf[i] = 0;
	}

	if (!doflag) return;

	// Calcolo dei vettori di tracciamento del texture mapping.

	Word li;
	if (nv == 3) {
		midx = (rxf[0] + rxf[1] + rxf[2]) * 0.3333333;
		midy = (ryf[0] + ryf[1] + ryf[2]) * 0.3333333;
		midz = (rzf[0] + rzf[1] + rzf[2]) * 0.3333333;
		for (vr = 0; vr < 3; vr++) {
			trxf[vr] = (rxf[vr] - midx) * x_antialias + midx;
			tryf[vr] = (ryf[vr] - midy) * y_antialias + midy;
			trzf[vr] = (rzf[vr] - midz) * z_antialias + midz;
		}
		trxf[3] = trxf[2];
		tryf[3] = tryf[2];
		trzf[3] = trzf[2];
		li = 2;
	}
	else {
		midx = (rxf[0] + rxf[1] + rxf[2] + rxf[3]) * 0.25;
		midy = (ryf[0] + ryf[1] + ryf[2] + ryf[3]) * 0.25;
		midz = (rzf[0] + rzf[1] + rzf[2] + rzf[3]) * 0.25;
		for (vr = 0; vr < 4; vr++) {
			trxf[vr] = (rxf[vr] - midx) * x_antialias + midx;
			tryf[vr] = (ryf[vr] - midy) * y_antialias + midy;
			trzf[vr] = (rzf[vr] - midz) * z_antialias + midz;
		}
		li = 3;
	}

	rx = trxf[0];
	mx = trxf[1] - rx;
	nx = rx - trxf[li];
	ry = tryf[0];
	my = tryf[1] - ry;
	ny = ry - tryf[li];
	rz = trzf[0];
	mz = trzf[1] - rz;
	nz = rz - trzf[li];

	hx = (rx * mz - rz * mx) * YCOEFF;
	hy = (rx * nz - rz * nx) * YCOEFF;
	hz = (nx * mz - nz * mx) * inv_dpp;
	vx = (rz * my - ry * mz) * XCOEFF;
	vy = (rz * ny - ry * nz) * XCOEFF;
	vz = (nz * my - ny * mz) * inv_dpp;
	ox = (ry * mx - rx * my) * EMU_K;
	oy = (ry * nx - rx * ny) * EMU_K;
	oz =  ny * mx - nx * my;

	// Fast-load per poligoni completamente visibili,
	// conversione dei punti alle spalle dell'osservatore.

	if (doflag == 4) {
		for (Word i = 0; i < 4; i++) {
			ultima_x[i] = rxf[i];
			ultima_y[i] = ryf[i];
			ultima_z[i] = rzf[i];
		}
		vr2 = 4;
	}
	else {
		vr2 = clip_3d (4, ultima_x, ultima_y, ultima_z);
	}
	vr22 = 2 * vr2;

	if (vr2<3) return;

	// Proiezioni prospettiche da 3d a 2d, con ricerca del minimo e del
	// massimo verticale dell'estensione del poligono sullo schermo.

	Dword dxm = ubyl, bxm = lbyl;
	for (Word i = vr2 - 1; i >= 0; i--) {
		float k = dpp / ultima_z[i];
		mp[2*i]   = fistp32 (k * ultima_x[i] + x_centro_f);
		mp[2*i+1] = fistp32 (k * ultima_y[i] + y_centro_f);
		if (mp[2*i+1] < dxm) dxm = mp[2*i+1];
		if (mp[2*i+1] > bxm) bxm = mp[2*i+1];
	}
	max_y = bxm;
	min_y = dxm;

	// Normalizzazione dei limiti.

	if (min_y < lbyl) min_y = lbyl;
	if (max_y > ubyl) max_y = ubyl;
	if (min_y > max_y) return;

	// Inizializzazione costanti di ottimizzazione.

	k1 = vx * _16;
	k2 = vy * _16;
	k3 = vz * _16;

	if (culling) {
		k1 *= 2;
		k2 *= 2;
		k3 *= 2;
	}

	// Inizializzazione delle matrici dei confini orizzontali.

	for (Dword i = min_y; i <= max_y; i++) {
		fpart[i & 255] = (Word)lbxl;
		ipart[i & 255] = (Word)ubxl;
	}

	// Calcolo confini orizzontali.

	mp[vr22]   = mp[0];
	mp[vr22+1] = mp[1];
	for (Word e = 0; e < vr2; e++) {
		Dword ey1 = mp[2*e+1];
		Dword ey2 = mp[2*e+3];
		Dword ex1 = mp[2*e];
		Dword ex2 = mp[2*e+2];
		if (ey2 < ey1) {
			x1 = ex2; y1 = ey2;
			x2 = ex1; y2 = ey1;
		}
		else {
			x1 = ex1; y1 = ey1;
			x2 = ex2; y2 = ey2;
		}
		if (y2 == y1) continue;
		kx = (float)((double)x2 - (double)x1) / (float)((double)y2 - (double)y1);
		if (y1 < lbyl) {
			ity = lbyl;
			x1 = fistp32 ((double)x1 + (double)(lbyl - y1) * kx);
		}
		else
			ity = y1;
		if (y2 > ubyl)
			jty = ubyl;
		else
			jty = y2;
		double bndx = x1;
		if (ity >= jty) continue;
		for (Dword h = ity; h <= jty; h++) {
			Dword b = fistp32 (bndx);
			if (b < i_low_lim) b = i_low_lim;
			if (b > i_hig_lim) b = i_hig_lim;
			Word b16 = (Word)b;
			if (b16 > fpart[h & 255]) {
				if (b16 < (Word)ubxl)
					fpart[h & 255] = b16;
				else
					fpart[h & 255] = (Word)ubxl;
			}
			if (b16 < ipart[h & 255]) {
				if (b16 > (Word)lbxl)
					ipart[h & 255] = b16;
				else
					ipart[h & 255] = (Word)lbxl;
			}
			bndx += kx;
		}
	}

	// Pre-assegnamenti di lavoro.

	const unsigned char *tex = dos_paragraph_base (txtr);
	adapted[0xFA00 - 4] = tinta;		// primo colore (come nell'originale)
	adapted[0xFA01 - 4] = escrescenze;	// primo colore bumps

	// Ciclo di tracciamento.

	Uword i = (Uword)min_y;
	for (;;) {
		Word ip = ipart[i & 255];
		Word fp = fpart[i & 255];
		float t = (float)ip - x_centro_f + uno;
		float iy = (float)(Word)i - y_centro_f;
		_z = vz * t + hz * iy + oz;
		k4 = uno / _z;
		_x = vx * t + hx * iy + ox;
		_y = vy * t + hy * iy + oy;
		v = fistp32 ((double)_y * tempYsize * k4);
		u = fistp32 ((double)_x * tempXsize * k4);
		Word sections = fp - ip;
		Uword base = riga[i & 255] + ip;

		if (!culling) {
			// tracciamento di una scanline: versione ad alto dettaglio
			Uword di = base;
			while (sections > 0) {
				unsigned char cl = (sections > 16) ? 16 : (unsigned char)sections;
				sections -= 16;
				if (!cl) continue;
				_z += k3;
				_x += k1;
				k4 = uno / _z;
				Dword u0 = u, v0 = v;
				_y += k2;
				u = fistp32 ((double)_x * tempXsize * k4);
				v = fistp32 ((double)_y * tempYsize * k4);
				Uword su = (Uword)((u - u0) >> 4);
				Uword sv = (Uword)((v - v0) >> 4);
				Uword au = (Uword)u0, dv = (Uword)v0;
				switch (_flares & 15) {
				case 0: // normale
					while (cl) {
						Uword bx = (dv & 0xFF00) | (au >> 8);
						adapted[di] = (unsigned char)(tinta + tex[bx]);
						di++; au += su; dv += sv; cl--;
					}
					break;
				default:
					if (_flares & 1) { // lucido
						while (cl) {
							Uword bx = (dv & 0xFF00) | (au >> 8);
							adapted[di] = (unsigned char)(adapted[di] + tex[bx]);
							di++; au += su; dv += sv; cl--;
						}
					}
					else if (_flares & 2) { // brillante
						while (cl) {
							Uword bx = (dv & 0xFF00) | (au >> 8);
							unsigned char ch = (unsigned char)((adapted[di] & 0x3F) + tex[bx]);
							if (ch > 0x3E) ch = 0x3E;
							adapted[di] = (adapted[di] & 0xC0) | ch;
							di++; au += su; dv += sv; cl--;
						}
					}
					else if (_flares & 4) { // traslucido
						while (cl) {
							Uword bx = (dv & 0xFF00) | (au >> 8);
							unsigned char ch = (unsigned char)((adapted[di] & 0x3F) + tex[bx]);
							ch = (unsigned char)(ch + tinta);
							ch >>= 1;
							adapted[di] = (adapted[di] & 0xC0) | ch;
							di++; au += su; dv += sv; cl--;
						}
					}
					else { // bump mapping
						while (cl) {
							Uword bx = (dv & 0xFF00) | (au >> 8);
							unsigned char ch = (unsigned char)(tinta + tex[bx]);
							adapted[di] = ch;
							Uword n = (ch & 7) + 1;
							unsigned char bump = (unsigned char)(2 * tex[bx] + escrescenze);
							adapted[(Uword)(di + 640 - 320 * n)] = bump;
							di++; au += su; dv += sv; cl--;
						}
					}
				}
			}
		}
		else {
			// tracciamento di una scanline: versione a basso dettaglio
			Uword di = base;
			while (sections > 0) {
				unsigned char cl = (sections > 32) ? 32 : (unsigned char)(sections + 2);
				sections -= 32;
				if ((signed char)cl < 2) continue;
				_z += k3;
				_x += k1;
				k4 = uno / _z;
				Dword u0 = u, v0 = v;
				cl >>= 1;
				_y += k2;
				u = fistp32 ((double)_x * tempXsize * k4);
				v = fistp32 ((double)_y * tempYsize * k4);
				Uword su = (Uword)((u - u0) >> 4);
				Uword sv = (Uword)((v - v0) >> 4);
				Uword au = (Uword)u0, dv = (Uword)v0;
				Uword pd = di;
				switch (_flares & 15) {
				case 0:
					while (cl) {
						Uword bx = (dv & 0xFF00) | (au >> 8);
						unsigned char ch = (unsigned char)(tinta + tex[bx]);
						adapted[pd] = ch;
						adapted[(Uword)(pd+1)] = ch;
						pd += 2; au += su; dv += sv; cl--;
					}
					break;
				default:
					if (_flares & 1) {
						while (cl) {
							Uword bx = (dv & 0xFF00) | (au >> 8);
							unsigned char ch = (unsigned char)(adapted[(Uword)(pd+1)] + tex[bx]);
							adapted[pd] = ch;
							adapted[(Uword)(pd+1)] = ch;
							pd += 2; au += su; dv += sv; cl--;
						}
					}
					else if (_flares & 2) {
						while (cl) {
							Uword bx = (dv & 0xFF00) | (au >> 8);
							unsigned char ch = (unsigned char)((adapted[pd] & 0x3F) + tex[bx]);
							if (ch > 0x3E) ch = 0x3E;
							ch |= adapted[pd] & 0xC0;
							adapted[pd] = ch;
							adapted[(Uword)(pd+1)] = ch;
							pd += 2; au += su; dv += sv; cl--;
						}
					}
					else if (_flares & 4) {
						while (cl) {
							Uword bx = (dv & 0xFF00) | (au >> 8);
							unsigned char ch = (unsigned char)((adapted[pd] & 0x3F) + tex[bx]);
							ch = (unsigned char)(ch + tinta);
							ch >>= 1;
							ch |= adapted[pd] & 0xC0;
							adapted[pd] = ch;
							adapted[(Uword)(pd+1)] = ch;
							pd += 2; au += su; dv += sv; cl--;
						}
					}
					else {
						while (cl) {
							Uword bx = (dv & 0xFF00) | (au >> 8);
							unsigned char ch = (unsigned char)(tinta + tex[bx]);
							adapted[pd] = ch;
							adapted[(Uword)(pd+1)] = ch;
							Uword n = (ch & 7) + 1;
							unsigned char bump = (unsigned char)(2 * tex[bx] + escrescenze);
							adapted[(Uword)(pd + 640 - 320 * n)] = bump;
							adapted[(Uword)(pd + 641 - 320 * n)] = bump;
							pd += 2; au += su; dv += sv; cl--;
						}
					}
				}
				di += 32;
			}
		}

		// codice inter-scanline: fra una scanline e la successiva...

		if (halfscan) {
			i++;
			if ((Uword)max_y < i) break;
			Word dx = ipart[(i-1) & 255];
			Word ax = fpart[(i-1) & 255];
			Uword di = riga[i & 255];
			if ((Uword)ax > (Uword)dx) {
				Uword n = (Uword)ax - (Uword)dx;
				di += dx;
				while (n) {
					unsigned char dl = adapted[(Uword)(di - 320)];
					adapted[di] = dl;
					adapted[(Uword)(di + 320)] = dl;
					di++;
					n--;
				}
			}
		}
		i++;
		if ((Uword)max_y < i) break;
	}
}

void Forward (float delta) /* Provoca l'avanzamento dell'osservatore
			      nella direzione di volo di <delta>
			      unita' virtuali. */
{
	cam_x -= delta * opt_tsinbeta * opt_tcosalfa;
	cam_z += delta * opt_tcosbeta * opt_tcosalfa;
	cam_y += delta * opt_tsinalfa;
}

Dword _x_, _y_;

char getcoords (float x, float y, float z)
{ // calcola le coordinate sullo schermo di un punto nello spazio,
  // usando lo stesso nucleo di calcolo di poly3d e di polymap,
  // se il punto rientra nello schermo ritorna 1, altrimenti ritorna 0.
  // le coordinate sono poi trasferite nelle variabili _x_ e _y_.

	float rx, ry, rz;

	if (!rototranslate (x, y, z, &rx, &ry, &rz, gamma_is_set ()))
		return (0);

	float my = dpp / rz;
	_x_ = fistp32 (my * rx + x_centro_f);
	_y_ = fistp32 (my * ry + y_centro_f);

	if (_x_ > lbxl && _x_ < ubxl && _y_ > lbyl && _y_ < ubyl)
		return (1);
	else
		return (0);
}

char facing (float *x, float *y, float *z)
{ /* Controlla se un poligono a una sola faccia e' visibile o meno. */
	float x1, y1, z1, x2, y2, z2, xr, yr, zr;

	x1 = x[0] - x[2];
	y1 = y[0] - y[2];
	z1 = z[0] - z[2];
	x2 = x[1] - x[2];
	y2 = y[1] - y[2];
	z2 = z[1] - z[2];
	xr = z2 * y1 - y2 * z1;
	yr = z1 * x2 - z2 * x1;
	zr = x1 * y2 - x2 * y1;
	float d = (cam_x - x[2]) * xr + (cam_y - y[2]) * yr + (cam_z - z[2]) * zr;
	if (d < 0) return (0);
	return (char)0xFF;
}
