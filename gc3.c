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
 */


#include "gcport.h"
#define GC_CURSES
#include "gc3.h"
#include "gclib.h"
#include "gcops.h"
#include "gckey.h"
#include "gccc.h"

#ifdef DEBUG
FILE *debug = NULL;
#endif

int l;				/* Current active panel	*/
int numfiles[2] = {0, 0};
int fIndex[2][MAXARGS];
int highlight[2];
int startLn[2];
int selCnt[2];
int metaKey = 0;	/* Special key for keyboards without Alt- */
int writeIP = 0;
int showIP = 0;
int forceCurrent = 0;
int metaMode = 0;
long selSize[2];
short escapeCh = -1;
short info2show = 1;

static short
 useColor = -1;

/***********************************************************/

#if	__STDC__
static void DisplayDefaultMsg(void)
#else
static void DisplayDefaultMsg()
#endif /* __STDC__ */
{
    if (!showIP)
    {
	char *msg = valspace[VAR_MESSAGE];
	clearCmdWin();
	if (msg && msg[0])
	    writeMsg(msg);
    }
}

/************************************************************/

#if	__STDC__
static void Fatal(char *msg)
#else
static void Fatal(msg)
    char *msg;
#endif
{
    fprintf(stderr, "FATAL ERROR: %s!\n", msg);
    exit(-1);
}

/************************************************************/

#if	__STDC__
static int GetNextKey(void)
#else
static int GetNextKey()
#endif /* __STDC__ */
{
    int key;
    static int repeatCount = 0;
    static int lastKey;

#ifdef __MSDOS__
    locateCursor(25, 1);
#endif

    if (repeatCount > 0)
    {
	repeatCount--;
	return lastKey;
    }

    if (escapeCh != -1)
    {
	key = escapeCh;
	escapeCh = -1;
    }
    else
	key = my_getch();

    if (key == metaKey)
    {
	metaMode = 1;
	showMsg("<META>");
	key = my_getch();
    }

    /* Is this an unbound numeric digit? If so, it is a repeat count */

    if (key >= '2' && key <= '9' && GetKeyBinding(key) < 0)
    {
	repeatCount = key - '1';
	lastKey = key = my_getch();
    }

    return key;
}

/************************************************************/

#if	__STDC__
static void Loop(short dumpIPs)
#else
static void Loop(dumpIPs)
    short dumpIPs;
#endif /* __STDC__ */
{
    writeIP = dumpIPs;

    /* Force a complete rescan and repaint of windows first time around */

    winInvalid[0] = winInvalid[1] = 2;

    DisplayDefaultMsg();

    for (;;)
    {
	int bnd, key;

	i_paint();
	showCursor(1 - l, 0);
	showCursor(l, 1);
	i_sync();
	key = GetNextKey();
	hideCursor(1 - l);
	hideCursor(l);
	DisplayDefaultMsg();

	/* Get key binding, executing bound function if there is one */

	if ((bnd = GetKeyBinding(key)) >= 0)
	{
	    if (ExecuteKey(bnd) != 0)	/* executed a QUIT? */
		break;
	}
	else
	{
	    /* key wasn't bound; see if we should do default action */
	    if (isprint((int) key))
	    {
		if (defaultEntryPoint)
		    if (Interpret(defaultEntryPoint, key, 0) == 1)
			break;
	    }
	    else
		BEEP;
	}

	/* If script has a hook function to be called after each
		key, call it */

	if (afterKeyEntryPoint)
	    if (Interpret(afterKeyEntryPoint, key, 0) == 1)
		break;
    }
}

/************************************************************/

#if __STDC__
static void MakeTempDir(void)
#else
static void MakeTempDir()
#endif
{
    int b = GRABBUF();
    char *tn = BUF[b];
#ifdef __MSDOS__
    if (getenv("TMP"))
	strcpy(tn, getenv("TMP"));
    else if (getenv("GCPATH"))
	strcpy(tn, getenv("GCPATH"));
    else
	strcpy(tn, "c:");
    if (tn[strlen(tn) - 1] != '\\')
	strcat(tn, "\\");
#endif
    strcat(tn, tmpnam(NULL));

    if ((valspace[VAR_TEMPPATH] = calloc(strlen(tn) + 1, sizeof(char))) == NULL)
	Fatal("Memory allocation error");
    strcpy(valspace[VAR_TEMPPATH], tn);

    sprintf(tn, "mkdir %s", valspace[VAR_TEMPPATH]);
    system(tn);
    if (checkType(getFileMode(valspace[VAR_TEMPPATH]), 1) == 0)
    {
	sprintf(tn, "Cannot make scratch directory %s", valspace[VAR_TEMPPATH]);
	Fatal(tn);
    }
    FREEBUF(b);
}

#if __STDC__
static void DeleteTempDir(void)
#else
static void DeleteTempDir()
#endif
{
    if (valspace[VAR_TEMPPATH])
    {
	int b = GRABBUF();
#ifdef __MSDOS__
	sprintf(BUF[b], "deltree /Y %s", valspace[VAR_TEMPPATH]);
#else
	sprintf(BUF[b], "rm -r %s", valspace[VAR_TEMPPATH]);
#endif
	system(BUF[b]);
	FREEBUF(b);
	free(valspace[VAR_TEMPPATH]);
	valspace[VAR_TEMPPATH] = NULL;
    }
}

#if	__STDC__
char *CreateTempName(void)
#else
char *CreateTempName()
#endif
{
    static char tn[MAXPATHNAME];
#ifdef __MSDOS__
    return strcpy(tn, makePath(valspace[VAR_TEMPPATH], tmpnam(NULL)));
#else
    char *p = STRRCHR(tmpnam(NULL), PATH_SEP) + 1;
    return strcpy(tn, makePath(valspace[VAR_TEMPPATH], p));
#endif
}

/************************************************************/

#if __STDC__
static void ResetCriticalVars(void)
#else
static void ResetCriticalVars()
#endif
{
    /* Disable display while fixing others */
    assign2var(VAR_DISPLAYON, "n");

    /* Always do the following few inits */
    assign2var(VAR_VIEWER, "n");
    assign2var(VAR_TREE, "0");
    assign2var(VAR_EXPANDING, "y");
    clearFilter(0);
    clearFilter(1);

    assign2var(VAR_DISPLAYON, "y");
}

#if	__STDC__
static void InitialiseOptionVars(void)
#else
static void InitialiseOptionVars()
#endif
{
    char *shell;
    /* Init option vars and call gcinit */
    assign2var(VAR_DIRSELECT, "n");
    assign2var(VAR_DIRSFIRST, "y");
    assign2var(VAR_SHOWHIDDEN, "y");
    assign2var(VAR_FOLLOWLINKS, "y");
    assign2var(VAR_DEBUGGING, "n");
    assign2var(VAR_NUMBERLNS, "n");
    assign2var(VAR_ISCOLOR, useColor == 0 ? "n" : "y");
#ifdef __MSDOS__
    assign2var(VAR_USEDOSPATTERNS, "y");
    if ((shell = getenv("COMSPEC")) == NULL)
	shell = "command";
#else
    assign2var(VAR_USEDOSPATTERNS, "n");
    if ((shell = getenv("SHELL")) == NULL)
	shell = "sh";
#endif
    assign2var(VAR_SHELL, shell);
}

/************************************************************/

#if	__STDC__
static void AllocateDirectories(void)
#else
static void AllocateDirectories()
#endif
{
    fInfo[0] = (fInfo_t *) calloc(MAXARGS, sizeof(fInfo_t));
    fInfo[1] = (fInfo_t *) calloc(MAXARGS, sizeof(fInfo_t));
    if (fInfo[0] == NULL || fInfo[1] == NULL)
	Fatal("Cannot allocate memory for directory info");
}

#if	__STDC__
static void FreeDirectories(void)
#else
static void FreeDirectories()
#endif
{
    freeNames(0);
    free(fInfo[0]);
    freeNames(1);
    free(fInfo[1]);
}

/************************************************************/

#if	__STDC__
static void AllocateBuffers(void)
#else
static void AllocateBuffers()
#endif
{
    int j;
    for (j = 0; j < MAX_BUFFERS; j++)
    {
	if ((BUF[j] = (char *) calloc(BUFFER_SIZE, sizeof(char))) == NULL)
	    Fatal("Failed to allocate temporary buffers");
    }
}

#if	__STDC__
static void FreeBuffers(void)
#else
static void FreeBuffers()
#endif
{
    int j;
    for (j = 0; j < MAX_BUFFERS; j++)
	free(BUF[j]);
}

/************************************************************/

#if	__STDC__
static void Initialise(char *p1, char *p2, int force)
#else
static void Initialise(p1, p2, force)
    char *p1, *p2;
    int force;
#endif
{
#ifdef DEBUG
    debug = OpenFile("gc.bug", "w");
#endif
    AllocateStringSpace();
    AllocateCodeSpace();
    sWinInit();
    AllocateDirectories();
    AllocateBuffers();
    InitialiseKeyTable();
    InitialiseScreenAttributes();

    /* Read the code file and the .ini file, and initialise
       the directory windows according to the paths, Note that
       ReadCode must must come before ReadInitFile, as ReadCode
       sets up initialiseEntryPoint. If there is an .ini file,
       we don't call gcinit unless force is set. */

    if (ReadCode() != 0)
    {
	extern int errno;
#ifdef NO_STRERROR
	fprintf(stderr, "Cannot open code file: errno %d\n", errno);
#else
	fprintf(stderr, "Cannot open code file: %s\n",
		(char *) strerror(errno));
#endif
	exit(0);
    }
    if (readIniFile() == 0 && !force)
	initialiseEntryPoint = 0;	/* Don't call gcinit */
    dirInit(p1, p2);

    MakeTempDir();
    strcpy(tempName, CreateTempName());

    setupCurses(1);
#ifndef __MSDOS__
    /* Get the stty settings for DEL and backspace */
    if (GetKeyCode(CED_CLEAR) == 0)
	SetKeyCode(CED_CLEAR, (short) killchar());
#ifdef USE_NCURSES
    SetKeyCode(CED_BKSPC, KEY_BACKSPACE);
    SetKeyCode(37, KEY_BACKSPACE);
#else
    if (GetKeyCode(CED_BKSPC) == 0)
	SetKeyCode(CED_BKSPC, (short) erasechar());
#endif
#endif
    ResetCriticalVars();
    if (initialiseEntryPoint)
    {
	InitialiseOptionVars();
	(void) Interpret(initialiseEntryPoint, 0, 0);
    }
    if (startupEntryPoint)
	(void) Interpret(startupEntryPoint, 0, 0);
}

/************************************************************/

#if __STDC__
void Shutdown(void)
#else
void Shutdown()
#endif
{
    ResetCriticalVars();
    SaveInitFile();
    if (catchFP)
	CloseFile(catchFP);
    unlink(tempName);
    DeleteTempDir();
    FreeVars();
    FreeRules();
    FreeDirectories();
    FreeBuffers();
    sWinShutdown();
#ifdef DEBUG
    CloseFile(debug);
#endif
#ifdef __MSDOS__
    FreeScreens();
#endif
    ShowOpenFiles();		/* For file leak checking if FILE_DEBUG defined */
    FreeCodeSpace();
    FreeStringSpace();
    exit(0);
}

/************************************************************/

#if __STDC__
static void Useage(void)
#else
static void Useage()
#endif
{
    fprintf(stderr, "Useage: gc3 [flags] [<path1> [<path2>] ]\n");
    fprintf(stderr, "where the flags are:\n");
    fprintf(stderr, "\t-c0 supresses the use of colour.\n");
    fprintf(stderr, "\t-c1 enables the use of colour.\n");
    fprintf(stderr, "\t-C[<script>] forces a script recompile; the default script\n");
    fprintf(stderr, "\t\tname if one is not specified is `gc3.key'.\n");
    fprintf(stderr, "\t-d[n][l] writes a disassembled listing of the compiled script\n");
    fprintf(stderr, "\t\tto stdout. The `n' option produces a numeric dump, while\n");
    fprintf(stderr, "\t\tthe `l' option causes the link phase to be skipped.\n");
    fprintf(stderr, "\t-D<id> allows you to define identifiers for conditional\n");
    fprintf(stderr, "\t\tcompilation of the script (used with -C).\n");
    fprintf(stderr, "\t-i shows the current instruction pointer value at bottom.\n");
    fprintf(stderr, "\t-I forces the gcinit function to be called.\n");
    fprintf(stderr, "\t-t and -T write an instruction trace to gc3.ip, useful for\n");
    fprintf(stderr, "\t\tpost-mortem debugging. -T starts from gcinit\n");
    fprintf(stderr, "\t\twhile -t starts from the key processing loop.\n");
    fprintf(stderr, "\t-v causes the compiler to output more verbose messages.\n");
    exit(0);
}

#if	__STDC__
char *ProcessArgs(int argc, char *argv[], char **path1, char **path2,
	   short *mustInit, short *dumpIPs, short *symDump, short *dontLink)
#else
char *ProcessArgs(argc, argv, path1, path2,
		   mustInit, dumpIPs, symDump, dontLink)
    int argc;
    char *argv[], **path1, **path2;
    short *mustInit, *dumpIPs, *symDump, *dontLink;
#endif
{
    int i = 1;
    char *script = NULL;
    while (i < argc && argv[i][0] == '-')
    {
	switch (argv[i][1])
	{
	case 'c':
	    {
		useColor = argv[i][2] - '0';
		if (useColor < 0 || useColor > 1)
		    Useage();
		break;
	    }
	case 'C':
	    if (argv[i][2])
		script = argv[i] + 2;
	    else
		script = "gc3.key";
	    break;
	case 'd':
	    {
		int j = 2;
		*symDump = 2;
		while (argv[i][j])
		{
		    if (argv[i][j] == 'n')
			*symDump = 1;
		    else if (argv[i][j] == 'l')
			*dontLink = 1;
		    j++;
		}
		break;
	    }
	case 'D':
	    STRUPR(argv[i] + 2);
	    Define(argv[i] + 2);
	    if (script == NULL)
		script = "gc3.key";	/* Force a compile */
	    break;
	case 'F':
	    forceCurrent = 1;
	    break;
	case 'I':
	    *mustInit = 1;
	    break;
	case 'i':
	    showIP = 1;
	    break;
	case 'T':
	    writeIP = 1;	/* Trace from now */
	case 't':
	    *dumpIPs = 1;	/* writeIP set to this after initialisation */
#ifdef __MSDOS__
	    unlink("\\gc3.ip");
#else
	    unlink("/tmp/gc3.ip");
#endif
	    break;
	case 'v':
	    verbose = 1;
	    break;
	default:
	    Useage();
	}
	i++;
    }
    *path1 = (i < argc) ? argv[i] : NULL;
    *path2 = (i < (argc - 1)) ? argv[i + 1] : NULL;
    return script;
}

/************************************************************/

#if __STDC__
static void Compile(char *script, short symdump, short dontLink)
#else
static void Compile(script, symdump, dontLink)
    char *script;
    short symdump, dontLink;
#endif
{
    AllocateStringSpace();
    AllocateCodeSpace();
    CompileScript(script, symdump, dontLink);
    /* A bit presumptious here... */
    HyperCompile("gc3help.txt");
    FreeCodeSpace();
    FreeStringSpace();
}

/************************************************************/

#if	__STDC__
int main(int argc, char *argv[])
#else
int main(argc, argv)
    int argc;
    char **argv;
#endif /* __STDC__ */
{
    char *path1, *path2, *script;
    short forceInit = 0, dumpIPs = 0, symdump = 0, dontLink = 0;

    script = ProcessArgs(argc, argv, &path1, &path2,
			 &forceInit, &dumpIPs, &symdump, &dontLink);

    if (symdump || script)
	Compile(script, symdump, dontLink);
    else
    {
	Initialise(path1, path2, forceInit);
	Loop(dumpIPs);
	Shutdown();
    }
    return 0;
}
