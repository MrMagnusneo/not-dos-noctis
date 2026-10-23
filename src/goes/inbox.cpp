/*
	GOES Net module INBOX (Linux port: built into the game).
	Original source by Alessandro Ghignola; NICE/NIV+ changes by their
	authors. The stand-alone DOS program becomes goes_inbox::module_main.
*/

#include "goes_common.h"

namespace goes_inbox {

/*

	INBOX command.
	GOES Net Module.

*/

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

double 	object_id;
char   	object_label[25];
double 	s_object_id;
char   	s_object_label[25];
double	mblock_subject;
char	mblock_message[77];
double	s_mblock_subject;
char	s_mblock_message[77];
double  idscale = 0.00001;

Dword	round;
Word 	fh, gh, ph, fh2, gh2;
char	*map = "..\\DATA\\STARMAP.BIN";
char	*guide = "..\\DATA\\GUIDE.BIN";
char	*packet = "..\\DATA\\INBOX.ZIP";
char	*tmap = "..\\DATA\\SM.TMP";
char	*tguide = "..\\DATA\\GG.TMP";

Dword	starmap_size, sm_in;
Dword	guide_size, gg_in;

char	textbuffer[40];

int module_main (Word _argc, char **_argv)
{

	warn ("UPDATING ARCHIVES...", 0);

	msg ("     GOES  INBOX     ");
	msg (divider);

	fh = _open (map, 4);
	if (fh == -1) {
		msg ("STARMAP NOT AVAILABLE");
		return 0;
	}

	gh = _open (guide, 4);
	if (gh == -1) {
		msg ("GUIDE NOT AVAILABLE");
		_close (fh);
		return 0;
	}

	ph = _open (packet, 0);
	if (fh == -1) {
		msg ("CANNOT FIND DATA");
		msg ("PACKET: PLEASE MAKE");
		msg ("SURE THE FILE CALLED");
		sprintf (textbuffer, "%cINBOX.BIN%c IS PRESENT", 34, 34);
		msg (textbuffer);
		msg ("IN THE DATA FOLDER.");
		_close (fh);
		_close (gh);
		return 0;
	}

	_read (ph, &object_id, 8);
	if (memcmp (&object_id, "STARMAP_", 8)) {
		msg ("ERROR ACCESSING INBOX:");
		msg ("INBOX PACKET FILE IS");
		msg ("INVALID OR CORRUPTED.");
		_close (fh);
		_close (gh);
		_close (ph);
		return 0;
	}

	fh2 = _creat (tmap, 0);
	if (fh2 == -1) {
		msg ("CAN'T CREATE SWAP FILE");
		sprintf (textbuffer, "%cSM.TMP%c: DISK ACCESS", 34, 34);
		msg (textbuffer);
		msg ("DENIED OR DISK FULL.");
		_close (fh);
		_close (gh);
		_close (ph);
		return 0;
	}

	gh2 = _creat (tguide, 0);
	if (gh2 == -1) {
		msg ("CAN'T CREATE SWAP FILE");
		sprintf (textbuffer, "%cGG.TMP%c: DISK ACCESS", 34, 34);
		msg (textbuffer);
		msg ("DENIED OR DISK FULL.");
		_close (fh);
		_close (gh);
		_close (ph);
		_close (fh2);
		remove (tmap);
		return 0;
	}

	////////////////////////////////////////////////////////////////////

	lseek (fh, 0, SEEK_SET);
	_read (fh, &starmap_size, 4);
	if (starmap_size > filelength (fh) || starmap_size < 0) {
		msg ("STARMAP FILE CORRUPT!");
		msg ("PLEASE REDOWNLOAD.");
		_close (fh);
		_close (gh);
		_close (ph);
		_close (fh2);
		_close (gh2);
		remove (tmap);
		remove (tguide);
		return 0;
	}
	lseek (fh, starmap_size, SEEK_SET);
	while (_read (fh, &object_id, 8) && _read (fh, object_label, 24) == 24) {
		if (memcmp (&object_id, "Removed:", 8)) {
			_write (fh2, &object_id, 8);
			round = _write (fh2, object_label, 24);
			if (round != 24) {
				msg ("ERROR: DISK FULL?");
				_close (fh);
				_close (gh);
				_close (ph);
				_close (fh2);
				_close (gh2);
				remove (tmap);
				remove (tguide);
				return 0;
			}
		}
	}
	sm_in = 0;
	chsize (fh, starmap_size);
	while (_read (ph, &object_id, 8) == 8) {
		if (!memcmp (&object_id, "GUIDE___", 8)) goto endmap;
		if (_read (ph, object_label, 24) != 24) goto smerror;
		lseek (fh, 4, SEEK_SET);
		while (_read (fh, &s_object_id, 8) && _read (fh, s_object_label, 24) == 24) {
			if (s_object_id >= object_id - idscale && s_object_id <= object_id + idscale)
				goto skiplabel;
		}
		lseek (fh2, 0, SEEK_SET);
		while (_read (fh2, &s_object_id, 8) && _read (fh2, s_object_label, 24) == 24) {
			if (s_object_id >= object_id - idscale && s_object_id <= object_id + idscale) {
				lseek (fh2, -32L, SEEK_CUR);
				_write (fh2, "Removed:", 8);
				lseek (fh2, +24L, SEEK_CUR);
			}
		}
		lseek (fh, 0, SEEK_END);
		_write (fh, &object_id, 8);
		round = _write (fh, object_label, 24);
		if (round != 24) {
			msg ("ERROR: DISK FULL?");
			_close (fh);
			_close (gh);
			_close (ph);
			_close (fh2);
			_close (gh2);
			remove (tmap);
			remove (tguide);
			return 0;
		}
		sm_in++;
	    skiplabel:;
	}
    smerror:
	msg ("ERROR ACCESSING INBOX");
	msg ("INBOX PACKET FILE IS");
	msg ("INVALID OR CORRUPTED.");
	msg (divider);
	msg ("STARMAP INVALIDATED:");
	msg ("PLEASE RE-DOWNLOAD");
	msg ("WHOLE NOCTIS PACKAGE");
	msg ("AND RE-UPDATE IN FULL");
	msg (divider);
	msg ("VERY SORRY.");
	_close (fh);
	_close (gh);
	_close (ph);
	_close (fh2);
	_close (gh2);
	remove (tmap);
	remove (tguide);
	return 0;
    endmap:
	starmap_size = filelength (fh);
	lseek (fh, 0, SEEK_SET);
	_write (fh, &starmap_size, 4);
	lseek (fh, 0, SEEK_END);
	lseek (fh2, 0, SEEK_SET);
	while (_read (fh2, &object_id, 8) && _read (fh2, object_label, 24) == 24) {
		if (memcmp (&object_id, "Removed:", 8)) {
			_write (fh, &object_id, 8);
			round = _write (fh, object_label, 24);
			if (round != 24) {
				msg ("ERROR: DISK FULL?");
				_close (fh);
				_close (gh);
				_close (ph);
				_close (fh2);
				_close (gh2);
				remove (tmap);
				remove (tguide);
				return 0;
			}
		}
	}

	////////////////////////////////////////////////////////////////////

	lseek (gh, 0, SEEK_SET);
	_read (gh, &guide_size, 4);
	if (guide_size > filelength (gh) || guide_size < 0) {
		msg ("GUIDE FILE CORRUPT!");
		msg ("PLEASE REDOWNLOAD.");
		_close (fh);
		_close (gh);
		_close (ph);
		_close (fh2);
		_close (gh2);
		remove (tmap);
		remove (tguide);
		return 0;
	}
	lseek (gh, guide_size, SEEK_SET);
	while (_read (gh, &mblock_subject, 8) && _read (gh, mblock_message, 76) == 76) {
		if (memcmp (&mblock_subject, "Removed:", 8)) {
			_write (gh2, &mblock_subject, 8);
			round = _write (gh2, mblock_message, 76);
			if (round != 76) {
				msg ("ERROR: DISK FULL?");
				_close (fh);
				_close (gh);
				_close (ph);
				_close (fh2);
				_close (gh2);
				remove (tmap);
				remove (tguide);
				return 0;
			}
		}
	}
	gg_in = 0;
	chsize (gh, guide_size);
	while (_read (ph, &mblock_subject, 8) == 8 && _read (ph, mblock_message, 76) == 76) {
		lseek (gh, 4, SEEK_SET);
		while (_read (gh, &s_mblock_subject, 8) && _read (gh, s_mblock_message, 76) == 76) {
			if (s_mblock_subject >= mblock_subject - idscale && s_mblock_subject <= mblock_subject + idscale) {
			if (!strcmp (s_mblock_message, mblock_message)) {
				goto skiprecord;
			}}
		}
		lseek (gh2, 0, SEEK_SET);
		while (_read (gh2, &s_mblock_subject, 8) && _read (gh2, s_mblock_message, 76) == 76) {
			if (s_mblock_subject >= mblock_subject - idscale && s_mblock_subject <= mblock_subject + idscale) {
			if (!strcmp (s_mblock_message, mblock_message)) {
				lseek (gh2, -84L, SEEK_CUR);
				_write (gh2, "Removed:", 8);
				lseek (gh2, +76L, SEEK_CUR);
			}}
		}
		lseek (gh, 0, SEEK_END);
		_write (gh, &mblock_subject, 8);
		round = _write (gh, mblock_message, 76);
		if (round != 76) {
			msg ("ERROR: DISK FULL?");
			_close (fh);
			_close (gh);
			_close (ph);
			_close (fh2);
			_close (gh2);
			remove (tmap);
			remove (tguide);
			return 0;
		}
		gg_in++;
	    skiprecord:;
	}
	guide_size = filelength (gh);
	lseek (gh, 0, SEEK_SET);
	_write (gh, &guide_size, 4);
	lseek (gh, 0, SEEK_END);
	lseek (gh2, 0, SEEK_SET);
	while (_read (gh2, &mblock_subject, 8) && _read (gh2, mblock_message, 76) == 76) {
		if (memcmp (&mblock_subject, "Removed:", 8)) {
			_write (gh, &mblock_subject, 8);
			round = _write (gh, mblock_message, 76);
			if (round != 76) {
				msg ("ERROR: DISK FULL?");
				_close (fh);
				_close (gh);
				_close (ph);
				_close (fh2);
				_close (gh2);
				remove (tmap);
				remove (tguide);
				return 0;
			}
		}
	}

	////////////////////////////////////////////////////////////////////

	_close (fh);
	_close (gh);
	_close (ph);
	_close (fh2);
	_close (gh2);

	remove (tmap);
	remove (tguide);

	msg ("IMPORTED LABELS:");
	if (sm_in == 0)
		strcpy (textbuffer, "   NONE");
	else
		sprintf (textbuffer, "   %d", sm_in);
	msg (textbuffer);
	msg ("IMPORTED COMMENTS:");
	if (gg_in == 0)
		strcpy (textbuffer, "   NONE");
	else
		sprintf (textbuffer, "   %d", gg_in);
	msg (textbuffer);
	return 0;
}

} // namespace goes_inbox
