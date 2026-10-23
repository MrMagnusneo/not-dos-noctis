/*
	Noctis: external declarations of the base module (NOCTIS-0.CPP).
	Linux port: integer types keep their 16/32-bit DOS sizes.
*/

#ifndef NOCTIS_0_H
#define NOCTIS_0_H

#include "noctis-d.h"

extern Word QUADWORDS;
extern unsigned char *adaptor;
extern unsigned char *adapted;
extern float moviefps;
extern void reach_your_dir (void);
extern void _320_200_256 (void);
extern void _80_25_C (void);
extern Word attendi_pressione_tasto (void);
extern Word tasto_premuto (void);
extern unsigned char range8088[64*3];
extern unsigned char tmppal[768];
extern unsigned char return_palette[768];
extern unsigned char surface_palette[768];
extern void tavola_colori (unsigned char *nuova_tavolozza,
		    Uword colore_di_partenza, Uword nr_colori,
		    char filtro_rosso, char filtro_verde, char filtro_blu);
extern Word mdltx, mdlty, mx, my, mpul;
extern void mouse_input (void);
extern char test_and_init_mouse (void);
extern void pcopy (unsigned char *dest, unsigned char *sorg);
extern void pclear (unsigned char *target, unsigned char pattern);
extern void areacopy (unsigned char *dest, unsigned char *sorg,
		      Word x, Word y, Word l, Word h);
extern void areaclear (unsigned char *dest, Word x, Word y,
		       Word x2, Word y2, Word l, Word h, unsigned char pattern);
extern void psmooth_grays (unsigned char *target);
extern void pfade (unsigned char *target, Uword segshift, unsigned char speed);
extern void psmooth_64 (unsigned char *target, Uword segshift);
extern void smootharound_64 (unsigned char *target, Dword cx, Dword cy, Dword r, char diffuse);
extern void mask_pixels (unsigned char *target, unsigned char mask);
extern char fcs_status_extended[42];
const Word TEXTURE_XSIZE = 256; // Larghezza delle BMP.
const Word TEXTURE_YSIZE = 256; // Altezza delle BMP.
extern Dword XSIZE, YSIZE; // calibrazione BMP.
extern float uneg;
extern float alfa, beta, gamma;
extern float cam_x, cam_y, cam_z;
extern float dpp;
extern Dword H_MATRIXS, V_MATRIXS;
extern Uword riga[256];
extern void initscanlines (void);
extern Uword ptr;
extern Udword xp, yp, xa, ya;
extern Udword global_x, global_y;
extern void Segmento (void);
extern char flares;
extern float x2, y2, z2;
extern float xx, yy, zz;
extern Dword lbxl;
extern Dword ubxl;
extern Dword lbyl;
extern Dword ubyl;
extern float lbxf;
extern float ubxf;
extern float lbyf;
extern float ubyf;
extern float x_centro_f;
extern float y_centro_f;
extern float opt_pcosbeta;
extern float opt_psinbeta;
extern float opt_tcosbeta;
extern float opt_tsinbeta;
extern float opt_pcosalfa;
extern float opt_psinalfa;
extern float opt_tcosalfa;
extern float opt_tsinalfa;
extern void change_angle_of_view (void);
extern void change_camera_lens (void);
extern void change_txm_repeating_mode (void);
extern unsigned char entity;
extern void poly3d (float *x, float *y, float *z,
	     Uword nrv, unsigned char colore);
extern void Forward (float delta);
extern float pnx, pny, pnz;
extern void pnorm (float *x, float *y, float *z);
extern unsigned char *txtr;
extern float x_antialias, y_antialias, z_antialias;
extern char culling_needed, halfscan_needed;
extern unsigned char escrescenze;
extern void polymap (float *x, float *y, float *z, char nv, unsigned char tinta);
extern Dword _x_, _y_;
extern char getcoords (float x, float y, float z);
extern char facing (float *x, float *y, float *z);

extern const char   *situation_file;
extern const char   *starmap_file;
extern const char   *goesoutputfile;
extern const char   *surface_file;
extern Word   sfh;

extern char   nsync;
extern char   anti_rad;
extern char   pl_search;
extern char   field_amplificator;
extern char   ilight;
extern char   ilightv;
extern char   charge;
extern char   revcontrols;
extern char   ap_targetting;
extern char   ap_targetted;
extern char   ip_targetting;
extern char   ip_targetted;
extern char   ip_reaching;
extern char   ip_reached;
extern char   ap_target_spin;
extern char   ap_target_r;
extern char   ap_target_g;
extern char   ap_target_b;
extern char   nearstar_spin;
extern char   nearstar_r;
extern char   nearstar_g;
extern char   nearstar_b;
extern char   gburst;
extern char   menusalwayson;
extern char   depolarize;
extern Word   sys;
extern Word   pwr;
extern Word   dev_page;
extern Word   ap_target_class;
extern Word   f_ray_elapsed;
extern Word   nearstar_class;
extern Word   nearstar_nop;
extern float  pos_x;
extern float  pos_y;
extern float  pos_z;
extern float  user_alfa;
extern float  user_beta;
extern float  navigation_beta;
extern float  ap_target_ray;
extern float  nearstar_ray;
extern double dzat_x;
extern double dzat_y;
extern double dzat_z;
extern double ap_target_x;
extern double ap_target_y;
extern double ap_target_z;
extern double nearstar_x;
extern double nearstar_y;
extern double nearstar_z;
extern double helptime;
extern double ip_target_initial_d;
extern double requested_approach_coefficient;
extern double current_approach_coefficient;
extern double reaction_time;
extern char   fcs_status[11];
extern Word   fcs_status_delay;
extern Word   psys;
extern double ap_target_initial_d;
extern double requested_vimana_coefficient;
extern double current_vimana_coefficient;
extern double vimana_reaction_time;
extern char   lithium_collector;
extern char   autoscreenoff;
extern char   ap_reached;
extern Word   lifter;
extern double secs;
extern char   data;
extern char   surlight;

extern Word   old_currentbin_length;
extern Dword  lastSnapshot;
extern char   option_mouseLook;
extern char   option_antialias_in_stardrifter;

/* Serialisation of the 245-byte "situation" block of CURRENT.BIN. */
extern void situation_to_bytes (unsigned char *out);
extern void situation_from_bytes (const unsigned char *in);

extern char   land_now;
extern char   landing_point;
extern Word   landing_pt_lon;
extern Word   landing_pt_lat;
extern char seconds[3], minutes[3], hours[3], day[3], month[3], year[5];
extern Word  epoc;
extern char          ctb[512];
extern char	      _delay;
extern char          stspeed;
extern char          bright;
extern char   	     elight;
extern Uword         gl_start;
extern Uword         point;
extern Uword         vptr;
extern Word          infoarea;
extern Word          s_control;
extern Word          s_command;
extern Word          isecs, p_isecs;
extern double	     fsecs;
extern Word	     gl_fps;
extern Word	     fps;
extern float         dlt_alfa;
extern float	     dlt_beta;
extern float	     dlt_nav_beta;
extern float	     step;
extern float	     shift;
extern double        s_m;
extern double	     plx, ply, plz;
extern double        pxx, pyy;
extern double        delta_x, delta_y;
extern double 	     nearstar_identity;
extern Word	     nearstar_nob;
extern Word	     nearstar_labeled;
extern Word 	     npcs;
extern Word 	     resident_map1, resident_map2;
extern char   	     ontheroof;
extern Word	     datasheetscroll;
extern Word	     datasheetdelta;

extern const char *ord[21];
extern const char *star_description[12];
extern char class_rgb[3*star_classes];
extern Word class_ray[star_classes];
extern Word class_rayvar[star_classes];
extern Word class_act[star_classes];
extern char class_planets[star_classes];
extern char   nearstar_p_type       [maxbodies];
extern Word   nearstar_p_owner	    [maxbodies];
extern char   nearstar_p_moonid     [maxbodies];
extern double nearstar_p_ring       [maxbodies];
extern double nearstar_p_tilt       [maxbodies];
extern double nearstar_p_ray        [maxbodies];
extern double nearstar_p_orb_ray    [maxbodies];
extern double nearstar_p_orb_seed   [maxbodies];
extern double nearstar_p_orb_tilt   [maxbodies];
extern double nearstar_p_orb_orient [maxbodies];
extern double nearstar_p_orb_ecc    [maxbodies];
extern Word   nearstar_p_rtperiod   [maxbodies];
extern Word   nearstar_p_rotation   [maxbodies];
extern Word   nearstar_p_term_start [maxbodies];
extern Word   nearstar_p_term_end   [maxbodies];
extern Word   nearstar_p_qsortindex [maxbodies];
extern float  nearstar_p_qsortdist  [maxbodies];
extern const char *planet_description[11];
extern unsigned char planet_rgb_and_var[44];
extern Word planet_possiblemoons[11];
extern float mindiff;
extern unsigned char  *p_background;
extern unsigned char  *s_background;
extern unsigned char  *p_surfacemap;
extern quadrant       *objectschart;
extern unsigned char  *ruinschart; // come objectschart, ma dichiarato in bytes
extern unsigned char  *pvfile;
extern unsigned char  *n_offsets_map;
extern unsigned char  *n_globes_map;
extern char sky_red_filter;
extern char sky_grn_filter;
extern char sky_blu_filter;
extern char gnd_red_filter;
extern char gnd_grn_filter;
extern char gnd_blu_filter;
extern float planet_grav;
extern float rainy;
extern Word albedo;
extern unsigned char sky_brightness;
extern unsigned char horiz_brt;
extern Uword m200[200];
extern float	rwp;
extern float    iwp;
extern float    wp;
extern float    wdir;
extern float    wdirsin;
extern float    wdircos;
extern char landed;
extern Dword atl_x, atl_z, atl_x2, atl_z2;
extern double qid;
extern Dword flat_rnd_seed;
extern void fast_srand (Dword seed);
static inline void fast_srand (double seed) { fast_srand ((Dword)bc_ftol (seed)); }
static inline void fast_srand (float seed) { fast_srand ((Dword)bc_ftol (seed)); }
extern Dword fast_random (Dword mask);
extern Word ranged_fast_random (Word range);
extern Word sa_open (Dword offset_of_virtual_file);
extern void shade (unsigned char *palette_buffer,
		   Word first_color, Word number_of_colors,
		   float start_r,  float start_g,  float start_b,
		   float finish_r, float finish_g, float finish_b);
extern char snapfilename[64];
extern void snapshot (Word forcenumber, char showdata);
extern void from_vehicle (void);
extern void from_user (void);
extern void watch (double cam_x, double cam_y, double cam_z,
	    double see_x, double see_y, double see_z);
extern char xy (double cam_x, double cam_y, double cam_z,
	 double point_x, double point_y, double point_z);
extern void p_Forward (float delta);
extern void copia (unsigned char *dest, unsigned char *sorg, Uword x, Uword y, Word l, Word a);
extern void setfx (char fx);
extern void chgfx (char fx);
extern void resetfx (void);
extern void Stick (Udword xp, Udword yp, Udword xa, Udword ya);
extern Dword fpx;
extern Dword fpy;
extern float p_rx, p_ry, p_rz;
extern float stick_uneg;
extern void stick3d (float p_x, float p_y, float p_z,
	      float x, float y, float z);
extern void link3d (float x, float y, float z);
extern void fline (Dword fx, Dword fy, Dword lx, Dword ly);
extern unsigned char map_color_a;
extern unsigned char map_color_b;
extern unsigned char map_color_c;
extern unsigned char map_color_d;
extern void randomic_mapper (float x0, float y0, float z0,
		      float x1, float y1, float z1,
		      float x2, float y2, float z2,
		      char divisions);
extern void unloadpv (Word handle);
extern void unloadallpv (void);
extern char loadpv (Word handle, Dword virtual_file_position,
		    float xscale, float yscale, float zscale,
		    float xmove,  float ymove,  float zmove,
		    unsigned char base_color,   char depth_sort);
extern void QuickSort (Word *index, float *mdist, Word start, Word end);
extern void drawpv (Word handle, Word mode, Word rm_iterations,
		    float center_x, float center_y, float center_z,
		    char use_depth_sort);
extern void copypv (Word dest_handle, Word src_handle);
extern void modpv (Word handle, Word polygon_id, Word vertex_id,
		   float x_scale, float y_scale, float z_scale,
		   float x_angle, float y_angle, float z_angle,
		   pvlist *vertexs_to_affect);
extern char *alphavalue (double value);
extern char *formatTriad (double value);
extern void background (Uword start,
		 unsigned char *target,
		 unsigned char *background,
		 unsigned char *offsetsmap,
		 Uword total_map_bytes,
		 Uword screenshift);
extern void sky (Uword limits);
extern double laststar_x, laststar_y, laststar_z;
extern char isthere (double star_id);
extern unsigned char glass_bubble;
extern void globe (Uword start,
	    unsigned char *target,
	    unsigned char *tapestry,
	    unsigned char *offsetsmap,
	    Uword total_map_bytes,
	    double x, double y, double z,
	    float mag_factor, char colormask,
	    char globe_saturation);
extern void glowinglobe (Word start,
	    unsigned char *target,
	    unsigned char *offsetsmap,
	    Uword total_map_bytes,
	    double x, double y, double z, float mag_factor,
	    Word terminator_start, Word terminator_arc, unsigned char color);
extern void whiteglobe (unsigned char *target,
	    double x, double y, double z,
	    float mag_factor, float fgm_factor);
extern double xsun_onscreen;
extern void whitesun (unsigned char *target,
	    double x, double y, double z,
	    float mag_factor, float fgm_factor);
extern float lft_sin[361];
extern float lft_cos[361];
extern char lens_flares_init (void);
extern void lens_flares_for (double cam_x, double cam_y, double cam_z,
		      double xlight, double ylight, double zlight,
		      double step, Word added, char on_hud, char condition,
		      Word xshift, Word yshift);
extern void single_pixel_at_ptr (Uword ptr, unsigned char pixel_color);
extern char far_pixel_at (double xlight, double ylight, double zlight, double radii, unsigned char unconditioned_color);
extern Udword *digimap2;
extern void cupola (float y_or, float brk);
extern void polycupola (float y_or, char textured);
extern void getsecs (void);
extern void extract_ap_target_infos (void);
extern float zrandom (Word range);
extern Word starnop (double star_x, double star_y, double star_z);
extern void prepare_nearstar (void);
extern void ssmooth (unsigned char *target);
extern void lssmooth (unsigned char *target);
extern void planet_xyz (Word n);
extern float rtp (Word n);
extern Word planet_viewpoint (double obs_x, double obs_z);
extern void surface (Word logical_id, Word type, double seedval, unsigned char colorbase);
extern Word nightzone, crepzone, sun_x_factor;
extern void planets (void);
extern void init_FP_segments (void);
extern Word smh;
extern double idscale;
extern Dword search_id_code (double id_code, char type);
extern Dword tgt_collect_lastpos;
extern Word targets;
extern Word topmost_target;
extern Word target_line;
extern char update_targets;
extern char collecting_targets;
extern double targets_table_id[50];
extern double targets_table_px[50];
extern double targets_table_py[50];
extern double targets_table_pz[50];
extern void collect_targets (void);
extern void status (const char *status_description, Word message_delay);
extern float flandom ();
extern float fast_flandom ();

extern clock_t gtime;
extern void sync_start ();
extern void sync_stop ();

extern Dword  global_surface_seed;
extern float  ppos_x, ppos_y, ppos_z;
extern double dsd;

extern void load_starface ();
extern void load_QVRmaps ();
extern void load_digimap2 ();

extern char outhudbuffer[96];
extern void wrouthud (Uword x, Uword y, Uword l, const char *text);
extern void surrounding (char compass_on, Word openhudcount);

extern float	pp_gravity;
extern float	pp_temp;
extern float	pp_pressure;
extern float    pp_pulse;

extern void additional_consumes (void);

/* Moviemaker and HUD helpers (noctis-0.cpp). */
extern void CalculatemovieFPS (Word moviefscap, Word moviestime);
extern void ShowMovieSetup (Word moviefsec, char movieflash, Word moviedeck);
extern void FitOutHudBuffer (Word min, Word max);
extern void ShowAboutPage (char surface);

/* Port additions. */
extern void port_present_now (void);
extern int port_quit_requested (void);

#endif
