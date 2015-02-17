/*
 * This program executes the command specified as its argument,
 * but attaches stderr to stdout first, to allow stderr to be
 * redirected to a file. This is intended for DOS only; under
 * UNIX it is easy to do this with the shell anyway.
 *
 * (c) 1993 by Graham Wheeler
 * gram@aztec.co.za
 */

#include <io.h>
#include <dos.h>
#include <string.h>
#include <process.h>

char cmdline[256];

main(int argc, char *argv[] )
{
	int i;
	if (argc>=2)
	{
		close(2);
		(void)dup(1);
		strupr(argv[1]);
		i=1;
		cmdline[0]='\0';
		while (i<argc)
		{
			strcat(cmdline,argv[i++]);
			strcat(cmdline," ");
		}
		return system(cmdline);
	}
	return 0;
}
