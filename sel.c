/*
 * sel - a general-purpose argument selection utility, v2.3
 *
 * YAGH! - Yet Another Gramish Hack
 *
 *    sel [-w<namewidth>] [-i [-n<field>] ] <cmd> [<flags>]) <entries>
 *
 * This command will present the user with a window containing a list
 * of all the file arguments specified by <entries>. If the -i
 * argument is given, the list is added to by reading standard input.
 * If the -n argument is given, the specified whitespace-separated
 * field is used from each line read from stdin (with the first field
 * being field 0).
 *
 * The user can move through the list with the cursor keys, and
 * select/deselect entries with the space bar. Upon pressing return
 * or escape, the command is executed with the set of selected entries
 * as arguments (as well as the flags, if present).
 *
 * While running, you can use the following keys:
 *
 * A : select all the arguments in the list
 * N : select no arguments (deselect all)
 * I : Invert the current selection
 * + : Lets you enter an ed(1) regular expression; all arguments that
 *	match the expression are selected
 * - : Similar to +, but the matching files are deselected
 * X : invoke a user specified command with the current argument
 * Z : invoke a user specified command with the currently selected arguments
 * H : get help
 * ? : get help
 * U : move cursor up
 * D : move cursor down
 * L : move cursor left
 * R : move cursor right
 *
 * Pressing space or Ins selects the argument at which the cursor is positioned,
 * and moves down to the next argument.
 *
 * Examples:
 *
 * 	sel vi *.c
 *	sel ls -l /bin/g*
 *	sel echo *.c | xargs cc -c (equiv. to `sel cc -c *.c')
 *	find . -name '*.c' -print | sel -i cc -c
 *
 * Notes:
 *	1) remember, quoted arguments have their quotes stripped by the shell
 *		*before* they are passed to sel.
 *
 *	2) remember, these are *ed* regular expressions. If you want
 *		your r.e. to match whole argument names only, you
 *		must delimit it with `^' and `$'.
 *
 *	3) when using `X' or `Z', you can use `@' as a placeholder
 *		in the command string to represent where the arguments
 *		must go. If there is no `@', they are appended.
 *
 * Written by Graham Wheeler.
 * 	gram@aztec.co.za
 *
 * Should work with most Unixen with decent curses, and under DOS with
 * Borland C/C++.
 *
 * Version History
 * ---------------
 * v1.0	  10 Oct 1992		Initial release, using ETI
 * v1.1   28 Oct 1992		Removed ETI dependencies; now requires
 *					Curses only (downside - whole
 *					screen now refreshed after help
 *					is displayed). Fixed a bug in
 *					use of islower(3).
 * v1.2	  11 Nov 1992		More portable version by Darren Bock.
 * v2.0    8 Nov 1992           Added select/deselect, info and edit commands.
 *					Added -o argument.
 *					Can now read files from stdin.
 *					ESC now aborts.
 * v2.1   12 Nov 1992		Merged in contributions by Darren.
 *					Made Makefile and manual page
 *					based on contributions.
 *					Added SysV regular expressions.
 *					Cleaned up commands and removed pop-up
 *						windows.
 *					Split F into F and S commands.
 *					Added X and Z commands.
 * v2.2	  31 Oct 1993		Removed E, F and S commands as these are
 *					more appropriate in gc3. Modified
 *					code to compile as part of gc3 package.
 *				Named `sel' instead of `pick' to avoid conflict
 *					with MH pick.
 *	v2.3    9 Dec 1993	Make use of the -DNO_WGETNSTR macro in site.def.
 *					Removed islower/toupper (bug) code.
 *				Renamed select() to Select() to avoid clash with network function.
 *
 * DISCLAIMER: The author is not responsible for any loss, damage,
 *	riots, earthquakes, catastrophes, etc that arise from the
 *	use of this program.
 *
 */

#include "gcport.h"
#include <fcntl.h>
#include "gckey.h"
#define GC_CURSES
#include "gclib.h"

/* If you make changes, add new version messages to the array below */

static char    *VersionMsg[] =
{
    "sel v1.0 by Graham Wheeler, gram@aim1.aztec.co.za\n",
    "\tInitial release; uses ETI and Curses libraries\n",
    "sel v1.1 by Graham Wheeler, gram@aim1.aztec.co.za\n",
    "\tRemoved ETI dependencies, for BSD support.\n",
    "\tFixed a bug in use of islower().\n",
    "sel v2.0 by Graham Wheeler, gram@aim1.aztec.co.za\n",
    "\tAdded -o argument option, and pipe support.\n",
    "\tAdded (F)ile info, (E)dit, select (+), and deselect (-) commands.\n",
    "sel v2.1 by Graham Wheeler et al\n",
    "\tAdded portability stuff, Makefile and manpage.\n",
"\tThe latter contributed to by George Sipe, gsipe@pyratl.ga.pyramid.com\n",
    "\tPorted to Pyramid, SunOS, RiscOS by Darren Bock, sgccdeb@citecuc.citec.oz.au\n",
    "sel v2.2 by Graham Wheeler\n\tStripped down and made part of the gc3 distribution\n",
    "sel v2.3 by Graham Wheeler\n\tMinor tweaks\n",
 /* Add new messages here, in the same format as the ones above */
    NULL
};

#if __MSDOS__
#define MAXFILENAME	15	/* Maximum displayed filename length */
#define MAXARGS		1024
#else
#define MAXFILENAME	35	/* Maximum displayed filename length */
#define MAXARGS		1024
#endif

static int      numfiles = 0, redraw, start, end, columns, height, fileNameLen, selCnt = 0, moved = 1;

static char    *cmdline, *selected, *lengths, **fnames;

static WINDOW  *cmdWin;

#if __MSDOS__

#define refresh()

int             insMode = 1;

int             my_getch()
{
    return wgetch(cmdWin);
}

#endif

#define FileAtPos(f,h,w,r,c)	(((f)*(w)+(c))*(h) + (r))

#if __STDC__
static void     refreshDisplay(WINDOW * w, int form, int rows, int cols)
#else
static void     refreshDisplay(w, form, rows, cols)
    WINDOW         *w;
    int             form;
    int             rows;
    int             cols;
#endif
{
    int             r, c, n;
    werase(w);
    for (c = 0; c < cols; c++)
    {
	for (r = 0; r < rows; r++)
	{
	    n = FileAtPos(form, rows, cols, r, c);
	    if (n >= numfiles)
		break;
	    if (selected[n])
		STANDOUT(w);
	    if (lengths[n] > fileNameLen)
	    {
		char            buf[MAXFILENAME + 1];
		strncpy(buf, fnames[n], fileNameLen);
		buf[fileNameLen] = 0;
		mvwprintw(w, r, 2 + c * (fileNameLen + 3), buf);
	    }
	    else
		mvwprintw(w, r, 2 + c * (fileNameLen + 3), fnames[n]);
	    if (selected[n])
		STANDEND(w);
	}
    }
}

#if __STDC__
static void     invert(void)
#else
static void     invert()
#endif
{
    int             i;
    for (i = 0; i < numfiles; i++)
    {
	selCnt += selected[i] ? (-1) : 1;
	selected[i] = 1 - selected[i];
    }
    redraw = 1;
}

#if __STDC__
static void global (int v)
#else
static void global (v)
    int             v;
#endif
{
    int             i;
    for (i = 0; i < numfiles; i++)
	selected[i] = v;
    selCnt = v ? numfiles : 0;
    redraw = 1;
}

#if __STDC__
static void     showHelp(void)
#else
static void     showHelp()
#endif
{
    clrscr();
    endwin();
    printf("\nUse the arrow keys to move around, space bar to toggle.\n");
    printf("Alternatively move with h, j, k, and l.\n");
    printf("I or i inverts the selection, A or a selects all, N or n selects none.\n");
    printf("X or x invokes a user specified command with the current argument\n");
    printf("Z or z invokes a user specified command with the currently selected arguments\n");
    printf("+ lets you select files matching a regular expression.\n");
    printf("- lets you deselect files matching a regular expression.\n");
    printf("? or H shows you this help.\n");
    printf("When done, use RETURN to execute the command\n");
    printf("or output the arguments, or ESC to abort.\n");
    printf("\nPress ENTER to resume sel\n");
    fflush(stdout);
    fgets(cmdline, 80, stdin);
#ifdef __MSDOS__
    invalidateScreen();
#endif
    refresh();
}

#if __STDC__
static void     selectByExpr(int adding)
#else
static void     selectByExpr(adding)
    int             adding;
#endif
{
    char            re[80];
    char           *dbg;
    werase(cmdWin);
    mvwprintw(cmdWin, 0, 1, "Expression? ");
    re[0] = '\0';
#ifdef __MSDOS__
    myRead(cmdWin, 80, 0, 12, re, 80, &insMode, NULL, my_getch);
    if ((dbg = DOScompileRE(re)) == NULL)
#else
    echo();
#ifdef NO_WGETNSTR
    wgetstr(cmdWin, re);
#else
    wgetnstr(cmdWin, re, 80);
#endif
    noecho();
    if ((dbg = compileRE(re)) == NULL)
#endif
    {
	int             i;
	for (i = start; i < end; i++)
	{
#ifdef __MSDOS__
	    if (DOSmatchRE(fnames[i - start]) == 1)
#else
	    if (matchRE(fnames[i - start]) == 1)
#endif
	    {
		selCnt += selected[i - start] ? (-1) : 1;
		selected[i - start] = adding;
	    }
	}
    }
    else
    {
	werase(cmdWin);
	mvwprintw(cmdWin, 0, 1, dbg);
    }
}

#if __STDC__
static int      getCommand(char *buf)
#else
static int      getCommand(buf)
    char           *buf;
#endif
{
    werase(cmdWin);
    mvwprintw(cmdWin, 0, 1, "Command? ");
    buf[0] = '\0';
#ifdef __MSDOS__
    myRead(cmdWin, 80, 0, 10, buf, 80, &insMode, NULL, my_getch);
#else
    echo();
#ifdef NO_WGETNSTR
    wgetstr(cmdWin, buf);
#else
    wgetnstr(cmdWin, buf, 80);
#endif
    noecho();
#endif
    return strlen(buf);
}

#if __STDC__
static char    *doPrefix(char *buf)
#else
static char    *doPrefix(buf)
    char           *buf;
#endif
{
    char           *suf;
    if ((suf = STRCHR(buf, '@')) == NULL)
	suf = "";
    else
	*suf++ = 0;
    strcpy(cmdline, buf);
    strcat(cmdline, " ");
    return suf;
}

#if __STDC__
static void     doCommand(void)
#else
static void     doCommand()
#endif
{
    clrscr();
    endwin();
    putchar('\n');
    system(cmdline);
    fprintf(stderr, "Press ENTER to return to sel...");
    fflush(stderr);
    while (getchar() != '\n') ;
    refresh();
}

#if __STDC__
static void     execSingle(char *name)
#else
static void     execSingle(name)
    char           *name;
#endif
{
    char            line[80];
    if (getCommand(line))
    {
	char           *suf = doPrefix(line);
	strcat(cmdline, name);
	strcat(cmdline, suf);
	doCommand();
    }
}

#if __STDC__
static void     execMulti(void)
#else
static void     execMulti()
#endif
{
    char            line[80];
    if (getCommand(line))
    {
	int             i;
	char           *suf = doPrefix(line);
	for (i = 0; i < numfiles; i++)
	{
	    if (selected[i])
	    {
		strcat(cmdline, fnames[i]);
		strcat(cmdline, " ");
	    }
	}
	strcat(cmdline, suf);
	doCommand();
    }
}

#if __STDC__
static void     cursUp(int *f, int *r, int *c, int n)
#else
static void     cursUp(f, r, c, n)
    int            *f, *r, *c, n;
#endif
{
    while (n--)
    {
	(*r)--;
	if (*r < 0)
	{
	    *r = height - 1;
	    (*c)--;
	    if (*c < 0)
	    {
		*c = columns - 1;
		(*f)--;
	    }
	}
    }
}

#if __STDC__
static void     cursDown(int *f, int *r, int *c, int n)
#else
static void     cursDown(f, r, c, n)
    int            *f, *r, *c, n;
#endif
{
    while (n--)
    {
	(*r)++;
	if (*r >= height)
	{
	    *r = 0;
	    (*c)++;
	    if (*c >= columns)
	    {
		*c = 0;
		(*f)++;		/* next form */
	    }
	}
    }
}

#if __STDC__
static int      Select(void)
#else
static int      Select()
#endif
{
    int             n, f, r, c, oldc, oldr, oldf, ch;
    int             rebox = 1;
    WINDOW         *w, *topW;
    initscr();
    nonl();
    raw();
    noecho();
    clrscr();
    columns = ((COLS - 4) / (fileNameLen + 3));
    height = (LINES - 3);
    topW = newwin(LINES - 1, COLS, 0, 0);
    w = subwin(topW, LINES - 3, COLS - 2, 1, 1);
    cmdWin = newwin(1, COLS, LINES - 1, 0);
    KEYENABLE(w);
    r = c = ch = f = 0;
    redraw = 1;
    while (ch != 27 && ch != 13)
    {
	if (rebox)
	{
	    box(topW, 0, 0);
	    UPDATE(topW);
	    rebox = 0;
	}
	if (redraw)
	{
	    refreshDisplay(w, f, height, columns);
	    redraw = 0;
	    UPDATE(w);
	}
	mvwprintw(w, r, c * (fileNameLen + 3), "=>");
	if (moved)
	{
	    mvwprintw(cmdWin, 0, 1, "Screen: %2d  Row: %2d  Col: %2d    Selected %3d arguments of %3d",
		      f + 1, r + 1, c + 1, selCnt, numfiles);
	    moved = 0;
	    UPDATE(cmdWin);
	}
	REFRESH();
	ch = wgetch(w);
	mvwprintw(w, r, c * (fileNameLen + 3), "  ");
	oldc = c;
	oldr = r;
	oldf = f;
	switch (ch)
	{
	case '?':
	case 'H':
	    showHelp();
	    rebox = moved = redraw = 1;
	    break;
	case 'I':
	case 'i':
	    invert();
	    break;
	case 'A':
	case 'a':
	    global          (1);
	    break;
	case 'N':
	case 'n':
	    global          (0);
	    break;
	case 'X':
	case 'x':
	    execSingle(fnames[FileAtPos(f, height, columns, r, c)]);
	    goto CLEAR;
	case 'Z':
	case 'z':
	    execMulti();
	    goto CLEAR;
	  CLEAR:
	    clrscr();
#ifdef __MSDOS__
	    invalidateScreen();
#endif
	    rebox = moved = redraw = 1;
	    break;
	case '+':
	    selectByExpr(1);
	    redraw = 1;
	    break;
	case '-':
	    selectByExpr(0);
	    redraw = 1;
	    break;
	case KEY_RIGHT:
	case 'l':
	    cursDown(&f, &r, &c, height);
	    moved = 1;
	    break;
	case 22:
	case KEY_PGUP:
	    if (r == 0)
		cursUp(&f, &r, &c, height);
	    else
		cursUp(&f, &r, &c, r);
	    moved = 1;
	    break;
	case 4:
	case KEY_PGDN:
	    if (r == height - 1)
		cursDown(&f, &r, &c, height);
	    else
		cursDown(&f, &r, &c, height - r - 1);
	    moved = 1;
	    break;
	case 'h':
	case KEY_LEFT:
	    cursUp(&f, &r, &c, height);
	    moved = 1;
	    break;
	case 'k':
	case KEY_UP:
	    cursUp(&f, &r, &c, 1);
	    moved = 1;
	    break;
	case KEY_INS:
	case ' ':
	    n = FileAtPos(f, height, columns, r, c);
	    if (!selected[n])
		STANDOUT(w);
	    selCnt += selected[n] ? (-1) : 1;
	    selected[n] = 1 - selected[n];
	    if (lengths[n] > fileNameLen)
	    {
		char            buf[MAXFILENAME + 1];
		strncpy(buf, fnames[n], fileNameLen);
		buf[fileNameLen] = 0;
		mvwprintw(w, r, 2 + c * (fileNameLen + 3), buf);
	    }
	    else
		mvwprintw(w, r, 2 + c * (fileNameLen + 3), fnames[n]);
	    STANDEND(w);
	    /* fall through */
	case KEY_DOWN:
	case 'j':
	    cursDown(&f, &r, &c, 1);
	    moved = 1;
	    break;
	case 13:
	case 27:
	    break;
	default:
	    BEEP;
	    break;
	}
	if (f < 0 || FileAtPos(f, height, columns, r, c) >= numfiles)
	{
	    BEEP;
	    f = oldf;
	    r = oldr;
	    c = oldc;
	}
	else if (f != oldf)
	    redraw = 1;
    }
    delwin(w);
    delwin(cmdWin);
    delwin(topW);
    clrscr();
    refresh();
    endwin();
    return (ch != 27);
}

#if __STDC__
static void     useage(char *msg)
#else
static void     useage(msg)
    char           *msg;
#endif
{
    int             i = 0;
    while (VersionMsg[i])
	fprintf(stderr, VersionMsg[i++]);
    fprintf(stderr, "!!%s\n", msg);
    fprintf(stderr, "\nUsage: sel [-w<width>] [-i [ -n<field>] ] <cmd> [<flags>] <entries>\n");
    fprintf(stderr, "where\n\t<width> is the maximum width used to display an entry\n");
    fprintf(stderr, "\t<field> is the number of the whitespace-seperated field (from 0)\n");
    fprintf(stderr, "\t\t(only has effect with -i argument)\n");
    fprintf(stderr, "\t<cmd>, if present, is a command to run with the selected\n\t\tentries as arguments\n");
    fprintf(stderr, "\t<flags> are an initial arguments to <cmd>\n");
    fprintf(stderr, "\t-i causes sel to read the entries from standard input\n");
    fprintf(stderr, "\t<entries> specifies the entries to use\n");
    exit(EINVAL);
}

#if __STDC__
static void     memfail(char *name, int nb)
#else
static void     memfail(name, nb)
    char           *name;
    int             nb;
#endif
{
    fprintf(stderr, "Failed to allocate %s of %d bytes\n", name, nb);
    exit(ENOMEM);
}

#if __STDC__
void            strip2field(int field, char *buff)
#else
void            strip2field(field, buff)
    int             field;
    char           *buff;
#endif
{
    char           *b = buff, *front;
    /* First trim trailing newline */
    int             l = strlen(buff);
    if (l && buff[l - 1] == '\n')
	buff[l - 1] = '\0';
    if (field < 0)
	return;
    while (field && *b)		/* skip to start */
    {
	while (isspace(*b))
	    b++;
	while (*b && !isspace(*b))
	    b++;
	field--;
    }
    /* skip whitespace */
    while (isspace(*b))
	b++;
    front = b;
    /* terminate field */
    while (*b && !isspace(*b))
	b++;
    if (*b)
	*b = '\0';
    /* move over */
    b = buff;
    while (*front)
	*b++ = *front++;
    *b = '\0';
}

#if __STDC__
void            main(int argc, char *argv[])
#else
void            main(argc, argv)
    int             argc;
    char           *argv[];
#endif
{
    int             i, l, totallen, gotarg = 0, inputList = 0, field = -1;
    int             cmdStart, cmdEnd;
    char            tmpName[80];
    if (argc < 2)
	useage("No arguments");
    i = 1;
    fileNameLen = 15;
    while (i < argc && argv[i][0] == '-')
    {
	switch (argv[i][1])
	{
	case 'i':
	    inputList = 1;
	    fileNameLen = 32;
	    break;
	case 'w':
	    if (argv[i][2])
		fileNameLen = atoi(argv[i] + 2);
	    else
		fileNameLen = atoi(argv[++i]);
	    break;
	case 'n':
	    if (argv[i][2])
		field = atoi(argv[i] + 2);
	    else
		field = atoi(argv[++i]);
	    break;
	default:
	    useage("Invalid argument");
	}
	i++;
    }
    if (field >= 0 && !inputList)
	useage("Can't have -n with no -i");
    if (fileNameLen > 78)
	fileNameLen = 78;

    /* Determine the head of the command, which consists of the
	    name itself, plus any flags (i.e. args starting with
	    `-' (as well as `/' under DOS). The length is accumulated
	    in totallen, while cmdStart and cmdEnd are set up to point
	    to the first and last args
    */
    cmdStart = i;
    totallen = strlen(argv[i++]) + 2;
#ifdef __MSDOS__
    while (i < argc && (argv[i][0] == '-' || argv[i][0] == '/'))
#else
    while (i < argc && argv[i][0] == '-')
#endif
	totallen += strlen(argv[i++]) + 1;

    cmdEnd = i;

    /* Make sure there is something left to choose from! */

    if (i >= argc && !inputList)
	useage("No entries were specified!");

    /* initialise the table of choices */

    start = end = numfiles = 0;
    if ((fnames = (char **) calloc(MAXARGS, sizeof(char *))) == NULL)
	memfail("entry list table", MAXARGS * sizeof(char *));

    /* Read the first part of the table from argv[] */

    while (i < argc && end < MAXARGS)
    {
	strncpy(tmpName, argv[i], fileNameLen);
	tmpName[fileNameLen] = '\0';
	fnames[end] = calloc(l = strlen(tmpName) + 1, sizeof(char));
	if (fnames[end] == NULL)
	    memfail("entry list table entry", l);
	strcpy(fnames[end++], tmpName);
	i++;
    }

    /* Read the rest from stdin, if -i flag was used */

    if (inputList)
    {
	while (!feof(stdin))
	{
	    if (fgets(tmpName, 80, stdin) == NULL)
		break;
	    tmpName[fileNameLen] = '\0';
	    strip2field(field, tmpName);
	    if (tmpName[0] == '\0')
		continue;
	    fnames[end] = calloc(l = strlen(tmpName) + 1, sizeof(char));
	    if (fnames[end] == NULL)
		memfail("entry list table entry", l);
	    strcpy(fnames[end++], tmpName);
	    if (end >= MAXARGS)
		break;
	}
	close(0);
	open("/dev/tty", O_RDWR);
    }
    numfiles = end;

    if ((selected = calloc(numfiles, sizeof(char))) == NULL)
	memfail("file selection table", numfiles);
    if ((lengths = calloc(numfiles, sizeof(char))) == NULL)
	memfail("filename length table", numfiles);

    /* Build a table of lengths, and add to running total */

    for (i = 0; i < numfiles; i++)
	totallen += (lengths[i] = strlen(fnames[i])) + 1;

    /* Allocate space for the command */

    if ((cmdline = calloc(totallen, sizeof(char))) == NULL)
	memfail("command line buffer", totallen);

    allowColor = 0;		/* needed by gclib's curses? */

    /* Select() does the actual pick part */
    if (Select())
    {
	/* something was selected - build the command line... */
	cmdline[0] = '\0';
	/* first the head... */
	while (cmdStart < cmdEnd)
	{
	    strcat(cmdline, argv[cmdStart++]);
	    strcat(cmdline, " ");
	}
	/* then the selections */
	for (i = 0; i < numfiles; i++)
	{
	    if (selected[i])
	    {
		gotarg = 1;
		strcat(cmdline, fnames[i]);
		strcat(cmdline, " ");
	    }
	}
	if (gotarg)
	    system(cmdline);
    }
    /* free up everything */
    free(cmdline);
    if (inputList)
    {
	for (i = 0; i < numfiles; i++)
	    free(fnames[i]);
	free(fnames);
    }
    free(selected);
    free(lengths);
    exit(0);
}
