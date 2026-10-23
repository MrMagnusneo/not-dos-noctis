/*
	GOES Net module DELE (Linux port: built into the game).
	Original source by Alessandro Ghignola; NICE/NIV+ changes by their
	authors. The stand-alone DOS program becomes goes_dele::module_main.
*/

#include "goes_common.h"

namespace goes_dele {

/*

	Noctis galactic guide / DELE command.
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

double 	object_id = 12345;
char   	object_label[25];
double 	s_object_id = 12345;
char   	s_object_label[25];

double	subject_id = 12345;
double  idscale = 0.00001;

Dword	round;
Dword	guide_consolidated;

Word 	i, fh, gh;
char	*file = "..\\DATA\\STARMAP.BIN";
char	*guide = "..\\DATA\\GUIDE.BIN";

char	outbuffer[40];
char	textbuffer[40];
char	parbuffer[160];
char	nullbuffer[128];
char	objectname[21];
char	subjectname[21];

double	mblock_subject;
char	mblock_message[68];

char find (char *starname)
{
	Word p, n, ctc, found;
	ctc = strlen (starname);
	if (ctc > 20 || ctc <= 0) {
		msg ("INVALID OBJECT NAME.");
		return(0);
	}
	n = 0;
	found = 0;
	lseek (fh, 4, SEEK_SET);
	while (_read (fh, &s_object_id, 8) && _read (fh, &s_object_label, 24) == 24) {
		if (memcmp (&s_object_id, "Removed:", 8)) {
			if (!memcmp (s_object_label, starname, ctc)) {
				n++;
				memcpy (object_label, s_object_label, 24);
				object_id = s_object_id;
				memcpy (subjectname, object_label, 20);
				subject_id = object_id;
				if (object_label[21] == 'S') found = 1;
				if (object_label[21] == 'P') found = 2;
				p = 20;
				while (p >= 0) {
					if (s_object_label[p] != 32) {
						if (s_object_label[p] == starname[p])
							return (found);
						else
							break;
					}
					p--;
				}
			}
		}
	}
	if (!n)
		msg ("OBJECT NOT FOUND.");
	if (n > 1) {
		msg ("AMBIGUOUS SEARCH KEY:");
		msg ("PLEASE EXPAND NAME...");
		msg (divider);
		msg ("POSSIBLE RESULTS ARE:");
		msg (divider);
		lseek (fh, 4, SEEK_SET);
		while (_read (fh, &s_object_id, 8) && _read (fh, &s_object_label, 24) == 24) {
		if (memcmp (&s_object_id, "Removed:", 8) && !memcmp (s_object_label, starname, ctc)) {
			s_object_label[21] = 0;
			msg (s_object_label);
		}}
		msg (divider);
		found = 0;
	}
	return (found);
}

int module_main (Word _argc, char **_argv)
{
	char query;
	Word tmessages;
	Word rmessages;
	Word is, rec, rec_start, rec_end;

	if (_argc<2) {
		msg ("________USAGE________");
		msg ("DELE OBJECTNAME");
		msg ("DELE OBJECTNAME:X..Y");
		msg ("^^^^^^^^^^^^^^^^^^^^^");
		msg ("PLEASE RUN AGAIN,");
		msg ("SPECIFYING PARAMETERS");
		return 0;
	}
	else {
		msg (" GOES GALACTIC GUIDE ");
		msg (divider);
	}

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

	i = 0;
	while (i < 21 && parbuffer[i] != ':' && parbuffer[i] != 0) {
		objectname[i] = parbuffer[i];
		i++;
	}

	if (i == 21) {
		msg ("INVALID SUBJECT NAME.");
		_close (fh);
	}

	if (parbuffer[i] == ':') {
		is = i+1; i++;
		while (parbuffer[i] != '.' && parbuffer[i] != 0) i++;
		parbuffer[i] = 0; rec_start = atoi (parbuffer + is);
		if (parbuffer[i+1] == '.')
			rec_end = atoi (parbuffer + i + 2);
		else
			rec_end = 32767;
	}
	else {
		rec_start = 1;
		rec_end = 32767;
	}

	if (parbuffer[i] != ':') {
		strupr (parbuffer);
		strupr (objectname);
		objectname[i] = 0;
		query = find (objectname);
		if (query) {
			if (query==1) msg ("SUBJECT: STAR;");
			if (query==2) msg ("SUBJECT: PLANET;");
			sprintf (outbuffer, "NAME: %s", subjectname);
			msg (outbuffer);
			msg (divider);
			gh = _open (guide, 4);
			if (gh == -1) {
				msg ("DATABASE ERROR.");
				msg ("(ERROR CODE 1004)");
			}
			else {
				rec = 0;
				query = 0;
				tmessages = 0;
				rmessages = 0;
				_read (gh, &guide_consolidated, 4);
				while (goes_read84 (gh, &mblock_subject, mblock_message) == 84) {
					if (mblock_subject > subject_id - idscale && mblock_subject < subject_id + idscale) {
						tmessages++;
						rec++;
						if (rec >= rec_start && rec <= rec_end) {
						query = 1;
						round = tell(gh);
						round -= 84;
						if (round >= guide_consolidated) {
							rmessages++;
							lseek (gh, round, SEEK_SET);
							_write (gh, "Removed:", 8);
							round += 84;
							lseek (gh, round, SEEK_SET);
						}
						}
					}
				}
				if (!query) {
					msg ("THERE WERE NO RECORDS");
					msg ("THAT COULD BE DELETED");
				}
				else {
					sprintf (outbuffer, "TOTAL RECORDS: %d", tmessages);
					msg (outbuffer);
					sprintf (outbuffer, "REMOVED: %d", rmessages);
					msg (outbuffer);
					sprintf (outbuffer, "PROTECTED: %d", tmessages - rmessages);
					msg (outbuffer);
				}
				_close (gh);
			}
		}
	}

	_close (fh);
	return 0;
}

} // namespace goes_dele
