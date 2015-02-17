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


#ifndef _GC3_H
#define _GC3_H

/* Special debugging macros for finding file leaks */

/*#define FILE_DEBUG*/

#ifndef FILE_DEBUG
#define OpenFile	fopen
#define CloseFile	fclose
#define OPEN	open
#define CLOSE	close
#define DUP	dup
#else
#define OpenFile(n,m)	gc_fopen(n,m,__FILE__,__LINE__, fileDebugID++)
#define CloseFile(f)	gc_fclose(f,__FILE__,__LINE__)
#define OPEN(n,m,a)	gc_open(n,m,a,__FILE__,__LINE__, fileDebugID++)
#define CLOSE(h)		gc_close(h,__FILE__,__LINE__)
#define DUP(h)			gc_dup(h,__FILE__,__LINE__, fileDebugID++)
#endif

#include "gcboth.h" /* stuff common to compiler and interpreter */

/********************************************************
	DOS-SPECIFIC STUFF
*********************************************************/

#ifdef __MSDOS__

#define CHDIR		mychdir
			/* We must position the cursor with beep to prevent
					a scroll... */

/*
 * Executable file permissions
 */

#define S_IXUSR		S_IEXEC
#define S_IXGRP		S_IEXEC
#define S_IXOTH		S_IEXEC

/* 
 * POSIX file attribute macros
 */

/* macro to test for directory file */
#define	S_ISDIR( mode )		((mode) & S_IFDIR)
/* macro to test for character special file */
#define	S_ISCHR( mode )		((mode) & S_IFCHR)
/* macro to test for block special file */
#define	S_ISBLK( mode )		((mode) & S_IFBLK)
/* macro to test for fifo */
#define	S_ISFIFO( mode )	((mode) & S_IFIFO)
/* The previous form causes lint to complain about signed/unsigned
   mixing. Here is a more explicit alternative. */
#define	S_ISREG( mode )		(((mode) & 0x7000)==0)

/***************************************************************/

#define NUM_INFO_LNS	3
#define MAX_FILES	(LINES-NUM_INFO_LNS-5)

typedef struct {
	char		*name;
	unsigned char	flag;
	char		typID;
	char		depth;	/* for dir tree */
	short		parent;	/* For dir tree */
	off_t		size;
	mode_t		mode;
	uid_t		uid;
	gid_t		gid;
	time_t		modtime;
	time_t		acctime;
} fInfo_t;

#define IsDirectory(l, n)	(fInfo[l][n].typID == PATH_SEP)
#endif

/********************************************************
	UNIX-SPECIFIC STUFF
*********************************************************/

#ifndef __MSDOS__

#define CHDIR		chdir

/* 
 * POSIX file attribute macros
 */

#ifndef S_ISDIR				/* macro to test for directory file */
#define	S_ISDIR( mode )		(((mode) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISCHR				/* macro to test for character special file */
#define	S_ISCHR( mode )		(((mode) & S_IFMT) == S_IFCHR)
#endif

#ifndef S_ISBLK				/* macro to test for block special file */
#define	S_ISBLK( mode )		(((mode) & S_IFMT) == S_IFBLK)
#endif

#ifndef S_ISFIFO			/* macro to test for fifo */
#define	S_ISFIFO( mode )	(((mode) & S_IFMT) == S_IFIFO)
#endif

#ifndef S_ISREG				/* macro to test for regular file */
#define	S_ISREG( mode )		(((mode) & S_IFMT) == S_IFREG)
#endif

#ifndef S_ISLNK
#ifndef S_IFLNK
#define S_ISLNK( mode )		0
#else
#define	S_ISLNK( mode )		(((mode) & S_IFMT) == S_IFLNK)
#endif
#endif

/***************************************************************/

#define NUM_INFO_LNS	4
#define MAX_FILES	(LINES-NUM_INFO_LNS-5)

typedef struct {
	char		*name;
	unsigned char	flag;
	char		typID;
	char		depth;	/* for dir tree */
	short		parent;	/* For dir tree */
	off_t		size;
	mode_t		mode;
	uid_t		uid;
	gid_t		gid;
	time_t		modtime;
	time_t		acctime;
} fInfo_t;

#define IsDirectory(l, n)	(S_ISDIR(fInfo[l][n].mode))
#endif

#if __STDC__
extern void clrscr(void);
void SHELL_MODE(char *msg);
void PROG_MODE(void);
#else
extern void clrscr();
void SHELL_MODE();
void PROG_MODE();
#endif

/********************************************************
	GENERIC STUFF
*********************************************************/

/*
 * Loop types
 */

#define PAT_LOOP	1
#define ALL_LOOP	2
#define MARK_LOOP	3

#define F_SELECTED	1			/* Only flag so far for each file */

#define UNKNOWN_ID		9123		/* We assume this is an unused user/group ID */

/*
 * Window states; 0 => mu
 */

#define W_LIST		1		/* DIrectory listing */
#define W_TREE		2		/* directroy tree */
#define W_VIEW		3		/* Viewer window */
#define W_EXPLODE	4		/* COntainer window */

/*************** PROTOTYPES OF OPCODE HANDLERS *******************/

#if __STDC__
typedef void (*fptr)(void);
#define DF(n)	void n(void)
#else
typedef void (*fptr)();
#define DF(n)	void n()
#endif

DF(i_access);
DF(i_action);
DF(i_append);
DF(i_assign);
DF(i_beep);
DF(i_bind);
DF(i_box);
DF(i_break);
DF(i_browse);
DF(i_cat);
DF(i_call);
DF(i_cd);
DF(i_clear);
DF(i_compare);
DF(i_contains);
DF(i_down);
DF(i_dump);
DF(i_echo);
DF(i_edittext);
DF(i_end);
DF(i_endloop);
DF(i_endrec);
DF(i_eval);
DF(i_exec);
DF(i_execcatch);
DF(i_exittext);
DF(i_expand);
DF(i_fcopy);
DF(i_filter);
DF(i_findarea);
DF(i_function);
DF(i_goto);
DF(i_hborder);
DF(i_head);
DF(i_help);
DF(i_helpall);
DF(i_helpkey);
DF(i_home);
DF(i_iassign);
DF(i_if);
DF(i_inpath);
DF(i_invalidate);
DF(i_keycode);
DF(i_length);
DF(i_loaditem);
DF(i_loadrules);
DF(i_locate);
DF(i_lookup);
DF(i_loopall);
DF(i_makepath);
DF(i_marked);
DF(i_metakey);
DF(i_mode);
DF(i_nextline);
DF(i_not);
DF(i_open);
DF(i_paint);
DF(i_patmark);
DF(i_patmatch);
DF(i_patsearch);
DF(i_patunmark);
DF(i_pgdn);
DF(i_pgup);
DF(i_playmac);
DF(i_procwin);
DF(i_radio);
DF(i_read);
DF(i_recmac);
DF(i_rescan);
DF(i_restore);
DF(i_return);
DF(i_retvar);
DF(i_search);
DF(i_selattrib);
DF(i_setattrib);
DF(i_setexpr);
DF(i_setmatch);
DF(i_show);
DF(i_sleep);
DF(i_sort);
DF(i_split);
DF(i_strip);
DF(i_strmark);
DF(i_strmatch);
DF(i_strunmark);
DF(i_swap);
DF(i_sync);
DF(i_tail);
DF(i_test);
DF(i_text);
DF(i_toggle);
DF(i_typeof);
DF(i_unbind);
DF(i_unlink);
DF(i_up);
DF(i_vborder);
DF(i_view);
DF(i_window);
DF(i_write);
DF(i_yntoggle);

/*************** OTHER PROTOTYPES *******************/

#if __STDC__

/* from gc3.c */

int	CompileScript(char *script, short symdump, short dontLink);
char *CreateTempName(void);
void	Shutdown(void);

/* from gcterp.c */

void	AllocateStringSpace(void);
void	FreeStringSpace(void);
void	nselect(int l, int n, int v);
void	clearFilter(int l);
int	testOption(int opt);
int	GetBuffer(char *fname, int lnum);
void	ReleaseBuffer(int buf, char *fname, int lnum);
int	stripwhitespace(char *buf);
void	FreeVars(void);
int	runCommand(char *cmd, int catch);
int	findFile(int l, int s, char *fname, int *idx);
int	readNextLine(FILE *fp);
void	doDefaultAction(int ch);
int	Interpret(short IP, int ch, short stackBot);
char  *lookupArg(short idx, int buf);
char	*lookupVar(short idx, int buf);
void	assign2var(short varnum, char *val);
int	doesMatch(char *pat, char *str, int len);
void	subExecute(short IP);

/* From eval.c */

char	*compute(char *str, int *rtn);

/* From gcdisk.c */

#ifdef FILE_DEBUG
FILE *gc_fopen(char *n, char *m, char *fn, int ln, int ID);
int	gc_fclose(FILE *f, char *fn, int ln);
int	gc_open(char *n, int m, int a, char *fn, int ln, int ID);
int	gc_close(int h, char *fn, int ln);
int	gc_dup(int h, char *fn, int ln, int ID);
#endif
void	ShowOpenFiles(void);
int	splitPath(char *path, char *exist, char *nonexist);
off_t	getFileSize(char *pathname);
mode_t	getFileMode(char *pathname);
gid_t	getFileGroup(char *pathname);
uid_t	getFileUser(char *pathname);
time_t	getFileModTime(char *pathname);
time_t	getFileAccTime(char *pathname);
char	*getPerms(mode_t tmd);
int		unlinkFile(char *f);
int	checkType(mode_t m, short f);
void	refilter(int l, char *oldname);
void getTreePath(int l, char *buf);
void	readList(int n, int newpath);
void	freeNames(int n);
void	getCWD(char *name);
void	doCD(char *d);
int	readIniFile(void);
void	SaveInitFile(void);
void	dirInit(char *p1, char *p2);
#ifdef __MSDOS__
void	mychdir(char *d);
#endif
void	getDiskSpace(int n, char *path);
int	doFileCopy(char *src, char *dest, int del);
void	cd2old(void);
int	loadItem(FILE *fp, char *name, char *val);
int	seekToArea(FILE *fp, char *name);
char *getParent(char *dr, char *pr);
char *getExtraction(char *cont, char *elt, char *rtn, int rule);
void	readRules(FILE *fp);
void	FreeRules(void);

/* From gcscreen.c */

#ifndef __MSDOS__
char	*GetUserID(int uid);
char	*GetGroupID(int gid);
#endif

int	isListWindow(int w);
void	InitialiseScreenAttributes(void);
void	showCursor(int n, int sel);
void	hideCursor(int n);
void	clearCmdWin(void);
void	writeMsg(char *txt);
void	showMsg(char *txt);
int	getInput(char *buf, int len, int mode);
void	enableSpecialKeys(void);
void	paintColour(void);
void	setupCurses(int makeWins);
void	showPath(int p);
void	showList(int n);
void	doViewNav(int w, int i);
void	normalise(int n);
void	invalidateWindows(void);
void	repaintScreen(void);
void	syncScreen(void);
void	redrawScreen(void);
void	cursorUp(void);
void	cursorDown(void);
void	cursorPgUp(void);
void	cursorPgDn(void);
void	cursorHome(void);
void	cursorEnd(void);
int	readKey(void);
void	ScreenMsg(char *msg);
void 	displayIP(int ip);
void	HyperCompile(char *fname);
void	HyperBrowse(char *fname, char *first);

void	sWinInit(void);
void	sWinShutdown(void);
int	sWinMake(short left, short top, short height, short width);
void	sWinBox(void);
void	sWinLocate(short row, short col);
void	sWinSelAttrib(short attrib);
int	sWinText(short arg);
int	sWinToggle(short arg1,short arg2);
int	sWinEditText(short arg, short len);
int	sWinExitText(short arg);
int	sWinYNToggle(short var, short fld);
int	sWinRadio(short var, short fld);
int	sWinAction(short text, short hook);
char *sWinProcess(void);

/* From gcsort.c */

void	sortList(int n,char *name);
void	resortList(int l, int dir, short key);

/* From gccode.c */

void	AllocateCodeSpace(void);
void	FreeCodeSpace(void);

#else /* NON_ANSI DECLARATIONS *************************************/

/* from gc3.c */

int	CompileScript();
char *CreateTempName();
void	Shutdown();

/* from gcterp.c */

void	AllocateStringSpace();
void	FreeStringSpace();
void	nselect();
void	clearFilter();
int	testOption();
int	GetBuffer();
void	ReleaseBuffer();
int	stripwhitespace();
void	FreeVars();
int	runCommand();
int	findFile();
int	readNextLine();
void	doDefaultAction();
int	Interpret();
char  *lookupArg();
char	*lookupVar();
void	assign2var();
int	doesMatch();
void	subExecute();

/* From eval.c */

char	*compute();

/* From gcdisk.c */

#ifdef FILE_DEBUG
FILE *gc_fopen();
int	gc_fclose();
int	gc_open();
int	gc_close();
int	gc_dup();
#endif
void	ShowOpenFiles();
int splitPath();
off_t	getFileSize();
mode_t	getFileMode();
gid_t	getFileGroup();
uid_t	getFileUser();
time_t	getFileModTime();
time_t	getFileAccTime();
char	*getPerms();
int		unlinkFile();
int	checkType();
void	refilter();
void getTreePath();
void	readList();
void	freeNames();
void	getCWD();
void	doCD();
int	readIniFile();
void	SaveInitFile();
void	dirInit();
int	doFileCopy();
void	getDiskSpace();
void	cd2old();
int	loadItem();
int	seekToArea();
char *getParent();
char *getExtraction();
void	readRules();
void	FreeRules();

/* From gcscreen.c */

int	isListWindow();
void	InitialiseScreenAttributes();
void	showCursor();
void	hideCursor();
void	clearCmdWin();
void	writeMsg();
void	showMsg();
int	getInput();
void	enableSpecialKeys();
void	paintColour();
void	setupCurses();
void	showPath();
void	showList();
void	doViewNav();
void	normalise();
void	invalidateWindows();
void	repaintScreen();
void	syncScreen();
void	redrawScreen();
void	exitCurses();
void	cursorUp();
void	cursorDown();
void	cursorPgUp();
void	cursorPgDn();
void	cursorHome();
void	cursorEnd();
int	readKey();
char	*GetUserID();
char	*GetGroupID();	     
void	ScreenMsg();
void 	displayIP();
void	HyperCompile();
void	HyperBrowse();

void	sWinInit();
void	sWinShutdown();
int	sWinMake();
void	sWinBox();
void	sWinLocate();
void	sWinSelAttrib();
int	sWinText();
int	sWinToggle();
int	sWinEditText();
int	sWinExitText();
int	sWinYNToggle();
int	sWinRadio();
int	sWinAction();
char *sWinProcess();


/* From gcsort.c */

void	resortList();
void	sortList();

/* From gccode.c */

void	AllocateCodeSpace();
void	FreeCodeSpace();

#endif

/*************** GLOBAL VARIABLES *******************/

extern long		freeSpace[2],
			usedSpace[2],
			selSize[2];
extern fInfo_t		*fInfo[2]; /* Directory info for windows */
extern char		tempName[MAXPATHNAME],
			viewLine[256],
			fileViewed[MAXPATHNAME],
			file2view[MAXPATHNAME],
#if __MSDOS__
			filters[2][14],
#else
			filters[2][64],
#endif
			*BUF[MAX_BUFFERS],
			paths[2][MAXPATHNAME];
extern int		metaMode,
			allowColor,
			forceCurrent,
			numfiles[2],
			fIndex[2][MAXARGS],
			highlight[2],
			startLn[2],
			selCnt[2],
			filtering[2],
			fnameLen,
			metaKey,
			leftMargin,
			topMargin,
			winWidth[2],
			winInvalid[2],
			showLines[2],
			statusVis[2],
			listVis[2],
			winState[2],
			hborder,
			vborder,
			viewer,
			viewingDir,
			inCurses,
			wrap,
			writeIP,
			showIP,
			Lines,
			fgCol,
			bgCol,
			changedWinSize,
			l;

extern char	*valspace[MAX_VARS];

extern short
			info2show,
			ip,
			escapeCh;

extern FILE
#ifdef DEBUG
			*debug,
#endif
			*catchFP;

#ifdef FILE_DEBUG
extern int fileDebugID;
#endif

#ifdef __FILE__
#define GRABBUF()		GetBuffer(__FILE__, __LINE__)
#define FREEBUF(b)	ReleaseBuffer(b, __FILE__, __LINE__)
#else
#define GRABBUF()		GetBuffer("?", 0)
#define FREEBUF(b)	ReleaseBuffer(b, "?", 0)
#endif

#endif



