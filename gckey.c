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


/*****************************************************************

		GC3 KEYSTROKE MANAGER

 The code in this file is responsible for:

	* maintaining key bindings
	* reading keystrokes from the keyboard
	* command line editing and recall
	* keystroke macros

 (c) 1993 by Graham Wheeler

****************************************************************/

#include "gcport.h"
#include "gc3.h"
#include "gckey.h"

#define GC_CURSES		/* for locateCursor */
#include "gclib.h"

static char cmdHist[HIST_LEN][256];
static int histNum = 0, histNow = 0;

#if __MSDOS__

static short keycodeTbl[MAX_KEYS] =
{
 /* Cursor keys */
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_PGUP, KEY_PGDN, KEY_HOME, KEY_END,
    KEY_CTRL_LEFT, KEY_CTRL_RIGHT, 0, 0,
    KEY_CTRL_PGUP, KEY_CTRL_PGDN, KEY_CTRL_HOME, KEY_CTRL_END,
    KEY_INS, KEY_DEL, KEY_BACKSPC,
 /* #19 */
    0,
 /* Function Keys 20-29 */
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F0,
 /* Ctrl-Keys 30-55 */
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
    14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
 /* #56-#59 */
    0, 0, 0, 0,
 /* Meta-0 .. Meta-9 */
    KEY_ALT_0, KEY_ALT_1, KEY_ALT_2, KEY_ALT_3, KEY_ALT_4,
    KEY_ALT_5, KEY_ALT_6, KEY_ALT_7, KEY_ALT_8, KEY_ALT_9,
 /* Meta-A .. Meta-Z 70-95 */
    KEY_ALT_A, KEY_ALT_B, KEY_ALT_C, KEY_ALT_D, KEY_ALT_E,
    KEY_ALT_F, KEY_ALT_G, KEY_ALT_H, KEY_ALT_I, KEY_ALT_J,
    KEY_ALT_K, KEY_ALT_L, KEY_ALT_M, KEY_ALT_N, KEY_ALT_O,
    KEY_ALT_P, KEY_ALT_Q, KEY_ALT_R, KEY_ALT_S, KEY_ALT_T,
    KEY_ALT_U, KEY_ALT_V, KEY_ALT_W, KEY_ALT_X, KEY_ALT_Y,
    KEY_ALT_Z,
 /* #96 - #99 */
    0, 0, 0, 0,
 /* Printable ASCII */
    32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
    60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
    70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
    90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
    110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126,
 /* #195..#239 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0,
 /* Command line edit keys */
    KEY_F1,
    KEY_CTRL_HOME,
    KEY_HOME, KEY_END, KEY_LEFT, KEY_RIGHT,
    KEY_DEL, KEY_BACKSPC, KEY_INS, 27,
    KEY_ALT_L, KEY_ALT_R,
    KEY_ALT_P, KEY_ALT_N,
    KEY_CTRL_LEFT, KEY_CTRL_RIGHT,
    KEY_CTRL_PGUP, KEY_CTRL_PGDN
};

#else

static int keycodeTbl[MAX_KEYS] =
{
 /* Cursor keys */
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_PPAGE, KEY_NPAGE, KEY_HOME, KEY_END,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, '\b',
 /* #19 */
    0,
 /* Function Keys 20-29 */
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F0,
 /* Ctrl-Keys 30-55 */
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
    14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
 /* #56-#59 */
    0, 0, 0, 0,
 /* Meta-0 .. Meta-9 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 /* Meta-A .. Meta-Z 70-95 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 /* #96 - #99 */
    0, 0, 0, 0,
 /* Printable ASCII */
    32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
    60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
    70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
    90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
    110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126,
 /* #195..#239 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0,
 /* Command line edit keys */
    KEY_F1,
    0, KEY_HOME, KEY_END, KEY_LEFT, KEY_RIGHT,
    0, 0, 0, 0,
    0, 0,
    0, 0,
    0, 0,
    0, 0
};

#endif

keydef_t KeyTbl[MAX_KEYS];

#if	__STDC__
void InitialiseKeyTable(void)
#else
void InitialiseKeyTable()
#endif
{
    int i;
    for (i = 0; i < MAX_KEYS; i++)
	KeyTbl[i].ip = 0;
}

/**********************************************************
	KEYSTROKE MACROS...
***********************************************************/

static short	Record = 0;
static short	PlayBack = 0;
static short	MacNum = 0;
static short	MacPos = 0;
static short	MacLen[NUMMACROS] = {0};
static int	Macros[NUMMACROS][MACROLEN];

#if __STDC__
void StopRecordingMacro(void)
#else
void StopRecordingMacro()
#endif
{
    if (Record)
    {
	Record = 0;
	MacLen[MacNum] = MacPos - 1;
	showMsg("Recording complete");
    }
}

#if __STDC__
void StartRecordingMacro(short MNum)
#else
void StartRecordingMacro(MNum)
    short MNum;
#endif
{
    int cnf;
    if (Record)
	StopRecordingMacro();
    else if (PlayBack)
	showMsg("Cannot record macro while playing back");
    else if (MNum < 0 || MNum >= NUMMACROS)
	showMsg("Illegal macro number");
    else
    {
	if (MacLen[MNum])
	{
	    showMsg("Overwrite existing macro y/n?");
	    BEEP;
	    cnf = readKey();
	}
	else
	    cnf = 'Y';
	if (cnf == 'Y' || cnf == 'y')
	{
	    Record = 1;
	    MacPos = 0;
	    MacLen[MNum] = 0;
	    MacNum = MNum;
	    showMsg("Recording macro");
	}
	else
	{
	    Record = 0;
	    clearCmdWin();
	}
    }
}

#if __STDC__
void playMacro(short MNum)
#else
void playMacro(MNum)
    short MNum;
#endif
{
    if (Record)
	showMsg("Cannot play a macro while already recording");
    else if (PlayBack)
	showMsg("Can play a macro while already playing");
    else if (MNum < 0 || MNum >= NUMMACROS)
	showMsg("Invalid macro number - cannot play back");
    else
    {
	PlayBack = 1;
	MacNum = MNum;
	MacPos = 0;
    }
}

#if __STDC__
int my_getch(void)
#else
int my_getch()
#endif
{
    int rtn;
    if (PlayBack)
    {
	rtn = Macros[MacNum][MacPos++];
	if (MacPos >= MacLen[MacNum])
	    PlayBack = 0;
    }
    else
	rtn = readKey();
    if (Record)
    {
	if (MacPos >= MACROLEN)
	{
	    showMsg("Macro length exceeded");
	    Record = 0;
	    MacLen[MacNum] = MACROLEN;
	}
	else
	    Macros[MacNum][MacPos++] = rtn;
    }
    return rtn;
}

#if __STDC__
void cursorAction(int i)
#else
void cursorAction(i)
    int i;
#endif
{
    int v = testOption(VAR_VIEWER), oldIdx = highlight[l];
    if (winState[l] == W_VIEW && !viewingDir)
	doViewNav(l, i);
    else
    {
	switch (i)
	{
	case 0:
	    cursorUp();
	    break;
	case 1:
	    cursorDown();
	    break;
	case 2:
	    if (!v)
	    {
		l = 0;
		cd2old();
	    }
	    break;
	case 3:
	    if (!v)
	    {
		l = 1;
		cd2old();
	    }
	    break;
	case 4:
	    cursorPgUp();
	    break;
	case 5:
	    cursorPgDn();
	    break;
	case 6:
	    cursorHome();
	    break;
	case 7:
	    cursorEnd();
	    break;
	}
	if (oldIdx != highlight[l])
	{
	    /* If we are viewing the file at the cursor, and the
		    cursor has moved, we invalidate the view window */
	    if (winState[1 - l] == W_VIEW && file2view[0] == '\0')
		winInvalid[1 - l] = 2;
	}
    }
}

#if __STDC__
int ExecuteKey(int i)
#else
int ExecuteKey(i)
    int i;
#endif
{
    if (i == HELP_HOOK)
    {
	int buf1 = GRABBUF(), buf2 = GRABBUF();
	char *hf = lookupVar(VAR_HELPFILE, buf1);
	char *he = lookupVar(VAR_HELPENTRY, buf2);
	HyperBrowse(hf, he);
	FREEBUF(buf2);
	FREEBUF(buf1);
    }
    else if (KeyTbl[i].ip)
    {
	if (Interpret(KeyTbl[i].ip, 0, 0))
	    return 1;		/* quit */
    }
    else if (i < 8)
	cursorAction(i);
    return 0;
}

/************************************************************
	 COMMAND LINE HISTORY
*************************************************************/

#if __STDC__
char *getPrevCmd(char *prefix)
#else
char *getPrevCmd(prefix)
    char *prefix;
#endif
{
#ifdef DEBUG
    fprintf(debug, "On entry in getPrevCmd - histNow = %d, histNum = %d, cmd = %s\n",
	    histNow, histNum, cmdHist[histNow]);
#endif
    if (histNum)
    {
	int len = strlen(prefix);
	if (len)
	{
	    int i = histNow;
	    do
	    {
		if (--i < 0)
		    i = histNum - 1;
		if (strncmp(prefix, cmdHist[i], len) == 0)
		    return cmdHist[histNow = i];
	    }
	    while (i != histNow);
	}
	histNow--;
	if (histNow < 0)
	    histNow = histNum - 1;
#ifdef DEBUG
	fprintf(debug, "On exit in getPrevCmd - histNow = %d, histNum = %d, cmd = %s\n",
		histNow, histNum, cmdHist[histNow]);
#endif
	return cmdHist[histNow];
    }
    return NULL;
}

#if __STDC__
char *getNextCmd(char *prefix)
#else
char *getNextCmd(prefix)
    char *prefix;
#endif
{
#ifdef DEBUG
    fprintf(debug, "On entry in getNextCmd - histNow = %d, histNum = %d, cmd = %s\n",
	    histNow, histNum, cmdHist[histNow]);
#endif
    if (histNum)
    {
	int len = strlen(prefix);
	if (len)
	{
	    int i = histNow;
	    do
	    {
		if (++i >= histNum)
		    i = 0;
		if (strncmp(prefix, cmdHist[i], len) == 0)
		    return cmdHist[histNow = i];
	    }
	    while (i != histNow);
	}
	histNow++;
	if (histNow >= histNum)
	    histNow = 0;
#ifdef DEBUG
	fprintf(debug, "On exit in getNextCmd - histNow = %d, histNum = %d, cmd = %s\n",
		histNow, histNum, cmdHist[histNow]);
#endif
	return cmdHist[histNow];
    }
    return NULL;
}

#if __STDC__
void saveHist(char *b)
#else
void saveHist(b)
    char *b;
#endif
{
    /* See if we have it already */
    int i;
    for (i = 0; i < histNum; i++)
    {
	if (strcmp(cmdHist[i], b) == 0)
	{
	    histNow = (i + 1) % HIST_LEN;
	    return;
	}
    }
    /* Put new command line in the history ring */
#ifdef DEBUG
    fprintf(debug, "On entry in saveHist - histNow = %d, histNum = %d\n",
	    histNow, histNum);
#endif
    if (histNum < HIST_LEN)
	histNow = histNum++;
    strcpy(cmdHist[histNow], b);
    histNow = (histNow + 1) % HIST_LEN;
#ifdef DEBUG
    fprintf(debug, "On exit in saveHist - histNow = %d, histNum = %d, cmd = %s\n",
	    histNow, histNum, b);
#endif
}

/************************************************************
	 COMMAND LINE EDITING....
*************************************************************/

#if __STDC__
int EditString(int c, char *start, char **end, char **now,
	        int *insmode, int maxlen)
{
#else
int EditString(c, start, end, now, insmode, maxlen)
    int c;
    char *start, **end, **now;
    int *insmode, maxlen;
{
#endif
    char *tptr;
    int *chList;
    int rtn = 0, chBuf, chN = 1, i;
    chBuf = GRABBUF();
    chList = (int *) BUF[chBuf];
    if (c == keycodeTbl[CED_INSFNAME])	/* Insert file name */
    {
	int buf = GRABBUF();
	char *n = lookupVar(VAR_PFILE, buf);
	chList[0] = ' ';
	while ((chList[chN++] = (int) (*n++)) != (int) '\0') ;
	chList[chN - 1] = ' ';
	chList[chN] = '0';
	FREEBUF(buf);
    }
    else
	chList[0] = c;
    for (i = 0; i < chN; i++)
    {
	c = chList[i];
	if (c == keycodeTbl[HELP_HOOK])
	{
	    (void) ExecuteKey(HELP_HOOK);
#ifdef __MSDOS__
	    repaintScreen();
#endif
	}
	else if (c == keycodeTbl[CED_CLEAR])	/* clear line */
	{
	    if (*start)
	    {
		*now = *end = start;
		*start = 0;
	    }
	    else
	    {
		clearCmdWin();
		rtn = 1;
		goto done;
	    }
	}
	else if (c == keycodeTbl[CED_BKSPC])	/* backspace */
	{
	    if ((tptr = *now - 1) >= start)
	    {
		tptr = *now - 1;
		while (tptr < *end)
		{
		    *tptr = *(tptr + 1);
		    tptr++;
		}
		(*now)--;
		(*end)--;
	    }
	}
	else if (c == keycodeTbl[CED_DEL])	/* DEL */
	{
	    if (*now < *end)	/* Delete under cursor */
	    {
		tptr = *now;
		while (tptr < *end)
		{
		    *tptr = *(tptr + 1);
		    tptr++;
		}
		(*end)--;
		if (*now > *end)
		    *now = *end;
	    }
	}
	else if (c == keycodeTbl[CED_MODE])	/* Toggle insert mode */
	{
	    *insmode = (1 - *insmode);
	}
	else if (c == keycodeTbl[CED_RIGHT])	/* right */
	{
	    if (*now < *end)
		(*now)++;
	}
	else if (c == keycodeTbl[CED_LEFT])	/* left */
	{
	    if (*now > start)
		(*now)--;
	}
	else if (c == keycodeTbl[CED_LWORD])	/* word left */
	{
	    int f = 0;
	    if (*now > start)
		(*now)--;
	    while (*now > start && isspace((int) **now))
		(*now)--;
	    while (*now >= start && !isspace((int) **now))
	    {
		(*now)--;
		f = 1;
	    }
	    if (f)
		(*now)++;
	}
	else if (c == keycodeTbl[CED_RWORD])	/* word right */
	{
	    while (*now < *end && !isspace((int) **now))
		(*now)++;
	    while (*now < *end && isspace((int) **now))
		(*now)++;
	}
	else if (c == keycodeTbl[CED_HOME])
	{
	    *now = start;	/* home */
	}
	else if (c == keycodeTbl[CED_END])
	{
	    *now = *end;	/* end */
	}
	else if (histNum && c == keycodeTbl[CED_PREV])
	{
	    char *tmp = getPrevCmd(start);
	    if (tmp)
	    {
		strcpy(start, tmp);
		*end = *now = start + strlen(start);
	    }
	    else
		BEEP;
	}
	else if (histNum && c == keycodeTbl[CED_NEXT])
	{
	    char *tmp = getNextCmd(start);
	    if (tmp)
	    {
		strcpy(start, tmp);
		*end = *now = start + strlen(start);
	    }
	    else
		BEEP;
	}
	else if (c == keycodeTbl[CED_DELLEFT])	/* */
	{
	    int p = 0;
	    while ((start[p++] = *((*now)++)) != '\0') ;
	    *now = start;
	    *end = start + p - 1;
	}
	else if (c == keycodeTbl[CED_DELRIGHT])	/* */
	{
	    *end = *now;
	    *(*end) = '\0';
	}
	else if (c == keycodeTbl[CED_DELLWORD])	/* */
	{
	    char *rst = *now, *t1;
	    if (rst > start)
		rst--;
	    while (rst > start && isspace((int) *rst))
		rst--;
	    while (rst > start && !isspace((int) *rst))
		rst--;
	    t1 = *now;
	    *now = rst;
	    while ((*rst++ = *t1++) != '\0') ;
	    *end = rst - 1;
	}
	else if (c == keycodeTbl[CED_DELRWORD])	/* */
	{
	    char *rst = *now, *t1;
	    while (*rst && isspace((int) *rst))
		rst++;
	    while (*rst && !isspace((int) *rst))
		rst++;
	    while (*rst && isspace((int) *rst))
		rst++;
	    t1 = *now;
	    while ((*t1++ = *rst++) != '\0') ;
	    *end = t1 - 1;
	}
	else if (isprint(c & 0xFF))
	{
	    if (*insmode || (chN > 1) || (*now >= *end))
	    {
		/* If inserting, shift line to make space */
		if (maxlen == -1 || (*end - start) < maxlen)
		{
		    tptr = ++(*end);
		    while (tptr > *now)
		    {
			*tptr = *(tptr - 1);
			tptr--;
		    }
		}
		else
		    BEEP;
	    }
	    /* Put the character in at the appropriate position */
	    *((*now)++) = (char) c;
	    *(*end) = 0;
	}
	else
	{
	    int bnd;
	    if ((bnd = GetKeyBinding(c)) >= 0)
	    {
		if (KeyTbl[bnd].ip)
		{
		    *now = *end = start;
		    *start = 0;
		    escapeCh = c;
		}
		else if (bnd < 8 || bnd == HELP_HOOK)
		{
		    int old_l = l;
		    (void) ExecuteKey(bnd);
		    repaintScreen();
		    syncScreen();
		    if (l == old_l)
		    {
			rtn = 0;
			goto done;
		    }
		}
		rtn = bnd;
		goto done;
	    }
	}
	if ((*end - start) >= maxlen)
	{
	    rtn = 1;
	    goto done;
	}
    }
  done:
    FREEBUF(chBuf);
    return rtn;
}

/***********************************************************
	CHANGE KEY BINDINGS, LOOK UP A KEY BINDING...
************************************************************/

#if __STDC__
void SetKeyCode(short key, short code)
#else
void SetKeyCode(key, code)
    short key, code;
#endif
{
    keycodeTbl[key] = code;
}

#if __STDC__
short GetKeyCode(short key)
#else
short GetKeyCode(key)
    short key;
#endif
{
    return keycodeTbl[key];
}

#if __STDC__
char *KeyName(int idx)
#else
char *KeyName(idx)
    int idx;
#endif
{
    int ch = keycodeTbl[idx];
    static char buf[8];
    if (((unsigned) ch) <= 32)
    {
	switch (ch)
	{
	case 0:
	    return "<None>";
	case 8:
#ifdef USE_NCURSES
	case KEY_BACKSPACE:
#endif
	    return "Bkspc";
	case 9:
	    return "Tab";
	case 13:
	    return "Enter";
	case 27:
	    return "Esc";
	case 32:
	    return "Space";
	default:
	    sprintf(buf, "^%c", ch + 'A' - 1);
	    return buf;
	}
    }
    else if (((unsigned) ch) < 127)
    {
	sprintf(buf, "%c", ch);
	return buf;
    }
    else
	switch (ch)
	{
	case KEY_UP:
	    return "Up";
	case KEY_DOWN:
	    return "Down";
	case KEY_LEFT:
	    return "Left";
	case KEY_RIGHT:
	    return "Right";
#if __MSDOS__
	case KEY_PGUP:
	    return "PgUp";
	case KEY_PGDN:
	    return "PgDn";
	case KEY_CTRL_LEFT:
	    return "^Left";
	case KEY_CTRL_RIGHT:
	    return "^Right";
	case KEY_CTRL_PGUP:
	    return "^PgUp";
	case KEY_CTRL_PGDN:
	    return "^PgDn";
	case KEY_CTRL_HOME:
	    return "^Home";
	case KEY_CTRL_END:
	    return "^End";
	case KEY_INS:
	    return "Ins";
	case KEY_DEL:
	    return "Del";
	case KEY_ALT_0:
	    return "Alt-0";
	case KEY_ALT_1:
	case KEY_ALT_2:
	case KEY_ALT_3:
	case KEY_ALT_4:
	case KEY_ALT_5:
	case KEY_ALT_6:
	case KEY_ALT_7:
	case KEY_ALT_8:
	case KEY_ALT_9:
	    sprintf(buf, "Alt-%d", (ch >> 8) - 0x77);
	    return buf;
	case KEY_ALT_A:
	    return "Alt-A";
	case KEY_ALT_B:
	    return "Alt-B";
	case KEY_ALT_C:
	    return "Alt-C";
	case KEY_ALT_D:
	    return "Alt-D";
	case KEY_ALT_E:
	    return "Alt-E";
	case KEY_ALT_F:
	    return "Alt-F";
	case KEY_ALT_G:
	    return "Alt-G";
	case KEY_ALT_H:
	    return "Alt-H";
	case KEY_ALT_I:
	    return "Alt-I";
	case KEY_ALT_J:
	    return "Alt-J";
	case KEY_ALT_K:
	    return "Alt-K";
	case KEY_ALT_L:
	    return "Alt-L";
	case KEY_ALT_M:
	    return "Alt-M";
	case KEY_ALT_N:
	    return "Alt-N";
	case KEY_ALT_O:
	    return "Alt-O";
	case KEY_ALT_P:
	    return "Alt-P";
	case KEY_ALT_Q:
	    return "Alt-Q";
	case KEY_ALT_R:
	    return "Alt-R";
	case KEY_ALT_S:
	    return "Alt-S";
	case KEY_ALT_T:
	    return "Alt-T";
	case KEY_ALT_U:
	    return "Alt-U";
	case KEY_ALT_V:
	    return "Alt-V";
	case KEY_ALT_W:
	    return "Alt-W";
	case KEY_ALT_X:
	    return "Alt-X";
	case KEY_ALT_Y:
	    return "Alt-Y";
	case KEY_ALT_Z:
	    return "Alt-Z";
#else
	case KEY_PPAGE:
	    return "PgUp";
	case KEY_NPAGE:
	    return "PgDn";
	case 127:
	    return "Del";
#endif
	case KEY_HOME:
	    return "Home";
	case KEY_END:
	    return "End";
	case KEY_F1:
	    return "F1";
	case KEY_F2:
	    return "F2";
	case KEY_F3:
	    return "F3";
	case KEY_F4:
	    return "F4";
	case KEY_F5:
	    return "F5";
	case KEY_F6:
	    return "F6";
	case KEY_F7:
	    return "F7";
	case KEY_F8:
	    return "F8";
	case KEY_F9:
	    return "F9";
	case KEY_F0:
	    return "F10";
	default:
	    sprintf(buf, "#%d", ch);
	    return buf;
	}
}

/****************************************************************/

/* FindKeyCode searches a subrange of the key table */

#if __STDC__
static int FindKeyCode(int idx, int ch, int lim)
#else
static int FindKeyCode(idx, ch, lim)
    int idx, ch, lim;
#endif
{
    for ( ; idx < lim ; idx++)
    {
	if (keycodeTbl[idx] == ch &&
	    (KeyTbl[idx].ip || idx<NUM_PRIM_KEYS))
	    	return idx;
    }
    return -1;
}

#if __STDC__
int GetKeyBinding(int key)
#else
int GetKeyBinding(key)
    int key;
#endif
{
    int i;
    /* Check user bindings */
    if (metaMode)
    {
	metaMode = 0;
	/* Check meta key combos */
	if ((i = FindKeyCode(START_META_KEYS, key, START_CED_KEYS)) == -1)
	{
		if (isupper(key))
		    i = 70 + key - (int) 'A';
		else if (islower(key))
		    i = 70 + key - (int) 'a';
		else if (isdigit(key))
		    i = 60 + key - (int) '0';
	    	if (KeyTbl[i].ip == 0)
			i = -1; /* unbound */
	}
    }
    else if ((i = FindKeyCode(NUM_PRIM_KEYS, key, START_META_KEYS)) == -1)
	i = FindKeyCode(0, key, NUM_PRIM_KEYS);
    return i;
}
