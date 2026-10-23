/*
	GOES Net module CLEAN (Linux port: built into the game).
	Original source by Alessandro Ghignola; NICE/NIV+ changes by their
	authors. The stand-alone DOS program becomes goes_clean::module_main.
*/

#include "goes_common.h"

namespace goes_clean {

/*

	Noctis database / CLEAN command.
	GOES Net Module.

*/

char msgbuffer[40];
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
double	mblock_subject;
char	mblock_message[77];

Dword	starmap_consolidated;
Dword	cleaned_starmap_consolidated;
Dword	guide_consolidated;
Dword	cleaned_guide_consolidated;

Dword	round, sm_pos, gg_pos;
Word 	fh, gh, fh2, gh2;

char	*starmap = "..\\DATA\\STARMAP.BIN";
char	*starmap_swp = "..\\DATA\\STARMAP.SWP";
char	*guide = "..\\DATA\\GUIDE.BIN";
char	*guide_swp = "..\\DATA\\GUIDE.SWP";

Dword	tmessages;
Dword	rmessages;

char	outbuffer[40];

int module_main (Word _argc, char **_argv)
{
	msg ("CLEANING STARMAP...");
	msg (divider);

	fh = _open (starmap, 0);
	if (fh == -1) {
		msg ("STARMAP NOT AVAILABLE");
		return 0;
	}

	fh2 = _creat (starmap_swp, 0);
	if (fh2 == -1) {
		msg ("CAN'T SETUP SWAP FILE");
		_close (fh);
		return 0;
	}

	_read (fh, &starmap_consolidated, 4);
	_write (fh2, &starmap_consolidated, 4);
	cleaned_starmap_consolidated = starmap_consolidated;

	if (starmap_consolidated > filelength (fh)) {
		msg ("STARMAP FILE CORRUPT:");
		msg ("PLEASE RE-DOWNLOAD");
		msg ("WHOLE NOCTIS PACKAGE");
		msg ("AND RE-UPDATE IN FULL");
		msg (divider);
		msg ("VERY SORRY.");
		_close (fh);
		_close (fh2);
		remove (starmap_swp);
		return 0;
	}

	tmessages = 0;
	rmessages = 0;

	while (_read (fh, &object_id, 8) && _read (fh, object_label, 24) == 24) {
		tmessages++;
		if (memcmp(&object_id, "Removed:", 8)) {
			_write (fh2, &object_id, 8);
			round = _write (fh2, object_label, 24);
			if (round != 24) {
				msg ("ERROR: DISK FULL?");
				_close (fh);
				_close (fh2);
				remove (starmap_swp);
				return 0;
			}
		}
		else {
			rmessages++;
			sm_pos = tell (fh); sm_pos -= 32;
			if (sm_pos < starmap_consolidated)
				cleaned_starmap_consolidated -= 32;
		}
	}

	lseek (fh2, 0, SEEK_SET);
	_write (fh2, &cleaned_starmap_consolidated, 4);

	_close (fh);
	_close (fh2);

	remove (starmap);
	rename (starmap_swp, starmap);
	sprintf (outbuffer, "RECORDS: %d", tmessages);
	msg (outbuffer);
	sprintf (outbuffer, "REMOVED: %d", rmessages);
	msg (outbuffer);

	////////////////////////////////////////////////////////////////////

	msg (divider);
	msg ("CLEANING GUIDE...");
	msg (divider);

	gh = _open (guide, 0);
	if (gh == -1) {
		msg ("GUIDE NOT AVAILABLE");
		return 0;
	}

	gh2 = _creat (guide_swp, 0);
	if (gh2 == -1) {
		msg ("CAN'T SETUP SWAP FILE");
		_close (gh);
		return 0;
	}

	_read (gh, &guide_consolidated, 4);
	_write (gh2, &guide_consolidated, 4);
	cleaned_guide_consolidated = guide_consolidated;

	if (guide_consolidated > filelength (gh)) {
		msg ("GUIDE FILE CORRUPT:");
		msg ("PLEASE RE-DOWNLOAD");
		msg ("WHOLE NOCTIS PACKAGE");
		msg ("AND RE-UPDATE IN FULL");
		msg (divider);
		msg ("VERY SORRY.");
		_close (gh);
		_close (gh2);
		remove (guide_swp);
		return 0;
	}

	tmessages = 0;
	rmessages = 0;

	while (_read (gh, &mblock_subject, 8) && _read (gh, mblock_message, 76) == 76) {
		tmessages++;
		if (memcmp(&mblock_subject, "Removed:", 8)) {
			_write (gh2, &mblock_subject, 8);
			round = _write (gh2, mblock_message, 76);
			if (round != 76) {
				msg ("ERROR: DISK FULL?");
				_close (gh);
				_close (gh2);
				remove (guide_swp);
				return 0;
			}
		}
		else {
			rmessages++;
			gg_pos = tell (gh); gg_pos -= 84;
			if (gg_pos < guide_consolidated)
				cleaned_guide_consolidated -= 84;
		}
	}

	lseek (gh2, 0, SEEK_SET);
	_write (gh2, &cleaned_guide_consolidated, 4);

	_close (gh);
	_close (gh2);

	remove (guide);
	rename (guide_swp, guide);
	sprintf (outbuffer, "RECORDS: %d", tmessages);
	msg (outbuffer);
	sprintf (outbuffer, "REMOVED: %d", rmessages);
	msg (outbuffer);

	////////////////////////////////////////////////////////////////////

	msg (divider);
	msg ("END");
	return 0;
}

} // namespace goes_clean
