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


#include "gcport.h"
#include "gclib.h"
#include "gckey.h"

#ifdef __MSDOS__

#define PTR_TO_LONG(p)	((((unsigned long)(p)) >> 16l)*16l +\
				(unsigned long)((unsigned short)(p)))

#ifndef PATH_SEP
# define PATH_SEP	'\\'
# define PATH_SEP_STR	"\\"
#endif

#ifndef FIELD_SEP
#  define FIELD_SEP	';'
#endif

#ifndef MAXPATHNAME
# define MAXPATHNAME	64
#endif

typedef unsigned char uchar;

#else /* UNIX */

#define PTR_TO_LONG(p)	((unsigned long)p)

#ifndef PATH_SEP
# define PATH_SEP	'/'
# define PATH_SEP_STR	"/"
#endif

#ifndef FIELD_SEP
#  define FIELD_SEP	':'
#endif

#ifndef MAXPATHNAME
# define MAXPATHNAME	1024
#endif

#endif

int allowColor=1;

/*
 * STRCHR - strchr() replacement. Useful as not all systems have
 *		strchr(), and it doesn't seem to be prototyped
 *		in <string.h> on many systems.
 */

#if __STDC__
char *STRCHR(const char *s, int c)
#else
char *STRCHR(s, c)
	char *s;
	int c;
#endif
{
	register char *sp = (char *)s;
	do { /* As NUL is considered part of it... */
		if (*sp==c) return sp;
	} while (*sp++);
	return NULL;
}

#if __STDC__
char *STRRCHR(const char *s, int c)
#else
char *STRRCHR(s, c)
	char *s;
	int c;
#endif
{
	register char *sp = (char *)s;
	char *rtn = NULL;
	do
	{
		if (*sp==c) rtn = sp;
	}
	while (*sp++);
	return rtn;
}

/*
 * STRUPR - Convert string to upper-case in place.
 */

#if	__STDC__
void STRUPR(char *s)
#else
void STRUPR(s)
	char *s;
#endif
{
	if (s)
		while (*s)
		{
			*s = toupper(*s);
			s++;
		}
}

/***********************************************************
			GET WORKING DIRECTORY
************************************************************/

#if	__STDC__
void getCWD(char *name)
#else
void getCWD(name)
	char *name;
#endif /* __STDC__ */
{
#ifdef NO_GETCWD
	FILE *fp;
	char *n = tmpnam(NULL), buf[64];
	sprintf(buf,"pwd > %s",n);
	system(buf);
	if ((fp = fopen(n,"r")) != NULL)
	{
		fscanf(fp,"%s",name);
		fclose(fp);
		unlink(n);
	}
#else
	(void)getcwd(name,MAXPATHNAME-1);
#endif
}

/* fixPath takes a path which can be abs or rel, with or
	without a drive specifier, and containing . and ..
	components. It returns a minimised absolute path
	(under DOS the return path includes the drive ID and
	is all upper case */

#if __STDC__
char *fixPath(char *path, char *rtn)
#else
char *fixPath(path, rtn)
	char *path, *rtn;
#endif
{
	char *p, *rslt = rtn;
#ifdef __MSDOS__
	char wb[MAXPATHNAME];
	int drv;
	assert(path && rtn);
	STRUPR(path);
	/* Put the drive ID in to get it over with... */
	if (path[1]==':')
	{
		rtn[0] = path[0];
		path+=2;
	} else
		rtn[0] = getdisk() + 'A';
	rtn[1] = ':';
	drv = rtn[0] - 'A';
	rtn+=2;
#else
	assert(path && rtn);
#endif
	if (path[0]==PATH_SEP) /* abs path */
		p = path;
	else
	{
#ifdef __MSDOS__
		p = makePath(_getdcwd(drv+1,rslt,MAXPATHNAME), path); /* relative path */
		rslt[2] = '\0'; /* return to state before calling _getdcwd */
#else
		getCWD(rtn);
		p = makePath(rtn, path);
#endif
	}
	/* Now strip .. amd . components */
	{
		char *bp = STRCHR(p, PATH_SEP), *sp;
		int addSep = 0;
		*bp++ = '\0'; /* Split at first sep */
		rtn[0] = PATH_SEP;
		rtn[1] = '\0';
		do
		{
			sp = STRCHR(bp, PATH_SEP);
			if (sp) *sp = '\0';
			if (strcmp(bp,"..")==0)
			{
				char *last = STRRCHR(rtn,PATH_SEP);
				if (last > rtn)	*last = '\0';
				else
				{
					rtn[1] = '\0';
					addSep = 0;
				}
			} else if (strcmp(bp,"."))
			{
				if (addSep) strcat(rtn,PATH_SEP_STR);
				strcat(rtn,bp);
				addSep = 1;
			}
			bp = sp+1;
		}
		while (sp);
	}
	return rslt;
}

/*
 * makePath is passed two string arguments `path' and `file' (the
 * former may be NULL). It concatenates the two, inserting a path
 * separator character if necessary. If the path is relative, it is
 * made absolute. Any `.' and `..' entries in the path are stripped,
 * and the path adjusted accordingly. makePath keeps the result in
 * a static string buffer and returns a pointer to this buffer.
 */

#if __STDC__
char *makePath(char *path, char *file)
#else
char *makePath(path, file)
	char *path, *file;
#endif
{
	int plen, flen = strlen(file);
	static char fbuf[MAXPATHNAME];
	if (path && path[0])
	{
		plen = strlen(path);
		assert(plen<MAXPATHNAME);
		strcpy(fbuf,path);
		if (path[plen-1]!=PATH_SEP && file[0])
		{
			plen++;
			strcat(fbuf,PATH_SEP_STR);
		}
		assert((plen+flen)<MAXPATHNAME);
		strcat(fbuf,file);
	} else
	{
		assert(flen<MAXPATHNAME);
		strcpy(fbuf,file);
	}
#ifdef __MSDOS__
	STRUPR(fbuf);
#endif
	return fbuf;
}

/*
 * searchDirectoryList searches a set of directories whose
 * names are given in the string `list', separated by
 * the character `sep'. If the file specified by `name'
 * is found in one of these directories, a pointer to
 * a string holding the full path name is returned. This
 * string is given by the parameter `result', and must
 * have space for 256 bytes. If this parameter is NULL,
 * the result is returned in a static local buffer.
 *
 * If the file is not found in any of the directories,
 * NULL is returned.
 *
 * An array of file extensions can be provided in the `exts'
 * argument; these will be tried in turn as well if they exist.
 *
 * Note that if `name' is NULL or empty, then `list' can
 * be a list of files rather than directories.
 * If the list is NULL, then `name' is returned in the
 * string if it exists, else NULL is returned.
 *
 */

#if __STDC__
static int extAccess(char *name, char **ext)
#else
static int extAccess(name, ext)
	char *name, **ext;
#endif
{
	if (ext && ext[0])
	{
		int i = 0;
		char path[MAXPATHNAME];
		while (ext[i])
		{
			strcpy(path,name);
			strcat(path,ext[i]);
			if (access(path,0)==0)
				return i;
			i++;
		}
		return -1;
	}
	else 
		return access(name,0);
}

#if __STDC__
char *searchDirectoryList(char *list, char sep, char *name,
					char **exts, char *result)
#else
char *searchDirectoryList(list, sep, name, exts, result)
	char *list, sep, *name, **exts, *result;
#endif
{
	char *path;
	static char pathbuf[256]; /* holds the path of the file, if found */
	int startPos=0, endPos, length, av;
	path = ( result ? result : pathbuf );
	path[0] = '\0';
	if (list==NULL)
	{
		av = extAccess(name,exts);
		if (av >= 0)
		{
		  	strcpy(path,name);
			if (exts) strcat(path,exts[av]);
			return path;
		}
		else return NULL;
	}
	while (list[startPos])
	{
		if (list[startPos]==sep)
		{
			startPos++;
			continue;
		}
		endPos = startPos+1;
		while (list[endPos] && list[endPos]!=sep)
			endPos++;
		length = endPos-startPos;
		assert(length<256);
		strncpy(path,list+startPos,length);
		path[length] = '\0';
		if (name && name[0])
		{
			if (path[length-1]!=PATH_SEP)
				strcat(path,PATH_SEP_STR);
			strcat(path,name);
		}
		av = extAccess(path,exts);
		if (av>=0)
		{
			if (exts) strcat(path,exts[av]);
			return path;
		}
		startPos = endPos;
	}
	return NULL;
}

/*
 * findReadPath searches for a file in the order given by
 * `order'. The latter is a string which can contain
 * environment variables, a `.' (for the current directory),
 * or a / or \ (the top level directory). Under DOS, \ is
 * treated as C:\, rather than the root directory on the
 * working drive.
 */

#if __STDC__
char *findReadPath(char *name, char *order, char **exts, char *result)
#else
char *findReadPath(name, order, exts, result)
	char *name, *order, **exts, *result;
#endif
{
	char *rtn=NULL, *evptr, evar[40];
	int len;
	if (order==NULL || name==NULL || *name==0)
		return NULL;
	while (*order && !rtn)
	{
		switch(*order++)
		{
		case '.':
			rtn = searchDirectoryList(".", 0, name, exts, result);
			break;
		case PATH_SEP:
#if __MSDOS__
			rtn = searchDirectoryList("C:\\", 0, name, exts, result);
#else
			rtn = searchDirectoryList("/", 0, name, exts, result);
#endif
			break;
		case '$': /* environment var */
			evptr = evar;
			len = 40;
			while (*order && *order!='.' && *order != PATH_SEP
					&& *order!='$')
			{
				*evptr++ = *order++;
				len--;
				assert(len);
			}
			*evptr = 0;
			rtn = searchDirectoryList(getenv(evar),FIELD_SEP, name, exts, result);
			break;
		}
	}
	return rtn;
}

/*
 * findWritePath is similar to findReadPath, except it just makes
 * the first path it can.
 */

#if __STDC__
char *findWritePath(char *name, char *order, char *result)
#else
char *findWritePath(name, order, result)
	char *name, *order, *result;
#endif
{
	char *evptr, evar[40], *rtn = NULL;
	int len;
	if (order==NULL || name==NULL || *name==0)
		return NULL;
	while (*order && rtn==NULL)
	{
		switch(*order++)
		{
		case '.':
			rtn = makePath(".",name);
			break;
		case PATH_SEP:
			rtn = makePath(PATH_SEP_STR,name);
			break;
		case '$': /* environment var */
			evptr = evar;
			len = 40;
			while (*order && *order!='.' && *order != PATH_SEP
					&& *order!='$')
			{
				*evptr++ = *order++;
				len--;
				assert(len);
			}
			*evptr = 0;
			if ((evptr = getenv(evar)) != NULL)
				rtn = makePath(evptr,name);
			break;
		}
	}
	if (rtn && result) strcpy(result,rtn);
	return rtn;
}

/*
 * searchPath uses findReadPath to search the list of directories
 * specified by the $PATH environment variable.
 */

#if __STDC__
char *searchPath(char *name)
#else
char *searchPath(name)
	char *name;
#endif
{
	return findReadPath(name, "$PATH", NULL, NULL);
}

#ifdef __MSDOS__

void doIrq(int irq, short ax, short bx, short cx, short dx)
{
	union REGS regs;
	regs.x.ax = ax;
	regs.x.bx = bx;
	regs.x.cx = cx;
	regs.x.dx = dx;
	int86(irq,&regs,&regs);
}

int getReturnCode(void)
{
	union REGS regs;
	regs.x.ax = 0x4d00; /* get return code function */
	intdos(&regs,&regs);
	/* low byte has 0 (ok) 1 (^C) 2 (Critical error) or 3 (TSR)
	   high byte has child return */
	return (regs.h.al<<8) + regs.h.ah; 
}

int isDOSinternal(char *cmd)
{
	char c[16]; int i;
	static char *intCmd[] = {
		/* this is not complete */
		"BREAK", "CHCP", "CD", "CHDIR", "CLS", "COPY", "CTTY", "DATE",
		"DEL", "DIR", "ECHO", "EXIT", "ERASE", "FOR", "GOTO", "IF",
		"MKDIR", "MOVE", "PATH", "PAUSE", "PROMPT", "REM", "REN",
		"RENAME", "RMDIR", "SET", "SHIFT", "TIME", "TYPE", "VER",
		"VERIFY", "VOL", NULL
	};
	strncpy(c,cmd,14);
	c[14] = '\0';
	STRUPR(c);
	i = 0;
	while (intCmd[i]) /* should do a binary search, but I'm lazy... */
		if (strcmp(intCmd[i++],c)==0) return 1;
	return 0;
}

/* findCommandType, given a command, returns one of the following:

		0 - unknown type
		1 - internal DOS command
		2 - DOS batch file
		3 - DOS .EXE
		4 - DOS .COM

	If the command has an extension, this is checked, and a value
	returned. Otherwise, if the file has a backslash in its name,
	we check for a .BAT, .EXE or .COM file of that name and path.
	If no backslash, we check the current directory and then the
	path.
*/

static char *DOSexts[] = { ".BAT", ".EXE", ".COM", NULL };

int findCommandType(char *cmd)
{
	char *sepPtr, *dotPtr, *pathPtr;
	char Cmd[MAXPATHNAME], ext[5];
	int rtn, i;
	if (isDOSinternal(cmd)) return 1;
	strcpy(Cmd,cmd);
	ext[4] = '\0'; /* for safety */
	dotPtr = STRRCHR(Cmd,'.');
	sepPtr = STRRCHR(Cmd,'\\');
	if (dotPtr && (sepPtr == NULL || dotPtr>sepPtr))
		pathPtr = dotPtr; /* we have an extension */
	else if (sepPtr) /* we have a path  - check that directory */
	{
		*sepPtr = '\0';
		pathPtr = findReadPath(sepPtr+1, Cmd, DOSexts, NULL);
	}
	else
		/* no path & no extension; check . then $PATH */
		pathPtr = findReadPath(Cmd, ".$PATH", DOSexts, NULL);
	if (pathPtr == NULL) return 0;
	strncpy(ext,pathPtr+strlen(pathPtr)-4,4);
	STRUPR(ext);
	for (i=0;i<3;i++)
	{
		if (strcmp(DOSexts[i],ext)==0)
		{
			/*fprintf(stderr,"Command %s is of type %s\n",cmd,DOSexts[i]);*/
			return i+2;
		}
	}
	/*fprintf(stderr,"Command %s is of unknown type\n",cmd);*/
	return 0;
}

#else

	/* UNIX version of DOS's spawnvp() */

#include <sys/wait.h>

#if __STDC__
int spawnvp(int flag, char *cmd, char *argv[])
#else
int spawnvp(flag, cmd, argv)
	int flag;
	char *cmd, *argv[];
#endif
{
	int status, pid, wpid;
	if ((pid = fork()) == 0)
	{
		execvp(cmd,argv);
		if (errno == ENOEXEC)
		{
			/* we assume no more than 100 args! */
			char *newArgs[102];
			int i;
			newArgs[0] = "sh";
			newArgs[1] = "-c";
			for (i=0;i<100;i++)
			{
				newArgs[i+2] = argv[i];
				if (argv[i]==NULL) break;
			}
			newArgs[101] = NULL; /* safety */
			execvp(cmd,newArgs);
		}
	}
	while ((wpid = wait(&status)) != pid)
		if (wpid == -1 && errno != EINTR)
			return -1;
	return (status >> 8) & 0377;
}
#endif

/****************************************************

	GC3 EXPRESSION EVALUATOR

 This code evaluates numeric expressions contained
 in a string. It should be portable across any
 decent C compiler.

 Entry to this code is through the routine:

	char *compute(char *str, int *rtn)
		
 which takes as arguments a pointer to the string
 containing the expression, and a pointer to the
 variable in which the result should be placed.
 If an error is found in the expression, a message
 is returned, but if the expression is acceptable
 then NULL is returned.

*****************************************************/

#ifdef __MSDOS__
#	ifndef __STDC__
#		define __STDC__	1
#	endif
#endif

/*
 * Lexical tokens
 */

#define NUM_TOK		1
#define OR_TOK		2
#define AND_TOK		3
#define EQU_TOK		4
#define NEQ_TOK		5
#define ADD_TOK		6
#define SUB_TOK		7
#define MUL_TOK		8
#define DIV_TOK		9
#define MOD_TOK		10
#define LFT_TOK		11
#define RGT_TOK		12
#define GEQ_TOK		13
#define GTR_TOK		14
#define LEQ_TOK		15
#define LES_TOK		16

/*
 * Local Variables
 */

static char	*ebuf,	/* Address of expression string	*/
		*msg,	/* Return message		*/
		*fail = "Syntax error in expression"; /* Failure message */

static int	epos,	/* Current position in ebuf buffer	*/
		tok,	/* Current lexical token		*/
		tokval;	/* Current token value, if a number	*/

/*
 * Forward declarations
 */

#if __STDC__
static int _cond_expr(void);
#endif

/*
 * Lexical analyser
 */

#if __STDC__
static void nexttok(void)
#else
static void nexttok()
#endif
{
	while (isspace(ebuf[epos])) epos++;
	if (ebuf[epos])
	{
		if (isdigit(ebuf[epos])) /* Integer? */
		{
			tok = NUM_TOK;
			tokval = 0;
			while (isdigit(ebuf[epos]))
			{
				tokval = tokval * 10 + ebuf[epos++] - '0';
			}
		}
		else
		{ /* Operator */
			switch(ebuf[epos])
			{
			case '|': tok = OR_TOK; break;
			case '&': tok = AND_TOK; break;
			case '=': tok = EQU_TOK; break;
			case '#': tok = NEQ_TOK; break;
			case '+': tok = ADD_TOK; break;
			case '-': tok = SUB_TOK; break;
			case '*': tok = MUL_TOK; break;
			case '/': tok = DIV_TOK; break;
			case '%': tok = MOD_TOK; break;
			case '(': tok = LFT_TOK; break;
			case ')': tok = RGT_TOK; break;
			case '>':
				if (ebuf[epos+1]=='=')
				{
					epos++;
					tok = GEQ_TOK;
				}
				else tok = GTR_TOK;
				break;
			case '<':
				if (ebuf[epos+1]=='=')
				{
					epos++;
					tok = LEQ_TOK;
				}
				else tok = LES_TOK;
				break;
			}
			epos++;
		}
	}
	else tok=-1;
}

/*
 * PARSER
 */

#if __STDC__
static int _cond_expr(void);
#else
static int _cond_expr();
#endif

/*
 * A primitive expression can be either a
 * conditional expression in parentheses,
 * or an integer.
 */

#if __STDC__
static int _prim_expr(void)
#else
static int _prim_expr()
#endif
{
	int sgn = 1, v;
	if (tok==LFT_TOK)
	{
		nexttok();
		v = _cond_expr();
		if (tok!=RGT_TOK)
		{
			msg = fail;
			return 0;
		}
	}
	else
	{
		if (tok==SUB_TOK)
		{
			sgn = -1;
			nexttok();
		}
		if (tok!=NUM_TOK)
		{
			msg = fail;
			return 0;
		}
		v = sgn * tokval;
	}
	nexttok();
	return v;
}

/*
 * Multiplicative expression
 */

#if __STDC__
static int _mul_expr(void)
#else
static int _mul_expr()
#endif
{
	int v = _prim_expr();
	for (;;)
	{
		if (tok==MUL_TOK)
		{
			nexttok();
			v *= _prim_expr();
		}
		else if (tok==DIV_TOK)
		{
			nexttok();
			v /= _prim_expr();
		}
		else if (tok==MOD_TOK)
		{
			nexttok();
			v %= _prim_expr();
		}
		else return v;
	}
}

/*
 * Additive expression
 */

#if __STDC__
static int _add_expr(void)
#else
static int _add_expr()
#endif
{
	int v = _mul_expr();
	for (;;)
	{
		if (tok==ADD_TOK)
		{
			nexttok();
			v += _mul_expr();
		}
		else if (tok==SUB_TOK)
		{
			nexttok();
			v -= _mul_expr();
		}
		else return v;
	}
}

/*
 * Relational expression
 */

#if __STDC__
static int _rel_expr(void)
#else
static int _rel_expr()
#endif
{
	int v = _add_expr();
	if (tok==GTR_TOK)
	{
		nexttok();
		return (v > _rel_expr());
	}
	else if (tok==GEQ_TOK)
	{
		nexttok();
		return (v >= _rel_expr());
	}
	else if (tok==LEQ_TOK)
	{
		nexttok();
		return (v <= _rel_expr());
	}
	else if (tok==LES_TOK)
	{
		nexttok();
		return (v < _rel_expr());
	}
	else return v;
}

/*
 * Equality expression
 */

#if __STDC__
static int _equ_expr(void)
#else
static int _equ_expr()
#endif
{
	int v = _rel_expr();
	if (tok==EQU_TOK)
	{
		nexttok();
		return (v == _equ_expr());
	}
	else if (tok==NEQ_TOK)
	{
		nexttok();
		return (v != _equ_expr());
	}
	else return v;
}

/*
 * AND conditional expressions
 */

#if __STDC__
static int _and_expr(void)
#else
static int _and_expr()
#endif
{
	int v = _equ_expr();
	if (tok==AND_TOK)
	{
		nexttok();
		return (v && _and_expr());
	}
	else return v;
}

/*
 * OR conditional expressions
 */

#if __STDC__
static int _cond_expr(void)
#else
static int _cond_expr()
#endif
{
	int v = _and_expr();
	if (tok==OR_TOK)
	{
		nexttok();
		return (v || _cond_expr());
	}
	else return v;
}

/*
 * Main entry point. The computed value is stored in the
 * area pointed to by *rtn. `compute' returns NULL upon
 * success, or an error message otherwise.
 */

#if __STDC__
char *compute(char *str, int *rtn)
#else
char *compute(str, rtn)
	char *str;
	int *rtn;
#endif
{
	msg = NULL;
	ebuf = str; epos = 0;
	nexttok();
	*rtn = _cond_expr();
	if (tok!=-1) msg = fail;
	return msg;
}

/***********************************************************
	CURSES LIBRARY
************************************************************/

#ifdef GC_CURSES

#ifdef __MSDOS__

#ifdef OPTIMIZE

static short scrImage[MAX_ROWS][MAX_COLS];
#endif

#endif

/*********************************************************************
	SPECIAL CURSES FUNCTION REPLACEMENTS FOR DOS...
**********************************************************************/

#ifdef __MSDOS__
/*
 * Terminal handling macros (for DOS only, to replace curses)
 *
 * NB - Turbo C's putch routine seem to be a bit buggy, so I haven't
 *	used its conio.h stuff other than clrscr()
 */

typedef unsigned short ushort;

#define SCR_CHAR(c,a)	((a)*256+((unsigned)(c&0xFF)))
#define FAR_PTR(s,o)	(char far *)(( (unsigned long)(s) << 16) + o)
#define SEGMENT(p)	(unsigned short)(((unsigned long)((char far *)p)) >>16)
#define OFFSET(p)	(unsigned short)(((unsigned long)((char far *)p)) &0xFFFF)

/* Attribute Classes for mono */

#define NORML_ATTR	0x07  /* White on black (CGA); normal (mono) 	*/
#define INVRS_ATTR	0x70  /* Black on white (CGA); inverse (mono)	*/

static short
	scr_segbase,
	row,
	col;

char Attribute[4] = { NORML_ATTR, INVRS_ATTR, 0x05, 0x50 };

void BOLD(WINDOW *w)		{ w->attr |=  2;	}
void BOLDEND(WINDOW *w)	{ w->attr &= ~2;	}
void STANDOUT(WINDOW *w)	{ w->attr |=  1;	}
void STANDEND(WINDOW *w)	{ w->attr &= ~1;	}

int setVideoMode(int mode)
{
	if (mode<0 || mode>3)
		if (mode!=7) return 0;
	doIrq(0x10, (unsigned short)mode|0x08, ((uchar)Attribute[0])<<8, 0, 0);
	return 1;
}

static void clearBlock(short ulx, short uly,
		       short brx, short bry)
{
#ifdef OPTIMIZE
	int r, c;
	for (r=uly;r<=bry;r++)
		for (c=ulx;c<=brx;c++)
			scrImage[r][c] = SCR_CHAR(' ',Attribute[0]);
#endif
	doIrq(0x10, 0x0600, Attribute[0]<<8, (uly<<8)+ulx, (bry<<8) + brx);
}

void wclear(WINDOW *w)
{
	clearBlock(w->c,w->r,w->c+w->w-1,w->r+w->h-1);
}

void delwin(WINDOW *w) { wclear(w); free((char *)w); }

void wputstraxy(WINDOW *w, int x, int y, char *str, int a)
{
	unsigned offset; int c;
	row = w->r+y;
	col = w->c+x;
	offset = (COLS*row+col)<<1;
	while ((c = *str++)!='\0')
	{
		if (c=='\t') c = ' '; /* hax */
#ifdef OPTIMIZE
		if (scrImage[row][col]!=SCR_CHAR(c,a)) {
			scrImage[row][col]=SCR_CHAR(c,a);
#endif
#ifdef USE_BIOS
			locateCursor(row,col);
			writeChAttr(c, a);
#else
			/* Talk to h/w */
			*(ushort far *)FAR_PTR(scr_segbase,offset)
				= SCR_CHAR(c,a);
#endif
#ifdef OPTIMIZE
		}
#endif
		if (++col>=COLS)
		{
			row++; col = 0;
		}
		offset += 2;
	}
}

void wputchaxy(WINDOW *w, int x, int y, int c, int a)
{
	unsigned offset;
	row = w->r+y;
	col = w->c+x;
	offset = (COLS*row+col)<<1;
#ifdef OPTIMIZE
	if (scrImage[row][col]!=SCR_CHAR(c,a))
		scrImage[row][col]=SCR_CHAR(c,a);
	else return;
#endif
#ifdef USE_BIOS
	locateCursor(row,col);
	writeChAttr(c, a);
#else
	*(unsigned short far *)FAR_PTR(scr_segbase,offset) = SCR_CHAR(c,a);
#endif
	if (++col>=COLS)
	{
		row++;
		col = 0;
	}
}

void wputchxy(WINDOW *w, int x, int y, int c)
{
	wputchaxy(w, x, y, c, Attribute[w->attr]);
}

void Box(WINDOW *w)
{
	register int i;
	short attr = Attribute[w->attr];
	wputchaxy(w,0,0,218,attr); /* top left */
	for (i=1; i< (w->w-1); ++i)
		wputchaxy(w,i,0,196,attr); /* top left */
	wputchaxy(w,w->w-1,0,191,attr); /* top left */
	for (i=1; i< (w->h-1); ++i)
	{
		wputchaxy(w,0,i,179,attr);
		wputchaxy(w,w->w-1,i,179,attr);
	}
	wputchaxy(w,0,w->h-1,192,attr);	/* bottom left */
	for (i=1; i< (w->w-1); ++i)
		wputchaxy(w,i,w->h-1,196,attr);	/* bottom */
	wputchaxy(w,w->w-1,w->h-1,217,attr);	/* bottom left */
}

WINDOW *newwin(int h, int w, int r, int c)
{
	WINDOW *W = (WINDOW *)calloc(1,sizeof(WINDOW));
	W->r = (char)r;
	W->c = (char)c;
	W->w = (char)w;
	W->h = (char)h;
	W->attr = 0;
	return W;
}

WINDOW *subwin(WINDOW *pw, int h, int w, int r, int c)
{
	(void)pw; /* get around compiler warning */
	return newwin(h,w,r,c);
}

void mvwprintw(WINDOW *w, int r, int c, char *fmt,...)
{
	va_list argptr;
	char buf[82];
	va_start(argptr,fmt);
	vsprintf(buf,fmt,argptr);
	va_end(argptr);
	wputstraxy(w,c,r,buf, Attribute[w->attr]);
}

short wgetch(WINDOW *w)
{
	unsigned short rtn;
	(void)w; /* get around compiler warning */
	_AH=0;
	geninterrupt(0x16);
	rtn = _AX;
	if (rtn & 0x7F) /* ASCII? */
		rtn &= 0x7F;
	return (short)rtn;
}

void selectAttrib(WINDOW *w, int attr)
{
	w->attr = attr;
}

static unsigned short far *scrSave = NULL;

int saveScreen(void)
{
	if (scrSave == NULL)
		scrSave = (unsigned short far *)malloc(sizeof(short)*MAX_ROWS*MAX_COLS);
	return scrSave ? gettext(1,1,MAX_COLS,MAX_ROWS,scrSave) : 0;
}

int restoreScreen(void)
{
	return scrSave ? puttext(1,1,MAX_COLS,MAX_ROWS,scrSave) : 0;
}

void FreeScreens(void)
{
	free(scrSave);
}

void initscr(void)
{
	union REGS rgs;
	(void)int86(0x11,&rgs,&rgs); /* BIOS equipment flag service */
	if ((rgs.x.ax & 0x0030) != 0x0030) { /* CGA? */
		scr_segbase=0xB800;
	} else scr_segbase=0xB000;
}

#endif

/*********************************************************
	,,, for both...
**********************************************************/

#if __STDC__
void setAttrEntry(short attr, short fg, short bg)
#else
void setAttrEntry(attr, fg, bg)
	short attr, fg, bg;
#endif
{
	long a = 0;
/*fprintf(stderr,"In setAttrEntry(%d, %d, %d)\n", (int)attr, (int)fg, (int)bg);*/
#if !defined(NO_COLOR) && !defined(NO_COLOUR)
	if (fg>=0 && allowColor)
		a = Scr_ColorAttr(fg,bg);
	else 
#endif
	{
#ifdef __MSDOS__
		if ((unsigned)scr_segbase==0xB800) /* must be colour; fake it */
		{
			a = Scr_ColorAttr(7,0);
			if (bg & 2) a = Scr_ColorAttr(0,7);
			if (bg & 4)
				if (bg & 2)
					a = Scr_ColorAttr(1,7);
				else
					a = Scr_ColorAttr(15,0);
			if (bg & 8) a |= 0x22; /* just ensure underline is something different */
			if (bg & 1) a |= 0x80; /* this is same as for mono */
		} else
		{
			/* real mono */
			if (bg & 1) a |= 0x80;
			if (bg & 2) a |= 0x77;
			if (bg & 4) a |= 0x08;
			if (bg & 8) a |= 0x01;
		}
#else
		if (bg == 0) a = A_NORMAL;
		else
		{
			if (bg & 1) a |= A_BLINK;
			if (bg & 2) a |= A_REVERSE;
			if (bg & 4) a |= A_BOLD;
			if (bg & 8) a |= A_UNDERLINE;
		}
#endif
	}
	Attribute[attr] = a;
/*fprintf(stderr,"At end, Attr[%d] is %ld\n", (int)attr, a);*/
}

#if __STDC__
void invalidateScreen(void)
#else
void invalidateScreen()
#endif
{
#ifdef __MSDOS__
#ifdef OPTIMIZE
	int r, c;
	for (r=0;r<MAX_ROWS;r++)
		for (c=0;c<MAX_COLS;c++)
			scrImage[r][c]=-1;
#endif /* OPTIMIZE */
#else
#ifdef NO_REDRAWWIN
	wclear(stdscr);
#else
	redrawwin(stdscr);
#endif
#endif
}

#if __STDC__
int simpleEditString(int c, char *start, char **end, char **now,
			int *_insmode, int maxlen)
#else
int simpleEditString(c, start, end, now, _insmode, maxlen)
	int c, *_insmode, maxlen;
	char *start, **end, **now;
#endif
{
	char *tptr;
	static int insmode = 1;
	if (*_insmode) insmode = *_insmode;
	switch (c)
	{
	case 27:
		if (*start)
		{
			*now=*end=start;
			*start=0;
		}
		else return 1;
		break;
	case 8:
		if ((tptr=*now-1) >= start)
		{
			tptr=*now-1;
			while (tptr<*end)
			{
				*tptr = *(tptr+1);
				tptr++;
			}
			(*now)--; (*end)--;
		}
		break;
	case KEY_DEL:
		if (*now < *end)
		{ /* Delete under cursor */
			tptr=*now;
			while (tptr<*end)
			{
				*tptr = *(tptr+1);
				tptr++;
			}
			(*end)--;
			if (*now > *end) *now=*end;
      		}
		break;
	case KEY_INS:
		if (_insmode) insmode = *_insmode = (1 - *_insmode);
		else insmode = 1-insmode;
		break;
	case KEY_RIGHT:
		if (*now<*end) (*now)++;
		break;
	case KEY_LEFT:
		if (*now>start) (*now)--;
		break;
	case KEY_HOME:
		*now = start; /* home */
		break;
	case KEY_END:
		*now = *end; /* end */
		break;
	default: if (isprint(c&0xFF))
		{
			if (insmode || (*now >= *end))
			{
				/* If inserting, shift line to make space */
				if (maxlen==-1 || (*end-start)<maxlen)
				{
					tptr = ++(*end);
					while (tptr>*now)
					{
						*tptr = *(tptr-1);
						tptr--;
					}
				} else BEEP;
			}
			/* Put the character in at the appropriate position */
			*((*now)++) = (char)c;
			*(*end)=0;
		}
		break;
	}
	return ((*end-start)>=maxlen);
}

#if __STDC__
void myRead(WINDOW *w, int width, int r,int c, char *buf,int len, int *mode,
		editFunc_t editFn, int (*reader)(void))
#else
void myRead(w, width, r, c, buf, len, mode, editFn, reader)
	WINDOW *w;
	int width, r, c, len, *mode;
	char *buf;
	editFunc_t editFn;
	int (*reader)();
#endif
{
	static char editBuff[120];
	char *end, *now;
	int ch, offset, _mode=1;
	if (mode==NULL) mode=&_mode;
	if (editFn==NULL) editFn = simpleEditString;
	strcpy(end=editBuff,buf);
	while (*end) end++;
	now = end;
	if (len>=120) len = 119;
	for (;;)
	{
		char tc;
		int pos;
		/* This is a scrolling window so work out where to start... */
		offset = (end-editBuff) - (width-4-c);
		if (offset < 0) offset = 0;
		if ((now-editBuff) < offset) offset = (now-editBuff);
		/* do the cursor and the rest of the buffer */
		selectAttrib(w,1);
		pos = c+now-editBuff-offset;
		if (*now)
		{
			wputchxy(w,pos,r,*now);
			selectAttrib(w,0);
			mvwprintw(w,r,pos+1,now+1);
			pos = c+end-editBuff-offset;
		}
		else
		{
			/* cursor is at end of buffer */
			wputchxy(w,pos++,r,' ');
			selectAttrib(w,0);
		}
		while (pos<width)
			wputchxy(w,pos++,0,' ');
		wputchxy(w,width-1,0,(*mode)?'+':'-');
		/* Write everything up to the cursor at the end
			to get physical cursor in same place */
		tc = *now; *now = '\0';
		mvwprintw(w,r,c,editBuff+offset);
		*now = tc;
		UPDATE(w); REFRESH();
		ch = (*reader)();
		/* Restore the default attribute at the cursor position */
		wputchxy(w,pos,r,tc?tc:' ');
		if (ch=='\n' || ch=='\r' ||
			(*editFn)(ch, editBuff, &end, &now, mode, len)) 
				break;
	}
	strcpy(buf,editBuff);
}

/*********************************************************************
	...AND FOR UNIX...
**********************************************************************/

#ifndef __MSDOS__

long Attribute[4] = { 0, 0, 0, 0 };

#if	__STDC__
void clrscr(void)
#else
void clrscr()
#endif
{
	wclear(stdscr);
	UPDATE(stdscr);
}

#if __STDC__
void STANDOUT(WINDOW *w)
#else
void STANDOUT(w)
	WINDOW *w;
#endif
{
#if !defined(NO_COLOR) && !defined(NO_COLOUR)
	if (allowColor) wattrset(w,COLOR_PAIR(2));
	else
#endif
#ifdef NO_STANDOUT
		wattron(w,A_REVERSE);
#else
		wstandout(w);
#endif
}

#if __STDC__
void STANDEND(WINDOW *w)
#else
void STANDEND(w)
	WINDOW *w;
#endif
{
#if !defined(NO_COLOR) && !defined(NO_COLOUR)
	if (allowColor) wattrset(w,COLOR_PAIR(1));
	else
#endif
#ifdef NO_STANDOUT
		wattroff(w,A_REVERSE);
#else
		wstandend(w);
#endif
}

#if __STDC__
void selectAttrib(WINDOW *w, int attrib)
#else
void selectAttrib(w, attrib)
	WINDOW *w;
	int attrib;
#endif
{
#if !defined(NO_COLOR) && !defined(NO_COLOUR)
	if (allowColor)
	{
		/* sanity check... */
		/*fprintf(stderr,"Before: Attribute[%d] = %X\n", attrib, Attribute[attrib]);*/
		Attribute[attrib] &= 0x77;
		init_pair((short)attrib+1,Attribute[attrib]&0xF,Attribute[attrib]>>4);
		wattrset(w,COLOR_PAIR((attrib+1)));
	} else
#endif
		wattrset(w,Attribute[attrib]);
}

#endif /* UNIX */

#endif /* GC_CURSES */

/****************************************************

	GC3 REGULAR EXPRESSION MATCHING

 Supports both BSD and System V regular expressions,
 as well as DOS file patterns. The latter are explicitly
 coded here; the others rely on library routines.

 Entry points to the code in this file are:

	char *compileRE(char *RE)
	int  matchRE(char *string)

 The former is used to prepare the regular expression
 while the latter returns a non-zero value if the
 given string matches the last regular expression set
 up with compileRE.

 (c) 1993 by Graham Wheeler

*****************************************************/

#ifdef USE_SYSV_REGEXP
static char REbuf[1024];
#else
static char REbuf[14];
#endif
static char *REpre, *REsuf; /* For DOS-style matching */

/*****************************************************
	SYSTEM V SPECIFIC REGEXPS
******************************************************/

#ifdef USE_SYSV_REGEXP

static char *REmsg, *REsrc;

#define INIT		register char *sp=REsrc;
#define GETC()		(*sp++)
#define UNGETC(c)	(sp--)
#define RETURN(c)	return c;
#define ERROR		REerr
#define PEEKC()		(*sp)

#if __STDC__
static void REerr(int c)
#else
static void REerr(c) 
	int c;
#endif
{
	switch(c) {
	case 11: REmsg = "range endpoint too large"; break;
	case 16: REmsg = "bad number"; break;
	case 25: REmsg = "\\ digit out of range"; break;
	case 36: REmsg = "illegal or missing delimeter"; break;
	case 41: REmsg = "no remembered search string"; break;
	case 42: REmsg = "\\( \\) imbalance"; break;
	case 43: REmsg = "too many \\('s"; break;
	case 44: REmsg = "more than two numbers in \\{ \\}"; break;
	case 45: REmsg = "} expected after \\"; break;
	case 46: REmsg = "first number exceeds second in \\{ \\}"; break;
	case 49: REmsg = "[ ] imbalance"; break;
	case 50: REmsg = "regular expression overflow"; break;
	default: REmsg = "unknown error"; break;
	}
}

#include <regexp.h>

#if __STDC__
char *compileRE(char *RE)
#else
char *compileRE(RE)
	char *RE;
#endif
{
	REmsg = NULL;
	REsrc = RE;
	(void)compile((char *)NULL,REbuf,REbuf+1024,'\0');
	return REmsg;
}

#if __STDC__
int  matchRE(char *string) 
#else
int matchRE(string)
	char *string;
#endif
{
	return (step(string,REbuf));
}
#endif

/*****************************************************
	BSD SPECIFIC REGEXPS
******************************************************/

#ifdef USE_BSD_REGEXP

#if __STDC__
char *compileRE(char *RE)
#else
char *compileRE(RE)
	char *RE;
#endif
{
	return (char *)re_comp(RE);
}

#if __STDC__
int  matchRE(char *string)
#else
int matchRE(string)
	char *string;
#endif
{
	return (re_exec(string)==1);
}

#endif

/*****************************************************
	DOS SPECIFIC STUFF
******************************************************/

#if __STDC__
static int matchPart(char *pat, char *str)
#else
static int matchPart(pat, str)
	char *pat, *str;
#endif
{
	if (pat==NULL)
		return *str;
	for ( ; *str ; str++, pat++)
	{
		if (*pat == '*')
			return 0;
		else if (*pat!='?' && *pat != *str)
			return 1;
	}
	return !(*pat=='*' || *pat==0 || *pat=='.');
}

#if __STDC__
char *DOScompileRE(char *RE)
#else
char *DOScompileRE(RE)
	char *RE;
#endif
{
	strcpy(REbuf,RE);
	STRUPR(REbuf);
	REpre = REbuf;
	REsuf = STRCHR(REbuf,'.');
	if (REsuf) *REsuf++ = '\0';
	return NULL;
}

#if __STDC__
int  DOSmatchRE(char *string)
#else
int  DOSmatchRE(string)
	char *string;
#endif
{
	char pat[80];
	char *pre, *suf, *tmp;
	int rtn = 0;
	strncpy(pat,string,79);
	pat[79] = '\0';
	STRUPR(pat);
	pre=pat;
	suf = STRCHR(pat,'.');
	if ((tmp=suf)!=NULL)
		*suf++ = '\0';
	else suf = "";
	if (matchPart(REpre,pre)==0 && matchPart(REsuf,suf)==0)
		rtn = 1;
	if (tmp) *tmp='.'; /* fixup */
	return rtn;
}

/******************************************************
	GC3 Directory Reading
******************************************************/

/* Once upon a time this was all simple, using opendir,
   readdir and closedir. Then I decided to handle hidden
   files. Trivial under UNIX, but a pain under DOS. This
   is the mess that resulted....
*/

#ifdef __MSDOS__
static int startDir = 1;
static char dirPath[MAXPATHNAME];
#else
#  ifdef NO_DIRENT_H
  static int startDir = -1;
#  else
  static DIR *startDir = NULL;
#  endif
#endif

#if __STDC__
void *initDir(char *pname)
#else
void *initDir(pname)
	char *pname;
#endif
{
#ifdef __MSDOS__
	startDir = 1;
	strcpy(dirPath,pname);
	if (dirPath[strlen(dirPath)-1]!='\\')
		strcat(dirPath,"\\*.*");
	else strcat(dirPath,"*.*");
	return (void *)1;
#else
#  ifdef NO_DIRENT_H
	return (void *)((startDir = open(pname,0,0))+1);
#  else
	return (void *)(startDir = opendir(pname));
#  endif
#endif
}

#if __STDC__
#ifdef __MSDOS__
char *getDirEntry(int hidden, struct ffblk *fdat)
#else
char *getDirEntry(void *dir, int hidden)
#endif
#else
char *getDirEntry(dir, hidden)
	void *dir;
	int hidden;
#endif
{
#if __MSDOS__
	struct ffblk fdat2;
	if (fdat==NULL) fdat=&fdat2;
	if (startDir)
	{
		startDir = 0;
		if (findfirst(dirPath,fdat,
			FA_RDONLY|FA_DIREC|FA_ARCH|(hidden ? FA_HIDDEN : 0)))
				return NULL;
	} else
		if (findnext(fdat)) return NULL;
	return fdat->ff_name;
#else
#ifdef NO_DIRENT_H
	static struct direct de;
	if ((int)dir>=0)
	{
		while (read((int)dir,&de,sizeof(de))==sizeof(de))
		{
			if (!hidden && de.d_name[0]=='.'
				&& strcmp(de.d_name,"..")!=0)
					continue;
			else return de.d_name;
		}
	}
	return NULL;
#else
	struct dirent *de;
	if ((DIR *)dir)
	{
		while ((de=(struct dirent *)readdir((DIR *)dir))!=NULL)
		{
			if (!hidden && de->d_name[0]=='.'
				&& strcmp(de->d_name,"..")!=0)
					continue;
			else return de->d_name;
		}
	}
	return NULL;
#endif
#endif
}

#if	__STDC__
void closeDir(void *dir)
#else
void closeDir(dir)
	void *dir;
#endif
{
#ifndef __MSDOS__
#  ifdef NO_DIRENT_H
	close((int)dir);
#  else
	closedir((DIR *)dir);
#  endif
#else
	(void)dir; /* Just to get around the compiler warning */
#endif
}

