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


/*
 * Configureable DOS-specific stuff for GC v3.3
 */

#ifndef _DOSCFG_H
#define _DOSCFG_H

#define STRINGSPACE	10000	/* Space for string literals & ID names	*/
#define CODESPACE		8000

#define MAX_BUFFERS	16		/* Number of temporary string buffers	*/
#define BUFFER_SIZE	1024	/* Size of each temp string buffer	*/

#define MAXPATHNAME	64		/* Longest allowed path name length	*/
#define MAXFILENAME	13		/* Longest possible file name 		*/
#define MAXARGS		512		/* Maximum number of file names per window */

#define HIST_LEN		8		/* Number of commands kept in history	*/
#define NUMMACROS		2		/* Number of keystroke macros			*/
#define MACROLEN		256		/* Maximum # of keystrokes/macro		*/
#define MAXVIEWLINES	2000	/* Max number of lines for viewer	*/

#define ININAME	"gc3.ini"	/* Name of file in which path is saved	*/
#define INIANAME "\\gc3.ini"	/* Absolute path of above 		*/
#define CODNAME	"gc3.cod"	/* Name of compiled script file		*/
#define CODANAME "\\gc3.cod"	/* Absolute path of above		*/

/*
 * Don't change below here!
 */

#define PATH_SEP	'\\'
#define PATH_SEP_STR	"\\"
#define FIELD_SEP	';'
#define ETX		26
#define COLOURS	"BLACK", "BLUE", "GREEN", "CYAN", "RED", "MAGENTA", "BROWN", "WHITE"

enum colour_t { Black, Blue, Green, Cyan, Red, Mgenta, Brown, White };
#endif

