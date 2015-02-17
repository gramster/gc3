/*
 * gc3 - Gram's Commander v3.3: A configureable file manager
 *
 * YAGH! (tm) (Yet Another Gramish Hack)
 *
 * (c) 1994 by Graham Wheeler.           All Rights Reserved.
 *             gram@aztec.co.za
 *
 * DISCLAIMER: The author is not responsible for any loss, damage,
 *        riots, earthquakes, catastrophes, marriages, annulments,
 *        etc that arise from the use of this program. Sorry...
 *
 * This software may be freely copied and distributed, provided it is
 * distributed with all files intact and unmodified. You may use this
 * software for a 30 day trial period, after which you should register
 * your copy with me. I have spent a large number of weekends and nights
 * trying to make a file manager for UNIX (and DOS) which combines ease
 * of use and power, and complements the power of UNIX to process the
 * contents of files with the power to *select* the files to be processed.
 * By registering, you will encourage me to keep improving gc3, instead
 * of seeking paying work which benefits only a few.
 *
 * Full details of how to register, and what you will receive if
 * you do, are contained in the file LEGAL.DOC. Of course, if you
 * don't register, I can't do much about it, but in that case
 * may an obscure bug in gc3 erase all of your files!
 * 
 * Enjoy using GC!
 *
 * See INSTALL.DOC for instructions on how to install GC3
 *
 * Version History
 * ---------------
 * v1.0   Nov 1992         Initial release
 * v2.0   Nov 1992         Largely rewritten to be table-driven
 * v2.1   Dec 1992         Support for non-ANSI C compilers contributed
 *                           by George Sipe
 *                           Support for old ioctls TCGETA and TCSETAW for
 *                           those with no POSIX-style tcgetattr and tcsetattr.
 * v2.2   Dec 1992         Termio ioctls removed.Hopefully much more
 *                           portable. Command mode removed. A few bugs
 *                           fixed. Placeholder replacement now done in
 *                           a preprocessing phase before parsing commands.
 * v2.3   Jan 1993         Split into a number of source files
 *                           Basic port to MS-DOS
 * v3.0   Feb/March '93    New script language; large parts rewritten
 * v3.1   March '93        Ported back to UNIX and cleaned up
 * v3.2b  July  '93        Many bugs fixed and features added
 * v3.2   Oct   '93        Viewer added, .gc3rul file handling
 *                           added, bugs fixed, local variables and
 *                           parameters, tree view and container handling.
 * v3.3   April '94        Fixed lots of bugs. Large parts of compiler
 *                           and interpreter are now table-driven. Changed
 *                           screen repainting, and added support for colour
 *                           and mono terminals. Added script-driven menus
 *                           and forms, and hypertext help browser.
 *                           Made windows moveable and resizeable.
 *                           Made command handling in the script much more
 *                           sophisticated with multiple prefix/suffixes.
 *                           Improved command line history facility. Added
 *                           repeat counts to most commands.
 */


#ifndef _GCPORT_H
#define _GCPORT_H

#include "gcsite.h"

/* A header file used to include most other system header files
	and add some typedefs if necessary. This is used as the first
	#include in each source file. It is inefficient, but keeps things
	simple (it is probably more efficient with compilers that support
	precompilation of header files).
*/

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAS_UNISTD
#include <unistd.h> /* POSIX Unix stuff */
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef NO_STRING_H
#include <strings.h>
#else
#include <string.h>
#endif
#include <time.h>
#include <errno.h>
#include <assert.h>

#if __MSDOS__

#	include <io.h>	      
#	include <conio.h>
#	include <dos.h>
#	include <dir.h>
#	include <direct.h>
#	include <alloc.h>
#	include <stdarg.h>

   typedef unsigned short mode_t;
   typedef long	uid_t;
   typedef long	gid_t;
   typedef long	off_t;
   typedef long	time_t;

#  ifndef __STDC__
#  	define __STDC__	1
#  endif

#define NO_SYMLINKS	1
#define NO_SRAND48	1

#else

#	ifdef NO_DIRENT_H
#	  include <sys/dir.h>
#	else
#	  include <dirent.h>
#	endif
#	include <grp.h>
#	include <pwd.h>

#  ifdef NO_MODE_T
	 typedef unsigned short mode_t;
#  endif
#  ifdef NO_UID_T
    typedef long	uid_t;
#  endif
#  ifdef NO_GID_T
    typedef long	gid_t;
#  endif
#  ifdef NO_OFF_T
    typedef long	off_t;
#  endif
#  ifdef NO_TIME_T
    typedef long	time_t;
#  endif
#endif
#ifndef __STDC__
	extern char *strerror();
#endif
#ifndef isupper
#	define isupper(c)	((c)>='A' && (c)<='Z')
#endif
#ifndef islower
#	define islower(c)	((c)>='A' && (c)<='Z')
#endif
#ifndef toupper
#	define toupper(c)	( islower(c) ? ( (c)-'a'+'A') : (c) )
#endif
#ifndef tolower
#	define tolower(c)	( isupper(c) ? ( (c)-'A'+'a') : (c) )
#endif

#if !defined(USE_SYSV_REGEXP) && !defined(USE_BSD_REGEXP)
#define compileRE	DOScompileRE
#define matchRE		DOSmatchRE
#endif


#endif


