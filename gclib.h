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


#ifndef _GCLIB_H
#define _GCLIB_H

#if __STDC__

/* string handling routines */

char	*STRCHR(const char *s, int c);
char	*STRRCHR(const char *s, int c);
void	STRUPR(char *s);

/*
 * Regexp handling
 */

char	*compileRE(char *RE);
int	matchRE(char *string);
char	*DOScompileRE(char *RE);
int	DOSmatchRE(char *string);

/* File finding routines */

char	*makePath(char *path, char *file);
char	*fixPath(char *path, char *rtn);
char	*searchDirectoryList(char *list, char sep, char *name, char **exts, char *result);
char	*findReadPath(char *name, char *order, char **exts, char *result);
char	*findWritePath(char *name, char *order, char *result);
char	*searchPath(char *name);

/* expression evaluator */

char	*compute(char *str, int *rtn);

#else

/* string handling routines */

void	STRUPR();
char	*STRCHR();
char	*STRRCHR();

/*
 * Regexp handling
 */

char	*compileRE();
int	matchRE();
char	*DOScompileRE();
int	DOSmatchRE();

/* File finding routines */

char   	*makePath();
char		*fixPath();
char   	*searchDirectoryList();
char   	*findReadPath();
char		*findWritePath();
char   	*searchPath();

/* expression evaluator */

char	*compute();

#endif

#ifdef __MSDOS__
void doIrq(int irq, short ax, short bx, short cx, short dx);
int	getReturnCode(void);
int	isDOSinternal(char *cmd);
int	findCommandType(char *cmd);

#define BEEP		(void)putch((locateCursor(24,0), 7))

#else 

#define P_WAIT	0

#if __STDC__
int spawnvp(int flag, char *cmd, char *argv[]);
#else 
int spawnvp();
#endif

#ifdef NO_BEEP
#  define BEEP		putc(7,stdout)
#else
#  if __STDC__
     extern int beep(void);
#  else
     extern int beep();
#  endif
#  define BEEP		beep()
#endif

#endif

#ifdef GC_CURSES

#define Scr_ColorAttr(fg,bg)	((unsigned char)((((unsigned char)bg)<<4) + ((fg)&0xF)))

#ifdef __MSDOS__

extern char Attribute[4];

/* Screen size limits */

#define MAX_COLS	80
#define MAX_ROWS	25

typedef struct {
	char c, r, w, h, attr;
} WINDOW;

/*
 * Dummy and simple curses function replacements
 */

#define locateCursor(r,c)	doIrq(0x10, 0x0200, 0, 0, (r<<8)+c)
#define writeChAttr(c,a)	doIrq(0x10, 0x0900+c, a, 1, 0)
#define doupdate()	
#define endwin()	
#define wnoutrefresh(w)
#define UPDATE(w)
#define REFRESH()
#define KEYENABLE(w)	
#define werase(w)	wclear(w)
#define redrawwin(w)
#define raw()
#define nonl()
#define echo()
#define noecho()
#define box(w,c1,c2)	Box(w)
#define WGETNSTR(w,r,c,b,l) my_wgetnstr(w,r,c,b,l)
#define LINES		25
#define COLS		80

void Box(WINDOW *w);
int setVideoMode(int mode);
void wclear(WINDOW *w);
void delwin(WINDOW *w);
void invalidateScreen(void);
void wputstraxy(WINDOW *w, int x, int y, char *str, int a);
void wputchaxy(WINDOW *w, int x, int y, int c, int a);
void wputchxy(WINDOW *w, int x, int y, int c);
WINDOW *newwin(int h, int w, int r, int c);
WINDOW *subwin(WINDOW *pw, int h, int w, int r, int c);
void mvwprintw(WINDOW *w, int r, int c, char *fmt,...);
short wgetch(WINDOW *w);
int saveScreen(void);
int restoreScreen(void);
void FreeScreens(void);
void initscr(void);


#else

/* UNIX ones */

extern long Attribute[4];

/* Screen size limits */

#define MAX_COLS	132
#define MAX_ROWS	50

#ifdef ultrix
#include <cursesX.h>
#else
#ifdef USE_NCURSES
#include <ncurses.h>
/* fix bug in ncurses.h */
#undef redrawwin
#define redrawwin(w)		wredrawln(w, 0, w->_maxy+1)
#else
#include <curses.h>
#endif
#endif

#define Box(w)		(box(w,vborder,hborder),UPDATE(w))
#define wputchxy(w,x,y,c) mvwaddch(w,y,x,c)

#ifdef NO_KEYPAD
#define KEYENABLE(w)	/* not supported */
#else
#define KEYENABLE(w)	keypad(w,1)
#endif

#ifdef NO_WGETNSTR
#define WGETNSTR(w,r,c,b,l) wgetstr(w,b) /* ignore r,c and l for now */
#else
#define WGETNSTR(w,r,c,b,l) wgetnstr(w,b,l) /* ignore r,c for now */
#endif

#ifndef NO_WNOUTREFRESH
#define UPDATE(w)	wnoutrefresh(w)
#define REFRESH		doupdate
#else
#define UPDATE(w)	wrefresh(w)
#define REFRESH		refresh
#endif

#if __STDC__
void clrscr(void);
void invalidateScreen(void);
#else
void clrscr();
void invalidateScreen();
#endif

#endif

#if __STDC__
void STANDOUT(WINDOW *w);
void STANDEND(WINDOW *w);
void BOLD(WINDOW *w);
void BOLDEND(WINDOW *w);
void setAttrEntry(short attr, short fg, short bg);
void selectAttrib(WINDOW *w, int attrib);
typedef int (*editFunc_t)(int,char*,char**,char**,int*,int);
void myRead(WINDOW *w, int width, int r,int c, char *buf,int len, int *mode,
		editFunc_t editFunc, int (*reader)(void));
#else
void STANDOUT();
void STANDEND();
void setAttrEntry();
void selectAttrib();
typedef int (*editFunc_t)();
void myRead();
#endif

extern int allowColor;

#endif /* GC_CURSES */

/* Directory reading */

#ifdef __MSDOS__
#include <dir.h>
#else
#ifdef NO_DIRENT_H
#include <sys/dir.h>
#else
#include <dirent.h>
#endif
#endif

#if __STDC__
void	*initDir(char *pname);
#ifdef __MSDOS__
char	*getDirEntry(int hidden, struct ffblk *fdat);
#else
char	*getDirEntry(void *dir, int hidden);
#endif
void	closeDir(void *dir);

#else

void	*initDir();
char	*getDirEntry();
void	closeDir();

#endif

/* Regexp handlers */

#if __STDC__
typedef char *((*REcompileF_t)(char *));
typedef int ((*REmatchF_t)(char *));
#else
typedef char *((*REcompileF_t)());
typedef int ((*REmatchF_t)());
#endif

extern REcompileF_t compileRegexp;
extern REmatchF_t matchRegexp;

#endif /* _GCLIB_H */


