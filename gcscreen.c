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


/***********************************************************
		GC3 SCREEN HANDLING
************************************************************/

#include "gcport.h"
#include "gc3.h"
#include "gckey.h"

#define GC_CURSES
#include "gclib.h"

int Lines;

static WINDOW *panels[2];
static WINDOW *panelTop[2];
static WINDOW *statusWin[2];
static WINDOW *statusTop[2];
static WINDOW *cmdWin;
int statusVis[2] =
{1, 1};
int listVis[2] =
{1, 1};
int topMargin = 0, leftMargin = 0;
int winWidth[2] =
{40, 40};
static int cmdWinCursorPos = 0;
int inCurses = 0;
char viewLine[256];	/* top line in view window; accessible in script
					as $viewline */
static char ipTrace[MAX_COLS] =
{0, 0, 0, 0, 0};

char fileViewed[MAXPATHNAME] =
{0};
static FILE *viewFP = NULL;
static long *viewPos = NULL;
static long topLine = 0l, viewLines = 0l, viewSize = 0l;
static int viewHex = 0;
static int viewColumn = 0;

static char *space = "                                                                                 ";

#define PADBUF(w,b)	( strcat(BUF[b],space), BUF[b][winWidth[w]-2] = '\0' )

/*********************************************************************
	O/S INDEPENDENT STUFF
**********************************************************************/

static int showFileCursor = 1;

#define CursorLine(n)	(highlight[n]-startLn[n])

static char *Cursors[] =
{"->", "=>", "  "};

#if	__STDC__
void InitialiseScreenAttributes(void)
#else
void InitialiseScreenAttributes()
#endif
{
    /* Set up very basic default screen attributes */
#ifdef __MSDOS__
    initscr();			/* Do this now as it just gets the screen type (color/mono) */
#endif
    setAttrEntry(0, -1, 0);
    setAttrEntry(1, -1, 2);
    setAttrEntry(2, -1, 2);
    setAttrEntry(3, -1, 0);

}

#if	__STDC__
int isListWindow(int n)
#else
int isListWindow(n)
    int n;
#endif /* __STDC__ */
{
    return (testOption(VAR_DISPLAYON) && listVis[n] &&
	    (winState[n] != W_VIEW || viewingDir));
}

#if	__STDC__
void showCursor(int n, int sel)
#else
void showCursor(n, sel)
    int n;
    int sel;
#endif /* __STDC__ */
{
    if (isListWindow(n))
    {
	mvwprintw(panels[n], CursorLine(n), 1, Cursors[sel]);
	UPDATE(panels[n]);
    }
}

#if	__STDC__
void hideCursor(int n)
#else
void hideCursor(n)
    int n;
#endif /* __STDC__ */
{
    if (isListWindow(n))
	mvwprintw(panels[n], CursorLine(n), 1, Cursors[2]);
}

#if	__STDC__
int Truncate(char *str, int maxlen)
#else
int Truncate(str, maxlen)
    char *str;
    int maxlen;
#endif /* __STDC__ */
{
    int len = strlen(str);
    if (len > maxlen)
    {
	len = maxlen;
	str[len] = 0;
    }
    return len;
}

#if	__STDC__
void CvtCtrl(char *msg)
#else
void CvtCtrl(msg)
    char *msg;
#endif /* __STDC__ */
{
    while (*msg)
    {
	if (*msg < 32)
	    *msg = ' ';
	msg++;
    }
}

#if	__STDC__
void fillString(char *str, int cnt, int ch)
#else
void fillString(str, cnt, ch)
    char *str;
    int cnt, ch;
#endif /* __STDC__ */
{
    str[cnt] = 0;
    while (--cnt >= 0)
	str[cnt] = ch;
}

#if	__STDC__
void writeMsg(char *msg)
#else
void writeMsg(msg)
    char *msg;
#endif /* __STDC__ */
{
    int len = Truncate(msg, COLS - 1);
    CvtCtrl(msg);
    if (inCurses)
    {
	clearCmdWin();
	mvwprintw(cmdWin, 0, 0, msg);
	cmdWinCursorPos = len + 1;
    }
    else
	fprintf(stderr, "%s\n", msg);
}

#if	__STDC__
void showMsg(char *msg)
#else
void showMsg(msg)
    char *msg;
#endif /* __STDC__ */
{
    writeMsg(msg);
    if (inCurses)
    {
	UPDATE(cmdWin);
	REFRESH();
    }
}

#if	__STDC__
void displayIP(int ipv)
#else
void displayIP(ipv)
    int ipv;
#endif /* __STDC__ */
{
    if (inCurses)
    {
	/* shunt over by 5 places */
	strncpy(ipTrace, ipTrace + 5, 70);
	sprintf(ipTrace + 70, "%5d", ipv);
	ipTrace[75] = '\0';
	showMsg(ipTrace);
    }
    else
	fprintf(stderr, "%d\n", ip);
}

#if	__STDC__
void clearCmdWin(void)
#else
void clearCmdWin()
#endif
{
    if (inCurses)
    {
#ifndef __MSDOS__
	wattrset(cmdWin, 0);
#endif
	werase(cmdWin);
	cmdWinCursorPos = 0;
	if (showIP)
	    displayIP(ip);
    }
}

#if __STDC__
void fillAttribute(WINDOW * w, int width, int height)
#else
void fillAttribute(w, width, height)
    WINDOW *w;
    int width, height;
#endif
{
    char buff[82];
    int r;
    fillString(buff, 80, ' ');
    buff[width] = '\0';
    for (r = 0; r < height; r++)
	mvwprintw(w, r, 0, buff);
}

#if __STDC__
int gcEditWrap(int c, char *start, char **end, char **now,
	        int *_insmode, int maxlen)
#else
int gcEditWrap(c, start, end, now, _insmode, maxlen)
    int c, *_insmode, maxlen;
    char *start, **end, **now;
#endif
{
    int rtn;
    /* Do cursor updates in file window while in command line editor */
    if (maxlen && showFileCursor)
	hideCursor(l);
    rtn = EditString(c, start, end, now, _insmode, maxlen);
    if (maxlen && showFileCursor)
	showCursor(l, 1);
    return rtn;
}

#if	__STDC__
int getInput(char *buf, int len, int mode)
#else
int getInput(buf, len, mode)
    char *buf;
    int len, mode;
#endif /* __STDC__ */
{
    int saveLine = 0;
    if (len < -1)
    {
	saveLine = 1;
	len = -len;
    }
    if (len)
	showCursor(l, 1);
    myRead(cmdWin, 80, 0, cmdWinCursorPos, buf, len, &mode, gcEditWrap, my_getch);
    if (len)
	hideCursor(l);
    if (buf[0] == '\0')
	clearCmdWin();
    else if (saveLine)
	saveHist(buf);
    return (int) strlen(buf);
}

#if	__STDC__
static void showTitle(void)
#else
static void showTitle()
#endif /* __STDC__ */
{
    WINDOW *w = newwin(13, 27, (LINES - 14) / 2, (COLS - 24) / 2);
    selectAttrib(w, 0);
    werase(w);
    fillAttribute(w, 27, 13);
    Box(w);
    mvwprintw(w, 2, 6, "Gram's Commander");
    mvwprintw(w, 4, 12, "by");
    mvwprintw(w, 6, 6, "Graham Wheeler");
    mvwprintw(w, 8, 2, "gram@aztec.co.za");
    mvwprintw(w, 10, 4, "Version %s (PL %s)", VERSION, PATCHLEVEL);
    UPDATE(w);
    REFRESH();
    sleep(1);
    delwin(w);
}

#if	__STDC__
void showPath(int p)
#else
void showPath(p)
    int p;
#endif /* __STDC__ */
{
    int useAttr = (p == l);
    int b;
    char *buf;
    if (!inCurses || !testOption(VAR_DISPLAYON) || !listVis[p])
	return;
    Box(panelTop[p]);
    buf = BUF[b = GRABBUF()];
    if (winState[p] == W_VIEW)
    {
	char *vf = file2view[0] ? file2view : INFO_NOW(1 - p).name;
	if (!viewingDir && !viewHex)
	    sprintf(buf, "[Viewing %s (%ld/%d/%ld)]", vf, topLine,
		    viewColumn, viewSize);
	else
	    sprintf(buf, "[Viewing %s]", vf);
    }
    else if (winState[p] == W_TREE)
	getTreePath(p, buf);
    else
    {
	int L1 = (int) strlen(paths[p]), L2 = (int) strlen(filters[p]);
	char c = paths[p][L1 - 1];
	if (L1 > (winWidth[p] - 6 - L2))
	    L1 -= (winWidth[p] - 6 - L2);
	else
	    L1 = 0;
	if (c != PATH_SEP)
	    sprintf(buf, "<%s%c%s>", paths[p] + L1, PATH_SEP, filters[p]);
	else
	    sprintf(buf, "<%s%s>", paths[p] + L1, filters[p]);
    }
    BUF[b][winWidth[p] - 6] = '\0';
    if (useAttr)
	selectAttrib(panelTop[p], 1);
    mvwprintw(panelTop[p], 0, 2, BUF[b]);
    if (useAttr)
	selectAttrib(panelTop[p], 0);
    FREEBUF(b);
}

#ifndef __MSDOS__
#if __STDC__
char *GetUserID(int uid)
#else
char *GetUserID(uid)
    int uid;
#endif
{
    static char uname[40];
    struct passwd *usr = getpwuid(uid);
    if (usr && usr->pw_name)
	strcpy(uname, usr->pw_name);
    else if (uid == UNKNOWN_ID)
	strcpy(uname, "?");
    else
	sprintf(uname, "%d", uid);
    return uname;
}

#if __STDC__
char *GetGroupID(int gid)
#else
char *GetGroupID(gid)
    int gid;
#endif
{
    static char gname[40];
    struct group *grp = getgrgid(gid);
    if (grp && grp->gr_name)
	strcpy(gname, grp->gr_name);
    else if (gid == UNKNOWN_ID)
	strcpy(gname, "?");
    else
	sprintf(gname, "%d", gid);
    return gname;
}

#endif

#if	__STDC__
static void showStatus(int n)
#else
static void showStatus(n)
    int n;
#endif /* __STDC__ */
{
#ifdef __MSDOS__
    long ts;
#endif
    int buf, w = winWidth[n] - 2;
    if (!inCurses || !testOption(VAR_DISPLAYON) || !statusVis[n])
	return;
    buf = GRABBUF();
    selectAttrib(statusTop[n], 0);
    Box(statusTop[n]);
    selectAttrib(statusWin[n], 0);

    if (winState[n] != W_VIEW)	/* Viewer doesn't have status pane */
    {
	if (w >= 38)
	    sprintf(BUF[buf], " %4d files selected (%-9ld bytes)", selCnt[n], selSize[n]);
	else if (w >= 12)
	    sprintf(BUF[buf], " %4d selected", selCnt[n]);
	else
	    BUF[buf][0] = '\0';
#ifdef __MSDOS__
	PADBUF(n, buf);
	mvwprintw(statusWin[n], 1, 0, BUF[buf]);
	sprintf(BUF[buf], " %s", asctime(localtime(&INFO_NOW(n).modtime)));
	BUF[buf][strlen(BUF[buf]) - 1] = '\0';	/* clear out newline */
	PADBUF(n, buf);
	mvwprintw(statusWin[n], 0, 0, BUF[buf]);
	ts = freeSpace[n] + usedSpace[n];
	if (w >= 36)
	    sprintf(BUF[buf], " %7ldkb of %7ld kb free (%3d%%)",
		    freeSpace[n] / 1024l,
		    ts / 1024l, (int) (freeSpace[n] / (ts / 100l)));
	else if (w > 15)
	    sprintf(BUF[buf], " %7ldkb free", freeSpace[n] / 1024l);
	else
	    BUF[buf][0] = '\0';
	PADBUF(n, buf);
	mvwprintw(statusWin[n], 2, 0, BUF[buf]);
#else
	PADBUF(n, buf);
	mvwprintw(statusWin[n], 0, 0, BUF[buf]);
	sprintf(BUF[buf], "  %9s  %10s/%-10s",
		getPerms(INFO_NOW(n).mode),
		GetUserID(INFO_NOW(n).uid),
		GetGroupID(INFO_NOW(n).gid));
	PADBUF(n, buf);
	mvwprintw(statusWin[n], 1, 0, BUF[buf]);
	sprintf(BUF[buf], "  Modified %26s",
		asctime(localtime(&INFO_NOW(n).modtime)));
	BUF[buf][36] = 0;	/* clear out the newline */
	PADBUF(n, buf);
	mvwprintw(statusWin[n], 2, 0, BUF[buf]);
	sprintf(BUF[buf], "  Accessed %26s",
		asctime(localtime(&INFO_NOW(n).acctime)));
	BUF[buf][36] = 0;	/* clear out the newline */
	PADBUF(n, buf);
	mvwprintw(statusWin[n], 3, 0, BUF[buf]);
#endif
    }
    FREEBUF(buf);
}

/*
 * showInfo - display the i'th entry for the n panel on the
 *	r'th row of the panel window
 */

#if	__STDC__
static void showInfo(int n, int r, int i)
#else
static void showInfo(n, r, i)
    int n;
    int r;
    int i;
#endif /* __STDC__ */
{
    extern short info2show;
    int j, buf, sel;/*, F = fIndex[n][i];*/
    fInfo_t *fip = &INFO(n,i);
    struct tm *tp;
    char buff[42];
    buf = GRABBUF();
    BUF[buf][0] = '\0';
    PADBUF(n, buf);
    mvwprintw(panels[n], r, 0, BUF[buf]);
    if (i >= numfiles[n])
	goto end;
    strncpy(buff, fip->name, (unsigned) fnameLen);
    buff[fnameLen] = 0;
    sel = fip->flag & F_SELECTED;
    sprintf(BUF[buf], "    %c ", fip->typID);
    for (j = 0; j < fip->depth; j++)
	strcat(BUF[buf], "  ");
    strcat(BUF[buf], buff);
    if (sel)
	selectAttrib(panels[n], 1);
    PADBUF(n, buf);
    mvwprintw(panels[n], r, 0, BUF[buf]);
    if (sel)
	selectAttrib(panels[n], 0);
    if (winState[n] != W_TREE)
    {
	switch (info2show)
	{
	case 1:		/* size */
	    sprintf(BUF[buf], "%8ld", fip->size);
	    break;
#ifndef __MSDOS__
	case 2:		/* owner */
	    sprintf(BUF[buf], "%8s", GetUserID(fip->uid));
	    break;
	case 3:		/* group */
	    sprintf(BUF[buf], "%8s", GetGroupID(fip->gid));
	    break;
	case 4:		/* perms */
	    strcpy(BUF[buf], getPerms(fip->mode));
	    break;
#endif
	case 5:		/* mtime */
	    tp = localtime(&fip->modtime);
	    sprintf(BUF[buf], "%2d:%02d:%02d %2d/%02d/%02d",
		    tp->tm_hour, tp->tm_min, tp->tm_sec,
		    tp->tm_mday, tp->tm_mon+1, tp->tm_year);
	    break;
#ifndef __MSDOS__
	case 6:		/* atime */
	    tp = localtime(&fip->acctime);
	    sprintf(BUF[buf], "%2d:%02d:%02d %2d/%02d/%02d",
		    tp->tm_hour, tp->tm_min, tp->tm_sec,
		    tp->tm_mday, tp->tm_mon+1, tp->tm_year);
	    break;
#endif
	default:
	    goto end;
	}
	if ((winWidth[n] - fnameLen - 10) >= 0)
	{
	    BUF[buf][winWidth[n] - fnameLen - 10] = '\0';
	    if (sel)
		selectAttrib(panels[n], 1);
	    mvwprintw(panels[n], r, fnameLen + 8, BUF[buf]);
	    if (sel)
		selectAttrib(panels[n], 0);
	}
    }
  end:
    FREEBUF(buf);
}

#define VB(p)	viewBufs[(p)/(16*1024)]
#define VC(p)	VB(p)[(p)%(16*1024)]

#if	__STDC__
static void showFile(int n, char *vf)
#else
static void showFile(n, vf)
    int n;
    char *vf;
#endif
{
    int r, c, buf = GRABBUF(), cnt, exBuf = -1;
    static int isExp = 0;
    int number = testOption(VAR_NUMBERLNS), w = (winWidth[n] - 2);
    static time_t view_mtime = (time_t) 0;
    long p, o;
    char *s;
    char *Line, *S = BUF[buf];	/* gross out! */
    s = (char *) calloc(4096, sizeof(char));
    if (s == NULL)
    {
	showMsg("Cannot allocate read buffer!");
	goto end2;
    }
    /* Compare with the file being viewed... */
    if (viewFP)
    {
	if (strcmp(vf, fileViewed) ||
	    (exBuf < 0 && view_mtime != getFileModTime(fileViewed)))
	{
	    CloseFile(viewFP);
	    viewFP = NULL;
	    if (isExp > 0)
		unlink(fileViewed);
	}
    }
    if (viewPos == NULL)
	viewPos = (long *) calloc(MAXVIEWLINES, sizeof(long));

    /* if viewFP is NULL, open the file and read it in */
    if (viewFP == NULL)
    {
#ifndef __MSDOS__
	/* If it is a device file or FIFO, just indicate that... */
	if (checkType(getFileMode(vf), 4 | 8 | 2))
	{
	    sprintf(BUF[buf], "Cannot view %s : device or pipe", vf);
	    BEEP;
	    sleep(1);
	    showMsg(BUF[buf]);
	    goto end;
	}
#endif
	viewSize = getFileSize(vf);
	view_mtime = getFileModTime(vf);
#ifdef __MSDOS__
	viewFP = OpenFile(vf, "rb");
#else
	viewFP = OpenFile(vf, "r");
#endif
	if (viewFP == NULL)
	{
	    showMsg("Cannot open view file!");
	    BEEP;
	    sleep(1);
	    goto end;
	}
	strcpy(fileViewed, vf);
	isExp = (exBuf >= 0);
	viewHex = viewColumn = 0;
	topLine = viewLines = 0l;
	/* check if it is binary */
	cnt = fread(S, sizeof(char), BUFFER_SIZE, viewFP);
	for (p = 0; p < cnt; p++)
	{
	    if (((unsigned char) S[p]) > 127)
		viewHex++;
	}
	if (viewHex > (cnt / 10))
	    viewLines = (viewSize + 7) / 8;
	else
	    viewHex = 0;
	if (!viewHex)
	{
	    /* Paginate as a text file */
	    fseek(viewFP, 0l, 0);
	    while (!feof(viewFP) && viewLines < MAXVIEWLINES)
	    {
		viewPos[viewLines] = ftell(viewFP);
		if (fgets(S, BUFFER_SIZE, viewFP) == NULL)
		    break;
		else
		    viewLines++;
	    }
	    if (!feof(viewFP))
	    {
		showMsg("Pagination table overflow!");
		viewSize = ftell(viewFP);
	    }
	}
	else if (w > 56)
	    viewHex = 16;
	else
	    viewHex = 8;
    }
    /* now we view the data in the file, starting from topLine */
    werase(panels[n]);
    Line = S;
    if (viewHex)
	fseek(viewFP, o = topLine * viewHex, 0);
    else
	fseek(viewFP, o = viewPos[topLine], 0);
    cnt = fread(s, sizeof(char), 4096, viewFP);
    for (p = r = 0;
       r < showLines[n] && (o + p) < viewSize /*&& (topLine+r)<viewLines*/ ;
	 r++)
    {
	/* pad up to the end of the line with spaces */
	strcpy(S = Line, space);
	if (viewHex)
	{
	    sprintf(S, "%06lX: ", o + p);
	    for (c = 0; c < viewHex && (o + p + c) < viewSize; c++)
	    {
		sprintf(S + 8 + (5 * c) / 2, "%02X ", (s[p + c]) & 0xFF);
#ifndef __MSDOS__
		if (s[p + c] < 32 || s[p + c] >= 127)
		    sprintf(S + 10 + (5 * viewHex) / 2 + c, ".");
		else
#endif
		    sprintf(S + 10 + (5 * viewHex) / 2 + c, "%c", s[p + c]);
	    }
	    if (viewColumn >= (10 + (7 * viewHex) / 2))
		viewColumn = (9 + (7 * viewHex) / 2);
	    /* fix up NUL put in by sprintf... */
	    S[8 + (5 * (viewHex - 1)) / 2 + 3] = ' ';
	    Line = S + viewColumn;
	    p += viewHex;
	}
	else
	{
	    /* Save the first line in a script variable */
	    if (r == 0)
	    {
		int i;
		strncpy(viewLine, s, 255);
		for (i = 0; i < 256; i++)
		{
		    if (viewLine[i] != 9 && viewLine[i] < 32)
		    {
			viewLine[i] = 0;
			break;
		    }
		}
	    }
	    /* Do line numbering if option set */
	    if (number)
	    {
		sprintf(S, "%-4d:", (int) (topLine + r + 1));
		S += 5;
	    }
	    /* Put in the text starting from the view column */
	    p = viewPos[topLine + r] - o;
	    for (c = 0; (o + p) < viewSize && p < 4096;)
	    {
		int si = c - viewColumn;
		/* Convert tabs to spaces */
		if (s[p] == '\t')
		{
		    int k = 8 - (c % 8);
		    while (k--)
		    {
			if (si >= 0)
			    S[si] = ' ';
			c++;
			si++;
		    }
		}
		else if (s[p] == '\n' || s[p] == '\r')
		    break;
#ifndef __MSDOS__
		/* Print non-printable chars as \XX hex numbers */
		else if (s[p] < 32 || s[p] >= 127)

		{
		    int msb = s[p] / 16;
		    int lsb = s[p] - 16 * msb;
		    msb %= 16;
		    if (si >= 0)
			S[si] = '\\';
		    c++;
		    si++;
		    if (si >= 0)
			S[si] = (msb > 9) ? ('A' + msb - 10) : ('0' + msb);
		    c++;
		    si++;
		    if (si >= 0)
			S[si] = (lsb > 9) ? ('A' + lsb - 10) : ('0' + lsb);
		    c++;
		    si++;
		}
#endif
		else
		    /* plain old printable ASCII character */
		{
		    if (si >= 0)
			S[si] = s[p];
		    c++;
		    si++;
		}
		p++;
	    }
	}
	Line[w] = 0;
	if (viewHex && r == 0)
	    strcpy(viewLine, Line);
	mvwprintw(panels[n], r, 0, "%s", Line);
    }
    /* Blank out any remaining lines */
    strcpy(Line, space);
    Line[w] = '\0';
    while (r < showLines[n])
	mvwprintw(panels[n], r++, 0, Line);
  end:
    free(s);
    if (exBuf >= 0)
	FREEBUF(exBuf);
  end2:
    FREEBUF(buf);
}

#if	__STDC__
void doViewNav(int w, int i)
#else
void doViewNav(w, i)
    int w, i;
#endif
{
    /* Fairly hackish approximations of the basic cursor key
	    movements */
    switch (i)
    {
    case 0:
	if (topLine > 0)
	    topLine--;
	break;
    case 1:
	if (topLine < (viewLines + 1))
	    topLine++;
	break;
    case 2:
	if (viewColumn > 0)
	    viewColumn--;
	break;
    case 3:
	viewColumn++;
	break;
    case 4:
	if (topLine >= showLines[w])
	    topLine -= showLines[w];
	else
	    topLine = 0;
	break;
    case 5:
	if (topLine < (viewLines - showLines[w]))
	    topLine += showLines[w];
	else
	    topLine = viewLines - 1;
	break;
    case 6:
	topLine = 0;
	break;
    case 7:
	topLine = viewLines - 1;
	break;
    }
    winInvalid[w] = 1;
    showList(w);
}

/* showList is now a bit of a misnomer; it should be showWindowContents
	or something */

#if	__STDC__
void showList(int n)
#else
void showList(n)
    int n;
#endif /* __STDC__ */
{
    int i;
    if (!testOption(VAR_DISPLAYON) || !listVis[n])
	return;

    selectAttrib(panels[n], 0);
    switch (winState[n])
    {
    case W_VIEW:
	{
	    if (winInvalid[n] == 2)
	    {
		int ex = GRABBUF(), ne = GRABBUF(), typ;
		char *vf;
		if (file2view[0] == '\0')	/* view file at cursor in window 1-n */
		    vf = makePath(paths[1 - n], INFO_NOW(1 - n).name);
		else		/* view the file given by file2view */
		    vf = file2view;
		typ = splitPath(vf, BUF[ex], BUF[ne]);
		if (typ >= 0 && BUF[ne][0])
		{
		    /* element of a container. Extract and view */
		    int exBuf = GRABBUF();
		    showFile(n, strcpy(fileViewed, getExtraction(BUF[ex], BUF[ne], BUF[exBuf], typ)));
		    FREEBUF(exBuf);
		    viewingDir = 0;
		}
		else if (typ >= 0 || typ == -3)	/* dir or container? - do a CD */
		{
		    int top = startLn[n];
		    strcpy(paths[n], BUF[ex]);
		    readList(n, 1);
		    if (strcmp(fileViewed, BUF[ex]))
		    {
			top = 0;
			strcpy(fileViewed, BUF[ex]);
			if (viewFP)
			{
			    CloseFile(viewFP);
			    viewFP = NULL;
			}
		    }
		    else if (top >= numfiles[n])
			top = numfiles[n] - 1;
		    startLn[n] = top;
		    for (i = 0; i < showLines[n]; i++)
			showInfo(n, i, startLn[n] + i);
		    viewingDir = 1;
		}
		else
		{
		    showFile(n, vf);
		    viewingDir = 0;
		}
		FREEBUF(ex);
		FREEBUF(ne);
	    }
	    else if (winInvalid[n] == 1)
	    {
		if (viewingDir)
		    for (i = 0; i < showLines[n]; i++)
			showInfo(n, i, startLn[n] + i);
		else
		    showFile(n, fileViewed);
	    }
	    break;
	}
    default:
	if (winInvalid[n] == 2)
	{
	    readList(n, 1);
	    startLn[n] = 0;
	}
	else if (winInvalid[n])
	    readList(n, 0);
	for (i = 0; i < showLines[n]; i++)
	    showInfo(n, i, startLn[n] + i);
	break;
    }
    winInvalid[n] = 0;
}

/*
 * normalise() is called after the cursor is moved, to ensure
 * the cursor is still displayed in the window.
 */

#if	__STDC__
void normalise(int n)
#else
void normalise(n)
    int n;
#endif /* __STDC__ */
{
    if ((highlight[n] - startLn[n]) >= showLines[n])
    {
	startLn[n] = highlight[n] - showLines[n] + 1;
    }
    else if (highlight[n] < startLn[n])
	startLn[n] = highlight[n];
    if (startLn[n] >= numfiles[n])
	startLn[n] = numfiles[n] - 1;
    if (highlight[n] >= numfiles[n])
	highlight[n] = numfiles[n] - 1;
}

#if __STDC__
void invalidateWindows(void)
#else
void invalidateWindows()
#endif
{
#ifdef __MSDOS__
    invalidateScreen();
#else
#ifdef NO_REDRAWWIN
    invalidateScreen();
#else
    int i;
    for (i = 0; i < 2; i++)
    {
	if (listVis[i])
	{
	    redrawwin(panelTop[i]);
	    redrawwin(panels[i]);
	}
	if (statusVis[i])
	{
	    redrawwin(statusTop[i]);
	    redrawwin(statusWin[i]);
	}
    }
    if (cmdWin)
	redrawwin(cmdWin);
#endif /* REDRAWWIN */
#endif /* MSDOS */
}

#if	__STDC__
void repaintScreen(void)
#else
void repaintScreen()
#endif
{
    int i;
    if (!inCurses || !testOption(VAR_DISPLAYON))
	return;

    /* Enable/disable status windows if necessary */

    for (i = 0; i < 2; i++)
    {
	if (statusVis[i] && (winState[i] == W_TREE || winState[i] == W_VIEW))
	{
	    showLines[i] += NUM_INFO_LNS + 2;
	    statusVis[i] = 0;
	    changedWinSize = 1;
	}
	else if (!statusVis[i] && winState[i] == W_LIST)
	{
	    showLines[i] -= NUM_INFO_LNS + 2;
	    statusVis[i] = 1;
	    changedWinSize = 1;
	}
    }

    /* exit and reenter curses to change the window sizes if necessary */

    if (changedWinSize)
    {
	exitCurses(1);
	setupCurses(1);
	invalidateWindows();
	changedWinSize = 0;
    }

    /* Because of various optimisations, some things happen for
		    the first time here. showList must come before anything
		    else, as it may do a readList and affect the status window,
		    etc. Also, we do a tree view first, as it can cause the
		    directory to change in the other window
    */

    /* Paint any trees */

    for (i = 0; i < 2; i++)
    {
	if (winState[i] == W_TREE)
	{
	    int buf = GRABBUF();
	    normalise(i);
	    showList(i);	/* must come first if readList has never been called */
	    showPath(i);
	    /* make sure the path in the other one corresponds */
	    getTreePath(i, BUF[buf]);
	    if (strcmp(BUF[buf], paths[1 - i]))
	    {
		winInvalid[1 - i] = 2;
		strcpy(paths[1 - i], BUF[buf]);
	    }
	    FREEBUF(buf);
	}
    }

    /* Paint any non-trees */

    for (i = 0; i < 2; i++)
    {
	if (winState[i] != W_TREE)
	{
	    normalise(i);
	    showList(i);	/* must come first if readList has never been called */
	    showPath(i);
	    showStatus(i);
	}
    }
}

#if	__STDC__
void syncScreen(void)
#else
void syncScreen()
#endif
{
    int i;
#if !defined(__MSDOS__) && !defined(USE_NCURSES)
    UPDATE(stdscr); /* Is this necessary?? */
#endif
    if (!inCurses || !testOption(VAR_DISPLAYON))
	return;
    for (i = 0; i < 2; i++)
    {
	if (listVis[i])
	{
	    UPDATE(panelTop[i]);
	    UPDATE(panels[i]);
	}
	if (statusVis[i])
	{
	    UPDATE(statusTop[i]);
	    UPDATE(statusWin[i]);
	}
    }
    UPDATE(cmdWin);
    REFRESH();
}

#if	__STDC__
static void winFail(void)
#else
static void winFail()
#endif
{
    clrscr();
    endwin();
    fprintf(stderr, "Failed to make a window - aborting\n");
    exit(-1);
}

#if	__STDC__
static void enableCursor(void)
#else
static void enableCursor()
#endif /* __STDC__ */
{
#ifdef __MSDOS__
    _setcursortype(_NORMALCURSOR);
#else
#ifndef NO_CURS_SET
    curs_set(1);
#endif
#endif
}

#if	__STDC__
static void disableCursor(void)
#else
static void disableCursor()
#endif /* __STDC__ */
{
#ifdef __MSDOS__
    _setcursortype(_NOCURSOR);
#else
#ifndef NO_CURS_SET
    curs_set(0);
#endif
#endif
}

#if	__STDC__
void enableSpecialKeys(void)
#else
void enableSpecialKeys()
#endif /* __STDC__ */
{
#if 0
    int i;
    for (i = 0; i < 2; i++)
    {
	if (statusVis[i] && statusTop[i])
	    KEYENABLE(statusTop[i]);
	if (listVis[i] && panels[i])
	    KEYENABLE(panels[i]);
    }
#else
    KEYENABLE(stdscr);
#endif
    if (cmdWin)
	KEYENABLE(cmdWin);
}

#if	__STDC__
void paintColour(void)
#else
void paintColour()
#endif
{
    int i;
    for (i = 0; i < 2; i++)
    {
	if (listVis[i])
	{
	    selectAttrib(panelTop[i], 0);
	    selectAttrib(panels[i], 0);
	}
	if (statusVis[i])
	{
	    selectAttrib(statusTop[i], 0);
	    selectAttrib(statusWin[i], 0);
	}
    }
    selectAttrib(cmdWin, 0);
}

#if	__STDC__
static void normaliseScreen(void)
#else
static void normaliseScreen()
#endif
{
    int i, maxL, maxF;
    int w0, w1;
    for (i = 0; i < 2; i++)
    {
	maxF = MAX_FILES + (statusVis[i] ? 0 : (NUM_INFO_LNS + 2));
	if (winWidth[i] < 20)
	    winWidth[i] = 20;
	else if (winWidth[i] > COLS)
	    winWidth[i] = COLS;
	if (showLines[i] < 3)
	    showLines[i] = 3;
	else if (showLines[i] > maxF)
	    showLines[i] = maxF;
    }
    w0 = listVis[0] * winWidth[0];
    w1 = listVis[1] * winWidth[1];
    if ((w0 + w1) > COLS)
    {
	int dif = (w0 + w1 - COLS + 1) / 2;
	winWidth[0] -= dif;
	winWidth[1] -= dif;
	w0 -= dif;
	w1 -= dif;
    }
    maxL = (showLines[0] > showLines[1]) ? showLines[0] : showLines[1];
    if (topMargin > (LINES - maxL - 2))
	topMargin = LINES - maxL - 2;
    if (leftMargin > (COLS - w0 - w1))
	leftMargin = COLS - w0 - w1;
    if (leftMargin < 0)
	leftMargin = 0;
    if (topMargin < 0)
	topMargin = 0;
}

#if	__STDC__
void setupCurses(int makeWins)
#else
void setupCurses(makeWins)
    int makeWins;
#endif /* __STDC__ */
{
    static int first = 1;
    if (first)
    {
	initscr();
#ifndef __MSDOS__
	nonl();
	raw();
	noecho();
#if !defined(NO_COLOR) && !defined(NO_COLOUR)
	if (allowColor && start_color() == ERR)
#else
	assign2var(VAR_ISCOLOR, "n");
#endif
#endif
	clrscr();
	showTitle();
	Lines = LINES;
	first = 0;
    }
    inCurses = 1;
    if (makeWins)
    {
	normaliseScreen();
	if (listVis[0])
	{
	    if ((panelTop[0] = newwin(showLines[0] + 2, winWidth[0], topMargin, leftMargin)) == NULL)
		winFail();
	    selectAttrib(panelTop[0], 0);
	    if ((panels[0] = subwin(panelTop[0], showLines[0], winWidth[0] - 2, topMargin + 1, leftMargin + 1)) == NULL)
		winFail();
	    selectAttrib(panels[0], 0);
	}
	else
	    panelTop[0] = panels[0] = NULL;
	if (listVis[1])
	{
	    if ((panelTop[1] = newwin(showLines[1] + 2, winWidth[1], topMargin, leftMargin + winWidth[0])) == NULL)
		winFail();
	    selectAttrib(panelTop[1], 0);
	    if ((panels[1] = subwin(panelTop[1], showLines[1], winWidth[1] - 2, topMargin + 1, leftMargin + winWidth[0] + 1)) == NULL)
		winFail();
	    selectAttrib(panels[1], 0);
	}
	else
	    panelTop[1] = panels[1] = NULL;
	if (statusVis[0])
	{
	    if ((statusTop[0] = newwin(NUM_INFO_LNS + 2, winWidth[0], topMargin + showLines[0] + 2, leftMargin)) == NULL)
		winFail();
	    selectAttrib(statusTop[0], 0);
	    if ((statusWin[0] = subwin(statusTop[0], NUM_INFO_LNS, winWidth[0] - 2, topMargin + showLines[0] + 3, leftMargin + 1)) == NULL)
		winFail();
	    selectAttrib(statusWin[0], 0);
	}
	else
	    statusTop[0] = statusWin[0] = NULL;
	if (statusVis[1])
	{
	    if ((statusTop[1] = newwin(NUM_INFO_LNS + 2, winWidth[1], topMargin + showLines[1] + 2, leftMargin + winWidth[0])) == NULL)
		winFail();
	    selectAttrib(statusTop[1], 0);
	    if ((statusWin[1] = subwin(statusTop[1], NUM_INFO_LNS, winWidth[1] - 2, topMargin + showLines[1] + 3, leftMargin + winWidth[0] + 1)) == NULL)
		winFail();
	    selectAttrib(statusWin[1], 0);
	}
	else
	    statusTop[1] = statusWin[1] = NULL;
	if ((cmdWin = newwin(1, COLS, LINES - 1, 0)) == NULL)
	    winFail();
	selectAttrib(cmdWin, 0);
	enableSpecialKeys();
    }
    disableCursor();
}

#if	__STDC__
static void freeupWin(WINDOW ** w)
#else
static void freeupWin(w)
    WINDOW **w;
#endif
{
    if (*w)
	delwin(*w);
    *w = NULL;
}

#if	__STDC__
void exitCurses(int delWins)
#else
void exitCurses(delWins)
    int delWins;
#endif
{
    int i;
    if (delWins)
    {
	for (i = 0; i < 2; i++)
	{
	    freeupWin(&panelTop[i]);
	    freeupWin(&panels[i]);
	    freeupWin(&statusTop[i]);
	    freeupWin(&statusWin[i]);
	}
	/* Next two lines are a hack to force attribute clearing on screen
		in clrscr() */
#ifndef __MSDOS__
	wattrset(cmdWin, 0);
	showMsg("");
	UPDATE(cmdWin);
	REFRESH();
#endif
	freeupWin(&cmdWin);
	enableCursor();
	clrscr();
	REFRESH();
    }
    endwin();
    inCurses = 0;
    if (viewPos)
    {
	free(viewPos);
	viewPos = NULL;
    }
}

#ifndef __MSDOS__

#ifndef USE_NCURSES
#ifdef HAS_TERM_H
#include <term.h>
#endif
#endif

#if __STDC__
void SHELL_MODE(char *msg)
#else
void SHELL_MODE(msg)
    char *msg;
#endif
{
    if (inCurses && msg)
    {
	wattrset(cmdWin, 0);
	showMsg(msg);
	UPDATE(cmdWin);
	REFRESH();
    }
#ifdef BUMP_CURSES
    endwin();
#else
    reset_shell_mode();
#endif
}

#if __STDC__
void PROG_MODE(void)
#else
void PROG_MODE()
#endif
{
    if (inCurses)
    {
#ifdef BUMP_CURSES
	doupdate();
#else
	mvcur(-1, -1, 0, 0);	/* Cursor repositioning hack */
	reset_prog_mode();
#ifndef __MSDOS__
	selectAttrib(cmdWin, 0);
	paintColour();
#endif
#endif
	enableSpecialKeys();
    }
}

#endif

#if	__STDC__
void cursorUp(void)
#else
void cursorUp()
#endif
{
    if (highlight[l] > 0)
    {
	highlight[l]--;
	normalise(l);
    }
}

#if	__STDC__
void cursorDown(void)
#else
void cursorDown()
#endif
{
    if (highlight[l] < (numfiles[l] - 1))
    {
	highlight[l]++;
	normalise(l);
    }
}

#if	__STDC__
void cursorPgUp(void)
#else
void cursorPgUp()
#endif
{
    if (highlight[l] != startLn[l])
	highlight[l] = startLn[l];
    else
    {
	int n = showLines[l];
	if (n > highlight[l])
	    n = highlight[l];
	startLn[l] = (highlight[l] -= n);
    }
    normalise(l);
}

#if	__STDC__
void cursorPgDn(void)
#else
void cursorPgDn()
#endif
{
    if (highlight[l] != (startLn[l] + showLines[l] - 1))
	highlight[l] = startLn[l] + showLines[l] - 1;
    else
    {
	if ((numfiles[l] - startLn[l]) >= showLines[l])
	{
	    startLn[l] += showLines[l];
	    highlight[l] = startLn[l] + showLines[l] - 1;
	}
    }
    normalise(l);
}

#if	__STDC__
void cursorHome(void)
#else
void cursorHome()
#endif
{
    highlight[l] = 0;
    normalise(l);
}

#if	__STDC__
void cursorEnd(void)
#else
void cursorEnd()
#endif
{
    highlight[l] = numfiles[l] - 1;
    normalise(l);
}


static WINDOW *readWin = NULL; /* hax I may need for ncurses */

#if	__STDC__
int readKey(void)
#else
int readKey()
#endif
{
    int rtn;
    if (readWin == NULL) readWin = cmdWin;
    KEYENABLE(readWin);
    rtn = wgetch(readWin);
#ifdef DEBUG
    if (testOption(VAR_DEBUGGING))
	fprintf(debug, "Got key press 0x%X\n", rtn);
#endif
    return rtn;
}

#if __STDC__
void ScreenMsg(char *msg)
#else
void ScreenMsg(msg)
    char *msg;
#endif
{
#ifdef __MSDOS__
    fprintf(stderr, "%s\n", msg);
#else
    wprintw(stdscr, "%s\n\r", msg);
    wrefresh(stdscr);
#endif
}

/***************************************
* Crude hypertext manual browser       *
***************************************/

#define MAX_REFS		16	/* maximum references per screen */
#define MAX_LINKS		250	/* maximum topics	*/
#define MAX_NAMES		3000	/* space for index names */
#define MAX_RHIST		16	/* max number of hypertext links that can be undone */
#define MAX_REF_LEN	72	/* Space for a reference's text */
#define EXTRA_REFS	5

#define H_NEXT	0
#define H_PREV	1
#define H_QUIT	2
#define H_BACK	3
#define H_INDEX	4

static char *extraRefs[] =
{"Next", "Prev", "Quit", "Back", "Index", NULL};

#define SkipSpace(p) while (buff[p] && (buff[p] == ' ' || buff[p]=='\t')) p++
#define SkipPast(p,c) while (buff[p] && buff[p++] != c)
#define SkipTo(p,c) while (buff[p] && buff[p] != c) p++
#define SkipIdent(p) while (buff[p] && (isalnum(buff[p]) || buff[p]=='_')) p++

/*
 * Compile the index file consisting of reference names and offsets
 */

#if __STDC__
void HyperCompile(char *fname)
#else
void HyperCompile(fname)
    char *fname;
#endif
{
    FILE *fp, *ofp;
    char *namespace, buff[132], iname[256], *sptr;
    long *indexes, pos;
    int icnt = 0, ncnt = 0, line = 0, p, q;
    strcpy(iname, fname);
    if ((sptr = STRRCHR(iname, '.')) != NULL)
	*sptr = '\0';
    strcat(iname, ".idx");
    if (STRCHR(iname, PATH_SEP))
	strcpy(buff, STRRCHR(iname, PATH_SEP) + 1);
    else
	strcpy(buff, iname);
#ifdef __MSDOS__
    if (!findWritePath(buff, forceCurrent ? "." : "$GCPATH\\", iname))
#else
    if (!findWritePath(buff, forceCurrent ? "." : "$GCPATH$HOME", iname))
#endif
    {
	showMsg("Cannot create output file for hypertext index");
	goto error0;
    }
    fp = OpenFile(fname, "r");
    if (fp == NULL)
    {
	showMsg("Cannot open hypertext manual");
	goto error0;
    }
    /* Allocate space */
    namespace = malloc(MAX_NAMES);
    if (namespace == NULL)
    {
	showMsg("Cannot allocate memory");
	goto error1;
    }
    indexes = (long *) malloc(MAX_LINKS * sizeof(long));
    if (indexes == NULL)
    {
	showMsg("Cannot allocate memory");
	goto error2;
    }
    while (!feof(fp))
    {
	pos = ftell(fp);
	line++;
	fgets(buff, 132, fp);
	if (strncmp(buff, "@entry{", 7) == 0)
	{
	    int pf = 7, pb;
	    while (buff[pf] == ' ' || buff[pf] == '\t')
		pf++;
	    pb = pf;
	    while ((buff[pb] >= 'A' && buff[pb] <= 'Z') || buff[pb] == '_' ||
		   (buff[pb] >= '0' && buff[pb] <= '9'))
		pb++;
	    if (pf == pb || buff[pb] != ',' || buff[pb] == '\0')
	    {
		sprintf(buff, "Bad @entry key on line %d\n", line);
		showMsg(buff);
		goto error3;
	    }
	    buff[pb] = '\0';
	    indexes[icnt++] = pos;
	    if (icnt == MAX_LINKS)
	    {
		showMsg("Maximum number of links exceeded");
		goto error3;
	    }
	    if ((ncnt + pb - pf + 1) >= MAX_NAMES)
	    {
		showMsg("Out of space for storing link names");
		goto error3;
	    }
	    strcpy(namespace + ncnt, buff + pf);
	    ncnt += pb - pf + 1;
/*		sprintf(buff,"Added hypertext entry %d", icnt);
			showMsg(buff);
*/
	}
    }
    /* Write the index file */
    ofp = OpenFile(iname, "w");
    if (ofp == NULL)
    {
	sprintf(buff, "Couldn't open index file %s for output", iname);
	showMsg(buff);
	goto error3;
    }
    fprintf(ofp, "Indices %d, Namespace %d\n", icnt, ncnt);
    for (p = q = 0; p < icnt; p++, q += strlen(namespace + q) + 1)
	fprintf(ofp, "%05ld %s\n", indexes[p], namespace + q);
    CloseFile(ofp);
    free(indexes);
    free(namespace);
    CloseFile(fp);
    return;
  error3:
    free(indexes);
  error2:
    free(namespace);
  error1:
    CloseFile(fp);
  error0:
    BEEP;
    sleep(1);
}

/* Parse and print a topic entry */

#if __STDC__
static char *printTopic(WINDOW * w, int row, char *buff, int centerText)
#else
static char *printTopic(w, row, buff, centerText)
    WINDOW *w;
    int row;
    char *buff;
    int centerText;
#endif
{
    int p = 0, q = strlen(buff);
    SkipPast(p, ',');
    /* SkipSpace(p); */
    while (--q, q > p && buff[q] != '}')
	;
    if (buff[q] == '}')
	buff[q] = '\0';
    mvwprintw(w, row, centerText ? (40 - (q - p) / 2) : 8, buff + p);
    return buff + p;
}

/*
 * Actually peruse the thing
 */

#if __STDC__
static void showHypertext(FILE * fp, int icnt, long *indexes,
			   char *namespace, short *nidx, short firstIdx)
#else
static void showHypertext(fp, icnt, indexes, namespace, nidx, firstIdx)
    FILE *fp;
    int icnt;
    long *indexes;
    char *namespace;
    short *nidx, firstIdx;
#endif
{
    short refRows[MAX_REFS + EXTRA_REFS], refCols[MAX_REFS + EXTRA_REFS], refIdx[MAX_REFS + EXTRA_REFS], r,
     c, i, refNow, refHist[MAX_RHIST], histNow = 0, gotEntry;
    char refText[MAX_REFS + EXTRA_REFS][MAX_REF_LEN], buff[100];
    int idxNow = firstIdx, key = 0, refCnt, p, q, cmd = -1, inIndex = 0,
     indexStart = 0;
    WINDOW *hw = newwin(24, 80, 0, 0);
    readWin = hw;
    for (p = 0; p < MAX_RHIST; p++)
	refHist[p] = 0;
    for (;;)
    {
	if (cmd >= 0)
	{
	    switch (cmd)
	    {
	    case H_NEXT:
		if (inIndex)
		{
		    indexStart += MAX_REFS;
		    if (indexStart >= icnt)
		    {
			indexStart -= MAX_REFS;
			if (indexStart < 0)
			    indexStart = 0;
		    }
		    idxNow = indexStart;
		}
		else
		{
		    idxNow++;
		    if (idxNow >= icnt)
			idxNow = icnt - 1;
		}
		break;
	    case H_PREV:
		if (inIndex)
		{
		    indexStart -= MAX_REFS;
		    if (indexStart < 0)
			indexStart = 0;
		    idxNow = indexStart;
		}
		else
		{
		    if (idxNow > 0)
			idxNow--;
		}
		break;
	    case H_QUIT:
		goto done;
	    case H_BACK:
		inIndex = 0;
		if (histNow > 0)
		    histNow--;
		if (histNow > 0)
		    histNow--;
		idxNow = refHist[histNow];
		break;
	    case H_INDEX:
		inIndex = 1;
		indexStart = idxNow - (idxNow % MAX_REFS);
		break;
	    }
	    cmd = -1;
	}
	/* Paint the screen */
	selectAttrib(hw, 0);
	fillAttribute(hw, 80, 24);
	Box(hw);
	if (inIndex)
	{
	    short ri;
	    refCnt = 0;
	    selectAttrib(hw, 2);
	    for (r = 0, ri = indexStart; r < MAX_REFS && ri < icnt; r++, ri++)
	    {
		refIdx[refCnt] = ri;
		refRows[refCnt] = r + 2;
		refCols[refCnt] = 8;
		fseek(fp, indexes[ri], 0);
		fgets(buff, 132, fp);
		strcpy(refText[refCnt], printTopic(hw, r + 2, buff, 0));
		refCnt++;
	    }
	    r += 2;
	}
	else
	{
	    if (histNow == MAX_RHIST)
	    {
		for (p = 1; p < MAX_RHIST; p++)
		    refHist[p - 1] = refHist[p];
		refHist[MAX_RHIST - 1] = idxNow;
	    }
	    else
		refHist[histNow++] = idxNow;
	    fseek(fp, indexes[idxNow], 0);
	    fgets(buff, 132, fp);
	    printTopic(hw, 2, buff, 1);
	    r = 4;
	    refCnt = 0;
	    while (!feof(fp) && r < 21 && refCnt < MAX_REFS)
	    {
		fgets(buff, 132, fp);
		buff[strlen(buff) - 1] = '\0';	/* remove \n */
		if (strncmp(buff, "@entry{", 7) == 0)
		    break;	/* next entry */
		if (STRCHR(buff, '@'))
		{
		    /* We have at least one reference. Process the line... */
		    p = q = 0;
		    c = 2;
		    for (;;)
		    {
		      NotRef:
			SkipTo(p, '@');
			if (!buff[p])
			{
			    mvwprintw(hw, r, c, buff + q);
			    break;
			}
			if (strncmp(buff + p + 1, "ref{", 4) != 0)
			{
			    p++;
			    goto NotRef;
			}
			buff[p++] = '\0';
			mvwprintw(hw, r, c, buff + q);
			c += strlen(buff + q);
			if (refCnt >= MAX_REFS)
			    break;
			SkipPast(p, '{');
			SkipSpace(p);
			q = p;
			SkipIdent(p);
			if (!buff[p])
			    break;
			if (buff[p] != ',')
			{
			    buff[p++] = '\0';
			    SkipPast(p, ',');
			}
			else
			    buff[p++] = '\0';
			/* Find the entry... */
			gotEntry = 0;
			for (i = 0; i < icnt; i++)
			{
			    if (strcmp(namespace + nidx[i], buff + q) == 0)
			    {
				refIdx[refCnt] = i;
				refRows[refCnt] = r;
				refCols[refCnt] = c;
				gotEntry = 1;
				break;
			    }
			}
			SkipSpace(p);
			if (!buff[p])
			    break;
			q = p;
			SkipTo(p, '}');
			if (!buff[p])
			    break;
			buff[p++] = '\0';
			if (gotEntry)
			{
			    strcpy(refText[refCnt], buff + q);
			    refCnt++;
			}
			/* Turn on bold attribute and print the word... */
			selectAttrib(hw, 2);
			mvwprintw(hw, r, c, buff + q);
			selectAttrib(hw, 0);
			c += strlen(buff + q);
			q = p;
		    }
		}
		else
		    mvwprintw(hw, r, 2, buff);
		r++;
	    }
	}
	/* Print the Previous, Next, Back, Return entries, and maintain history */
	p = 0;
	q = 20;
	r++;
	refNow = refCnt;	/* default is `next' */
	while (extraRefs[p])
	{
	    refIdx[refCnt] = p;
	    refRows[refCnt] = r;
	    refCols[refCnt] = q;
	    strcpy(refText[refCnt], extraRefs[p]);
	    selectAttrib(hw, 2);
	    mvwprintw(hw, r, q, extraRefs[p]);
	    selectAttrib(hw, 0);
	    q += 8;
	    p++;
	    refCnt++;
	}
	/* get a key press */
	key = 0;
	while (key != 27 && key != 10 && key != 13)
	{
	    /* Show the current reference */
	    if (refCnt)
	    {
		selectAttrib(hw, 3);
		mvwprintw(hw, refRows[refNow], refCols[refNow], refText[refNow]);
	    }
	    UPDATE(hw);
	    REFRESH();
	    key = my_getch();
	    if (refCnt)
	    {
		selectAttrib(hw, 2);
		mvwprintw(hw, refRows[refNow], refCols[refNow], refText[refNow]);
	    }
	    switch (key)
	    {
	    case KEY_LEFT:
	    case KEY_UP:
		refNow--;
		if (refNow < 0)
		    refNow = refCnt - 1;
		break;
	    case KEY_RIGHT:
	    case KEY_DOWN:
	    case 9:
		refNow++;
		if (refNow >= refCnt)
		    refNow = 0;
		break;
	    case 27:
		if (histNow > 1)
		    cmd = H_BACK;
		else if (refNow == (refCnt - EXTRA_REFS + H_QUIT))
		    cmd = H_QUIT;
		else
		{
		    refNow = (refCnt - EXTRA_REFS + H_QUIT);
		    key = 0;	/* don't quit this screen yet! */
		}
		break;
	    case 10:
	    case 13:
		if (refNow >= (refCnt - EXTRA_REFS))
		    cmd = refIdx[refNow];
		else
		{
		    inIndex = 0;
		    idxNow = refIdx[refNow];
		}
		/* clear the ref attributes */
/*			while (refCnt--)
					paintAttrib(hw, Attribute[0], refRows[refCnt], refCols[refCnt], refText[refCnt]); */
		refCnt = 0;
		break;
	    default:
		if (refCnt)
		{
		    int rf = 0, key2;
		    if (islower(key))
			key -= 'a' - 'A';
		    while (rf < refCnt)
		    {
			key2 = (int) refText[rf][0];
			if (islower(key2))
			    key2 -= 'a' - 'A';
			if (key2 == key)
			{
			    refNow = rf;
			    break;
			}
			else
			    rf++;
		    }
		}
		break;
	    }
	}
    }
  done:
    readWin = NULL;
    delwin(hw);
    repaintScreen();
    /*syncScreen();*/
}

/*
 * Interface to above peruser
 */

#if __STDC__
void HyperBrowse(char *fname, char *start)
#else
void HyperBrowse(fname, start)
    char *fname, *start;
#endif
{
    FILE *fp, *ofp;
    char *sptr, *namespace, helpFile[256], helpIndex[256], indexName[128];
    long *indexes;
    short *nidx, firstIdx = 0;
    int icnt = 0, ncnt = 0, p, q;
    assert(strlen(fname) < 125);
    strcpy(indexName, fname);
    if ((sptr = STRRCHR(indexName, '.')) != NULL)
	*sptr = '\0';
    strcat(indexName, ".idx");
#ifdef __MSDOS__
    if (!findReadPath(fname, "$GCPATH\\.$PATH", NULL, helpFile))
#else
    if (!findReadPath(fname, "$GCPATH$HOME$PATH/.", NULL, helpFile))
#endif
    {
	showMsg("Cannot find hypertext manual");
	goto error00;
    }
#ifdef __MSDOS__
    if (!findReadPath(indexName, "$GCPATH\\.$PATH", NULL, helpIndex) ||
#else
    if (!findReadPath(indexName, "$GCPATH$HOME$PATH/.", NULL, helpIndex) ||
#endif
	getFileModTime(helpFile) > getFileModTime(helpIndex))
    {
	HyperCompile(helpFile);
#ifdef __MSDOS__
	if (!findReadPath(indexName, "$GCPATH\\.$PATH", NULL, helpIndex))
#else
	if (!findReadPath(indexName, "$GCPATH$HOME.$PATH/", NULL, helpIndex))
#endif
	{
	    showMsg("Cannot find hypertext index");
	    goto error00;
	}
    }
    fp = OpenFile(helpFile, "r");
    if (fp == NULL)
    {
	showMsg("Cannot open hypertext manual");
	goto error00;
    }
    /* Read in the indices */
    ofp = OpenFile(helpIndex, "r");
    if (ofp == NULL)
    {
	showMsg("Couldn't open index file for input");
	goto error0;
    }
    fscanf(ofp, "Indices %d, Namespace %d\n", &icnt, &ncnt);
    /* Allocate space */
    namespace = malloc(ncnt);
    if (namespace == NULL)
    {
	showMsg("Cannot allocate memory");
	goto error1;
    }
    indexes = (long *) malloc(icnt * sizeof(long));
    if (indexes == NULL)
    {
	showMsg("Cannot allocate memory");
	goto error2;
    }
    nidx = (short *) malloc(icnt * sizeof(short));
    if (nidx == NULL)
    {
	showMsg("Cannot allocate memory");
	goto error3;
    }
    STRUPR(start);
    for (p = q = 0; p < icnt; p++, q += strlen(namespace + q) + 1)
    {
	nidx[p] = q;
	fscanf(ofp, "%05ld %s", &indexes[p], namespace + q);
	if (strcmp(namespace + q, start) == 0)
	    firstIdx = p;
    }
    if (start[0] == '\0')
	firstIdx = 0;
    CloseFile(ofp);
    showHypertext(fp, icnt, indexes, namespace, nidx, firstIdx);
    free(nidx);
    free(indexes);
    free(namespace);
    CloseFile(fp);
    return;
  error3:
    free(indexes);
  error2:
    free(namespace);
  error1:
    CloseFile(ofp);
  error0:
    CloseFile(fp);
  error00:
    BEEP;
    sleep(1);
}

/* Script-defined windows. Each window consists of a size and
		position, and up to 40 items. Each item in turn has a
		position, attribute index, type, and type-dependent value (2 shorts).
		The types and values are:

			Type			Value 1			Value 2
			---------------------------------------------------------
			W_TEXT			text arg		unused (done)
			W_YNTOGGLE		var arg			column num in var
			W_MULTITOGL		var arg			opt string arg
			W_ACTION		text arg		function name arg
			W_EXITTEXT		text arg		unused (done, untested)
			W_EXITKEY		keyspec			text arg
			W_RADIO			var arg			column # in var
			W_EDITTEXT		var arg			unused

			Only one W_RADIO can be set at any time per column.
*/

#define MAX_SWINS		4
#define MAX_SWIN_ENTS	40

typedef enum
{
    W_TEXT, W_YNTOGGLE, W_MULTITOGL, W_ACTION, W_EXITTEXT, W_EXITKEY, W_RADIO,
    W_EDITTEXT
} sWinElt_t;

typedef struct
{
    short r, c, a, n;		/* row, col, attrib, # vals if toggle */
    sWinElt_t type;
    short a1, a2;		/* args as above */
} sWinEltEntry_t;

typedef struct
{
    short b, r, c, w, h, e, a;	/* size, position, entry counts, attrib */
    sWinEltEntry_t entries[MAX_SWIN_ENTS];
} sWin_t;

sWin_t *sWindows;
static short topSWin = -1, sWinRow, sWinCol, sWinAttr;

#ifdef _STDC__
void sWinInit(void)
#else
void sWinInit()
#endif
{
    if (sWindows == NULL)
	sWindows = (sWin_t *) malloc(MAX_SWINS * sizeof(sWin_t));
    assert(sWindows);
}

#ifdef _STDC__
void sWinShutdown(void)
#else
void sWinShutdown()
#endif
{
    if (sWindows)
    {
	free(sWindows);
	sWindows = NULL;
    }
}

#if __STDC__
int sWinMake(short top, short left, short height, short width)
#else
int sWinMake(top, left, height, width)
    short left, top, height, width;
#endif
{
    if (++topSWin < MAX_SWINS)
    {
	if ((top + height) >= (LINES - 1))
	    height = LINES - 1 - top;
	if ((left + width) >= COLS)
	    width = COLS - left;
	sWindows[topSWin].r = top;
	sWindows[topSWin].c = left;
	sWindows[topSWin].w = width;
	sWindows[topSWin].h = height;
	sWindows[topSWin].e = 0;
	sWindows[topSWin].b = 0;
	sWindows[topSWin].a = sWinAttr;
	return 0;
    }
    else
    {
	showMsg("Too many windows!");
	topSWin--;
	return -1;
    }
}

#if __STDC__
void sWinLocate(short row, short col)
#else
void sWinLocate(row, col)
    short row, col;
#endif
{
    sWinRow = row;
    sWinCol = col;
}

#if __STDC__
void sWinBox(void)
#else
void sWinBox()
#endif
{
    sWindows[topSWin].b = 1;
}

#if __STDC__
void sWinSelAttrib(short attrib)
#else
void sWinSelAttrib(attrib)
    short attrib;
#endif
{
    sWinAttr = attrib;
}

#if __STDC__
static int makeSWinEntry(sWinElt_t type, short a1, short a2)
#else
static int makeSWinEntry(type, a1, a2)
    sWinElt_t type;
    short a1, a2;
#endif
{
    if (sWindows[topSWin].e < MAX_SWIN_ENTS)
    {
	int e = sWindows[topSWin].e++;
	sWindows[topSWin].entries[e].r = sWinRow;
	sWindows[topSWin].entries[e].c = sWinCol;
	sWindows[topSWin].entries[e].a = sWinAttr;
	sWindows[topSWin].entries[e].type = type;
	sWindows[topSWin].entries[e].a1 = a1;
	sWindows[topSWin].entries[e].a2 = a2;
	sWindows[topSWin].entries[e].n = 0;
	return 0;
    }
    else
	return -1;
}

#if __STDC__
int sWinText(short arg)
#else
int sWinText(arg)
    short arg;
#endif
{
    return makeSWinEntry(W_TEXT, arg, 0);
}

#if __STDC__
int sWinToggle(short arg1, short arg2)
#else
int sWinToggle(arg1, arg2)
    short arg1, arg2;
#endif
{
    return makeSWinEntry(W_MULTITOGL, arg1, arg2);
}

#if __STDC__
int sWinEditText(short arg, short len)
#else
int sWinEditText(arg, len)
    short arg, len;
#endif
{
    return makeSWinEntry(W_EDITTEXT, arg, len);
}

#if __STDC__
int sWinExitText(short arg)
#else
int sWinExitText(arg)
    short arg;
#endif
{
    return makeSWinEntry(W_EXITTEXT, arg, 0);
}

#if __STDC__
int sWinYNToggle(short var, short fld)
#else
int sWinYNToggle(var, fld)
    short var, fld;
#endif
{
    return makeSWinEntry(W_YNTOGGLE, var, fld);
}

#if __STDC__
int sWinRadio(short var, short fld)
#else
int sWinRadio(var, fld)
    short var, fld;
#endif
{
    return makeSWinEntry(W_RADIO, var, fld);
}

#if __STDC__
int sWinAction(short text, short hook)
#else
int sWinAction(text, hook)
    short text, hook;
#endif
{
    return makeSWinEntry(W_ACTION, text, hook);
}

#if __STDC__
int testToggle(short arg)
#else
int testToggle(arg)
    short arg;
#endif
{
    int b = GRABBUF();
    lookupArg(arg, b);
    FREEBUF(b);
    return ((BUF[b][0] >= '1' && BUF[b][0] <= '9') || BUF[b][0] == 'y' || BUF[b][0] == 'Y');
}

#if __STDC__
static int showSWinElt(WINDOW * sw, sWinEltEntry_t * ep, int invert)
#else
static int showSWinElt(sw, ep, invert)
    WINDOW *sw;
    sWinEltEntry_t *ep;
    int invert;
#endif
{
    char *a1p;
    int rtn = 0, buf = GRABBUF();
    if (invert)			/* hackish! */
	selectAttrib(sw, ep->a + ((ep->a % 2) ? -1 : 1));
    else
	selectAttrib(sw, ep->a);
    switch (ep->type)
    {
    case W_EDITTEXT:
    case W_EXITTEXT:
    case W_TEXT:
    case W_ACTION:
	a1p = lookupArg(ep->a1, buf);
	mvwprintw(sw, ep->r, ep->c, a1p[0] ? a1p : "[none]");
	break;
    case W_MULTITOGL:
	{
	    char *cp, *es = NULL;
	    int i = atoi(lookupVar(ep->a1, buf)), N = 0;	/* N holds count of options */
	    cp = lookupArg(ep->a2, buf);
	    /* The joy of C!... skip to and extract the option name field */
	    while ((cp = STRCHR(cp, ':')) != NULL && (N++, i-- > 0))
		cp++;
	    if (cp)
		es = STRCHR(++cp, ':');
	    if (cp && es)
	    {
		*es++ = '\0';
		mvwprintw(sw, ep->r, ep->c, "<%s>", cp);
		rtn = 2 + strlen(cp);
		if (*es && ep->n == 0)
		{
		    /* Wasn't last option, so count the rest */
		    while ((es = STRCHR(es, ':')) != NULL)
		    {
			es++;
			N++;
		    }
		}
	    }
	    else
	    {
		mvwprintw(sw, ep->r, ep->c, "<?>");
		showMsg("Bad toggle value!");
	    }
	    if (ep->n == 0)
		ep->n = N;
	}
	break;
    case W_YNTOGGLE:
	mvwprintw(sw, ep->r, ep->c, testToggle(ep->a1) ? "<Y>" : "<N>");
	break;
    case W_RADIO:
	mvwprintw(sw, ep->r, ep->c, testToggle(ep->a1) ? "(*)" : "( )");
	break;
    default:
	showMsg("Form entry type not implemented yet");
	break;
    }
    FREEBUF(buf);
    return rtn;
}

#if __STDC__
static void setToggleVal(sWinEltEntry_t * e, char *v)
#else
static void setToggleVal(e, v)
    sWinEltEntry_t *e;
    char *v;
#endif
{
    if (e->a2 >= 0)
    {
	int b = GRABBUF();
	lookupVar(e->a1, b);
	if ((int) strlen(BUF[b]) > (int) e->a2)
	    BUF[b][e->a2] = *v;
	assign2var(e->a1, BUF[b]);
	FREEBUF(b);
    }
    else
	assign2var(e->a1, v);
}

#if __STDC__
char *sWinProcess(void)
#else
char *sWinProcess()
#endif
{
    /* Build and process the topmost window. Free it after use,
		    and return the exit text
    */
    static char rtnString[32];
    sWin_t *wp;
    WINDOW *sw;
    sWinEltEntry_t *ep;
    int Len, E, e, r = -1, k = 0, buf;
    if (topSWin < 0)
	return "";
    wp = &sWindows[topSWin];
    sw = newwin(wp->h, wp->w, wp->r, wp->c);
    if (sw == NULL)
	return "";
    readWin = sw;
    buf = GRABBUF();
  restart:
    syncScreen();
    KEYENABLE(sw);
    selectAttrib(sw, wp->a);
    fillAttribute(sw, wp->w, wp->h);
    if (wp->b)
	Box(sw);
    for (e = 0; e < wp->e; e++)
    {
	ep = &wp->entries[e];
	(void) showSWinElt(sw, ep, 0);
	switch (ep->type)
	{
	case W_EDITTEXT:
	case W_EXITTEXT:
	case W_YNTOGGLE:
	case W_MULTITOGL:
	case W_RADIO:
	case W_ACTION:
	    if (r < 0)
		r = e;
	default:
	    break;
	}
    }
    strcpy(rtnString, "ESC");
    e = r;
    while (k != 27)
    {
	ep = &wp->entries[e];
	(void) showSWinElt(sw, ep, 1);
	UPDATE(sw);
	REFRESH();
	k = readKey();
	Len = showSWinElt(sw, ep, 0);
	if (k == GetKeyCode(HELP_HOOK))
	{
	    (void) ExecuteKey(HELP_HOOK);
	    repaintScreen();
	    goto restart;	/* repaint screen */
	}
	else
	    switch (k)
	    {
	    case 27:
		break;
	    case 'n':
	    case 'N':
		if (ep->type != W_YNTOGGLE)
		    break;
	    case KEY_LEFT:
		if (ep->type == W_YNTOGGLE)
		    setToggleVal(ep, "n");
		else if (ep->type == W_MULTITOGL)
		{
		    int v = atoi(lookupArg(ep->a1, buf));
		    if (v > 0)
		    {
			v--;
			sprintf(BUF[buf], "%d", v);
			assign2var(ep->a1, BUF[buf]);
			goto eraseArea;
		    }
		}
		else		/* fall thru ... */
	    case KEY_UP:
		{
		    E = e;
		    while (E-- > 0)
		    {
			if (wp->entries[E].type != W_TEXT)
			{
			    e = E;
			    break;
			}
		    }
		}
		break;
	    case 'y':
	    case 'Y':
		if (ep->type != W_YNTOGGLE)
		    break;
	    case KEY_RIGHT:
		if (ep->type == W_YNTOGGLE)
		    setToggleVal(ep, "y");
		else if (ep->type == W_MULTITOGL)
		{
		    int v = atoi(lookupArg(ep->a1, buf));
		    if (++v < ep->n)
		    {
			sprintf(BUF[buf], "%d", v);
			assign2var(ep->a1, BUF[buf]);
			goto eraseArea;
		    }
		}
		else		/* fall thru ... */
	    case KEY_DOWN:
	    case 9:		/* tab */
	    case 10:		/* lf */
		{
		    E = e;
		    while (++E < wp->e)
		    {
			if (wp->entries[E].type != W_TEXT)
			{
			    e = E;
			    break;
			}
		    }
		}
		break;
	    case 13:		/* cr */
		if (ep->type == W_EDITTEXT)
		{
		    int mode = 1;
		    WINDOW *ew = newwin(1, ep->a2 + 1, wp->r + ep->r, wp->c + ep->c);
		    lookupVar(ep->a1, buf);
		    showFileCursor = 0;
		    myRead(ew, ep->a2 + 1, 0, 0, BUF[buf], ep->a2, &mode,
			   gcEditWrap, my_getch);
		    showFileCursor = 1;
		    delwin(ew);
		    assign2var(ep->a1, BUF[buf]);
		    Len = ep->a2 + 1;
		    goto eraseArea;
		}
		else if (ep->type == W_EXITTEXT)
		{
		    strncpy(rtnString, lookupArg(ep->a1, buf), 30);
		    goto done;
		}
		else if (ep->type == W_ACTION)
		{
		    subExecute(ep->a2);
		    r = e;
		    repaintScreen();
		    goto restart;
		}
		else		/* fall thru... */
	    case ' ':
		if (ep->type == W_RADIO && !testToggle(ep->a1))
		{
		    /* turn on the radio button and clear others in column */
		    int R, C = ep->c;
		    for (R = 0; R < wp->e; R++)
		    {
			if (wp->entries[R].type == W_RADIO &&
			    wp->entries[R].c == C)
			{
			    setToggleVal(&wp->entries[R], "n");
			    (void) showSWinElt(sw, &wp->entries[R], 0);
			}
		    }
		    setToggleVal(ep, "y");
		}
		else if (ep->type == W_MULTITOGL)
		{
		    int v = atoi(lookupArg(ep->a1, buf));
		    if (++v >= ep->n)
			v = 0;
		    sprintf(BUF[buf], "%d", v);
		    assign2var(ep->a1, BUF[buf]);
		    goto eraseArea;
		}
		else if (ep->type == W_YNTOGGLE)
		{
		    if (testToggle(ep->a1))
			setToggleVal(ep, "n");
		    else
			setToggleVal(ep, "y");
		}
		break;
	      eraseArea:
		strcpy(BUF[buf], "                                                                               ");
		/* should use memset */
		BUF[buf][Len] = '\0';
		selectAttrib(sw, wp->a);
		mvwprintw(sw, ep->r, ep->c, BUF[buf]);
		break;
	    default:
		{
		    int b = GRABBUF(), ch;
		    if (k >= 'a' && k <= 'z')
			k -= 'a' - 'A';
		    E = e;
		    while (++E < wp->e)
		    {
			switch (wp->entries[E].type)
			{
			case W_EXITTEXT:
			case W_ACTION:
			    ch = lookupArg(wp->entries[E].a1, buf)[0];
			    if (ch >= 'a' && ch <= 'z')
				ch -= 'a' - 'A';
			    if (ch == k)
			    {
				e = E;
				goto matched;
			    }
			default:;
			}
		    }
		    E = 0;
		    while (++E < e)
		    {
			switch (wp->entries[E].type)
			{
			case W_EXITTEXT:
			case W_ACTION:
			    ch = lookupArg(wp->entries[E].a1, buf)[0];
			    if (ch >= 'a' && ch <= 'z')
				ch -= 'a' - 'A';
			    if (ch == k)
			    {
				e = E;
				goto matched;
			    }
			default:;
			}
		    }
		  matched:;
		    FREEBUF(b);
		}
		break;
	    }
    }
  done:
    readWin = NULL;
    delwin(sw);
    repaintScreen();
    topSWin--;
    FREEBUF(buf);
    return rtnString;
}
