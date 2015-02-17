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


#ifndef _GCOPS_H
#define _GCOPS_H

enum codeops {
	OP_ACCESS, OP_ACTION, OP_APPEND, OP_ASSIGN,
	OP_BEEP, OP_BIND, OP_BOX, OP_BREAK, OP_BROWSE,
	OP_CAT, OP_CALL, OP_CD, OP_CLEAR, OP_COMPARE, OP_CONTAINS,
	OP_DOWN, OP_DUMP,
	OP_ECHO, OP_EDITTEXT, OP_END, OP_ENDLOOP, OP_ENDREC, OP_EVAL, OP_EXEC,
		OP_EXECCATCH, OP_EXITTEXT, OP_EXPAND,
	OP_FCOPY, OP_FILTER, OP_FINDAREA, OP_FUNCTION,
	OP_GOTO,
	OP_HBORDER, OP_HEAD, OP_HELP, OP_HELPALL, OP_HELPKEY, OP_HOME,
	OP_IASSIGN, OP_IF, OP_INPATH, OP_INVALIDATE,
	OP_KEYCODE,
	OP_LABEL, OP_LENGTH, OP_LOADITEM, OP_LOADRULES, OP_LOCATE,
		OP_LOOKUP, OP_LOOPALL,
	OP_MAKEPATH, OP_MARKED, OP_METAKEY, OP_MODE,
	OP_NEXTLINE, OP_NOT,
	OP_OPEN,
	OP_PAINT, OP_PATMARK, OP_PATMATCH, OP_PATSEARCH, OP_PATUNMARK,
		OP_PGDN, OP_PGUP, OP_PLAYMAC, OP_PROCWIN,
	OP_QUIT,
	OP_RADIO, OP_READ, OP_RECMAC, OP_RESCAN, OP_RESTORE,
	OP_RETURN, OP_RETVAR,
	OP_SEARCH, OP_SELATTRIB, OP_SETATTRIB, OP_SETEXPR, OP_SETMATCH,
		OP_SHOW, OP_SLEEP, OP_SORT, OP_SPLIT, OP_STRIP, OP_STRMARK,
		OP_STRMATCH, OP_STRUNMARK, OP_SWAP, OP_SYNC,
	OP_TAIL, OP_TEST, OP_TEXT, OP_TOGGLE, OP_TYPEOF,
	OP_UNBIND, OP_UNLINK, OP_UP,
	OP_VBORDER, OP_VIEW,
	OP_WINDOW, OP_WRITE,
	OP_YNTOGGLE
};

/*
 * Opcode table entry
 */

#if __STDC__
typedef void (*opfptr)(void);
#else
typedef void (*opfptr)();
#endif

typedef struct {
	enum codeops op;	/* Just used as an integrity check	*/
	char	cnt;		/* number of parameters			*/
	char	*name;		/* name of this op			*/
	char	*link;		/* how linker must resolve arguments	*/
	opfptr	handler;	/* the function to handle this op	*/
} gcop_t;

#if __STDC__
extern void	printArg(FILE *f, short v);
extern short	dumpOp(FILE *fp, short Ip, int symbolic);
extern int	setOpt(int opt, int val);
extern int	getOpt(int opt);
#else
extern void	printArg();
extern short	dumpOp();
extern int	setOpt();
extern int	getOpt();
#endif

extern gcop_t	ops[];
extern int	*(optTable[]);
extern char	*optNames[];

#endif


