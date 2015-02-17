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


#ifndef _GCCC_H
#define _GCCC_H

#include "gcboth.h"
#include "gcerr.h"

/*
 * LEXICAL TOKEN TYPES
 */

enum symboltype {

/* RESERVED WORDS, COLOURS FIRST */

	SYM_BLACK, SYM_BLUE, SYM_GREEN, SYM_CYAN,
	SYM_RED, SYM_MAGENTA, SYM_BROWN, SYM_LIGHTGRAY,
	
	SYM_ACCESS, SYM_ACTION, SYM_ALL, SYM_AND, SYM_APPEND, SYM_ASCENDING,
		SYM_ATIME, SYM_ATTRIBUTE,
	SYM_BEEP, SYM_BDEV, SYM_BIND, SYM_BOX, SYM_BREAK, SYM_BROWSE,
	SYM_CAT, SYM_CD, SYM_CDEV, SYM_CLEAR, SYM_COMPARE, SYM_CONTAINER,
		SYM_CONTAINS, SYM_CONTENTS, SYM_CONTINUE, SYM_COPY,
	SYM_DESCENDING, SYM_DIR, SYM_DO, SYM_DOWN, SYM_DUMP,
	SYM_ECHO, SYM_EDITTEXT, SYM_ELSE, SYM_END, SYM_ENDREC, SYM_EVAL, SYM_EXEC,
		SYM_EXECCATCH, SYM_EXITTEXT, SYM_EXPAND, SYM_EXTENS,
	SYM_FILTER, SYM_FINDAREA, SYM_FROM,
	SYM_GCAFTERKEY, SYM_GCDEFAULT, SYM_GCINIT, SYM_GCSTART, SYM_GROUP,
	SYM_HBORDER, SYM_HEAD, SYM_HELP, SYM_HOME,
	SYM_IF, SYM_IN, SYM_INPATH, SYM_INVALIDATE, SYM_IS,
	SYM_KEYCODE,
	SYM_LEFT, SYM_LENGTH, SYM_LINK, SYM_LOADITEM, SYM_LOADRULES,
		SYM_LOCAL, SYM_LOCATE, SYM_LOCK, SYM_LOOKUP, SYM_LOOP,
	SYM_MAKEPATH, SYM_MARK, SYM_MARKED, SYM_MATCHES, SYM_METAKEY,
		SYM_MODE, SYM_MOVE, SYM_MTIME,
	SYM_NAME, SYM_NEXTLINE, SYM_NONE, SYM_NOT,
	SYM_ON, SYM_OPEN, SYM_OR, SYM_OWNER, 
	SYM_PAINT, SYM_PERMS, SYM_PGDN, SYM_PGUP, SYM_PIPE, SYM_PLAYMAC, SYM_PROCWIN,
	SYM_QUIT,
	SYM_RADIO, SYM_READ, SYM_RECMAC, SYM_REG,
		SYM_RESCAN, SYM_RESTORE, SYM_RETURN, SYM_RIGHT,
	SYM_SEARCH, SYM_SHOW, SYM_SIZE, SYM_SLEEP,
	SYM_SORTBY, SYM_SPLIT, SYM_STRIP, SYM_SWAP, SYM_SYNC,
	SYM_TAIL, SYM_TEXT, SYM_TO, SYM_TOGGLE, SYM_TYPEOF,
	SYM_UNBIND, SYM_UNLINK, SYM_UNMARK, SYM_UP,
	SYM_VBORDER, SYM_VIEW,
	SYM_WHILE, SYM_WINDOW, SYM_WRITE,
	SYM_YNTOGGLE,

/* THEN PLAIN TOKENS */

	SYM_EOF, SYM_LBRACE, SYM_RBRACE, SYM_SEMICOLON,
	SYM_KEYSPEC, SYM_REGEXP, SYM_IDENTIFIER, SYM_COMMA,
 	SYM_INTEGER, SYM_EQUALS, SYM_LPARENTHESIS, SYM_RPARENTHESIS, 
	SYM_STRING, SYM_UNDEF, SYM_ASSIGN, SYM_LBRACKET, SYM_RBRACKET,
	SYM_PLUS, SYM_DECL, SYM_COLON
};

#include "gcops.h"

typedef struct
{
	enum symboltype sym;
	enum codeops op;
} ParserRule_t;

#define MAXINT 32767

#define MAX_DEFS	10	/* max number of IF_ IDs, including
					two predefined DOS/UNIX and ANY */

/*
 * ID Types
 */

#if 0 /* these are now static,,, */
extern char		symtext[];
extern int		symvalue;
extern enum symboltype	symbol;
extern int		newID;
extern int		inDecl;
extern short		labelcnt;
extern int		EM;
extern short		pnames[],
			vnames[];
#endif

extern FILE		*keydeffile;
extern unsigned		ch;
extern int		np;
extern int		nv;
extern int		verbose;
extern short		optionStr[MAX_OPTS];

/*
 * Prototypes
 */

#if __STDC__
void warn(enum errort typ);
void config_file(void);
void Define(char *name);
int gclink(void);
void freeStab(void);

#else

void warn();
void config_file();
void define();
int gclink();
void freeStab();

#endif

#endif
