/*
	GOES Net module SL (Linux port: built into the game).
	Original source by Alessandro Ghignola; NICE/NIV+ changes by their
	authors. The stand-alone DOS program becomes goes_sl::module_main.
*/

#include "goes_common.h"

namespace goes_sl {

/*

	Noctis galactic guide / SL command.
	GOES Net Module.

*/

//////////////////////////////////////////////////////////////////////////////

char   *situation_file  = "..\\DATA\\Current.BIN";

char   nsync               = 1;		// 0
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
Word    sys                = 4;		// 25
Word    pwr                = 15000;	// 27
Word    dev_page           = 0;		// 29
Word    ap_target_class    = 0;		// 31
Word    f_ray_elapsed	  = 0;		// 33
Word    nearstar_class     = 0;		// 35
Word    nearstar_nop       = 0;		// 37
float  pos_x              = 0;		// 39
float  pos_y              = 0;		// 43
float  pos_z              = -500;	// 47
float  user_alfa          = 0;		// 51
float  user_beta          = 0;		// 55
float  navigation_beta    = 0;		// 59
float  ap_target_ray      = 0;		// 63
float  nearstar_ray       = 1000;	// 67
double dzat_x             = +3797120;
double dzat_y             = -4352112;
double dzat_z             = -925018;
double ap_target_x        = 0;
double ap_target_y        = 1E9;
double ap_target_z        = 0;
double nearstar_x         = 0;
double nearstar_y         = 1E9;
double nearstar_z         = 0;
double helptime           = 0;
double ip_target_initial_d= 0;
double requested_approach_coefficient = 1;
double current_approach_coefficient = 1;
double reaction_time      = 0.01;
char   fcs_status[11]     = "STANDBY";
Word    fcs_status_delay   = 0;
Word    psys               = 4;
double ap_target_initial_d= 0;
double requested_vimana_coefficient = 1;
double current_vimana_coefficient = 1;
double vimana_reaction_time = 0.01;
char   lithium_collector  = 0;
char   autoscreenoff	  = 0;
char   ap_reached         = 0;
Word    lifter		  = 0;
double secs		  = 0;

void unfreeze ()
{
	Word fh = _open (situation_file, 0);
	if (fh>-1) {
		GOES_READ_SITUATION (fh);
		_close (fh);
	}
}

//////////////////////////////////////////////////////////////////////////////

char digimap[65*5] = {
	 0, 0, 0, 0, 0, // 32. spazio vuoto
	 2, 2, 2, 0, 2, // 33. punto esclamativo
	 5, 0, 0, 0, 0, // 34. virgolette
	 0, 0, 3, 5, 5, // 35. cancelletto (rappresentato da una piccola n.)
	 2, 2, 6, 2, 2, // 36. sistro
	 1, 4, 2, 1, 4, // 37. percento
	 0, 0, 2, 0, 0, // 38. e commericale (non ' possibile visualizzarla)
	 0, 2, 2, 0, 0, // 39. apice
	 4, 2, 2, 2, 4, // 40. parentesi tonda aperta
	 1, 2, 2, 2, 1, // 41. parentesi tonda chiusa
	 0, 0, 7, 2, 2, // 42. asterisco
	 0, 2, 7, 2, 0, // 43. segno pi'
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
	 0, 1, 2, 4, 0, // 92. barra sinistrorsa
	 2, 2, 6, 0, 0, // 93. parentesi quadra chiusa
	 2, 2, 2, 2, 2, // 94. ordinale femminile
	 0, 0, 0, 0, 7, // 95. sottolinea
	 1, 2, 0, 0, 0  // 96. accento
};



void wrouthud (Uword x, Uword y, Uword l, const char *text)
{
	Word j, i, n;
	Uword spot;

	n = 0; if (!l) l = 32767;
	spot = y * 320 + x;

	while (text[n] && n < l) {
		j = (text[n] - 32) * 5;
		for (i = 0; i < 5; i++) {
			if (digimap[j + i] & 1) adaptor[spot+0] = 191 - adaptor[spot+0];
			if (digimap[j + i] & 2) adaptor[spot+1] = 191 - adaptor[spot+1];
			if (digimap[j + i] & 4) adaptor[spot+2] = 191 - adaptor[spot+2];
			spot += 320;
		}
		spot -= 320 * 5;
		spot += 4;
		n++;
	}
}

void warn (const char *text, Word line)
{
	Word w2 = strlen(text) * 2;

	areaclear (adaptor, 160 - w2 - 1, 100 + line * 8, 0, 0, w2*2 + 2, 8, 127);
	wrouthud (160 - w2, 100 + line * 8 + 1, NULL, text);
	goes_refresh ();
}

//////////////////////////////////////////////////////////////////////////////

char msgbuffer[77];
char *divider = "&&&&&&&&&&&&&&&&&&&&&";

void msg (const char *string)
{
	Word x;

	strcpy (msgbuffer, string);
	msgbuffer[21] = 0;
	printf (msgbuffer);

	x = strlen(msgbuffer);
	while (x < 21) {
		printf (" ");
		x++;
	}
}

double 	object_id = 12345;
char   	object_label[25];
double 	s_object_id = 12345;
char   	s_object_label[25];

double	subject_id = 12345;
double  idscale = 0.00001;

Word	sts;
char	query;

Dword	round;
Word 	i, fh, gh;
Word 	analyzed_sectors_range;
char	*file = "..\\DATA\\STARMAP.BIN";
char	*guide = "..\\DATA\\GUIDE.BIN";

char	outbuffer[40];
char	textbuffer[40];
char	parbuffer[160];
char	nullbuffer[128];
char	objectname[21];
char	subjectname[21];

double	mblock_subject;
char	mblock_message[77];

//////////////////////////////////////////////////////////////////////////////

double laststar_x, laststar_y, laststar_z;
Dword   current_star, total_stars;

char isthere (double star_id)
{
	// (the original 32-bit assembly scan, see goes_scan)
	char r = goes_scan (star_id, idscale, (Uword)analyzed_sectors_range, 0,
		dzat_x, dzat_y, dzat_z, &laststar_x, &laststar_y, &laststar_z,
		[&] (Uword done, Uword total) -> char {
			char warnbuffer[41];
			sprintf (warnbuffer, "SLICE %d OF %d", done, total);
			warn (warnbuffer, 0);
			sprintf (warnbuffer, "SUBJECT %d OF %d", (int)current_star, (int)total_stars);
			warn (warnbuffer, 1);
			while (kbhit()) {
				if (getch() == 27) return (char)2;
			}
			return (char)0;
		});
	return r;
}

//////////////////////////////////////////////////////////////////////////////

void liststars ()
{
	char	least = 0, retval;
	double	xdist, ydist, zdist, dist;

	lseek (fh, 4, SEEK_SET);
	while (_read (fh, &object_id, 8) && _read (fh, &object_label, 24) == 24) {
		if (memcmp (&object_id, "Removed:", 8) && object_label[21] == 'S') {
			least = 1;
			if (sts) {
				current_star++;
				retval = isthere (object_id);
				if (retval == 2) {
					msg (divider);
					msg ("INTERRUPTED!");
					return;
				}
				if (retval) {
					sprintf (textbuffer, "*%s", object_label);
					msg (textbuffer);
					sprintf (textbuffer, "$X=%1.0f", laststar_x);
					msg (textbuffer);
					sprintf (textbuffer, "$Y=%1.0f", -laststar_y);
					msg (textbuffer);
					sprintf (textbuffer, "$Z=%1.0f", laststar_z);
					msg (textbuffer);
					xdist = dzat_x - laststar_x;
					ydist = dzat_y - laststar_y;
					zdist = dzat_z - laststar_z;
					dist = sqrt(xdist*xdist+ydist*ydist+zdist*zdist);
					sprintf (textbuffer, "$D=%1.2f L.Y.", dist * 5E-5);
					msg (textbuffer);
					msg ("[&&&&&&&&&&&&&&&&&&&&");
				}
			}
			else {
				sprintf (textbuffer, "*%s", object_label);
				msg (textbuffer);
			}
		}
	}

	if (least) {
		msg (divider);
		msg ("STARS LISTING END.");
	}
	else
		msg ("NO KNOWN STARS.");
}

int module_main (Word _argc, char **_argv)
{
	msg ("________USAGE________");
	msg ("SL (OPTIONAL RANGE)");
	msg ("^^^^^^^^^^^^^^^^^^^^^");

	if (_argc < 2)
		msg ("GLOBAL STARS LISTING:");
	else
		msg ("RANGED STARS LISTING:");

	msg (divider);

	fh = _open (file, 4);
	if (fh == -1) {
		msg ("STARMAP NOT AVAILABLE");
		return 0;
	}

	i = 2;
	strcpy (parbuffer, _argv[1]);
	while (i < _argc) {
		strcat (parbuffer, " ");
		strcat (parbuffer, _argv[i]);
		i++;
	}

	i = 0;
	while (parbuffer[i]) {
		if (parbuffer[i] == '_')
			parbuffer[i] = 32;
		i++;
	}

	if (_argc > 1) {
		sts = atoi (parbuffer);
		if (sts <= 2 || sts > 10000)
			analyzed_sectors_range = 100;
		else
			analyzed_sectors_range = sts;
		unfreeze ();
		warn ("SCANNING THE GALAXY: ESC TO STOP", -1);
		total_stars = 0;
		current_star = 0;
		lseek (fh, 4, SEEK_SET);
		while (_read (fh, &object_id, 8) && _read (fh, &object_label, 24) == 24) {
			if (memcmp (&object_id, "Removed:", 8) && object_label[21] == 'S')
				total_stars++;
		}
	}
	else
		sts = 0;

	liststars ();

	_close (fh);
	return 0;
}

} // namespace goes_sl
