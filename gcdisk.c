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
#include "gc3.h"
#include "gcops.h"

#define GC_CURSES
#include "gclib.h"

/* Name of file holding old paths */

long freeSpace[2], usedSpace[2];

fInfo_t *fInfo[2];		/* Directory info for each panel */

char paths[2][MAXPATHNAME];

/* CONTAINER FILE RULES */

/* string fields */

#define R_NAME			0
#define R_TYPE			1
#define R_LIST			2
#define R_ADD			3
#define R_EXTRACT		4
#define R_DELETE		5

/* single digit fields */

#define R_FIELDS		0
#define R_ITEM			1
#define R_SIZE			2

/* field and offset pair fields */

#define R_YEAR			0
#define R_MONTH		1
#define R_DAY			2
#define R_HOUR			3
#define R_MIN			4
#define R_SEC			5

#define MAX_RULES		8

typedef struct
{
    char *sf[R_DELETE + 1];
    char sif[R_SIZE + 1];
    char dif[R_SEC + 1][2];
} rule_t;

static rule_t Rules[MAX_RULES];
static int numRules = 0;

static char *oldSelNames[MAXARGS] =
{NULL};
static int totfiles[2] =
{0, 0};

/******************************************************************
					FILE LEAK DEBUGGING
*******************************************************************/

#ifdef FILE_DEBUG

#include <fcntl.h>

static int fileOwners[40] =
{0};
int fileDebugID = 1;

#if __STDC__
FILE *gc_fopen(char *n, char *m, char *fn, int ln, int ID)
#else
FILE *gc_fopen(n, m, fn, ln, ID)
    char *n, *m, *fn;
    int ln, ID;
#endif
{
    FILE *rtn = fopen(n, m);
    if (rtn)
    {
	fprintf(stderr, "%2d (%s) fopened by %s line %d (ID %d)\n",
		fileno(rtn), n, fn, ln, ID);
	fileOwners[fileno(rtn)] = ID;
    }
    return rtn;
}

#if __STDC__
int gc_fclose(FILE * f, char *fn, int ln)
#else
int gc_fclose(f, fn, ln)
    FILE *f;
    char *fn;
    int ln;
#endif
{
    if (f)
    {
	fprintf(stderr, "%2d fclosed by %s line %d, ID %d\n",
		fileno(f), fn, ln, fileOwners[fileno(f)]);
	fileOwners[fileno(f)] = 0;
    }
    else
	fprintf(stderr, "   Illegal fclose(NULL) by %s line %d\n", fn, ln);
    return f ? fclose(f) : 0;
}

#if __STDC__
int gc_open(char *n, int m, int a, char *fn, int ln, int ID)
#else
int gc_open(n, m, a, fn, ln, iD)
    char *n, *fn, int m, a, int ln, ID;
#endif
{
    int rtn = open(n, m, a);
    if (rtn >= 0)
    {
	fprintf(stderr, "%2d (%s) opened by %s line %d, ID %d\n",
		rtn, n, fn, ln, ID);
	fileOwners[rtn] = ID;
    }
    return rtn;
}

#if __STDC__
int gc_close(int h, char *fn, int ln)
#else
int gc_close(h, fn, ln)
    int h, ln;
    char *fn;
#endif
{
    if (h >= 0)
    {
	fprintf(stderr, "%2d closed by %s line %d (ID %d)\n",
		h, fn, ln, fileOwners[h]);
	fileOwners[h] = 0;
    }
    else
	fprintf(stderr, "   Illegal fclose(%d) by %s line %d\n",
		h, fn, ln);
    return (h >= 0) ? close(h) : 0;
}

#if __STDC__
int gc_dup(int h, char *fn, int ln, int ID)
#else
int gc_dup(h, fn, ln, ID)
    int h, ln, ID;
    char *fn;
#endif
{
    int rtn = -1;
    if (h >= 0)
	rtn = dup(h);
    if (rtn >= 0)
    {
	fprintf(stderr, "%2d duplicated to %d by %s line %d, old ID %d, new ID %d\n",
		h, rtn, fn, ln, fileOwners[h], ID);
	fileOwners[rtn] = ID;
    }
    else
	fprintf(stderr, "   dup(%d) by %s line %d failed! (old ID %d)\n",
		h, fn, ln, fileOwners[h]);
    return rtn;
}

#endif

#if __STDC__
void ShowOpenFiles(void)
#else
void ShowOpenFiles()
#endif
{
#ifdef FILE_DEBUG
    int i;
    for (i = 0; i < 40; i++)
	if (fileOwners[i])
	    fprintf(stderr, "%d still open, ID %d\n", i, fileOwners[i]);
#endif
}

/***************************************************************
		BUILD PERMISSIONS STRING FROM FILE MODE
****************************************************************/

#ifndef __MSDOS__

#if	__STDC__
char *getPerms(mode_t tmd)
#else
char *getPerms(tmd)
    mode_t tmd;
#endif
{
    static char perms[14];
    int i = 0;
    perms[i++] = (tmd & S_ISUID) ? 's' : '-';
    perms[i++] = (tmd & S_ISGID) ? 'S' : '-';
#ifdef S_ISVTX
    perms[i++] = (tmd & S_ISVTX) ? 't' : '-';
#endif
#ifdef S_IRUSR
    perms[i++] = (tmd & S_IRUSR) ? 'r' : '-';
    perms[i++] = (tmd & S_IWUSR) ? 'w' : '-';
    perms[i++] = (tmd & S_IXUSR) ? 'x' : '-';
    perms[i++] = (tmd & S_IRGRP) ? 'r' : '-';
    perms[i++] = (tmd & S_IWGRP) ? 'w' : '-';
    perms[i++] = (tmd & S_IXGRP) ? 'x' : '-';
    perms[i++] = (tmd & S_IROTH) ? 'r' : '-';
    perms[i++] = (tmd & S_IWOTH) ? 'w' : '-';
    perms[i++] = (tmd & S_IXOTH) ? 'x' : '-';
#else /* None-POSIX; owners permissions only */
    perms[i++] = (tmd & S_IREAD) ? 'r' : '-';
    perms[i++] = (tmd & S_IWRITE) ? 'w' : '-';
    perms[i++] = (tmd & S_IEXEC) ? 'x' : '-';
#endif
    perms[i++] = 0;
    return perms;
}

#endif

/*****************************************************************
	CHECK IF FILE IS OF A CERTAIN TYPE(S)
*******************************************************************/

#if __STDC__
int checkType(mode_t m, short f)
#else
int checkType(m, f)
    mode_t m;
    short f;
#endif
{
    int rtn = 0;
    if (m & 128)
    {
    }
#ifndef __MSDOS__
    if (S_ISLNK(m) && (f & 64))
	rtn = 1;
#endif
    if (S_ISCHR(m) && (f & 4))
	rtn = 1;
    if (S_ISBLK(m) && (f & 8))
	rtn = 1;
    if (S_ISFIFO(m) && (f & 2))
	rtn = 1;
    if (S_ISDIR(m) && (f & 1))
	rtn = 1;
    if (S_ISREG(m))
    {
	if (f & 32)
#ifdef S_IXUSR
	    if ((m & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0)
#else
	    if ((m & (S_IEXEC)) != 0)
#endif
		rtn = 1;
	if (f & 16)
	    rtn = 1;
    }
    return rtn;
}

/*********************************************************
			DIRECTORY FILTERING
**********************************************************/

#if __STDC__
void refilter(int L, char *oldname)
#else
void refilter(L, oldname)
    int L;
    char *oldname;
#endif
{
    int j = 0;
    if (!isListWindow(L))
	return;
    if (filtering[L])
    {
	int i;
	(*(compileRegexp)) (filters[L]);
	for (i = 0; i < totfiles[L]; i++)
	{
	    register char *n = fInfo[L][i].name;
	    if (IsDirectory(L,i) || ((*(matchRegexp)) (n)) == 1)
		fIndex[L][j++] = i;
	    else
		nselect(L, i, 0);
	}
    }
    else for ( ; j < totfiles[L]; j++)
	    fIndex[L][j] = j;
    numfiles[L] = j;
    sortList(L, oldname);
    normalise(L);
}

/**********************************************************
		FIND A CONTAINER FILE RULE FOR A FILE
***********************************************************/

#if __STDC__
static int findRule(char *path)
#else
static int findRule(path)
    char *path;
#endif
{
    int rule, first = 1;
    int b, p;
    char *name = STRRCHR(path, PATH_SEP);	/* get the basename in `name' */
    if (name == NULL)
	name = path;
    if (numRules <= 0)
	return -1;
    b = GRABBUF();
    p = GRABBUF();
    for (rule = 0; rule < numRules; rule++)
    {
	/* If the name rule is present and fails, continue */

	if (Rules[rule].sf[R_NAME])
	    if (!doesMatch(Rules[rule].sf[R_NAME], name, 0))
		continue;

	/* if there is a type rule, we run `file' if it hasn't
			been done yet, and then apply the rule, continuing if
			it fails */

	if (Rules[rule].sf[R_TYPE])
	{
	    if (first)
	    {
		sprintf(BUF[b], "file %s", path);
		runCommand(BUF[b], 1);
		first = 0;
	    }
	    if (!doesMatch(Rules[rule].sf[R_TYPE], valspace[VAR_LINEBUFF], 0))
		continue;
	}

	/* Both rules passed! exit the loop */

	break;
    }
    if (rule >= numRules)
	rule = -1;		/* exited loop without finding a rule */
    FREEBUF(p);
    FREEBUF(b);
    return rule;
}

/****************************************************************
	SPLIT A PATH AND IDENTIFY THE FILE TYPE
****************************************************************/

/*
 * This routine, given a path, splits the path into
 * a part that corresponds to something that exists, and something
 * (if anything) that doesn't. On entry `exist' and `nonexist'
 * should point to a buffer big enough to hold the full absolute
 * pathname + 2 bytes. The returned existing part is a normalised,
 * absolute path.
 *
 * The return value is a file type code for the exist part, one of:
 *
 *			 n - existing file is a container; rule number n
 * 		-3 - existing file is a directory
 *			-2 - existing file is a device or pipe
 *			-1 - other
 */

#if __STDC__
int splitPath(char *path, char *exist, char *nonexist)
#else
int splitPath(path, exist, nonexist)
    char *path, *exist, *nonexist;
#endif
{
    char *p;
    strcpy(nonexist, path);	/* Use nonexist as workspace */
    fixPath(nonexist, exist);
    *nonexist = '\0';
    for (;;)
    {
	/* does it exist? */
	if (access(exist, 0) == 0)
	{
	    mode_t m = getFileMode(exist);
	    if (checkType(m, 1))
		return -3;
	    else if (checkType(m, 2 | 4 | 8))
		return -2;
	    else
		return findRule(exist);
	}
	/* didn't exist - shunt up to parent */
	p = STRRCHR(exist, PATH_SEP);
	assert(p);
	strcpy(nonexist, makePath(p + 1, nonexist));
#ifdef __MSDOS__
	if (p <= (exist + 2))
#else
	if (p <= exist)
#endif
	{
	    *++p = '\0';
	    return -3;		/* root directory */
	}
	*p = '\0';
    }
}

/**********************************************************
	ROUTINES TO GET INFO ABOUT FILES
***********************************************************/

#if __STDC__
static struct stat *getFileInfo(char *pathname)
#else
static struct stat *getFileInfo(pathname)
    char *pathname;
#endif
{
    static struct stat st;
#ifdef NO_SYMLINKS
    if (stat(pathname, &st))
#else
    int fl = testOption(VAR_FOLLOWLINKS);
    if ((fl && stat(pathname, &st) != 0) ||
	(!fl && lstat(pathname, &st) != 0))
#endif
    {
	int buf = GRABBUF();
	sprintf(BUF[buf], "Cannot stat %s!", pathname);
	showMsg(BUF[buf]);
	FREEBUF(buf);
	st.st_size = 0;
	st.st_mode = 0;
	st.st_uid = 0;
	st.st_gid = 0;
	st.st_mtime = 0;
	st.st_atime = 0;
    }
    return &st;
}

#if __STDC__
off_t getFileSize(char *pathname)
#else
off_t getFileSize(pathname)
    char *pathname;
#endif
{
    return getFileInfo(pathname)->st_size;
}

#if __STDC__
mode_t getFileMode(char *pathname)
#else
mode_t getFileMode(pathname)
    char *pathname;
#endif
{
    return getFileInfo(pathname)->st_mode;
}

#if __STDC__
gid_t getFileGroup(char *pathname)
#else
gid_t getFileGroup(pathname)
    char *pathname;
#endif
{
    return getFileInfo(pathname)->st_gid;
}

#if __STDC__
uid_t getFileUser(char *pathname)
#else
uid_t getFileUser(pathname)
    char *pathname;
#endif
{
    return getFileInfo(pathname)->st_uid;
}

#if __STDC__
time_t getFileModTime(char *pathname)
#else
time_t getFileModTime(pathname)
    char *pathname;
#endif
{
    return getFileInfo(pathname)->st_mtime;
}

#if __STDC__
time_t getFileAccTime(char *pathname)
#else
time_t getFileAccTime(pathname)
    char *pathname;
#endif
{
    return getFileInfo(pathname)->st_atime;
}

/***************************************************************
		DATE FIXER NEEDED FOR BORLAND C
****************************************************************/

#ifdef __MSDOS__
static time_t normaliseDate(unsigned yr, unsigned mon, unsigned day,
			     unsigned hr, unsigned mins, unsigned sec)
{
    struct date dt;
    struct time tm;
    tm.ti_hour = hr;
    tm.ti_min = mins;
    tm.ti_sec = sec;
    dt.da_year = (yr < 100) ? (1900 + yr) : yr;
    dt.da_mon = mon;
    dt.da_day = day;
    /* Borland's dostounix crashes on bad dates, so we do a reality check */
    if (dt.da_year < 1980)
	dt.da_year = 1980;
    if (dt.da_year > 1999)
	dt.da_year = 1999;
    if (dt.da_mon < 1)
	dt.da_mon = 1;
    if (dt.da_mon > 12)
	dt.da_mon = 12;
    if (dt.da_day > 31)
	dt.da_day = 31;
    if (dt.da_day < 1)
	dt.da_day = 1;
    if (tm.ti_hour > 23)
	tm.ti_min = 23;
    if (tm.ti_min > 59)
	tm.ti_min = 59;
    if (tm.ti_sec > 59)
	tm.ti_sec = 59;
    return (time_t) dostounix(&dt, &tm);
}

#endif

/************************************************************
	ADD AN ENTRY TO THE DIRECTORY LIST
*************************************************************/

#if __STDC__
#ifdef __MSDOS__
static int addEntry(int n, int i, char *fname,
		     int dpth, int dirOnly, struct ffblk * fdat)
#else
static int addEntry(char *pathname, int n, int i, char *fname,
		     int dpth, int dirOnly)
#endif
#else
static int addEntry(pathname, n, i, fname, dpth, dirOnly)
    char *pathname, *fname;
    int n, i, dpth, dirOnly;
#endif
{
    fInfo_t *f = &fInfo[n][i];
#ifndef __MSDOS__
    int fl = testOption(VAR_FOLLOWLINKS);
    struct stat *st = getFileInfo(pathname);
    {
	register mode_t m = st->st_mode;
	if (!fl && !dirOnly && S_ISLNK(m))
	    f->typID = 'l';
	else if (S_ISDIR(m))
	    f->typID = PATH_SEP;
	else if (dirOnly)
	    return 0;
	else if (S_ISCHR(m))
	    f->typID = 'c';
	else if (S_ISBLK(m))
	    f->typID = 'b';
	else if (S_ISFIFO(m))
	    f->typID = 'p';
#ifdef S_IXUSR
	else if (m & (S_IXUSR | S_IXGRP | S_IXOTH))
	    f->typID = '*';
#else
	else if (m & (S_IEXEC))
	    f->typID = '*';
#endif
	else if (fl && S_ISLNK(m))
	    f->typID = 'l';
	else
	    f->typID = ' ';
    }
    f->size = st->st_size;
    f->mode = st->st_mode;
    f->uid = st->st_uid;
    f->gid = st->st_gid;
    f->modtime = st->st_mtime;
    f->acctime = st->st_atime;
#else
    /* Handle special case of "." for recovery... */
    if (strcmp(fname, ".") == 0)
    {
	struct stat *st = getFileInfo(".");
	f->typID = PATH_SEP;
	f->size = st->st_size;
	f->mode = st->st_mode;
	f->modtime = st->st_mtime;
	f->acctime = st->st_atime;
    }
    else
    {
	struct time tm;
	struct date dt;
	/* re-use info in fdat ffblk for efficiency */
	if (fdat->ff_attrib & FA_DIREC)
	    f->typID = PATH_SEP;
	else if (dirOnly)
	    return 0;
	else if (fdat->ff_attrib & FA_SYSTEM)
	    f->typID = 's';
	else if (fdat->ff_attrib & FA_HIDDEN)
	    f->typID = 'h';
	else if (fdat->ff_attrib & FA_RDONLY)
	    f->typID = 'r';
	/*	  	else if (m & (S_IEXEC)) f->typID = '*';*/
	else
	    f->typID = ' ';
	f->size = fdat->ff_fsize;
	f->mode = fdat->ff_attrib;
	f->uid = f->gid = 0;
	f->acctime = f->modtime = normaliseDate(
				      1980 + ((fdat->ff_fdate >> 9) & 0x3f),	/* years */
						(fdat->ff_fdate >> 5) & 0xf,	/* months */
						   fdat->ff_fdate & 0x1f,	/* days */
					      (fdat->ff_ftime >> 11) & 0x1f,	/* hours */
					       (fdat->ff_ftime >> 5) & 0x3f,	/* mins */
					      (fdat->ff_ftime & 0x1f) * 2l);	/* secs */
    }
#endif
    if (f->name)
	free(f->name);
    if ((f->name = calloc(strlen(fname) + 1, sizeof(char))) != NULL)
	strcpy(f->name, fname);
    f->flag = 0;
    f->depth = dpth;
    f->parent = -1;
    fIndex[n][i] = i;
    return 1;
}

/*******************************************************************
	SAVE AND RESTORE FILE SELECTIONS BETWEEN WINDOW RESCANS
********************************************************************/

#if __STDC__
static int saveSelected(int n)
#else
static int saveSelected(n)
    int n;
#endif
{
    int i, j;
    for (i = j = 0; i < totfiles[n]; i++)
    {
	if (fInfo[n][i].flag & F_SELECTED)
	{
	    oldSelNames[j++] = fInfo[n][i].name;
	    fInfo[n][i].name = NULL;
	}
    }
    return j;
}

#if __STDC__
static void reselect(int n, int i, int *selected)
#else
static void reselect(n, i, selected)
    int n, i, *selected;
#endif
{
    int k;
    for (k = 0; k < (*selected); k++)
    {
	if (strcmp(oldSelNames[k], fInfo[n][i].name) == 0)
	{
	    fInfo[n][i].flag = F_SELECTED;
	    selCnt[n]++;
	    selSize[n] += fInfo[n][i].size;
	    (*selected)--;
	    free(oldSelNames[k]);
	    oldSelNames[k] = oldSelNames[*selected];
	    oldSelNames[*selected] = NULL;
	    break;
	}
    }
}

/*************************************************************
	BUILD THE FULL FULL PATH OF A TREE VIEW ENTRY
**************************************************************/

#ifdef __MSDOS__
static struct ffblk dStk[20];
#else
static void *dStk[20];
#endif

#define MAXTREE	20

#if __STDC__
void getTreePath(int n, char *buf)
#else
void getTreePath(n, buf)
    int n;
    char *buf;
#endif
{
    int f = fIndex[n][highlight[n]];
    int stk[MAXTREE], i, Len = strlen(paths[n]);
    strcpy(buf, paths[n]);
    if (Len && buf[Len - 1] == PATH_SEP)
	buf[Len - 1] = '\0';
    for (i = 0; i < MAXTREE; i++)
    {
	stk[i] = f;
	if (fInfo[n][f].parent < 0)
	    break;
	else
	    f = fInfo[n][f].parent;
    }
    for (;;)
    {
	strcat(buf, PATH_SEP_STR);
	strcat(buf, fInfo[n][stk[i]].name);
	if (i-- <= 0)
	    break;
    }
}

/********************************************************
	DIRECTORY TREE VIEWING
*********************************************************/

static int treeDepth = 0;

#if __STDC__
static void readTree(int n, char *path, int *cnt, int parent, int depth)
#else
static void readTree(n, path, cnt, parent, depth)
    int n, depth, *cnt, parent;
    char *path;
#endif
{
    int buf;
    char *fname;

    if (depth >= treeDepth || *cnt >= MAXARGS)
    {
	if (*cnt >= MAXARGS)
	    showMsg("Too many directories - some will be dropped");
	return;
    }
#ifdef __MSDOS__
    if (initDir(path) == NULL)
#else
    if ((dStk[depth] = initDir(path)) == NULL)
#endif
    {
	showMsg("initDir failed!");
	return;			/* Should produce an error */
    }
    if ((buf = GRABBUF()) == 0)
	return;
    while ((*cnt) < MAXARGS &&
#ifdef __MSDOS__
    (fname = getDirEntry(testOption(VAR_SHOWHIDDEN), &dStk[depth])) != NULL)
#else
     (fname = getDirEntry(dStk[depth], testOption(VAR_SHOWHIDDEN))) != NULL)
#endif
    {
	if (strcmp(fname, ".") == 0)
	    continue;
	if (strcmp(fname, "..") == 0)
	    continue;
	strcpy(BUF[buf], path);
#ifdef __MSDOS__
	if (BUF[buf][strlen(BUF[buf]) - 1] != '\\')
	    strcat(BUF[buf], "\\");
	strcat(BUF[buf], fname);
	if (addEntry(n, (*cnt), fname, depth, 1, &dStk[depth]))
#else
	if (BUF[buf][strlen(BUF[buf]) - 1] != '/')
	    strcat(BUF[buf], "/");
	strcat(BUF[buf], fname);
	if (addEntry(BUF[buf], n, (*cnt), fname, depth, 1))
#endif
	{
	    fInfo[n][*cnt].parent = parent;
	    *cnt += 1;
	    if ((*cnt) >= MAXARGS)
		break;
	    readTree(n, BUF[buf], cnt, (*cnt) - 1, depth + 1);
	}
    }
#ifndef __MSDOS__
    closeDir((void *) dStk[depth]);
#endif
    FREEBUF(buf);
}

/*****************************************************
	DIRECTORY CHANGING
******************************************************/

#ifdef __MSDOS__
void mychdir(char *d)
{
    if (d[1] == ':')
    {
	(void) _chdrive(toupper(d[0]) - 'A' + 1);
	d += 2;
    }
    (void) chdir(d);
}

#endif

#if	__STDC__
void cd2old(void)
#else
void cd2old()
#endif
{
    CHDIR(paths[l]);
}

#if	__STDC__
void doCD(char *d)
#else
void doCD(d)
    char *d;
#endif /* __STDC__ */
{
    char *p = NULL;
    int ex = GRABBUF(), ne = GRABBUF();
    int rtn = 0, typ = splitPath(d, BUF[ex], BUF[ne]);


    if (BUF[ne][0])
	goto end;		/* d doesn't exist - fail */
    if (typ >= 0)		/* container */
    {
	strcpy(BUF[ne], (p = STRRCHR(BUF[ex], PATH_SEP)) + 1);	/* container name */
#ifdef __MSDOS__
	if (p > (BUF[ex] + 2))
#else
	if (p > BUF[ex] + 2)
#endif
	    *p = '\0';
	else
	    *(p + 1) = '\0';
    }
    else if (typ != -3)
	goto end;		/* not a dir */

#ifdef __MSDOS__
    {
	/* Change the drive if necessary */
	unsigned drv, ndrv;
	drv = BUF[ex][0] - 'A';
	if (drv != getdisk())
	{
	    _dos_setdrive(drv + 1, &ndrv);
	    if (_dos_getdrive(&ndrv), ndrv != (drv + 1))
		rtn = -1;
	}
    }
    if (rtn == 0)
	rtn = chdir(BUF[ex] + 2);
#else
    if (rtn == 0)
	rtn = chdir(BUF[ex]);
#endif
    if (rtn == 0)
    {
	char *path = (typ >= 0) ? makePath(BUF[ex], BUF[ne]) : BUF[ex];
	if (strcmp(paths[l], path))
	{
	    strcpy(paths[l], path);
	    if (winState[l] == W_LIST)
		winInvalid[l] = 2;
	}
    }
    else
    {
#ifndef NO_STRERROR
	sprintf(BUF[ne], "Failed to change to directory %s - %s", BUF[ex],
		(char *) strerror(errno));
#else
	sprintf(BUF[ne], "Failed to change to directory %s", BUF[ex]);
#endif
	showMsg(BUF[ne]);
    }
  end:
    FREEBUF(ne);
    FREEBUF(ex);
}

/******************************************************************
	SPLIT THE FIELDS OF A CONTAINER CONTENTS LISTING
******************************************************************/

#define MAX_FIELDS 16

static char *fieldBufs[MAX_FIELDS];

#if __STDC__
static int splitFields(char *b)
#else
static int splitFields(b)
    char *b;
#endif
{
    int i = 0;
    if (b == NULL)
	return 0;
    while (i < MAX_FIELDS)
    {
	/* skip leading whitespace */
	while (*b && (*b == ' ' || *b == '\t'))
	    b++;
	if (*b)
	{
	    /* save the offset of the start of the field */

	    fieldBufs[i++] = b;

	    /* skip to end or next whitespace */

	    while (*b && *b != ' ' && *b != '\t')
		b++;

	    /* put in a NUL terminator */

	    if (*b)
		*b++ = '\0';
	}
	if (*b == '\0')
	    break;
    }
    return i;
}

/****************************************************************
EXPAND THE @f, @b AND @x PLACEHOLDERS IN A CONTAINER RULE
*****************************************************************/

#if __STDC__
static void expandRule(char *dest, char *src, char *cont, char *elt)
#else
static void expandRule(dest, src, cont, elt)
    char *dest, *src, *cont, *elt;
#endif
{
    char *p = src, *b = src, eltname[64];
    *dest = '\0';
    if (STRRCHR(elt, PATH_SEP))
	strcpy(eltname, STRRCHR(elt, PATH_SEP) + 1);
    else if (elt)
	strncpy(eltname, elt, 63);
    else
	eltname[0] = 0;
    eltname[63] = 0;
#ifdef DEBUG
    fprintf(debug, "In expandRule(?, %s, %s, %s)\n", src, cont, elt);
#endif
    for (;;)
    {
	p = STRCHR(p, '@');
	if (p)
	    *p = '\0';
	strcat(dest, b);
	if (p == NULL)
	    break;
	*p = '@';
	switch (p[1])
	{
	case 'x':
	    if (cont)
		strcat(dest, cont);
	    p += 2;
	    b = p;
	    break;
	case 'f':
	    if (elt)
		strcat(dest, elt);
	    p += 2;
	    b = p;
	    break;
	case 'b':
	    strcat(dest, eltname);
	    p += 2;
	    b = p;
	    break;
	default:
	    b = p++;
	    break;
	}
    }
}

/*******************************************************
FIND AND LOAD A NAMED RULE ENTRY IN THE CURRENT AREA
*******************************************************/

static long areaStart = 0;

#if __STDC__
int loadItem(FILE * fp, char *name, char *val)
#else
int loadItem(fp, name, val)
    FILE *fp;
    char *name, *val;
#endif
{
    int b = GRABBUF(), len, rtn = 0;
    char *p;
    *val = '\0';
    if (fp)
    {
	fseek(fp, areaStart, 0);
	while (!feof(fp))
	{
	    fgets(BUF[b], BUFFER_SIZE, fp);
	    len = stripwhitespace(BUF[b]);
	    if (BUF[b][0] == '#')
		continue;
	    if (BUF[b][0] == '[' && BUF[b][len - 1] == ']')
		break;
	    p = STRCHR(BUF[b], '=');
	    if (p)
	    {
		*p = '\0';
		(void) stripwhitespace(BUF[b]);
		STRUPR(BUF[b]);
		if (strcmp(BUF[b], name) == 0)
		{
		    /* Got it! */
		    int more = 0;
		    rtn = 1;
		    len = stripwhitespace(p + 1);
		    if (p[len] == '\\')
		    {
			more = 1;
			p[len] = '\0';
		    }
		    strcpy(val, p + 1);
		    while (more && !feof(catchFP))
		    {
			fgets(BUF[b], BUFFER_SIZE, catchFP);
			len = stripwhitespace(BUF[b]);
			if (BUF[b][0] == '#')
			    continue;
			if (len > 0 && BUF[b][len - 1] == '\\')
			{
			    BUF[b][len - 1] = '\0';
			    stripwhitespace(BUF[b]);
			}
			else
			    more = 0;
			strcat(val, BUF[b]);
		    }
		    break;
		}
	    }
	}
    }
    FREEBUF(b);
    return rtn;
}

/*************************************************
	SEEK TO A RULE AREA
**************************************************/

#if __STDC__
int seekToArea(FILE * fp, char *name)
#else
int seekToArea(fp, name)
    FILE *fp;
    char *name;
#endif
{
    int rtn = 0, len, b = GRABBUF();
    if (fp)
    {
	while (!feof(fp))
	{
	    fgets(BUF[b], BUFFER_SIZE, fp);
	    len = stripwhitespace(BUF[b]);
	    if (BUF[b][0] == '[' && BUF[b][len - 1] == ']')
	    {
		BUF[b][len - 1] = '\0';
		STRUPR(BUF[b]);
		if (strcmp(BUF[b] + 1, name) == 0)
		{
		    rtn = 1;
		    areaStart = ftell(fp);
		    break;
		}
	    }
	}
    }
    FREEBUF(b);
    return rtn;
}

/**************************************************************
	READ A CONTAINER FILE LISTING INTO fInfo
***************************************************************/

static char *months[] =
{
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

#if __STDC__
int readListLine(int rule, char *name, long *size, time_t * tm)
#else
int readListLine(rule, name, size, tm)
    int rule;
    char *name;
    long *size;
    time_t *tm;
#endif
{
    int i, rtn, b = GRABBUF(), gotOne = 0;
    char *line, *SIFlds = (char *) Rules[rule].sif;	/* single integer fields */
    char (*DIFlds)[2] = (char (*)[2]) Rules[rule].dif;	/* double integer fields */
    int totF = SIFlds[R_FIELDS];
    unsigned fld[R_SEC + 1];

    do
    {
	stripwhitespace(line = strcpy(BUF[b], valspace[VAR_LINEBUFF]));

	/* Is this line a valid entry? */

	if (splitFields(line) == totF)
	{
	    /* To help weed out heading lines that pass the previous check,
		    we ensure the size field is numeric */

	    if (SIFlds[R_SIZE] <= 0 || isdigit(fieldBufs[SIFlds[R_SIZE] - 1][0]))
	    {
		/* Get the name */

		if (name)
		    if (SIFlds[R_ITEM] > 0)
			strcpy(name, fieldBufs[SIFlds[R_ITEM] - 1]);

		/* get the file size */

		if (size)
		    if (SIFlds[R_SIZE] > 0)
			*size = atol(fieldBufs[SIFlds[R_SIZE] - 1]);

		/* Get the date */

		if (tm)
		{
		    for (i = 0; i < R_SEC; i++)
		    {
			int fn = DIFlds[i][0] - 1, fo = DIFlds[i][1] - 1;
			fld[i] = 0;
			if (fn > 0)
			{
			    if (i == R_MONTH && !isdigit(fieldBufs[fn][fo]))
			    {
				int m;
				STRUPR(fieldBufs[fn]);
				for (m = 0; m < 12; m++)
				{
				    if (strncmp(fieldBufs[fn], months[m], 3) == 0)
				    {
					fld[i] = m + 1;
					break;
				    }
				}
			    }
			    else
				sscanf(fieldBufs[fn] + fo, "%u", &fld[i]);
			}
		    }
		    /* convert the date info into the required format */
#ifdef __MSDOS__
		    *tm = normaliseDate(fld[0], fld[1], fld[2],
					fld[3], fld[4], fld[5]);
#else
		    {
			struct tm tms;
			if (fld[0] > 1900)
			    fld[0] -= 1900;
			tms.tm_year = fld[0];
			tms.tm_mon = fld[1] - 1;
			tms.tm_mday = fld[2];
			tms.tm_hour = fld[3];
			tms.tm_min = fld[4];
			tms.tm_sec = fld[5];
			*tm = (time_t) mktime(&tms);
		    }
#endif
		}
		gotOne = 1;
	    }
	}
	rtn = readNextLine(catchFP);
    }
    while (rtn && !gotOne);
    FREEBUF(b);
    return rtn;
}

#if __STDC__
static int readContainer(int n, int rule, char *cname, int *selct)
#else
static int readContainer(n, rule, cname, selct)
    int n, rule, *selct;
    char *cname;
#endif
{
    int cnt = 1, rtn, b = GRABBUF();

    /* Create a parent directory entry */

    fInfo_t *f = &fInfo[n][0];
    f->name = calloc(3, sizeof(char));
    if (f->name)
	strcpy(f->name, "..");
    f->size = 0;
    fIndex[n][0] = 0;

    /* Get a directory listing of the container */

    expandRule(BUF[b], Rules[rule].sf[R_LIST], cname, NULL);
    rtn = runCommand(BUF[b], 1);

    /* Read the listing line by line, parsing the lines */

    if (rtn)
    {
	for (;;)
	{
	    f = &fInfo[n][cnt];
	    rtn = readListLine(rule, BUF[b], &f->size, &f->modtime);
	    if (rtn)
	    {
		if ((f->name = calloc(strlen(BUF[b]) + 1, sizeof(char))) != NULL)
		    strcpy(f->name, BUF[b]);
		f->acctime = f->modtime;
		/* we don't do owners, groups or modes */
		f->uid = (uid_t) UNKNOWN_ID;
		f->gid = (gid_t) UNKNOWN_ID;
		f->typID = ' ';
		f->mode = (mode_t) 0;
		f->parent = -1;
		f->flag = 0;
		fIndex[n][cnt] = cnt;
		/* if the file was selected before, reselect it */
		if (*selct)
		    reselect(n, cnt, selct);
		cnt++;
	    }
	    else
		break;
	}
    }
    FREEBUF(b);
    return cnt;
}

/*****************************************************************
	EXTRACT A FILE FROM A CONTAINER
******************************************************************/

#if __STDC__
char *getExtraction(char *cont, char *elt, char *rtnbuf, int rule)
#else
char *getExtraction(cont, elt, rtnbuf, rule)
    char *cont, *elt, *rtnbuf;
    int rule;
#endif
{
    int oldPath = GRABBUF(), cmdB = GRABBUF();
    *rtnbuf = '\0';
    getCWD(BUF[oldPath]);
    CHDIR(valspace[VAR_TEMPPATH]);
    expandRule(BUF[cmdB], Rules[rule].sf[R_EXTRACT], cont, elt);
    if (runCommand(BUF[cmdB], 1))
	/* OK - set the new path */
	strcpy(rtnbuf, makePath(valspace[VAR_TEMPPATH], elt));
    CHDIR(BUF[oldPath]);
    FREEBUF(oldPath);
    FREEBUF(cmdB);
    return rtnbuf;
}

/*********************************************************************
	LOAD THE CONTAINER RULES
**********************************************************************/

static char *SFields[] =
{
    "NAME", "TYPE", "LIST",
    "ADD", "EXTRACT", "DELETE",
    NULL
};

static char *SIFields[] =
{
    "FIELDS", "ITEM", "SIZE",
    NULL
};

static char *DIFields[] =
{
    "YEAR", "MONTH", "DAY", "HOUR", "MIN", "SEC",
    NULL
};

#if	__STDC__
void readRules(FILE * fp)
#else
void readRules(fp)
    FILE *fp;
#endif
{
    int r = 0;
    if (fp)
    {
	int b = GRABBUF(), f, len;
	for (;;)
	{
	    fseek(fp, 0, 0);
#ifdef __MSDOS__
	    sprintf(BUF[b], "DOSRULE_%d", r + 1);	/* MUST BE UPPER CASE */
#else
	    sprintf(BUF[b], "UNIXRULE_%d", r + 1);
#endif
	    if (!seekToArea(fp, BUF[b]))
		goto done;

	    /* Read the string fields */

	    for (f = 0; SFields[f]; f++)
	    {
		Rules[r].sf[f] = NULL;
		if (loadItem(fp, SFields[f], BUF[b]))
		{
		    len = strlen(BUF[b]);
		    if (len)
		    {
			if ((Rules[r].sf[f] = calloc(len + 1, sizeof(char))) != NULL)
			    strcpy(Rules[r].sf[f], BUF[b]);
		    }
		}
	    }

	    /* A valid rule must have at least one of a name or type field */

	    if (Rules[r].sf[R_NAME] == NULL && Rules[r].sf[R_TYPE] == NULL)
	    {
		while (f-- > 0)
		    if (Rules[r].sf[f])
		    {
			free(Rules[r].sf[f]);
			Rules[r].sf[f] = NULL;
		    }
#ifdef __MSDOS__
		sprintf(BUF[b], "Invalid DOSRule_%d", r + 1);
#else
		sprintf(BUF[b], "Invalid UNIXRule_%d", r + 1);
#endif
		showMsg(BUF[b]);
		sleep(1);
		goto done;
	    }

	    /* Get the single integer fields */

	    for (f = 0; SIFields[f]; f++)
	    {
		Rules[r].sif[f] = 0;
		if (loadItem(fp, SIFields[f], BUF[b]))
		{
		    int fv;
		    if (sscanf(BUF[b], "%d", &fv) == 1)
			Rules[r].sif[f] = fv;
		}
	    }

	    /* Get the double integer fields */

	    for (f = 0; DIFields[f]; f++)
	    {
		Rules[r].dif[f][0] = Rules[r].dif[f][1] = 0;
		if (loadItem(fp, DIFields[f], BUF[b]))
		{
		    int fv, fo;
		    if (sscanf(BUF[b], "%d,%d", &fv, &fo) == 2)
		    {
			Rules[r].dif[f][0] = fv;
			Rules[r].dif[f][1] = fo;
		    }
		}
	    }
#ifdef __MSDOS__
	    sprintf(BUF[b], "Loaded DOSRule_%d", r + 1);	/* MUST BE UPPER CASE */
#else
	    sprintf(BUF[b], "Loaded UNIXRule_%d", r + 1);
#endif
	    showMsg(BUF[b]);
	    r++;
	    if (r >= MAX_RULES)
		break;
	}
      done:
	FREEBUF(b);
    }
    numRules = r;
}

/**********************************************************
	FREE RULES AT END
**********************************************************/

#if	__STDC__
void FreeRules(void)
#else
void FreeRules()
#endif
{
    while (numRules-- > 0)
    {
	int f = R_DELETE + 1;
	while (f-- > 0)
	    if (Rules[numRules].sf[f])
		free(Rules[numRules].sf[f]);
    }
}

/*********************************************************************
	FILE COPYING, MOVING AND DELETING
**********************************************************************/

#if __STDC__
static int getConfirmation(char *action, char *file)
#else
static int getConfirmation(action, file)
    char *action, *file;
#endif
{
    int rtn = 1;
    if (testOption(VAR_CONFIRM))
    {
	int b = GRABBUF();
	sprintf(BUF[b], "%s %s y/n/a? ", action, file);
	showMsg(BUF[b]);
	BUF[b][0] = '\0';
	if (getInput(BUF[b], 1, 1))
	{
	    char c = BUF[b][0];
	    c = toupper(c);
	    if (c == 'A')
		assign2var(VAR_CONFIRM, "n");
	    else if (c != 'Y')
		rtn = 0;
	}
	else
	    rtn = 0;
	FREEBUF(b);
    }
    return rtn;
}

/*
 *	unlinkFile - delete a file or container element
 *
 *	If the file does exist it is removed with unlink().
 *	If the file does not exist, it is split into "parent"
 *	and "base". If the parent does not exist, there is nothing to do.
 *	Otherwise, if the parent is not a directory, pipe or device file,
 *	and there is a container rule for it, apply the delete rule
 *	to $parent to delete $base.
 * In either case, if $CONFIRM is set, we ask for confirmation.
 */

#if __STDC__
int unlinkFile(char *f)
#else
int unlinkFile(f)
    char *f;
#endif
{
    if (access(f, 0) == 0)
	return getConfirmation("Delete", f) ? unlink(f) : -1;
    else
    {
	int rul, ex = GRABBUF(), ne = GRABBUF(), b = GRABBUF(), rtn = -1;
	rul = splitPath(f, BUF[ex], BUF[ne]);
	if (rul >= 0)
	{
	    if (getConfirmation("Delete", f))
	    {
		expandRule(BUF[b], Rules[rul].sf[R_DELETE], BUF[ex], BUF[ne]);
		rtn = (runCommand(BUF[b], 1) == 0);
		showMsg(valspace[VAR_LINEBUFF]);
	    }
	}
	FREEBUF(ex);
	FREEBUF(ne);
	FREEBUF(b);
	return rtn;
    }
}

#if __STDC__
static int copyFile(char *src, char *dest)
#else
static int copyFile(src, dest)
    char *src, *dest;
#endif
{
#ifdef __MSDOS__
    int rtn = 0;
    unsigned bsiz = 4096;
    FILE *inF, *outF;
    char *fbuff;
    /* check if dest is a directory or not */
    if (access(dest, 0) == 0)
    {
	struct stat s;
	if (stat(dest, &s) == 0)
	{
	    if (S_ISDIR(s.st_mode))
	    {
		int i = (int) strlen(src);
		while (src[i] != '\\' && src[i] != ':' && i >= 0)
		    i--;
		dest = makePath(dest, src + i + 1);
	    }
	}
    }
    if ((fbuff = calloc(bsiz, sizeof(char))) == NULL)
    {
	bsiz /= 2;
	if ((fbuff = calloc(bsiz, sizeof(char))) == NULL)
	{
	    bsiz /= 2;
	    if ((fbuff = calloc(bsiz, sizeof(char))) == NULL)
	    {
		showMsg("Cannot allocate disk buffer");
		rtn = -4;
		goto fail1;
	    }
	}
    }
    if ((inF = OpenFile(src, "rb")) == NULL)
    {
#ifdef NO_STRERROR
	sprintf(fbuff, "Cannot open %s for input: errno %d", src, errno);
#else
	sprintf(fbuff, "Cannot open %s for input: %s", src,
		strerror(errno));
#endif
	showMsg(fbuff);
	rtn = -2;
	goto fail2;
    }
    if ((outF = OpenFile(dest, "wb")) == NULL)
    {
#ifdef NO_STRERROR
	sprintf(fbuff, "Cannot open %s for output: errno %d", dest, errno);
#else
	sprintf(fbuff, "Cannot open %s for output: %s", dest,
		strerror(errno));
#endif
	showMsg(fbuff);
	rtn = -3;
	goto fail3;
    }
    while (!feof(inF))
    {
	unsigned i;
	if ((i = fread(fbuff, sizeof(char), bsiz, inF)) > 0)
	    fwrite(fbuff, sizeof(char), i, outF);
    }
    CloseFile(outF);
  fail3:
    CloseFile(inF);
  fail2:
    free(fbuff);
  fail1:

#else /* UNIX version */
    int buf = GRABBUF(), L, rtn = 9999;
    L = (int) strlen(src) + (int) strlen(dest) + 6;
    if (L < BUFFER_SIZE)
    {
	sprintf(BUF[buf], "cp %s %s", src, dest);
	rtn = (runCommand(BUF[buf], 1) == 0);
	showMsg(valspace[VAR_LINEBUFF]);
    }
    else
	showMsg("Buffer overflow!");
    FREEBUF(buf);
#endif
    return rtn;
}

#if __STDC__
static int contains(int rule, char *cname, char *ename)
#else
static int contains(rule, cname, ename)
    int rule;
    char *cname, *ename;
#endif
{
    /* This should return 0 if container cname contains file ename */

    int rtn, b = GRABBUF();

    /* Get a directory listing of the container */

    expandRule(BUF[b], Rules[rule].sf[R_LIST], cname, NULL);
    rtn = runCommand(BUF[b], 1);

#ifdef __MSDOS__
    STRUPR(ename);
#endif

    /* Read the listing line by line, parsing the lines */

    if (rtn)
    {
	for (;;)
	{
	    rtn = readListLine(rule, BUF[b], NULL, NULL);
	    if (rtn)
	    {
#ifdef __MSDOS__
		STRUPR(BUF[b]);
#endif
		if (strcmp(BUF[b], ename) == 0)
		{
		    FREEBUF(b);
		    return 0;
		}
	    }
	    else
		break;
	}
    }
    FREEBUF(b);
    return 1;
}

#if __STDC__
int doFileCopy(char *src, char *dest, int del)
#else
int doFileCopy(src, dest, del)
    char *src, *dest;
    int del;
#endif
{
    int sex = GRABBUF(), sne = GRABBUF(), dex = GRABBUF(), dne = GRABBUF(), buf = GRABBUF(),
     rul = GRABBUF();

    int styp = splitPath(src, BUF[sex], BUF[sne]);
    int dtyp = splitPath(dest, BUF[dex], BUF[dne]);
    int rtn = 0, madeTmp = 0;
    char *Src = BUF[sex];

    getCWD(BUF[buf]);
    if (BUF[sne][0])		/* container element */
    {
	if (styp < 0)
	    goto fail;

	/* Try to extract the file from the container */

	CHDIR(valspace[VAR_TEMPPATH]);
	expandRule(BUF[rul], Rules[styp].sf[R_EXTRACT], BUF[sex], BUF[sne]);
	rtn = runCommand(BUF[rul], 1);
	showMsg(valspace[VAR_LINEBUFF]);
	if (rtn)		/* expand OK? */
	{
	    /* try to rename it appropriately */

	    madeTmp = 1;
	    Src = BUF[sne];
	    if (dtyp >= 0 && BUF[dne])
	    {
		if (strcmp(BUF[sne], BUF[dne]))
		{
#ifdef __MSDOS__
		    sprintf(BUF[rul], "ren %s %s", BUF[sne], BUF[dne]);
#else
		    sprintf(BUF[rul], "mv %s %s", BUF[sne], BUF[dne]);
#endif
		    rtn = runCommand(BUF[rul], 1);
		    if (rtn)
			Src = BUF[dne];
		    else
		    {
			unlink(BUF[sne]);
			sprintf(BUF[rul], "Failed to rename %s/%s to %s/%s",
				valspace[VAR_TEMPPATH], BUF[sne],
				valspace[VAR_TEMPPATH], BUF[dne]);
			showMsg(BUF[rul]);
			sleep(1);
			goto done;
		    }
		}
	    }
	}
	else
	    goto fail;
    }
    if (dtyp >= 0 && BUF[dne][0])
    {
	/* Destination is a container. */
	if (contains(dtyp, BUF[dex], BUF[dne]) == 0)
	{
	    char *cp = makePath(BUF[dex], BUF[dne]);
	    if (!getConfirmation("Overwrite", cp))
	    {
		rtn = -1;
		goto done;
	    }
	}
	expandRule(BUF[rul], Rules[dtyp].sf[R_ADD], BUF[dex], Src);
	rtn = (runCommand(BUF[rul], 1) == 0);
	showMsg(valspace[VAR_LINEBUFF]);
    }
    else
    {
	char *Dest = makePath(BUF[dex], BUF[dne]);
	/* destination is not a container */
	if (access(Dest, 0) || getConfirmation("Overwrite", Dest))
	    rtn = copyFile(Src, Dest);
	else
	    rtn = -1;
    }
    goto done;
  fail:
    rtn = -1;
    sprintf(BUF[sex], "Cannot open %s!", src);
    showMsg(BUF[sex]);
    sleep(1);
  done:
    if (madeTmp)
	unlink(Src);
    if (del && rtn == 0)
    {
	if (madeTmp)
	{
	    expandRule(BUF[rul], Rules[styp].sf[R_DELETE], BUF[sex], BUF[sne]);
	    rtn = (runCommand(BUF[rul], 1) == 0);
	    showMsg(valspace[VAR_LINEBUFF]);
	}
	else
	    unlink(BUF[sex]);
    }
    CHDIR(BUF[buf]);
    FREEBUF(sex);
    FREEBUF(sne);
    FREEBUF(dex);
    FREEBUF(dne);
    FREEBUF(buf);
    FREEBUF(rul);
    return rtn;
}

/**********************************************************/

#if __STDC__
static int readDirectory(int n, char *dname, int *selected)
#else
static int readDirectory(n, dname, selected)
    int n, *selected;
    char *dname;
#endif
{
#ifdef __MSDOS__
    struct ffblk fdat;
#endif
    void *dir;
    char *fname;
    int buf, i = 0;
    if ((dir = initDir(dname)) == NULL)
	return 0;		/* Should produce an error */
#ifdef __MSDOS__
    while ((fname = getDirEntry(testOption(VAR_SHOWHIDDEN), &fdat)) != NULL)
    {
	if (addEntry(n, i, fname, 0, 0, &fdat))
#else
    buf = GRABBUF();
    while ((fname = getDirEntry(dir, testOption(VAR_SHOWHIDDEN))) != NULL)
    {
	strcpy(BUF[buf], makePath(paths[n], fname));
	if (addEntry(BUF[buf], n, i, fname, 0, 0))
#endif
	{
	    if (strcmp(fname, "."))	/* overwrite this if not only one */
	    {
		if (*selected)
		    reselect(n, i, selected);
		i++;
		if (i >= MAXARGS)
		    break;
	    }
	}
    }
    closeDir(dir);
#ifndef __MSDOS__
    FREEBUF(buf);
#endif
    return i;
}

/******************************************************************
	REREAD THE WINDOW CONTENTS
*******************************************************************/

/*
 * readList is called by showList when winInvalid is set.
 *
 * readList is called upon startup, by doCD after a change in paths[l],
 * and by i_rescan.
 */

#if	__STDC__
void readList(int n, int newpath)
#else
void readList(n, newpath)
    int n;
    int newpath;
#endif /* __STDC__ */
{
    int i, selected, selTot = 0;
    char *oldname = NULL, nbuf[256], msg[60];
#ifdef __MSDOS__
    struct ffblk fdat;
#endif
    msg[0] = '\0';
    if (!inCurses || !testOption(VAR_DISPLAYON))
	return;

    if (!newpath)
	strcpy(oldname = nbuf, INFO_NOW(n).name);

    if (winState[n] == W_TREE)
    {
	treeDepth = atoi(valspace[VAR_TREE]);
	if (treeDepth < 1)
	    treeDepth = 1;
	if (winInvalid[n] == 2)
	{
	    strcpy(msg, "Reading directory tree - please wait...");
	    showMsg(msg);
	    freeNames(n);
	    selCnt[n] = selSize[n] = 0;
	    i = selTot = 0;
	    readTree(n, paths[n], &i, -1, 0);	/* recurse */
	}
	else
	    i = totfiles[n];
    }
    else
    {
	int ex = GRABBUF(), ne = GRABBUF();
	int rul = splitPath(paths[n], BUF[ex], BUF[ne]);
	sprintf(msg, "Reading %s contents - please wait...",
		rul >= 0 ? "container" : "directory");
	showMsg(msg);
	/* Save old selected file names, free the rest */
	selected = selTot = (newpath ? 0 : saveSelected(n));
	freeNames(n);
	selCnt[n] = selSize[n] = 0;
	if (rul >= 0)
	    i = readContainer(n, rul, BUF[ex], &selected);
	else
	    i = readDirectory(n, BUF[ex], &selected);
	FREEBUF(ex);
	FREEBUF(ne);
    }
    strcat(msg, "done");
    showMsg(msg);
    if (i == 0)			/* Must have one entry at least */
#ifdef __MSDOS__
	addEntry(n, i++, ".", 0, 0, &fdat);
#else
	addEntry(".", n, i++, ".", 0, 0);
#endif
    totfiles[n] = i;
    if (newpath)
	highlight[n] = 0;
    while (selTot-- > 0)
	if (oldSelNames[selTot])
	    free(oldSelNames[selTot]);
    refilter(n, oldname);
    getDiskSpace(n, paths[n]);
}

/***************************************************************
	INITIALISATION
****************************************************************/

#if __STDC__
void dirInit(char *p1, char *p2)
#else
void dirInit(p1, p2)
    char *p1, *p2;
#endif
{
    if (p1)
	strcpy(paths[0], p1);
    else if (paths[0][0] == '\0')
	getCWD(paths[0]);
    if (p2)
	strcpy(paths[1], p2);
    else if (paths[1][0] == '\0')
	getCWD(paths[1]);
    /* Make sure boths paths are accessible and absolute */
    CHDIR(paths[1]);
    getCWD(paths[1]);
    CHDIR(paths[0]);
    getCWD(paths[0]);
}

/**********************************************************
	FREE ALLOCATED NAMES AT END
***********************************************************/

#if	__STDC__
void freeNames(int n)
#else
void freeNames(n)
    int n;
#endif /* __STDC__ */
{
    int i;
    for (i = 0; i < totfiles[n]; i++)
    {
	if (fInfo[n][i].name)
	{
	    free(fInfo[n][i].name);
	    fInfo[n][i].name = NULL;
	}
    }
}

/*****************************************************************
	SAVE AND RESTORE STATE
******************************************************************/

#if	__STDC__
int readIniFile(void)
#else
int readIniFile()
#endif /* __STDC__ */
{
    FILE *fp;
    int codesize;
#ifdef __MSDOS__
    char *iniPath = findReadPath(ININAME, "$GCPATH\\.$PATH", NULL, NULL);
#else
    char *iniPath = findReadPath(ININAME, "$GCPATH$HOME.$PATH", NULL, NULL);
#endif
    paths[0][0] = paths[1][0] = '\0';
    if (iniPath && (fp = OpenFile(iniPath, "r")) != NULL)
    {
	int buf = GRABBUF(), i, j, len;
	int vernum;
	fscanf(fp, "Version %X\n", &vernum);
	fscanf(fp, "Scriptsize %d\n", &codesize);
	if (vernum != VERCODE || codesize != codep)
	{
	    CloseFile(fp);
	    return -1;		/* Old init file is no good */
	}
	fscanf(fp, "Windows %d %d %d %d %d %d\n",
	       &leftMargin, &topMargin,
	       &winWidth[0], &showLines[0],
	       &winWidth[1], &showLines[1]);
	fscanf(fp, "Path %s\n", paths[1]);
	for (i = 0; i < 4; i++)
	{
	    long a;
	    fscanf(fp, "Attribute %*d %ld\n", &a);
	    Attribute[i] = a;
	}
	i = LAST_RDONLY_VAR + 1;
	while (!feof(fp))
	{
	    while (i < idents)
	    {
		if ((objects[i].type & FUNC_TYPE) == 0)
		    break;
		i++;
	    }
	    if (i == idents)
		break;
	    fgets(BUF[buf], BUFFER_SIZE, fp);
	    j = 0;
	    while (BUF[buf][j] && BUF[buf][j] != '=')
		j++;
	    if (BUF[buf][j] == '=')
	    {
		j++;
		if (BUF[buf][j] == ' ')
		    j++;
		else
		    continue;
	    }
	    else
		continue;
	    /* Strip trailing CR/LF stuff */
	    if ((len = strlen(BUF[buf] + j)) > 0)
		while (--len, len >= 0 && BUF[buf][j + len] < 32)
		    BUF[buf][j + len] = 0;
	    assign2var(i++, BUF[buf] + j);
	}
	CloseFile(fp);
	FREEBUF(buf);
	return 0;
    }
    else
	return -1;
}

#if	__STDC__
void SaveInitFile(void)
{
#else
void SaveInitFile()
{
#endif /* __STDC__ */
    FILE *fp;
#ifdef __MSDOS__
    char *iniPath = findWritePath(ININAME, "$GCPATH\\", NULL);
#else
    char *iniPath = findWritePath(ININAME, "$GCPATH$HOME", NULL);
#endif
    if (iniPath && (fp = OpenFile(iniPath, "w")) != NULL)
    {
	char *v;
	int i, buf = GRABBUF();
	fprintf(fp, "Version %X\n", VERCODE);
	fprintf(fp, "Scriptsize %d\n", codep);
	fprintf(fp, "Windows %d %d %d %d %d %d\n",
		leftMargin, topMargin,
		winWidth[0], showLines[0],
		winWidth[1], showLines[1]);
	if (getenv("GCPATH"))
	    fprintf(fp, "Path %s\n", paths[l]);
#ifndef __MSDOS__
	else if (getenv("HOME"))
	    fprintf(fp, "Path %s\n", paths[l]);
#endif
	/* The active directory is the directory
	   the file gets saved in, so in
	   this case we save the other directory. */
	else
	    fprintf(fp, "%s\n", paths[1 - l]);
	for (i = 0; i < 4; i++)
	    fprintf(fp, "Attribute %d %ld\n", i + 1, (long) Attribute[i]);
	for (i = LAST_RDONLY_VAR + 1; i < idents; i++)
	{
	    if ((objects[i].type & FUNC_TYPE) == 0)
	    {
		v = lookupVar(i, buf);
		if (strcmp(v, "<undef>") == 0)
		    v = "";
		printArg(fp, i);
		fprintf(fp, "\t= %s\n", v);
	    }
	}
	CloseFile(fp);
	FREEBUF(buf);
    }
}

/********************************************
	GET FREE DISK SPACE
*********************************************/

#ifdef __MSDOS__
void getDiskSpace(int n, char *path)
{
    unsigned char drv = 0;
    struct dfree df;
    long csiz;			/* bytes/cluster */
    if (path)
	if (path[1] == ':')
	    if (path[0] > 'Z')
		drv = (unsigned char) (path[0] - 'a' + 1);
	    else
		drv = (unsigned char) (path[0] - 'A' + 1);
    getdfree(drv, &df);
    csiz = (long) df.df_sclus * (long) df.df_bsec;
    freeSpace[n] = csiz * (long) df.df_avail;
    usedSpace[n] = csiz * (long) df.df_total - freeSpace[n];
}

#else
#if __STDC__
void getDiskSpace(int n, char *path)
#else
void getDiskSpace(n, path)
    int n;
    char *path;
#endif
{
    /* Should do a df here and display results */
}

#endif

/*****************************************************************/
