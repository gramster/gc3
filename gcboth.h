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
 * v3.3pl4 June '94	   Fixed a bug in moving the cursor thru file list
 *			     while in command line editor. Accredited
 *			     early beta testers in help file.
 * v3.3pl5 Late '94	   Various minor fixes and improvements. Better
 *			     support for Linux and ncurses.
 * v3.3pl6 Feb '95	   Fixed bug in display of month in dates.
 *			     Fixed script bug involving $show vars.
 */

/*
 * Stuff common to the compiler and interpreter
 */

#ifndef _GCBOTH_H
#define _GCBOTH_H

#define VERSION		"3.3"
#define PATCHLEVEL	"6"
#define VERCODE		0x3306	/* 16-bit version code */

#ifdef __MSDOS__
#include "doscfg.h"
#else
#include "unixcfg.h"
#endif

/* Sort keys */

#define S_NAME		0
#define S_SIZE		1
#define S_MTIME		2
#define S_ATIME		3
#define S_OWNER		4
#define S_GROUP		5
#define S_EXTENS	6


#define MAX_VARS	200	/* Maximum number of global variables	*/
#define MAX_IDS		300	/* Maximum number of globvars & funcs	*/
#define MAX_OPTS	20	/* Maximum number of options		*/
#define MAX_LOOPS	8	/* max nested loops per function	*/
#define MAX_PARAMS	8	/* max parameters per function		*/
#define MAX_LOCVARS	20	/* max local vars per function		*/

/*
 * Lookup macros for strings, etc
 */

#define STRING(o)	(stringspace-(o))
#define REGEXP(r)	(stringspace-(r))
#define LITERAL(l)	(stringspace+(l))
#define ID_NAME(n)	(stringspace+namemap[n])

/*
 * grok the type of an argument
 */

#define IS_LITERAL(v)	((v)<0)
#define IS_GLOBAL(v)	((v)>=0 && (v)<MAX_IDS)
#define IS_PARAM(v)	((v)>=MAX_IDS && (v)<(MAX_IDS+MAX_PARAMS))
#define IS_LOCAL(v)	((v)>=(MAX_IDS+MAX_PARAMS) && (v)<(MAX_IDS+MAX_PARAMS+MAX_LOCVARS))
#define IS_LOCPAR(v)	((v)>=(MAX_IDS+MAX_PARAMS+MAX_LOCVARS))
#define IS_GLOBVAR(v)	(IS_GLOBAL(v) && (objects[v].type & VAR_TYPE))
#define IS_VAR(v)	(IS_GLOBVAR(v) || ((v)>=MAX_IDS))
#define IS_FUNC(v)	(IS_GLOBAL(v) && (objects[v].type & FUNC_TYPE))
#define IS_LVAL(v)	(v>LAST_RDONLY_VAR && (IS_GLOBVAR(v) || IS_LOCAL(v)))

/*
 * Convert an index/offset to an argument
 */

#define TO_LITERAL(s)	(-(s))
#define TO_GLOBAL(v)	(v)
#define TO_PARAM(p)	((p)+MAX_IDS)
#define TO_LOCAL(l)	((l)+MAX_IDS+MAX_PARAMS)
#define TO_LOCPAR(l)	((l)+MAX_PARAMS)

/*
 * And vice-versa
 */

#define FROM_LITERAL(s)	(-(s))
#define FROM_GLOBAL(v)	(v)
#define VARNUM(v)	(objects[v].p)
#define FROM_PARAM(p)	((p)-(MAX_IDS))
#define FROM_LOCAL(l)	((l)-(MAX_IDS+MAX_PARAMS))
#define DEREF_LOCPAR(l)	((l)-MAX_LOCVARS)

/*
 * File index at highlight
 */

#define FILE_NOW(w)	(fIndex[w][highlight[w]]) /* idx of file at cursor */
#define INFO_NOW(w)	fInfo[w][FILE_NOW(w)]	/* info for above file */
#define INFO(w, i)	fInfo[w][fIndex[w][i]] /* info for n'th file in window */

/*
 * predefined read-only global variables
 */

#define VAR_DUMMY		0
#define VAR_PFILE		1
#define VAR_SFILE		2
#define VAR_PFILES	3
#define VAR_SFILES	4
#define VAR_PPATH		5
#define VAR_SPATH		6
#define VAR_PSEL		7
#define VAR_SSEL		8
#define VAR_PSELSIZE	9
#define VAR_SSELSIZE	10
#define VAR_PSELCNT	11
#define VAR_SSELCNT	12
#define VAR_PSIZE		13
#define VAR_SSIZE		14
#define VAR_PMTIME	15
#define VAR_SMTIME	16
#define VAR_PATIME	17
#define VAR_SATIME	18
#define VAR_POWNER	19
#define VAR_SOWNER	20
#define VAR_PGROUP	21
#define VAR_SGROUP	22
#define VAR_PPERMS	23
#define VAR_SPERMS	24
#define VAR_USERNAME	25
#define VAR_TEMPNAME	26
#define VAR_TEMPFILE	27
#define VAR_TEMPPATH	28
#define VAR_DATE		29
#define VAR_TIME		30
#define VAR_RANDOM	31
#define VAR_PREVCMD	32
#define VAR_NEXTCMD	33
#define VAR_DISKFREE	34
#define VAR_DISKUSED	35
#define VAR_VERSION	36
#define VAR_PATCHLEVEL	37
#define VAR_CODEPATH	38
#define VAR_VIEWLINE	39
#define VAR_VIEWFILE	40
#define VAR_WINDOW	41

#define LAST_RDONLY_VAR		41

/* R/W predefined option variables */

#define VAR_DIRSELECT			42
#define VAR_DIRSFIRST			43
#define VAR_SHOWHIDDEN		44
#define VAR_FOLLOWLINKS		45
#define VAR_USEDOSPATTERNS	46
#define VAR_VIEWER			47
#define VAR_DEBUGGING			48
#define VAR_EXPANDING			49
#define VAR_NUMBERLNS			50
#define VAR_DISPLAYON			51
#define VAR_TREE				52
#define VAR_CONFIRM			53
#define VAR_WAIT			54

/* R/W predefined variables */

#define VAR_LINEBUFF			55
#define VAR_MESSAGE			56
#define VAR_CH				57
#define VAR_WIDTH			58
#define VAR_HEIGHT			59
#define VAR_LEFTMARGIN			60
#define VAR_TOPMARGIN			61
#define VAR_SHOWVIEW			62
#define VAR_SHOWSTATUS			63
#define VAR_SHELL			64
#define VAR_HELPFILE			65
#define VAR_HELPENTRY			66
#define VAR_ISCOLOR			67

#define LASTPREDEFVAR		67

#define FUNC_TYPE	((short)1)
#define VAR_TYPE	((short)2)
#define REFERENCED	((short)4)

typedef struct
{
    short p;			/* Pointer into code space, or var space */
    short np;			/* Number of parameters, if a function	*/
    short type;			/* Type flags */
    short partyp;		/* Parameter type bit vector - which ones *must* be vars */
} objectrecord;

extern short idents, varcnt, codep, stringp;
extern int lineno;
extern short namemap[];
extern objectrecord objects[];
extern char *stringspace, codepath[];
extern short *codespace;
extern short initialiseEntryPoint;
extern short startupEntryPoint;
extern short defaultEntryPoint;
extern short afterKeyEntryPoint;
extern short linked;

/*
 * Prototypes from gclib.c
 */

#if __STDC__
extern void STRUPR(char *s);
extern char *searchPath(char *name);
#else
extern void STRUPR();
extern char *searchPath();
#endif

/*
 * Prototypes from gcutil.c
 */

#if __STDC__
/*extern int	getBuffer(int who);
extern void	releaseBuffer(int buf, int who);*/
extern int ReadCode(void);
/*extern int	readFile(char *name, char *aname,int (*f)(char *));*/
extern void writecodefile(void);
extern void exitCurses(int delWins);
extern FILE *tryOpen(char *path, char *file, char *mode, int showmsg);
#else
/*extern int	getBuffer();
extern void	releaseBuffer();*/
extern int ReadCode();
extern int readFile();
extern void writecodefile();
extern void exitCurses();
extern FILE *tryOpen();
#endif

#endif
