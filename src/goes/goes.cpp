/*
 * Noctis IV Plus - Linux port.
 * GOES Net command dispatcher and the small modules whose DOS sources
 * were not distributed (HELP, REPAIR, WRI, TXT), rewritten from their
 * documented behaviour.
 */

#include "goes_common.h"
#include "goes.h"

#undef printf

FILE *goes_out = nullptr;
static FILE *goes_prn = nullptr;

FILE *goes_printer (void)
{
	if (!goes_prn) goes_prn = dos_fopen ("..\\GALLERY\\PRINTOUT.TXT", "ab");
	return goes_prn ? goes_prn : stderr;
}

int goes_printf (const char *fmt, ...)
{
	if (!goes_out) return 0;
	va_list ap;
	va_start (ap, fmt);
	int n = vfprintf (goes_out, fmt, ap);
	va_end (ap);
	return n;
}

#define printf goes_printf

#define DECLARE_MODULE(n) namespace goes_##n { int module_main (Word _argc, char **_argv); }
DECLARE_MODULE (cast)
DECLARE_MODULE (cat)
DECLARE_MODULE (clean)
DECLARE_MODULE (dele)
DECLARE_MODULE (dl)
DECLARE_MODULE (inbox)
DECLARE_MODULE (outbox)
DECLARE_MODULE (par)
DECLARE_MODULE (pri)
DECLARE_MODULE (rep)
DECLARE_MODULE (sl)
DECLARE_MODULE (st)
DECLARE_MODULE (where)

/* Prints one 21-column line of the GOES output screen. */
static void line21 (const char *s)
{
	char buf[22];
	snprintf (buf, sizeof (buf), "%s", s);
	printf ("%-21s", buf);
}

/* Text longer than a line is wrapped every 21 characters. */
static void wrap21 (const char *s)
{
	size_t n = strlen (s);
	if (!n) {
		line21 ("");
		return;
	}
	for (size_t i = 0; i < n; i += 21) {
		char buf[22];
		size_t k = n - i < 21 ? n - i : 21;
		memcpy (buf, s + i, k);
		buf[k] = 0;
		line21 (buf);
	}
}

static void join_args (Word argc, char **argv, char *out, size_t outlen)
{
	out[0] = 0;
	for (Word i = 1; i < argc; i++) {
		if (i > 1) strncat (out, " ", outlen - strlen (out) - 1);
		strncat (out, argv[i], outlen - strlen (out) - 1);
	}
}

/* HELP: list of the available modules (the NICE "N_Help" module). */
static int help_main (Word, char **)
{
	line21 ("ST   SL   DL    PAR  ");
	line21 ("WHERE     CLEAN      ");
	line21 ("CAST CAT  REP   DELE ");
	line21 ("PRI       REPAIR     ");
	line21 ("INBOX     OUTBOX     ");
	line21 ("WRI       TXT        ");
	line21 ("CLR       HELP       ");
	return 0;
}

/* WRI: appends a line of text to DATA\NOTES.TXT. */
static int wri_main (Word argc, char **argv)
{
	if (argc < 2) {
		line21 ("__WRI MODULE USAGE:__");
		line21 ("CALL WRI FOLLOWED BY ");
		line21 ("THE TEXT YOU WANT TO ");
		line21 ("WRITE TO THE NOTES FI");
		line21 ("LE THAT YOU CAN LATER");
		line21 ("FIND IN THE DATA FOLD");
		line21 ("ER.                  ");
		return 0;
	}
	char text[256];
	join_args (argc, argv, text, sizeof (text));
	FILE *f = dos_fopen ("..\\data\\notes.txt", "ab");
	if (!f) {
		line21 ("CANNOT WRITE NOTES.");
		return 0;
	}
	fprintf (f, "%s\r\n", text);
	fclose (f);
	line21 ("______WRITTEN:_______");
	wrap21 (text);
	return 0;
}

/* TXT: shows a text file on the GOES screen. As on DOS the path is
   relative to the MODULES directory and uses '/' instead of '\'. */
static int txt_main (Word argc, char **argv)
{
	if (argc < 2) {
		line21 ("________USAGE________");
		line21 ("TXT FILENAME");
		wrap21 ("WHERE FILENAME IS A LOCATION TO A FILE, USING A SLASH INSTEAD OF A BACKSLASH.");
		wrap21 ("EXAMPLE: TXT ../DATA/NOTES.TXT");
		line21 ("^^^^^^^^^^^^^^^^^^^^^");
		line21 ("PLEASE RUN AGAIN,");
		line21 ("SPECIFYING PARAMETERS");
		return 0;
	}
	char name[256];
	join_args (argc, argv, name, sizeof (name));
	for (char *p = name; *p; p++)
		if (*p == '*') *p = '~';
	FILE *f = dos_fopen (name, "rb");
	if (!f) {
		line21 ("FILE NOT FOUND.");
		return 0;
	}
	char line[1024];
	while (fgets (line, sizeof (line), f)) {
		size_t n = strcspn (line, "\r\n");
		line[n] = 0;
		for (char *p = line; *p; p++) {
			if (*p == '\t') *p = ' ';
			if (*p >= 'a' && *p <= 'z') *p -= 32;
			if ((unsigned char)*p < 32 || (unsigned char)*p > 96) *p = ' ';
		}
		wrap21 (line);
	}
	fclose (f);
	return 0;
}

/* REPAIR: removes duplicated labels and comments from the archive by
   marking the later copies as "Removed:", like the DOS module. */
static int repair_main (Word, char **)
{
	line21 (" GOES REPAIR UTILITY ");
	line21 ("&&&&&&&&&&&&&&&&&&&&&");
	line21 ("THIS PROGRAM SEARCHES");
	line21 ("FOR DUPLICATED LABELS");
	line21 ("AND COMMENTS,REMOVING");
	line21 ("THEM FROM THE ARCHIVE");
	line21 ("&&&&&&&&&&&&&&&&&&&&&");
	line21 ("VERIFYING STARMAP...");

	FILE *f = dos_fopen ("..\\DATA\\STARMAP.BIN", "r+b");
	if (!f) {
		line21 ("STARMAP NOT AVAILABLE");
		return 0;
	}
	fseek (f, 0, SEEK_END);
	long len = ftell (f);
	if (len < 4 || (len - 4) % 32) {
		fclose (f);
		line21 ("STARMAP FILE CORRUPT:");
		line21 ("CAN'T REPAIR; PLEASE");
		line21 ("RE-DOWNLOAD THE WHOLE");
		line21 ("NOCTIS PACKAGE, THEN");
		line21 ("PERFORM A FULL UPDATE");
		line21 ("VERY SORRY.");
		return 0;
	}
	line21 ("(PROCESSING STARMAP)");
	long count = (len - 4) / 32;
	unsigned char *recs = (unsigned char *)malloc (count * 32 + 1);
	fseek (f, 4, SEEK_SET);
	if (fread (recs, 32, count, f) != (size_t)count) count = 0;
	long errors = 0;
	for (long i = 0; i < count; i++) {
		unsigned char *a = recs + i * 32;
		if (!memcmp (a, "Removed:", 8)) continue;
		for (long j = 0; j < i; j++) {
			unsigned char *b = recs + j * 32;
			if (memcmp (b, "Removed:", 8)) {
				if (!memcmp (a, b, 32)) {
					memcpy (a, "Removed:", 8);
					fseek (f, 4 + i * 32, SEEK_SET);
					fwrite (a, 8, 1, f);
					errors++;
					break;
				}
			}
		}
	}
	free (recs);
	fclose (f);

	f = dos_fopen ("..\\DATA\\GUIDE.BIN", "r+b");
	if (f) {
		fseek (f, 0, SEEK_END);
		len = ftell (f);
		count = len >= 4 ? (len - 4) / 84 : 0;
		recs = (unsigned char *)malloc (count * 84 + 1);
		fseek (f, 4, SEEK_SET);
		if (fread (recs, 84, count, f) != (size_t)count) count = 0;
		for (long i = 0; i < count; i++) {
			unsigned char *a = recs + i * 84;
			if (!memcmp (a, "Removed:", 8)) continue;
			for (long j = 0; j < i; j++) {
				unsigned char *b = recs + j * 84;
				if (memcmp (b, "Removed:", 8) && !memcmp (a, b, 84)) {
					memcpy (a, "Removed:", 8);
					fseek (f, 4 + i * 84, SEEK_SET);
					fwrite (a, 8, 1, f);
					errors++;
					break;
				}
			}
		}
		free (recs);
		fclose (f);
	}

	line21 ("&&&&&&&&&&&&&&&&&&&&&");
	if (!errors) {
		line21 ("ERRORS FOUND:");
		line21 ("   NONE");
		line21 ("NO ERRORS IN STARMAP.");
	}
	else {
		char buf[32];
		line21 ("ERRORS FOUND:");
		snprintf (buf, sizeof (buf), "   %d", (int)errors);
		line21 (buf);
		line21 ("OK, STARMAP REPAIRED.");
	}
	return 0;
}

struct Module {
	const char *name;
	int (*entry) (Word, char **);
};

static const Module modules[] = {
	{ "CAST", goes_cast::module_main },
	{ "CAT", goes_cat::module_main },
	{ "CLEAN", goes_clean::module_main },
	{ "DELE", goes_dele::module_main },
	{ "DL", goes_dl::module_main },
	{ "INBOX", goes_inbox::module_main },
	{ "OUTBOX", goes_outbox::module_main },
	{ "PAR", goes_par::module_main },
	{ "PRI", goes_pri::module_main },
	{ "REP", goes_rep::module_main },
	{ "SL", goes_sl::module_main },
	{ "ST", goes_st::module_main },
	{ "WHERE", goes_where::module_main },
	{ "HELP", help_main },
	{ "WRI", wri_main },
	{ "TXT", txt_main },
	{ "REPAIR", repair_main },
};

int goes_execute (const char *command_line, const char *outfile)
{
	// Split the command line like the DOS C runtime did.
	static char buf[256];
	snprintf (buf, sizeof (buf), "%s", command_line);
	char *argv[64];
	Word argc = 0;
	char *p = buf;
	while (*p && argc < 63) {
		while (*p == ' ' || *p == '\t') p++;
		if (!*p) break;
		argv[argc++] = p;
		while (*p && *p != ' ' && *p != '\t') p++;
		if (*p) *p++ = 0;
	}
	argv[argc] = nullptr;
	if (!argc) return 0;

	// Module name: strip any path and the ".EXE"/".COM" extension.
	char name[64];
	const char *base = argv[0];
	for (const char *q = argv[0]; *q; q++)
		if (*q == '\\' || *q == '/' || *q == ':') base = q + 1;
	snprintf (name, sizeof (name), "%s", base);
	char *dot = strchr (name, '.');
	if (dot) *dot = 0;
	for (char *q = name; *q; q++)
		if (*q >= 'a' && *q <= 'z') *q -= 32;

	for (const Module &m : modules) {
		if (strcmp (m.name, name)) continue;
		goes_out = dos_fopen (outfile, "wb");
		if (!goes_out) return 1;
		m.entry (argc, argv);
		fclose (goes_out);
		goes_out = nullptr;
		if (goes_prn) {
			fclose (goes_prn);
			goes_prn = nullptr;
		}
		return 1;
	}
	return 0;
}
