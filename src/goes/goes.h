/*
 * Noctis IV Plus - Linux port.
 * GOES Net: the modules run from the onboard computer console.
 *
 * On DOS every module was a separate executable started through
 * COMMAND.COM with its output redirected to DATA\GOESFILE.TXT. Some of
 * them also drew progress messages directly into the VGA memory, so in
 * this port they are built into the game and run in-process.
 */

#ifndef NOCTIS_GOES_H
#define NOCTIS_GOES_H

/* Runs a GOES command line (e.g. "CAT FELYSIA"), writing the module's
   output into 'outfile'. Returns 0 if no module with that name exists. */
int goes_execute (const char *command_line, const char *outfile);

#endif
