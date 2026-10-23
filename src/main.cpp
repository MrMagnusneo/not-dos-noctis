/*
 * Noctis IV Plus - Linux port.
 * Program entry point.
 */

#include "dos/dos.h"
#include "platform/platform.h"

#include <cstdio>

extern unsigned char *adaptor;
extern int noctis_main ();

int main (int argc, char **argv)
{
	PlatConfig cfg;
	plat_default_config (&cfg);
	if (!plat_parse_args (&cfg, argc, argv)) return 0;

	dos_init_memory ();
	if (!dos_setup_paths (argv[0])) return 1;

	if (!plat_init (&cfg)) return 1;
	adaptor = dos_vga_memory;
	plat_set_vram (adaptor);

	int rc = noctis_main ();

	plat_shutdown ();
	return rc;
}
