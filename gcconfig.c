/*
 * Makefile configurator for GC3.3
 * 
 * (c) 1993 by Graham Wheeler. All Rights Reserved.
 * 
 * gram@aztec.co.za
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>

#if __MSDOS__
#include <ctype.h>
#include <string.h>
#define FIELDSEP		';'
#define PATHSEP			'\\'
#else
#define FIELDSEP		':'
#define PATHSEP			'/'
#endif

FILE           *of, *df, *hf;
int             comp = 0, trgt = 0;
char            buf[1024];
char            path[1024];
char            manPath[128], manSuffix[16], installPath[256];
char            ansi[5], compiler[20];

struct stat     st;

#ifndef R_OK
#define R_OK	4
#endif

#ifndef F_OK
#define F_OK	0
#endif

#define T_GENERIC	1
#define T_MIPS		2	/* Needs -systype sysv?			 */
#define T_NEXT		3	/* Needs -DUCBCURSES for some reason? */
#define T_SCO		4	/* Special manpage handling	 */

char           *tgtNames[] = {
	"Choose the target system",
	"Generic SysV/BSD/SunOS",
	"MIPS Risc OS",
	"Next",
	"SCO Unix",
	NULL
};

#define C_ANSICC	1
#define C_KRCC		2
#define C_GCCKR		3
#define C_GCCANS	4
#define C_SUN5		5

char           *ccNames[] = {
	"Choose the compiler to use",
	"ANSI cc",
	"K&R cc",
	"Gnu GCC using K&R",
	"Gnu GCC using ANSI",
	NULL
};

typedef struct {
	char           *name;
	char           *description;
	char           *invocation;
}               proginfo;

#ifdef __MSDOS__

int             doDebug;

proginfo        compilers[] = {
	{"tcc.exe", "Turbo C/C++", "tcc"},
	{"bcc.exe", "Borland C/C++", "bcc"},
	{NULL, NULL, NULL}
};

#else

typedef struct {
	char           *fname;	/* file name */
	char           *text;	/* text to search for */
	int            *val;	/* pointer to flag */
}               context_t;

int	useSysVregexp = 0,
	usePOSIXregexp = 0,
	isLinux = 0,
	hasResetModes = 0,
	hasLibucb = 0,
	hasLibCurses = 0,
	hasLib42Curses = 0,
	hasNroff42 = 0,
	hasLibNCurses = 0,
	hasNcursesDir = 0,
	hasLibXCurses = 0,
	hasLibCursesX = 0,
	hasLibTinfo = 0,
	hasLibTermcap = 0,
	hasLibXTermcap = 0,
	hasUnistdH = 0,
	hasTermH = 0,
	hasColour = 0,
	hasCursSet = 0,
	hasStrError = 0,
	hasSrand48 = 0,
	hasModeT = 0,
	hasUidT = 0,
	hasGidT = 0,
	hasOffT = 0,
	hasTimeT = 0,
	hasStringH = 0,
	hasDirentH = 0,
	hasSymlinks = 0,
	hasBeep = 0,
	hasKeypad = 0,
	hasWGetNStr = 0,
	hasGetCWD = 0,
	hasWnOutRefresh = 0,
	hasRedrawWin = 0,
	hasMore = 0,
	hasLess = 0,
	hasPg = 0,
	useSun5CC = 0,
	doDebug = 0;

static context_t checks[] = {

	/* SunOS System V compatibility compiler */

	{"/usr/5bin/cc", NULL, &useSun5CC},

	/* See what libraries are available */

	{"/usr/lib/libucb.a", NULL, &hasLibucb},
	{"/usr/lib/libcursesX.a", NULL, &hasLibCursesX},
	{"/usr/lib/libtinfo.a", NULL, &hasLibTinfo},
	{"/usr/lib/libtermcap.a", NULL, &hasLibTermcap},
	{"/usr/lib/libxtermcap.a", NULL, &hasLibXTermcap},
	{"/usr/lib/libxcurses.a", NULL, &hasLibXCurses},
	{"/usr/lib/libcurses.a", NULL, &hasLibCurses},
	{"/usr/lib/libncurses.a", NULL, &hasLibNCurses},
	{"/usr/local/lib/libncurses.a", NULL, &hasLibNCurses},

	/* Recent SVR4.2 releases go put libraries in /usr/ccs/lib... */

	{"/usr/ccs/lib/libcurses.a", NULL, &hasLib42Curses},

	/* ...and nroff in /usr/ucb... */

	{"/usr/ucb/nroff", NULL, &hasNroff42},

	/* See what header files are available */

	{"/usr/include/unistd.h", NULL, &hasUnistdH},
	{"/usr/include/string.h", NULL, &hasStringH},
	{"/usr/include/dirent.h", NULL, &hasDirentH},
	{"/usr/include/term.h",	  NULL, &hasTermH},
	{"/usr/include/ncurses/ncurses.h",  NULL, &hasNcursesDir},

	/* See what basic types are available */

	{"/usr/include/sys/types.h", "mode_t", &hasModeT},
	{"/usr/include/sys/types.h", "uid_t", &hasUidT},
	{"/usr/include/sys/types.h", "gid_t", &hasGidT},
	{"/usr/include/sys/types.h", "off_t", &hasOffT},
	{"/usr/include/sys/types.h", "time_t", &hasTimeT},

	/* Linux has the types in /usr/include /linux :-( */

	{"/usr/include/linux/types.h", NULL, &isLinux},

	/* Under SunOS we need to check stdtypes.h */

	{"/usr/include/sys/stdtypes.h", "mode_t", &hasModeT},
	{"/usr/include/sys/stdtypes.h", "uid_t", &hasUidT},
	{"/usr/include/sys/stdtypes.h", "gid_t", &hasGidT},
	{"/usr/include/sys/stdtypes.h", "off_t", &hasOffT},
	{"/usr/include/sys/stdtypes.h", "time_t", &hasTimeT},

	/* See what newer curses library routines are available */

	{"/usr/include/curses.h", "start_color", &hasColour},
	{"/usr/include/curses.h", "curs_set", &hasCursSet},
	{"/usr/include/curses.h", "beep", &hasBeep},
	{"/usr/include/curses.h", "keypad", &hasKeypad},
	{"/usr/include/curses.h", "wgetnstr", &hasWGetNStr},
	{"/usr/include/curses.h", "wnoutrefresh", &hasWnOutRefresh},
	{"/usr/include/curses.h", "redrawwin", &hasRedrawWin},
	{"/usr/include/curses.h", "reset_shell_mode", &hasResetModes},

	/* Do it all again for ncurses */

	/* Under SunOS, we check for /usr/5include/curses.h as well */

	{"/usr/5include/curses.h", "start_color", &hasColour},
	{"/usr/5include/curses.h", "curs_set", &hasCursSet},
	{"/usr/5include/curses.h", "beep", &hasBeep},
	{"/usr/5include/curses.h", "keypad", &hasKeypad},
	{"/usr/5include/curses.h", "wgetnstr", &hasWGetNStr},
	{"/usr/5include/curses.h", "wnoutrefresh", &hasWnOutRefresh},
	{"/usr/5include/curses.h", "redrawwin", &hasRedrawWin},
	{"/usr/5include/curses.h", "reset_shell_mode", &hasResetModes},

	/* and on SCO we check tinfo.h */

	{"/usr/include/tinfo.h", "start_color", &hasColour},
	{"/usr/include/tinfo.h", "curs_set", &hasCursSet},
	{"/usr/include/tinfo.h", "beep", &hasBeep},
	{"/usr/include/tinfo.h", "keypad", &hasKeypad},
	{"/usr/include/tinfo.h", "wgetnstr", &hasWGetNStr},
	{"/usr/include/tinfo.h", "wnoutrefresh", &hasWnOutRefresh},
	{"/usr/include/tinfo.h", "redrawwin", &hasRedrawWin},
	{"/usr/include/tinfo.h", "reset_shell_mode", &hasResetModes},

	/* See what other library routines are available */

	{"/usr/include/regexp.h", "step", &useSysVregexp},	/* System V regexps */
	{"/usr/include/regex.h", "regcomp", &usePOSIXregexp},
	{"/usr/include/string.h", "strerror", &hasStrError},
	{"/usr/include/stdlib.h", "srand48", &hasSrand48},
	{"/usr/include/sys/stat.h", "lstat", &hasSymlinks},	/* Symbolic links */
	{"/usr/include/unistd.h", "getcwd", &hasGetCWD},
	{NULL, NULL, NULL}
};

typedef struct {
	char           *flag;
	int            *val;
}               config_t;

static config_t missingTbl[] =
{
	{"STRING_H", &hasStringH},
	{"DIRENT_H", &hasDirentH},
	{"TERM_H", &hasTermH},
	{"MODE_T", &hasModeT},
	{"UID_T", &hasUidT},
	{"GID_T", &hasGidT},
	{"OFF_T", &hasOffT},
	{"TIME_T", &hasTimeT},
	{"COLOUR", &hasColour},
	{"CURS_SET", &hasCursSet},
	{"BEEP", &hasBeep},
	{"KEYPAD", &hasKeypad},
	{"WGETNSTR", &hasWGetNStr},
	{"WNOUTREFRESH", &hasWnOutRefresh},
	{"REDRAWWIN", &hasRedrawWin},
	{"STRERROR", &hasStrError},
	{"SRAND48", &hasSrand48},
	{"SYMLINKS", &hasSymlinks},
	{"GETCWD", &hasGetCWD},
	{"LINUX", &isLinux},
	{NULL, NULL}
};

static config_t libraryTbl[] =	/* Order is important here! */
{
	{"curses", &hasLibCurses},
	{"curses", &hasLib42Curses},
	{"curses", &hasLibXCurses},
	{"ncurses", &hasLibNCurses},
	{"cursesX", &hasLibCursesX},
	{"termcap", &hasLibTermcap},
	{"termcap", &hasLibXTermcap},
	{"tinfo", &hasLibTinfo},
	{"ucb", &hasLibucb},
	{NULL, NULL}
};

/* crude text search function */

#if __STDC__
int 
search(int i)
{
#else
int 
search(i)
{
#endif
	int             j, l, L;
	char           *s = checks[i].text;
	char            c = s[0];
	FILE           *fp = fopen(checks[i].fname, "r");
	L = strlen(s);
	if (fp == NULL)
		return 0;	/* fail */
	while (!feof(fp)) {
		fgets(buf, 160, fp);
		l = strlen(buf) - L;
		if (l >= 0) {
			for (j = 0; j < l; j++)
				if (buf[j] == c)
					if ((int) strncmp(buf + j, s, L) == 0)
						return 1;
		}
	}
	fclose(fp);
	return 0;
}

void 
checkSys()
{
	int             i;
	for (i = 0; checks[i].fname; i++) {
		if (*(checks[i].val) == 0) {
			if (stat(checks[i].fname, &st) == 0) {
				if (checks[i].text == NULL) {
					printf("You do have %s\n", checks[i].fname);
					*(checks[i].val) = 1;
				} else {
					if (search(i)) {
						*(checks[i].val) = 1;
						printf("You do have %s in %s\n",
						       checks[i].text, checks[i].fname);
					} else
						printf("You don't have %s in %s\n",
						       checks[i].text, checks[i].fname);
				}
			} else
				printf("You don't have %s\n", checks[i].fname);
		}
	}
}

#if __STDC__
int 
hasSuffix(char *s)
#else
int 
hasSuffix(s)
	char           *s;
#endif
{
	return (_hasSuffix(s, "cat") || _hasSuffix(s, "man"));
}

proginfo        compilers[] = {
	{"/usr/5bin/cc", "Sun C compiler with SysV Curses compatibility", "/usr/5bin/cc"},
	{"cc", "System C compiler", "cc"},
	{"gcc", "Gnu C compiler", "gcc"},
	{NULL, NULL, NULL}
};

#endif				/* no MSDOS */

#if __STDC__
int getYesNo(char *msg, int defalt)
{
#else
int getYesNo(msg, defalt)
	char           *msg;
	int             defalt;
{
#endif
	char            buf[20];
	printf("%s (y/n)\n(Press ENTER for default of %s) ? ", msg,
	       defalt ? "yes" : "no");
	fflush(stdout);
	for (;;) {
		gets(buf);
		if (buf[0] == 0)
			return defalt;
		else if (buf[0] == 'Y' || buf[0] == 'y')
			return 1;
		else if (buf[0] == 'N' || buf[0] == 'n')
			return 0;
	}
}

#if __STDC__
int 
getReply(char *n[], int def)
{
#else
int 
getReply(n, def)
	char           *n[];
	int             def;
{
#endif
	int             i, rtn = 0;
	printf("%s\n\n", n[0]);
	i = 0;
	while (n[++i])
		printf("%d\t%s\n", i, n[i]);
	printf("\nPress ENTER to get the default of %s\n>", n[def]);
	gets(buf);
	if (buf[0] >= '1' && buf[0] <= '9')
		sscanf(buf, "%d", &rtn);
	else
		rtn = def;
	return rtn;
}

#if __STDC__
int 
searchPath(char *name)
#else
int 
searchPath(name)
	char           *name;
#endif
{
	int             s = 0, e, l;
	char           *ps = getenv("PATH"), x[2];
	if (ps)
		strcpy(path, ps);
	else
		path[0] = 0;
	x[0] = PATHSEP;
	x[1] = 0;
	while (path[s]) {
		if (path[s] == FIELDSEP) {
			s++;
			continue;
		}
		e = s;
		do {
			e++;
		} while (path[e] && path[e] != FIELDSEP);
		l = e;
		if (path[e]) {
			path[e] = '\0';
			e++;
		}
		strcpy(buf, path + s);
		if (path[l - 1] != PATHSEP)
			strcat(buf, x);
		strcat(buf, name);
		if (access(buf, 0) == 0)
			return 1;
		s = e;
	}
	return 0;		/* failed */
}

#if __STDC__
char           *
getProgram(char *type, proginfo checks[], char *defalt)
{
#else
char           *
getProgram(type, checks, defalt)
	char           *type, *defalt;
	proginfo        checks[];
{
#endif
	static char     buf[20];
	int             i = 0, j = -1, def = -1, l;
	printf("You have the following possible %ss:\n", type);
	while (checks[i].name) {
		if (access(checks[i].name, 0) == 0 ||
		    searchPath(checks[i].name)) {
			printf("\t%s `%s'\n", checks[i].description, checks[i].invocation);
			if (j == -1)
				j = i;
			if (defalt && defalt[0] &&
			    strcmp(defalt, checks[i].name) == 0)
				def = i;
		}
		i++;
	}
	if (def == -1)
		def = j;
	printf("\tOther (you can specify the program name)\n");
	printf("Which should I use? (enter the name as shown in the quotes)\n");
	if (def != -1)
		printf("Press ENTER to get the default of %s `%s'\n>",
		       checks[def].description, checks[def].invocation);
	fflush(stdout);
retry:
	gets(buf);
	if (buf[0] < 32)
		if (def == -1)
			goto retry;
		else
			strcpy(buf, checks[def].invocation);
	l = strlen(buf);
	/* Strip trailing newline, if any */
	if (l > 0) {
		if (buf[--l] < 32)
			buf[l] = 0;
	}
	return buf;
}

#ifdef __MSDOS__
void 
DOSdef(char *cc, int hasSpawn0, int doDebug)
{
	char           *tmp;
	fputs("OSUF\t=\t.obj\n", of);
	fputs("XSUF\t=\t.exe\n", of);
	fputs("GC3OUT\t=\t-ogc3\n", of);
	fputs("SELOUT\t=\t-osel\n", of);
	fputs("CODEFILE\t=\tgc3.cod\n", of);
	fprintf(of, "CC\t=\t%s\n", cc);
	fputs("#define OPTIMIZE\n", hf);
	if (hasSpawn0) {
		fputs("#define HAS_SPAWN0\n", hf);
		fputs("LIBS\t=\tspawnl.lib\n", of);
	} else
		fputs("LIBS\t=\t\n", of);
	if (doDebug) {
		fputs("CFLAGS\t=\t-ml -H -v\n", of);
		fputs("LDFLAGS\t=\t-ml -ls -v\n", of);
		fputs("#define DEBUG\n", hf);
	} else {
		fputs("CFLAGS\t=\t-O -ml -H\n", of);
		fputs("LDFLAGS\t=\t-ml -ls\n", of);
	}
	fprintf(of, "BINDEST\t=%s\n", installPath);
	fputs("COPY\t=\tcopy\n", of);
	/* fputs("RENAME\t=\tren\n",of); */
	fputs("DELETE\t=\tdel\n", of);
	fprintf(of, "WILDARGS\t=\t%s\\wildargs.obj\n", manPath);
	fputs("GC3CC\t=\t.\\gc3 -C\n", of);
	fputs("RULES\t=\t$(BINDEST)\\gc3.rul\n", of);
	fputs("\ndefault: dosdeflt\n\ninstall: dosinst\n\nclean: bothclean dosclean\n\n", of);
}
#endif

#if __STDC__
int 
_hasSuffix(char *s, char *p)
#else
int 
_hasSuffix(s, p)
	char           *s, *p;
#endif
{
	strcpy(buf, manPath);
	strcat(buf, "/");
	strcat(buf, p);
	strcat(buf, s);
	if (access(buf, 0) == 0)
		return 1;
	else
		return 0;
}

#if __STDC__
int 
isAnsi(void)
#else
int 
isAnsi()
#endif
{
	int             def;
	printf("Does your compiler support ANSI C (Press ENTER for default of `%c') >",
	       def = ((comp == C_ANSICC || comp == C_GCCANS) ? 'y' : 'n'));
	gets(buf);
	if (buf[0] < 32)
		return (def == 'y');
	else
		return (buf[0] == 'Y' || buf[0] == 'y');
}

#ifndef __MSDOS__

/* default install paths... */

char           *installPaths[] = {
	"/home/bin", "/local/bin", "/usr/acct/bin", "/home2/bin",
	"/usr/local/bin", "/usr/bin", NULL
};

char           *manPaths[] = {
	"/usr/share/man", "/usr/man", "/usr/catman", "/usr/flib/books/man", NULL
};


#if __STDC__
static void 
getDefaultPath(char *rtn, char **list)
#else
static void 
getDefaultPath(rtn, list)
	char           *rtn, **list;
#endif
{
	int             i = 0;
	while (list[i]) {
		if (access(list[i], 0) == 0) {
			strcpy(rtn, list[i]);
			return;
		} else
			i++;
	}
	strcpy(rtn, getenv("HOME"));	/* default to this */
}
#endif


main()
{
	char            c, *tmp;
	int             i, hasSpawn0;
	if ((of = fopen("site.def", "w")) == NULL) {
		fprintf(stderr, "Cannot open output file site.def\n");
		exit(-1);
	}
	if ((hf = fopen("gcsite.h", "w")) == NULL) {
		fprintf(stderr, "Cannot open output file site.h\n");
		exit(-1);
	}
	df = fopen("site.ans", "r");
#ifdef __MSDOS__
	if (df) {
		fscanf(df, "%s%s%s%d%d",
		       compiler, installPath, manPath, &doDebug, &hasSpawn0);
		fclose(df);
	} else {
		installPath[0] = compiler[0] = manPath[0] = '\0';
		doDebug = hasSpawn0 = 0;
	}
	strcpy(compiler, getProgram("C compiler", compilers, NULL));
	printf("Which directory do you want to install gc3 in [%s]? ", installPath);
	fflush(stdout);
	gets(buf);
	if (buf[0] > 32)
		strcpy(installPath, buf);
	printf("Which directory has your compiler libraries (for wildargs.obj) [%s]? ", manPath);
	fflush(stdout);
	gets(buf);
	if (buf[0] > 32)
		strcpy(manPath, buf);
	hasSpawn0 = getYesNo("Do you have Ralf Brown's Spawn0 EMS/XMS swapper?", hasSpawn0);
	doDebug = getYesNo("Do you want debugging info compiled in?", doDebug);
	DOSdef(compiler, hasSpawn0, doDebug);
	df = fopen("site.ans", "w");
	if (df) {
		fprintf(df, "%s\n%s\n%s\n%d\n%d\n",
			compiler, installPath, manPath, doDebug, hasSpawn0);
		fclose(df);
	}
#else

	/* Fixed for all sites */

	fputs("OSUF\t=\t.o\n", of);
	fputs("XSUF\t=\t\n", of);
	fputs("SELMANPAGE\t=\tsel.cat\n", of);

	/* Get the default answers */

	if (df) {
		fscanf(df, "%s%d%d%s%s%s%d",
		       compiler, &trgt, &comp,
		       installPath, manSuffix, manPath, &doDebug);
		fclose(df);
	} else {
		/* First time - make rational defaults */
		trgt = 1;
		getDefaultPath(installPath, installPaths);
		getDefaultPath(manPath, manPaths);

		/* Manual page suffix */
		if (hasSuffix("l"))
			strcpy(manSuffix, "l");
		else if (trgt == T_SCO && hasSuffix(".LOCAL"))
			strcpy(manSuffix, "LOCAL");
		else if (trgt == T_SCO && hasSuffix(".CMD"))	/* CHECK !! */
			strcpy(manSuffix, "CMD");
		else
			strcpy(manSuffix, "1");
	}
	puts("Checking your system...");
	checkSys();
	if (hasLibCurses || hasLibXCurses) {
		hasLibCurses = 1;
		hasLibXCurses = 0;
	}
	if (hasLibCurses && hasLibNCurses)
	{
		hasLibCurses = 0;
		hasLibNCurses = 1;
		hasColour = hasCursSet = hasBeep = hasKeypad
			= hasWGetNStr = hasWnOutRefresh
			= hasRedrawWin = hasResetModes  = 1;
	}
	if (hasLibTermcap || hasLibXTermcap)
	{
		hasLibTermcap = 1;
		hasLibXTermcap = 0;
	}
	if (useSun5CC)
		comp = C_SUN5;
	else
		comp = C_KRCC;
	if (isLinux)
	{
		hasModeT = hasUidT = hasGidT = hasOffT = hasTimeT = 1;
	}
	puts("\nCheck complete\n");
	if (strcmp(compiler, "gcc") == 0)
		comp = C_GCCANS;
	else if (strcmp(compiler, "cc") == 0)
		comp = C_KRCC;
	else if (strcmp(compiler, "/usr/5bin/cc") == 0)
		comp = C_SUN5;
	goto confirm;
retry:
	strcpy(compiler, getProgram("C compiler", compilers, compiler));
	trgt = getReply(tgtNames, trgt);
	if (strcmp(compiler, "gcc") == 0)
		comp = isAnsi() ? C_GCCANS : C_GCCKR;
	else if (strcmp(compiler, "cc") == 0)
		comp = isAnsi() ? C_ANSICC : C_KRCC;
	else if (strcmp(compiler, "/usr/5bin/cc") == 0)
		comp = C_SUN5;
	else {
		fprintf(stderr, "Warning - unknown C compiler `%s'!\n", compiler);
		comp = C_KRCC;
	}

	/* Installation targets */

	printf("\nWhich directory do you want to install gc3 in [%s]? ", installPath);
	fflush(stdout);
	gets(buf);
	if (buf[0] > 32)
		strcpy(installPath, buf);
	printf("Which directory are the manual pages in [%s]? ", manPath);
	fflush(stdout);
	gets(buf);
	if (buf[0] > 32)
		strcpy(manPath, buf);
	printf("What is the manual page suffix [%s] ? ", manSuffix);
	fflush(stdout);
	gets(buf);
	if (buf[0] > 32)
		strcpy(manSuffix, buf);
	doDebug = getYesNo("Do you want debugging info compiled in", doDebug);
confirm:
	puts("\nPlease confirm your settings:\n");
	printf("\tTarget Architecture: \t%s\n", tgtNames[trgt]);
	printf("\tC Compiler:          \t%s%s\n",
	       (comp == C_ANSICC || comp == C_GCCANS) ? "ANSI " : "",
	       (comp == C_GCCANS || comp == C_GCCKR) ? "Gnu CC" :
	    ((comp == C_ANSICC || comp == C_KRCC) ? "cc" : "/usr/5bin/cc"));
	printf("\tInstallation path:   \t%s\n", installPath);
	printf("\tManual area suffix:  \t%s\n", manSuffix);
	printf("\tManual area path:    \t%s\n", manPath);
	printf("\tDebugging info:      \t%s\n\n", doDebug ? "Yes" : "No");
	if (!getYesNo("\nIs this correct", 1))
		goto retry;

	/* Save the answers for next time */

	df = fopen("site.ans", "w");
	if (df) {
		fprintf(df, "%s\n%d\n%d\n%s\n%s\n%s\n%d\n",
			compiler, trgt, comp,
			installPath, manSuffix, manPath, doDebug);
		fclose(df);
	}
	if (searchPath("nroff")) {
		fputs("ROFF\t=\tnroff\n", of);
		fputs("GC3MTRGT\t=\tgc3.cat\n", of);
		fputs("SELMTRGT\t=\tsel.cat\n", of);
	} else if (hasNroff42) {
		fputs("ROFF\t=\t/usr/ucb/nroff\n", of);
		fputs("GC3MTRGT\t=\tgc3.cat\n", of);
		fputs("SELMTRGT\t=\tsel.cat\n", of);
	} else {
		fputs("ROFF\t=\t\n", of);
		fputs("GC3MTRGT\t=\t\n", of);
		fputs("SELMTRGT\t=\t\n", of);
	}
	fputs("RFLAGS\t=\t-man\n", of);

	fprintf(of, "BINDEST\t=\t%s\n", installPath);
	fprintf(of, "MANSUF\t=%s\n", manSuffix);
	if (trgt != T_SCO) {
		fprintf(of, "MANDEST\t=\t%s/man$(MANSUF)\n", manPath);
		fprintf(of, "CATDEST\t=\t%s/cat$(MANSUF)\n", manPath);
	} else {
		fprintf(of, "MANDEST\t=\t%s/man.$(MANSUF)\n", manPath);
		fprintf(of, "CATDEST\t=\t%s/cat.$(MANSUF)\n", manPath);
	}
	fputs("GC3OUT\t=\t-o gc3\n", of);
	fputs("SELOUT\t=\t-o sel\n", of);
	fputs("CODEFILE\t=\t.gc3cod\n", of);

	/* Compiler options */

	switch (comp) {
	case C_GCCKR:
		fputs("CC\t=\tgcc\n", of);
		fprintf(of, "CFLAGS\t=\t-traditional -Wall -Wshadow -Wpointer-arith -O");
		break;
	case C_GCCANS:
		fputs("CC\t=\tgcc\n", of);
		fprintf(of, "CFLAGS\t=\t-ansi -pedantic -O -Wall -Wshadow -Wpointer-arith");
		break;
	case C_KRCC:
		fputs("CC\t=\tcc\n", of);
		fprintf(of, "CFLAGS\t=\t-O ");
		break;
	case C_ANSICC:
		fputs("CC\t=\tcc\n", of);
		fprintf(of, "CFLAGS\t=\t-O -Xc");
		break;
	case C_SUN5:
		fputs("CC\t=\t/usr/5bin/cc\n", of);
		fprintf(of, "CFLAGS\t=\t-O ");
	}
	if (trgt == T_MIPS)
		fputs(" -systype sysv", of);
	if (doDebug)
		fputs(" -g", of);
	if (hasLibNCurses)
		if (hasNcursesDir)
			fputs(" -I/usr/include/ncurses", of);
		else
			fputs(" -I/usr/local/include", of);
	fputs("\n", of);

	fprintf(of, "LDFLAGS\t=\t%s %s\n",
		hasLib42Curses ? "-L/usr/ccs/lib" : "",
		doDebug ? "-g" : "");

	/* site.h #define's */

	if (useSun5CC || !hasResetModes || hasLibNCurses)
		fputs("#define BUMP_CURSES\n", hf);
	if (hasUnistdH)
		fputs("#define HAS_UNISTD\n", hf);
	if (doDebug)
		fputs("#define DEBUG\n", hf);
	if (useSysVregexp)
		fputs("#define USE_SYSV_REGEXP\n", hf);
	else if (!usePOSIXregexp)
		fputs("#define USE_BSD_REGEXP\n", hf);
	if (trgt == T_NEXT)
		fputs("#define UCBCURSES\n", hf);
	i = 0;
	while (missingTbl[i].val) {
		if (!(*(missingTbl[i].val)))
			fprintf(hf, "#define NO_%s\n", missingTbl[i].flag);
		i++;
	}
	if (hasLibNCurses)
		fprintf(hf, "#define USE_NCURSES\n");

	/* LIBS macro */

	fputs("LIBS\t=\t", of);
	i = 0;
	while (libraryTbl[i].val) {
		if (*(libraryTbl[i].val))
			fprintf(of, "-l%s ", libraryTbl[i].flag);
		i++;
	}
	fputs("\n", of);

	fputs("COPY\t=\tcp\n", of);
	/* fputs("RENAME\t=\tmv\n",of); */
	fputs("DELETE\t=\trm -f \n", of);
	fputs("CHMOD\t=\tchmod\n", of);
	fputs("WILDARGS\t=\t\n", of);
	fputs("GC3CC\t=\t./gc3 -C\n", of);
	fputs("RULES\t=\t$(BINDEST)/.gc3rul\n", of);
	fputs("\n\ndefault: unxdeflt\n\ninstall: unxinst\n\nclean: bothclean unixclean\n\n", of);
#endif
	fclose(of);
	fclose(hf);
	exit(0);
}
