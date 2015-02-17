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

/********************************************************
		GC3 DIRECTORY SORTING

 (c) 1993 by Graham Wheeler
*********************************************************/

#include "gcport.h"
#include "gc3.h"
#include "gclib.h"

#if __STDC__
typedef int (*cmpFunc)(fInfo_t *, fInfo_t *);
#else
typedef int (*cmpFunc)();
#endif

static int
	sortStyle[2],
	secSortStyle[2],
	reverseSort[2],
	secReverseSort[2];

static int
	sortingL,
	priStyle,
	secStyle,
	priRev,
	secRev;

/*
 * sortList - sort the fInfo entry indexes according to the
 *	sort keys sortStyle[l] and secSortStyle[l].
 */


/*
 * File name extension compare
 */

#if	__STDC__
static int extCmp(fInfo_t *a, fInfo_t *b)
#else
static int extCmp(a, b)
	fInfo_t *a, *b;
#endif /* __STDC__ */
{
	char *ap = (char *)STRRCHR(a->name,'.');
	char *bp = (char *)STRRCHR(b->name,'.');
	if (ap==NULL) ap=a->name;
	if (bp==NULL) bp=b->name;
	return strcmp(ap,bp);
}

/*
 * File name compare
 */

#if	__STDC__
static int nameCmp(fInfo_t *a, fInfo_t *b)
#else
static int nameCmp(a, b)
	fInfo_t *a, *b;
#endif /* __STDC__ */
{
	return strcmp(a->name, b->name);
}

/*
 * File size compare
 */

#if	__STDC__
static int sizeCmp(fInfo_t *a, fInfo_t *b)
#else
static int sizeCmp(a, b)
	fInfo_t *a, *b;
#endif /* __STDC__ */
{
	if (a->size == b->size) return 0;
	else return ((a->size - b->size) <0l) ? -1 : 1;
}

/*
 * Modification time compare
 */

#if	__STDC__
static int mtimCmp(fInfo_t *a, fInfo_t *b)
#else
static int mtimCmp(a, b)
	fInfo_t *a, *b;
#endif /* __STDC__ */
{
	if (a->modtime == b->modtime) return 0;
	else return ((a->modtime - b->modtime) <0l) ? -1 : 1;
}

/*
 * Access time compare
 */

#if	__STDC__
static int atimCmp(fInfo_t *a, fInfo_t *b)
#else
static int atimCmp(a, b)
	fInfo_t *a, *b;
#endif /* __STDC__ */
{
	if (a->acctime == b->acctime) return 0;
	else return ((a->acctime - b->acctime) <0l) ? -1 : 1;
}

/*
 * Owner compare
 */

#if	__STDC__
static int ownCmp(fInfo_t *a, fInfo_t *b)
#else
static int ownCmp(a, b)
	fInfo_t *a, *b;
#endif /* __STDC__ */
{
	return (int)(a->uid - b->uid);
}

/*
 * Group compare
 */

#if	__STDC__
static int grpCmp(fInfo_t *a, fInfo_t *b)
#else
static int grpCmp(a, b)
	fInfo_t *a, *b;
#endif /* __STDC__ */
{
	return (int)(a->gid - b->gid);}

/*
 * Table of pointers to above functions. The order is important
 * as the sort style is used as an index into this table.
 */

static cmpFunc cmpTbl[] =
{
	nameCmp,
	sizeCmp,
	mtimCmp,
	atimCmp,
	ownCmp,
	grpCmp,
	extCmp
};

/*
 * The `generic' compare routine. This uses the primary and
 * secondary sort keys and reverse sorting, if applicable.
 */

#if	__STDC__
static int fcmp(const void *a, const void *b)
#else
static int fcmp(a, b)
	char *a;
	char *b;
#endif /* __STDC__ */
{
	int v;
	/*
	 * If directories must come first, and one (and only one)
	 * of the files being comared is a directory, we use this
	 * as the basis of the compare.
	 */
	if (testOption(VAR_DIRSFIRST))
	{
		if (IsDirectory(sortingL, *((int *)a) ))
		{
			if (!IsDirectory(sortingL, *((int *)b) ))
				return -1;
		}
		else if (IsDirectory(sortingL, *((int *)b) ))
			return 1;
	}

	/* Compare by primary key, and reverse if necessary */

	v = (*(cmpTbl[priStyle]))(&fInfo[sortingL][*((int *)a)],
				&fInfo[sortingL][*((int *)b)]);
	if (priRev) v = -v;

	if (v==0) /* Primary key doesn't distinguish, so we use secondary */
	{
		v = (*(cmpTbl[secStyle]))(&fInfo[sortingL][*((int *)a)],
				&fInfo[sortingL][*((int *)b)]);
		if (secRev) v = -v;
	}
	return v;
}

/*****************************************************************
	External entry points
******************************************************************/

/*
 * sortList(n, name) sorts the list of files in the window given
 *	by n. If name is given, then an attempt to position the
 *	cursor on that name is made after the sort is complete.
 */

#if	__STDC__
void sortList(int n, char *name)
#else
void sortList(n, name)
	int n;
	char *name;
#endif /* __STDC__ */
{
	if (isListWindow(n))
	{
		int (*ftmp)() = fcmp; /* Attempt to fool SPARC compiler */
		sortingL = n;
		priStyle = sortStyle[n];	priRev = reverseSort[n];
		secStyle = secSortStyle[n];	secRev = secReverseSort[n];
		if (numfiles[n]>1)
#if __STDC__
			qsort((void *)fIndex[n],(size_t)(numfiles[n]),
				sizeof(int), ftmp);
#else
			qsort((char *)fIndex[n],numfiles[n],sizeof(int),ftmp);
#endif
		if (name)
		{
			int i = findFile(n,0,name,NULL);
#ifdef DEBUG
			if (testOption(VAR_DEBUGGING))
				fprintf(debug,"findFile(%s) returned %di (%s,%s)\n",name,i,fInfo[n][i].name,fInfo[n][fIndex[n][i]].name);
#endif
			if (i>=0)
				highlight[n] = i;
		}
		else startLn[n] = highlight[n]= 0;
	}
}

/*
 * resortList is used when a new sort key and direction are
 * given. The old primary key/direction are made secondary.
 */

#if __STDC__
void resortList(int n, int dir, short key)
#else
void resortList(n, dir, key)
	int n, dir;
	short key;
#endif
{
	secReverseSort[n] = reverseSort[n];
	secSortStyle[n] = sortStyle[n];
	reverseSort[n] = (dir < 0);
	sortStyle[n] = (int)key;
	sortList(n, INFO_NOW(n).name);
}

