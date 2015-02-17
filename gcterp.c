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
#include "gckey.h"
#include "gcops.h"

#define GC_CURSES
#include "gclib.h"

#include <fcntl.h>

#define MAX_REGS	16

static int	regBuf[MAX_REGS];
static char	*regPtr[MAX_REGS];

#define PARAM(n)	codespace[old_ip+n]

#undef HAS_SPAWN0

#ifndef min
#define min(a,b)	(((a)<(b))?(a):(b))
#endif

#define MAX_NEST	16	/* Maximum nesting of function calls	*/
#define MAX_LOOP	8	/* Maximum nesting of loops in function	*/

typedef struct {
	short v;
	short type;
	short idx;
	short s, e;
	short pos;
} loopInfo;

typedef struct {
	short	ip;			/* instruction pointer for return */
	short	loopP;			/* Loop nest level		*/
	short	name;			/* function name index		*/
	short	np;			/* number of params		*/
	short	nv;			/* number of vars		*/
	short	param[MAX_PARAMS];	/* Parameter indices		*/
	short	pnames[MAX_PARAMS];	/* Parameter names		*/
	short	vnames[MAX_LOCVARS];	/* Local variable names		*/
	char	*locvar[MAX_LOCVARS];	/* Local var value pointers	*/
	loopInfo loopStack[MAX_LOOP];
} fcallInfo_t;

static int	returnVal;	/* Last return value	*/
fcallInfo_t	callStack[MAX_NEST];
static short	callStkLvl = 0,
		old_ip,
		lookupLevel;
char	*valspace[MAX_VARS] = {NULL};

short		ip = 0;
FILE	  *catchFP=NULL;
int		fgCol=White,
			bgCol=Blue;
int		fnameLen = 22;
int		winInvalid[2] = { 2, 2 };
int		winState[2] = { W_LIST, W_LIST };
int		showLines[2] = { 99, 99 };
int		hborder = 0;
int		vborder = 0;
int		filtering[2] = { 0, 0 };
int		changedWinSize = 0;
short		helpFile = 0, helpEntry = 0;
char		tempName[MAXPATHNAME];
char	  *BUF[MAX_BUFFERS];
static char  *bufFile[MAX_BUFFERS] = { NULL };
static int	bufLine[MAX_BUFFERS] = { 0 };
#ifdef DEBUG
static int	maxBufsUsed = 0;
#endif
char		file2view[MAXPATHNAME];
int		viewingDir = 0;

#ifdef __MSDOS__
REcompileF_t	compileRegexp = DOScompileRE;
REmatchF_t		matchRegexp = DOSmatchRE;
#else
REcompileF_t	compileRegexp = compileRE;
REmatchF_t		matchRegexp = matchRE;
#endif


#ifdef __MSDOS__
	char	filters[2][14] = { "*.*", "*.*" };
#else
	char	filters[2][64] = { ".", "." };
#endif

#ifndef __MSDOS__
#ifdef NO_SRAND48
#define randomize()	srand((int)time(NULL))
#define random(n)	(rand() % (n))
#else
#define randomize()	srand48((long)time(NULL))
#define random(n)	(lrand48() % (n))
#endif
#endif

#if	__STDC__
static short	lookupName(char *name);
#else
static short	lookupName();
#endif

#if	__STDC__
int testOption(int opt)
#else
int testOption(opt)
	int opt;
#endif
{
	char c = 0;
	if (valspace[opt]) c = valspace[opt][0];
	return (c=='y' || c=='Y' || (isdigit(c) && c!='0'));
}

/************************************************************/

char *stringspace = NULL;

#if __STDC__
void AllocateStringSpace(void)
#else
void AllocateStringSpace()
#endif
{
	assert(stringspace == NULL);
	stringspace = (char *)calloc(STRINGSPACE,sizeof(char));
	assert(stringspace);
}

#if __STDC__
void FreeStringSpace(void)
#else
void FreeStringSpace()
#endif
{
	assert(stringspace);
	free(stringspace);
	stringspace = NULL;
}

#if	__STDC__
int GetBuffer(char *fname, int lnum)
#else
int GetBuffer(fname, lnum)
	char *fname;
	int lnum;
#endif
{
	char msg[80];
	int i = 1;
	while (i<MAX_BUFFERS)
	{
		if (bufFile[i] == NULL)
		{
			bufFile[i] = fname;
			bufLine[i] = lnum;
#ifdef DEBUG
			if (i>=maxBufsUsed)
				maxBufsUsed = i+1;
#endif
			BUF[i][0] = '\0';
			return i;
		}
		else i++;
	}
	sprintf(msg,"Out of buffers at %s (%d)! (press ENTER for more)",
		fname, lnum);
	showMsg(msg); (void)my_getch();
	for (i=1;i<MAX_BUFFERS;i++)
	{
		sprintf(msg,"Buf %d allocated at %s (%d) (press ENTER for more)",
			i,bufFile[i], bufLine[i]);
		showMsg(msg); (void)my_getch();
	}
	return 0; /* Reserved for this case to hopefully recover */
}

#if	__STDC__
void ReleaseBuffer(int buf, char *fname, int lnum)
#else
void ReleaseBuffer(buf, fname, lnum)
	int buf, lnum;
	char *fname;
#endif
{
	if (bufFile[buf]) bufFile[buf] = NULL;
	else
	{
		char msg[60];
		sprintf(msg,"ReleaseBuffer called in error from %s (%d)!\n",
			fname, lnum);
		showMsg(msg);
		sleep(1);
	}
}

#if __STDC__
static int promptForKey(void)
#else
static int promptForKey()
#endif
{
	if (inCurses)
		showMsg("Press a key...");
	else
	{
		fprintf(stderr,"\nPress a key...");
		fflush(stderr);
	}
	return my_getch();
}

#if	__STDC__
static int prepareRE(char *re)
#else
static int prepareRE(re)
	char *re;
#endif
{
	char *dbg = compileRegexp(re);
	if (dbg)
	{
		showMsg(dbg);
		return -1;
	} else return 0;
}

#if __STDC__
static char *randname(void)
#else
static char *randname()
#endif
{
	static char _randname[10];
	int i=0;
	(void)randomize();
	while (i<8)
		_randname[i++] = 'A' + (char)random(26);
	_randname[8] = '\0';
	return _randname;
}

/********************************************************
		FILE SELECTION
*********************************************************/

#if	__STDC__
void nselect(int L, int n, int v)
#else
void nselect(L, n, v)
	int L, n, v;
#endif /* __STDC__ */
{
	fInfo_t *p = &fInfo[L][n];
	if (v)
	{
#ifdef __MSDOS__
		if (!testOption(VAR_DIRSELECT) && p->typID==PATH_SEP) return;
#else
		if (!testOption(VAR_DIRSELECT) && S_ISDIR(p->mode)) return;
#endif
		if ((p->flag & F_SELECTED)==0)
		{
			p->flag |= F_SELECTED;
			selCnt[L]++;
			selSize[L]+=p->size;
		}
	}
	else
	{
		if (p->flag & F_SELECTED)
		{
			p->flag &= ~F_SELECTED;
			selCnt[L]--;
			selSize[L]-=p->size;
		}
	}
}

/***************************************************************/

#if __STDC__
short dereference(short var, char ***space)
#else
short dereference(var, space)
	short var;
	char ***space;
#endif
{
	int lev = lookupLevel-1;
#ifdef DEBUG
	short i, startIdx=var;
	int dbg = testOption(VAR_DEBUGGING);
	if (dbg)
	{
		fprintf(debug,"\tIn deref(%d), lookupLevel=%d\n\tStack:\n",
					var,lookupLevel);
		for (i=0;i<lookupLevel;i++)
		{
			short j;
			fprintf(debug,"\t\tLevel: %d %-16s ( ",i,
					LITERAL(callStack[i].name));
			for (j=0;j<callStack[i].np;j++)
				fprintf(debug,"%d ",callStack[i].param[j]);
			fprintf(debug," ) [ ");
			for (j=0;j<callStack[i].nv;j++)
				fprintf(debug,"%s ",callStack[i].locvar[j]);
			fprintf(debug,"]\n");
		}
	}
#endif
	*space = valspace;
	while (var>=MAX_IDS && lev>=0)
	{
		if (IS_PARAM(var))
		{
			var = callStack[lev].param[FROM_PARAM(var)];
#ifdef DEBUG
			if (dbg) fprintf(debug,"\tparameter - derefed to lev %d, idx %d\n",lev,var);
#endif
		}
		else
		{
			if (IS_LOCPAR(var))
			{
				var = DEREF_LOCPAR(var);
#ifdef DEBUG
				if (dbg) fprintf(debug,"\tlocvar parameter - derefed to lev %d, idx %d\n",lev,var);
#endif
			}
			if (IS_LOCAL(var))
			{
				*space = callStack[lev].locvar;
#ifdef DEBUG
				if (dbg) fprintf(debug,"\tlocvar - derefed to lev %d, idx %d - %s\n",lev,var,(*space)[FROM_LOCAL(var)]);
#endif
				return var;
			}
		}
		lev--;
	}
	if (var>=MAX_IDS)
	{
		showMsg("Variable dereference error! (ignoring)");
		sleep(1);
#ifdef DEBUG
		fprintf(debug,"Dereference error! Index %d, at end %d (levs %d)\n",
				startIdx, var, lookupLevel);
#endif
		return 0; /* semi-recovery */
	}
#ifdef DEBUG
	if (dbg)
	{
		fprintf(debug,"\tdereference returns %d from level %d, callStkLvl=%d (",
				var, lev+1, callStkLvl);
		printArg(debug,var);
		fprintf(debug,")\n");
	}
#endif
	return var;
}

#if __STDC__
static int _expand(char *str, int buf, int esc)
#else
static int _expand(str, buf,esc)
	char *str;
	int buf, esc;
#endif
{
	int i, j, L, more=0;
	for (i=j=0, L = (int)strlen(str); i<L && j<BUFFER_SIZE;i++)
	{
		if (str[i]=='#')
		{
			/* if esc is zero we don't do it yet */
			if (esc==0)
			{
				BUF[buf][j++] = '#';
				BUF[buf][j++] = str[++i];
			}
			/* check for escape #nnn */
			else if (i<(L-3) && isdigit(str[i+1]) &&
				isdigit(str[i+2]) && isdigit(str[i+3]))
			{
				char num[4];
				strncpy(num,str+i+1,3);
				num[3]=0;
				BUF[buf][j++] = atoi(num);
				i+=3;
			}
			else if (i<(L-1))
			{
				i++;
				if (str[i]=='n')		BUF[buf][j++] = '\n';
				else if (str[i]=='t')	BUF[buf][j++] = '\t';
				else if (str[i]=='r')	BUF[buf][j++] = '\r';
				else if (str[i]=='w')
				{
					BUF[buf][j++] = ' ';
					BUF[buf][j++] = '\t';
				}
				else BUF[buf][j++] = str[i];
			}
		}
		else if (!esc && str[i]=='$')
		{
			char varname[40]; int p=0; short idx;
			i++;
			while (isalnum((int)str[i]) || str[i]=='_') 
				varname[p++] = str[i++];
			varname[p] = '\0';
			i--;
			STRUPR(varname);
			if ((idx = lookupName(varname))>=0)
			{
				int buf2 = GRABBUF();
				char *s = lookupVar(idx,buf2);
				if (STRCHR(s,'$')) more=1;
				strcpy(BUF[buf]+j,s);
				FREEBUF(buf2);
			}
			else /* No such variable */
			{
				BUF[buf][j++] = '$';
				strcpy(BUF[buf]+j,varname);
			}
			j = (int)strlen(BUF[buf]);
		}
		else BUF[buf][j++] = str[i];
	}
	BUF[buf][min(j,(BUFFER_SIZE-1))]='\0';
	if (j>=BUFFER_SIZE) showMsg("Buffer overflow!");
	return more;
}

#if __STDC__
static char *expand(char *str, int buf)
#else
static char *expand(str, buf)
	char *str;
	int buf;
#endif
{
	int t, tmpBuffer = GRABBUF(), b=buf, more;
	int B = buf, O = tmpBuffer;
	lookupLevel = callStkLvl;
	strcpy(BUF[buf],str);
	for (;;)
	{
		if (lookupLevel<0) break;
		B = tmpBuffer; O = buf;
		more = _expand(BUF[O],B,0);
		if (more==0) break;
		else if (more==2) lookupLevel--; /* lower scope */
		t = tmpBuffer; tmpBuffer=buf;buf=t; /* faster than strcpy! */
	}
	lookupLevel = callStkLvl;
	if (b==B)
	{
		_expand(BUF[b],O,1);
		strcpy(BUF[b],BUF[O]);
		FREEBUF(O);
	}
	else
	{
		_expand(BUF[B],b,1);
		FREEBUF(B);
	}
#ifdef DEBUG
	if (testOption(VAR_DEBUGGING))
		fprintf(debug,"After expand, str %s=%s\n",str,BUF[b]);
#endif
	return BUF[b];
}

#if	__STDC__
int findFile(int L, int s, char *fname, int *idx)
#else
int findFile(L, s, fname, idx)
	int L, s, *idx;
	char *fname;
#endif
{
	int i = s;
	do
	{
		if (strcmp(INFO(L,i).name,fname)==0)
		{
			if (idx) *idx=fIndex[L][i];
			return i;
		}
		i = (i+1) % numfiles[L];
	}
	while (i!=s);
	return -1;
}

#if	__STDC__
static int findMatch(int s, char *re, int *idx)
#else
static int findMatch(s, re, idx)
	int s, *idx;
	char *re;
#endif
{
	returnVal=1;
#ifdef DEBUG
	if (testOption(VAR_DEBUGGING))
	{
		fprintf(debug,"In findMatch(%d,%s,)\n",s,re); fflush(debug);
	}
#endif
	if (re==NULL || prepareRE(re)==0)
	{
		int n, m;
		m = n = numfiles[l];
		do
		{
			s = (s+1) % m;
			if ((*matchRegexp)(INFO(l,s).name)==1)
			{
				if (idx) *idx=fIndex[l][s];
				return s;
			}
		}
		while (n--);
	}
	returnVal = 0;
	return -1;
}

#if	__STDC__
static short lookupName(char *name)
#else
static short lookupName(name)
	char *name;
#endif
{
	int i, n;
	short L = callStkLvl-1;
	if (name==NULL || name[0]=='\0') return -1;
	/* first check locvars and params */
	n = callStack[L].nv;
	for (i=0;i<n;i++)
		if (strcmp(name,LITERAL(callStack[L].vnames[i]))==0)
			return TO_LOCAL(i);
	n = callStack[L].np;
	for (i=0;i<n;i++)
		if (strcmp(name,LITERAL(callStack[L].pnames[i]))==0)
			return TO_PARAM(i); /* param 0 predefed */
	/* then check the symbol table for global vars */
	for (i=0;i<idents;i++)
		if ((objects[i].type & VAR_TYPE) &&
			strcmp(name,ID_NAME(i))==0)
				return i;
	return -1;
}

#if __STDC__
static void buildList(char *buf, int L, int doAll)
#else
static void buildList(buf, L, doAll)
	char *buf;
	int L, doAll;
#endif
{
	int i, len = 1;
	for (i=0;i<numfiles[L];i++)
	{
		if (doAll || (INFO(L,i).flag & F_SELECTED))
		{
			char *name = INFO(L,i).name;
			len += strlen(name)+1;
			if (len>=BUFFER_SIZE)
			{
				showMsg("Buffer overflow!");
				break;
			}
			if (buf[0]) strcat(buf," ");
			strcat(buf,name);
		}
	}
}

#if __STDC__
char *lookupVar(short idx, int buf)
#else
char *lookupVar(idx, buf)
	short idx;
	int buf;
#endif
{
	struct tm *tp; time_t tv;
	char *cmd, *rtn = "";
	char **space;
#ifdef DEBUG
	int dbg = testOption(VAR_DEBUGGING);
	if (dbg)
	{
		fprintf(debug,"in lookupVar(%d, ",idx);
		printArg(debug,idx);
		fprintf(debug,")\n");
	}
#endif
	BUF[buf][0] = '\0';
	idx = dereference(idx,&space);
#ifdef DEBUG
	if (dbg)
	{
		fprintf(debug,"in lookupVar, after deref (%d, ",idx);
		printArg(debug,idx);
		fprintf(debug,")\n");
	}
#endif
	if (IS_LITERAL(idx))
	{
		char *tmp = STRING(idx);
#ifdef DEBUG
		if (dbg) fprintf(debug,"\tlookupVar: returning %s\n",tmp);
#endif
		return tmp;
	}
#ifdef DEBUG
	else if (dbg)
	{
		if (IS_GLOBAL(idx))
			fprintf(debug,"\tlookupVar: space[%d] is %s\n",idx,
				space[VARNUM(idx)]);
		else if (IS_LOCAL(idx)) 
			fprintf(debug,"\tlookupVar: space[%d] is %s\n",idx,
				space[FROM_LOCAL(idx)]);
	}
#endif
	rtn = BUF[buf];
	switch(idx)
	{
	case VAR_PFILE:
		rtn = INFO_NOW(l).name;
		break;
	case VAR_SFILE:
		rtn = INFO_NOW(1-l).name;
		break;
	case VAR_PFILES:
		buildList(rtn,l,1);
		break;
	case VAR_PSEL:
		buildList(rtn,l,0);
		break;
	case VAR_SFILES:
		buildList(rtn,1-l,1);
		break;
	case VAR_SSEL:
		buildList(rtn,1-l,0);
		break;
	case VAR_PPATH:
		rtn = paths[l];
		break;
	case VAR_SPATH:
		rtn = paths[1-l];
		break;
	case VAR_PSELSIZE:
		sprintf(rtn,"%ld",selSize[l]);
		break;
	case VAR_SSELSIZE:
		sprintf(rtn,"%ld",selSize[1-l]);
		break;
	case VAR_PSELCNT:
		sprintf(rtn,"%d",selCnt[l]);
		break;
	case VAR_SSELCNT:
		sprintf(rtn,"%d",selCnt[1-l]);
		break;
	case VAR_PSIZE:
		sprintf(rtn,"%ld",INFO_NOW(l).size);
		break;
	case VAR_SSIZE:
		sprintf(rtn,"%ld",INFO_NOW(1-l).size);
		break;
	case VAR_PMTIME:
		rtn = asctime(localtime(&INFO_NOW(l).modtime));
		break;
	case VAR_SMTIME:
		rtn = asctime(localtime(&INFO_NOW(1-l).modtime));
		break;
#ifndef __MSDOS__
	case VAR_PATIME:
		rtn = asctime(localtime(&INFO_NOW(l).acctime));
		break;
	case VAR_SATIME:
		rtn = asctime(localtime(&INFO_NOW(1-l).acctime));
		break;
	case VAR_POWNER:
		rtn = GetUserID(INFO_NOW(l).uid);
		break;
	case VAR_SOWNER:
		rtn = GetUserID(INFO_NOW(1-l).uid);
		break;
	case VAR_PGROUP:
		rtn = GetGroupID(INFO_NOW(l).gid);
		break;
	case VAR_SGROUP:
		rtn = GetGroupID(INFO_NOW(1-l).gid);
		break;
#endif
	case VAR_PPERMS:
#ifdef __MSDOS__
		rtn[0] = INFO_NOW(l).typID;
		rtn[1] = '\0';
#else
		rtn = getPerms(INFO_NOW(l).mode);
#endif
		break;
	case VAR_SPERMS:
#ifdef __MSDOS__
		rtn[0] = INFO_NOW(1-l).typID;
		rtn[1] = '\0';
#else
		rtn = getPerms(INFO_NOW(1-l).mode);
#endif
		break;
#ifndef __MSDOS__
	case VAR_USERNAME:
		rtn = (char *)getlogin();
		break;
#endif
	case VAR_TEMPNAME:
		rtn = CreateTempName();
		break;
	case VAR_TEMPFILE:
		rtn = tempName;
		break;
	case VAR_DATE:
		tp = localtime((tv=time(NULL),&tv));
		sprintf(rtn,"%2d/%02d/%02d",
			tp->tm_year,tp->tm_mon+1,tp->tm_mday);
		break;
	case VAR_TIME:
		tp = localtime((tv=time(NULL),&tv));
		sprintf(rtn,"%2d:%02d:%02d",
			tp->tm_hour,tp->tm_min,tp->tm_sec);
		break;
	case VAR_RANDOM:
		rtn = randname();
		break;
	case VAR_PREVCMD:
		cmd = getPrevCmd("");
		strncpy(rtn,cmd?cmd:"",BUFFER_SIZE-1);
		break;
	case VAR_NEXTCMD:
		cmd = getNextCmd("");
		strncpy(rtn,cmd?cmd:"",BUFFER_SIZE-1);
		break;
	case VAR_DISKFREE: /* Disk space free */
		getDiskSpace(l, paths[l]);
		sprintf(rtn,"%ld",freeSpace[l]);
		break;
	case VAR_DISKUSED: /* Disk space used */
		getDiskSpace(l, paths[l]);
		sprintf(rtn,"%ld",usedSpace[l]);
		break;
	case VAR_VERSION: rtn = VERSION; break;
	case VAR_PATCHLEVEL: rtn = PATCHLEVEL; break;
	case VAR_CODEPATH: rtn = codepath; break;
	case VAR_VIEWLINE: rtn = viewLine; break;
	case VAR_VIEWFILE:
		rtn = fileViewed; break;
	case VAR_WINDOW:
		switch(winState[l])
		{
		case W_VIEW: 	rtn[0] = 'v'; break;
		case W_TREE:	rtn[0] = 't'; break;
		default:			rtn[0] = 'd'; break;
		}
		rtn[1] = (l==0)? 'l' : 'r';
		rtn[2] = '\0';
		break;
	case VAR_WIDTH:
		sprintf(rtn,"%d",winWidth[l]); break;
	case VAR_HEIGHT:
		sprintf(rtn,"%d",showLines[l]); break;
	case VAR_LEFTMARGIN:
		sprintf(rtn,"%d",leftMargin); break;
	case VAR_TOPMARGIN:
		sprintf(rtn,"%d",topMargin); break;
	case VAR_SHOWVIEW:
		sprintf(rtn,"%d",listVis[l]); break;
	case VAR_SHOWSTATUS:
		sprintf(rtn,"%d",statusVis[l]); break;
	default:
		if (IS_LOCAL(idx)) idx = FROM_LOCAL(idx);
		else if (IS_GLOBAL(idx)) idx = VARNUM(idx);
		rtn = space[idx] ? space[idx] : "";
		break;
	}
	if (rtn != BUF[buf]) strcpy(BUF[buf],rtn);
#ifdef DEBUG
	if (testOption(VAR_DEBUGGING))
		fprintf(debug,"lookupVar returning %s\n",BUF[buf]);
#endif
	return BUF[buf];
}

#if __STDC__
char *lookupArg(short idx, int buf)
#else
char *lookupArg(idx, buf)
	short idx;
	int buf;
#endif
{
	char *tmp;
	if (idx>=0) tmp = lookupVar(idx,buf);
	else tmp = STRING(idx);
	return testOption(VAR_EXPANDING) ? expand(tmp,buf) : strcpy(BUF[buf],tmp);
}

#if	__STDC__
void assign2var(short varnum, char *val)
#else
void assign2var(varnum, val)
	short varnum;
	char *val;
#endif /* __STDC__ */
{
	unsigned nb = strlen(val)+1;
	int isGlob = 0, changed;
	char **space;
#ifdef DEBUG
	int dbg = testOption(VAR_DEBUGGING);
	if (dbg) fprintf(debug,"In assign2var(%d,%s)\n",varnum,val);
#endif
	varnum = dereference(varnum,&space); /* look up positional parameters */
	if (!IS_LVAL(varnum))
	{
		/* shouldn't happen if script is right! */
		char msg[80];
		sprintf(msg,"Script error: bad reference parameter %d at %d\n",varnum,ip);
		showMsg(msg);
		sleep(1);
		returnVal = 0;
		return;
	}
	if (IS_LOCAL(varnum)) varnum = FROM_LOCAL(varnum);
	else if (IS_GLOBAL(varnum))
	{
		isGlob = 1;
		varnum = VARNUM(varnum);
	}
#ifdef DEBUG
	if (dbg) fprintf(debug,"\tassign2var - after deref varnum=%d, space[v]=%s\n",
		varnum,space[varnum]);
#endif
#ifdef __MSDOS__
	if (isGlob && varnum==VAR_USEDOSPATTERNS)
	{
		val = "y";
		nb = 2;
	}
#endif
#if defined(NO_COLOR) || defined(NO_COLOUR)
	if (isGlob && varnum==VAR_ISCOLOR)
	{
		val = "n";
		nb = 2;
	}
#endif
	if (space[varnum])
	{
		changed = strcmp(space[varnum],val);
		if (changed) space[varnum] = realloc(space[varnum],nb);
	}
	else
	{
		changed = 1;
		space[varnum] = calloc(nb,sizeof(char));
	}
	if (!changed) goto end;
	if (space[varnum]) strcpy(space[varnum],val);
	else showMsg("Failed to allocate space for variable!");

	/* The rest of the code in this routine makes changes that are
		required when certain global variables are modified
	*/

	if (isGlob)
	{
		int isNZ = ( val[0]>='1' && val[0]<='9'),
			isY = (val[0]=='y' || val[0]=='Y'), enbl, change = 0, ws = 0;
		enbl = isNZ || isY;
		if (varnum == VAR_USEDOSPATTERNS)
		{
			change = (testOption(VAR_USEDOSPATTERNS)) ? (!enbl) : enbl ;
			if (change)
			{
#ifdef __MSDOS__
				compileRegexp = DOScompileRE;
				matchRegexp = DOSmatchRE;
#else
				if (enbl)
				{
					compileRegexp = DOScompileRE;
					matchRegexp = DOSmatchRE;
				}
				else
				{
					compileRegexp = compileRE;
					matchRegexp = matchRE;
				}
#endif
			}
		}
		else if (varnum==VAR_TREE)
		{
			enbl = isNZ;
			ws=W_TREE;
		}
		else if (varnum==VAR_VIEWER)
			ws=W_VIEW;
		else if (varnum == VAR_ISCOLOR)
			allowColor = enbl;
		change = (winState[l]==ws) ? (!enbl) : enbl ;
		if (change && ws)
		{
			if (enbl)
			{
				winState[l] = ws;
				/* Enabling a tree or viewer affects the other window */
				if (ws == W_TREE || (ws==W_VIEW && winState[1-l] != W_LIST))
				{
					winState[1-l] = W_LIST;
					winInvalid[1-l] = 2;
				}
			}
			else
			{
				winState[l] = W_LIST;
				if (ws==W_VIEW)
					fileViewed[0] = '\0';
			}
			winInvalid[l] = 2;
		}

		/* Handle changes to window dimensions and position */

		changedWinSize = 1;
		switch (varnum)
		{
			case VAR_HEIGHT:		showLines[l] = atoi(val);	break;
			case VAR_WIDTH:		winWidth[l] = atoi(val);	break;
			case VAR_LEFTMARGIN:	leftMargin = atoi(val);		break;
			case VAR_TOPMARGIN:	topMargin = atoi(val);		break;
			case VAR_SHOWVIEW:	listVis[l] = enbl;			break;
			case VAR_SHOWSTATUS:	statusVis[l] = enbl;			break;
			default:					changedWinSize = 0;			break;
		}
	}
end:;
}

#if	__STDC__
void FreeVars(void)
#else
void FreeVars()
#endif
{
	int i;
	for (i=0;i<MAX_VARS;i++)
	{
		if (valspace[i])
		{
			free(valspace[i]);
			valspace[i] = NULL;
		}
	}
}

#ifdef __MSDOS__
#include <process.h>
#ifdef HAS_SPAWN0
#include "spawno.h"
#endif
#endif

#ifdef __MSDOS__
	/* Under DOS we use spawnvp, as we can then do EMS/XMS with a
		decent spawnvp replacement, and we get error returns. This
		causes too many difficulties under UNIX, so I have reverted
		back to using system() there. */

int runCommand(char *cmd, int catch)
{
	int newf = -1, fail = 0, oldstderr=0, oldstdout=0;
	int i, len = strlen(cmd), buf;
	int args = 0, argbuf, shellbuf, batbuf = 0, maxarg;
	char **argv;
	int DOSerr;
	if (len>256)
	{
		showMsg("Command too long!");
		return (returnVal = 0);
	}
	buf = GRABBUF();
	shellbuf = GRABBUF();
	argbuf = GRABBUF();
	argv = (char **)BUF[argbuf];
	maxarg = BUFFER_SIZE / sizeof(char *);

	/* If the command contains pipes or < or >, then write it to
			a scratch .BAT file and make this the command */
	i = 0;
	while (cmd[i])
	{
		if (cmd[i] == '>' || cmd[i]=='<' || cmd[i] == '|')
		{
			/* Yes, we must make a batch file */
			FILE *batfp;
			batbuf = GRABBUF();
			strcpy(BUF[batbuf], makePath(valspace[VAR_TEMPPATH], "gc3.bat"));
			if ((batfp = fopen(BUF[batbuf],"w")) != NULL)
			{
				fputs(cmd,batfp);
				fclose(batfp);
			}
			argv[0] = valspace[VAR_SHELL];
			argv[1] = "/c";
			argv[2] = BUF[batbuf];
			argv[3] = NULL;
			break;
		}
		i++;
	}
	if (batbuf == 0)
	{
		/* split the command up into an argv[] array of args */
		strcpy(BUF[buf], cmd);
		for (i = 0;;)
		{
			while (i<len && (BUF[buf][i]==' ' || BUF[buf][i]=='\t')) i++;
			if (i>=len) break;
			argv[args++] = BUF[buf] + i;
			if (args>=(maxarg-1)) break;
			while (i<len && (BUF[buf][i]!=' ' && BUF[buf][i]!='\t')) i++;
			if (i<len) BUF[buf][i++] = '\0';
			if (args==1 && findCommandType(argv[0])<3)
			{
				/* Not a .COM or .EXE - slap a shell on the front */
				argv[2] = argv[0];
				argv[0] = valspace[VAR_SHELL];
				argv[1] = "/c";
				args = 3;
			}
		}
		argv[args] = NULL;
	}

	/* Prepare the temp file for catching output */

	if (catch && catchFP)
	{
		CloseFile(catchFP);
		unlink(tempName);
		catchFP = NULL;
	}
	clearCmdWin(); /* Force cursor to bottom of screen */
	if (!catch)
	{
		/* DOS wants a screen clear; else you don't see any output! */
		clrscr();
		fprintf(stderr,"%s\n",cmd); /* show the command */
	}
	else
	{
		/* Redirect stderr to stdout, and stdout to the temp file. */
		oldstdout = DUP(1);
		oldstderr = DUP(2);
		CLOSE(1); CLOSE(2);
		newf = OPEN(tempName,O_CREAT,S_IWRITE|S_IREAD);
		if (newf>0)
			(void)DUP(newf); /* dup stdout as stderr */
		else
		{
			/* undo */
			(void)DUP(oldstdout);	(void)DUP(oldstderr);
			CLOSE(oldstdout);			CLOSE(oldstderr);
		}
		showMsg(cmd);
	}
	if (! (returnVal = (spawnvp(P_WAIT,argv[0],argv)==0) ) )
	{
		fail = errno;
		DOSerr = _doserrno;
	}
	if (!catch) (void)saveScreen();
	FREEBUF(shellbuf);
	FREEBUF(argbuf);
	if (batbuf)
	{
		unlink(BUF[batbuf]);
		FREEBUF(batbuf);
	}
	if (catch && newf > 0)
	{
		/* restore the files */
		CLOSE(1); CLOSE(2);
		(void)DUP(oldstdout);	(void)DUP(oldstderr);
		CLOSE(oldstdout);	CLOSE(oldstderr);
	}
	if (testOption(VAR_WAIT))
	{
		fprintf(stderr, "Press ENTER to return to Gram's Commander...");
		fflush(stderr);
		while (getchar() != '\n');
	}
	enableSpecialKeys();
	if (fail)
	{
		int len;
		sleep(1);
		strcpy(BUF[buf],strerror(fail));
		len = strlen(BUF[buf]);
		sprintf(BUF[buf]+len-1," (DOS err: %s!",strerror(DOSerr));
		BUF[buf][strlen(BUF[buf])-2] = ')'; /* Clear out \n from strerror */
		showMsg(BUF[buf]);
		sleep(1);
	}
	FREEBUF(buf);
	if (catch)
	{
		catchFP = OpenFile(tempName,"r");
		i_nextline();
	}
	else invalidateWindows();
	return returnVal = (fail==0);
}

#else

/* Old version for UNIX */

#if __STDC__
int runCommand(char *cmd, int catch)
#else
int runCommand(cmd, catch)
	char *cmd;
	int catch;
#endif
{
	int buf = GRABBUF();
	char *msg;
	int fail = 0;
	int len = (7*catch)+strlen(tempName)+1;
	BUF[buf][0]='\0';
	len += strlen(cmd);
	if (len>BUFFER_SIZE)
	{
		showMsg("Buffer overflow!");
		FREEBUF(buf);
		return returnVal = 0;
	}
	strcat(msg = BUF[buf], cmd);
	if (catch)
	{
		strcat(msg," >");
		strcat(msg,tempName);
	 	strcat(msg," 2>&1");
		if (catchFP)
		{
			CloseFile(catchFP);
			unlink(tempName);
			catchFP = NULL;
		}
	}
	else
	{
		int buf2 = GRABBUF();
		strcpy(BUF[buf2],cmd);
		STRUPR(BUF[buf2]);
		if (strcmp(BUF[buf2],"$SHELL")==0)
			msg = "Type `exit' to return to GC";
		FREEBUF(buf2);
	}
	SHELL_MODE(catch ? NULL : msg);
/*	if (!catch) fprintf(stderr,"\n%s\n",msg);*/
	if (system(msg)) fail = errno;
	if (testOption(VAR_WAIT))
	{
		fprintf(stderr, "Press ENTER to return to Gram's Commander...");
		fflush(stderr);
		while (getchar() != '\n');
	}
	PROG_MODE();
	FREEBUF(buf);
	if (catch)
	{
		catchFP = OpenFile(tempName,"r");
		i_nextline();
	}
	else invalidateWindows();
#ifndef NO_STRERROR
	if (fail)
		showMsg(strerror(fail));
#endif
	return returnVal = (fail == 0);
}

#endif

/**********************************************************************/

DF(i_access)	{ returnVal = !access(regPtr[1], atoi(regPtr[2]));	}

DF(i_action)
{
	returnVal = !sWinAction(PARAM(1),PARAM(2));
}

DF(i_append)
{
	FILE *fp = OpenFile(regPtr[1],"a");
	if (fp) {
		fputs(regPtr[2],fp);
		CloseFile(fp);
		returnVal = 1;
	} else returnVal = 0;
}

DF(i_assign)	{ assign2var(PARAM(1), regPtr[2]);		}
DF(i_beep)	{ BEEP; 					}

DF(i_bind)
{
	short k = PARAM(1);
	KeyTbl[k].ip = PARAM(2);
	KeyTbl[k].hlp = STRING(PARAM(3));
}

DF(i_box)
{
	sWinBox();
}

DF(i_break)
{
	if (callStack[callStkLvl-1].loopP)
		callStack[callStkLvl-1].loopP--;
	ip = PARAM(1);
}

DF(i_browse)
{
	int buf1 = GRABBUF(), buf2 = GRABBUF();
	expand(regPtr[1],buf1);
	if (buf2) expand(regPtr[2],buf2);
	HyperBrowse(BUF[buf1], buf2 ? BUF[buf2] : "");
	FREEBUF(buf1);
	FREEBUF(buf2);
}

#if __STDC__
void callFunction(short p)
#else
void callFunction(p)
	short p;
#endif
{
	short n, i;
	if (p<0)
	{
		/* run-time link */
		int buf = GRABBUF();
		STRUPR(lookupArg(p,buf));
		for (i=0;i<idents;i++)
		{
			if (objects[i].type&FUNC_TYPE)
			{
				if (strcmp(BUF[buf],ID_NAME(i))==0)
				{
					p = objects[i].p;
					break;
				}
			}
		}
		if (p<0)
		{
			int buf2 = GRABBUF();
			sprintf(BUF[buf2],"%s is undefined",BUF[buf]);
			showMsg(BUF[buf2]);
			FREEBUF(buf2);
			FREEBUF(buf);
			returnVal = 0;
			return;
		}
		FREEBUF(buf);
	}
	n = PARAM(2);
	for (i=0;i<n;i++)
	{
		short pr = PARAM(3+i);
		if (pr>(MAX_IDS+MAX_PARAMS)) pr+=MAX_LOCVARS; /* local */
		callStack[callStkLvl].param[i] = pr;
	}
	callStack[callStkLvl].ip = ip+n;
	/* we increment the stack level only after doing the OP_FUNCTION */
	ip = p;
}

DF(i_call)
{
	callFunction(PARAM(1));
}

DF(i_cat)
{
	short na = PARAM(2), i;
	for (i=1;i<na;i++) strcat(regPtr[3], regPtr[3+i]);
	assign2var(PARAM(1), regPtr[3]);
}

DF(i_cd)
{
	doCD(regPtr[1]);
}

DF(i_clear)
{
	clrscr();
#ifdef __MSDOS__
	invalidateWindows();
#endif
}

DF(i_compare)
{
	int key = PARAM(1);
	char *f1 = regPtr[2];
	char *f2 = regPtr[3];
	char *p1, *p2;
	/* If either file does not exist, compare fails */
     	if (access(f1,0)!=0) returnVal = -1;
	else if (access(f2,0)!=0) returnVal = 1;
	else {
		int neq = 0;
		if (key>=100) { neq=1; key-=100; }
		switch(key)
		{
		case 1: /* Compare names, first stripping off path */
			p1 = STRRCHR(f1,PATH_SEP);
			p2 = STRRCHR(f2,PATH_SEP);
			if (p1 == NULL) p1 = f1;
			if (p2 == NULL) p2 = f2;
			returnVal = strcmp(p1,p2);
			break;
#ifndef __MSDOS__
		case 2:	/* Compare groups */
			returnVal = getFileGroup(f1)-getFileGroup(f2);
			break;
		case 3: /* Compare owners */
			returnVal = getFileUser(f1)-getFileUser(f2);
			break;
		case 4: /* Compare access time */
			returnVal = getFileAccTime(f1)-getFileAccTime(f2);
			break;
#endif
		case 5: /* Compare mod time */
			returnVal = getFileModTime(f1)-getFileModTime(f2);
			break;
		case 6: /* Compare contents */
			{
				char c1, c2;
				FILE *fp1 = OpenFile(f1,"r");
				FILE *fp2 = OpenFile(f2,"r");
				if (fp1 && fp2)
			 	{
			  		returnVal = 0;
				  	while (!feof(fp1) && !feof(fp2))
					{
				  		c1 = fgetc(fp1);
				  		c2 = fgetc(fp2);
				  		if (c1!=c2)
						{
				  			returnVal = 1;
				  			break;
				  		}
				  	}
				  	if (!returnVal && (!feof(fp1) || !feof(fp2)))
				  		returnVal = 1;
			  	}
				else returnVal = 1;
			  	if (fp1) CloseFile(fp1);
			  	if (fp2) CloseFile(fp2);
			}
			break;
		case 7: /* Compare size */
			returnVal = getFileSize(f1)-getFileSize(f2);
			break;
		}
		if (neq) returnVal = (returnVal > 0);
	}
}

/* crude text search function */

#if __STDC__
int search(char *f, char *s)
#else
int search(f, s)
	char *f, *s;
#endif
{
	int j, len, L, buf, rtn = 0;
	char c = s[0];
	FILE *fp = OpenFile(f,"r");
	L = strlen(s);
	if (fp==NULL) return 0; /* fail */
	buf = GRABBUF();
	while (!feof(fp))
	{
		fgets(BUF[buf],128,fp);
		len = strlen(BUF[buf])-L;
		if (len>=0)
		{
			for (j=0;j<len;j++)
			{
				if (BUF[buf][j]==c)
				{
					if ((int)strncmp(BUF[buf]+j,s,L)==0)
					{
						rtn = 1;
						goto done;
					}
				}
			}
		}
	}
done:
	FREEBUF(buf);
	CloseFile(fp);
	return rtn;
}

DF(i_contains)
{
	/* See if file `s' contains string `s' */
	returnVal = search(regPtr[1], regPtr[2]);
}

/* Must move to gcscreen.c */

#if __STDC__
void showVariables(void)
#else
void showVariables()
#endif
{
	int i, r;
	char line[MAX_COLS];
	clrscr();
	for (i=r=0;i<idents;i++)
	{
		if ((objects[i].type&FUNC_TYPE)==0)
		{
			int len, buf = GRABBUF();
			(void)lookupVar(i,buf);
			/* Do all sorts of things to make sure we fit... */
			len = strlen(BUF[buf]);
			if (len>(COLS-25))
			{
				strcpy(BUF[buf]+(COLS-25),"...");
				len = COLS-22;
			}
			/* Turn ctrl chars to spaces */
			while (len>0)
				if (BUF[buf][--len] <32 )
					BUF[buf][len] = ' ';
			sprintf(line,"%-16s %s", ID_NAME(i),BUF[buf]);
			ScreenMsg(line);
			FREEBUF(buf);
			r++;
			if (r==20)
			{
				if (promptForKey()==27) goto done;
				r = 0;
				clrscr();
			}
		}
	}
	if (r) (void)promptForKey();
done:
	invalidateWindows();
	clearCmdWin();
}

DF(i_down)	{ cursorAction(1);				}
DF(i_dump)	{ showVariables();				}

DF(i_echo)
{
	int buf = GRABBUF();
	expand(regPtr[1],buf);
	showMsg(BUF[buf]);
	FREEBUF(buf);
}

DF(i_edittext)
{
	returnVal = !sWinEditText(PARAM(1),PARAM(2));
}

DF(i_end)	{ cursorAction(7);				}

DF(i_endloop)
{
	int L = callStkLvl-1;
	int p = callStack[L].loopP-1;
	if (callStack[L].loopP)
	{
		if (callStack[L].loopStack[p].v == 0) /* infinite loop */
		{
			ip = callStack[L].loopStack[p].s;
		}
		else if (callStack[L].loopStack[p].pos < numfiles[l])
		{
			assign2var(callStack[L].loopStack[p].v,
				INFO(l,callStack[L].loopStack[p].pos++).name);
			ip = callStack[L].loopStack[p].s;
		}
		else callStack[L].loopP--;
	}
}

DF(i_endrec)	{ StopRecordingMacro();		}

DF(i_eval) /* evaluate expression in string */
{
	int rtn;
	char *msg = compute(regPtr[2],&rtn);
	if (msg)
	{
		showMsg(msg);
		returnVal = 0;
	}
	else returnVal = PARAM(1) ? (rtn>0) : rtn;
}

DF(i_exec)	{ runCommand(regPtr[1],0);			}
DF(i_execcatch)	{ runCommand(regPtr[1],1);			}

DF(i_exittext)
{
	returnVal = !sWinExitText(PARAM(1));
}

DF(i_expand)
{
	int buf = GRABBUF();
	assign2var(PARAM(1), expand(regPtr[1],buf));
	FREEBUF(buf);
}

DF(i_fcopy)
{
#ifndef __MSDOS__
		if (catchFP)
		{
			CloseFile(catchFP);
			unlink(tempName);
			catchFP = NULL;
		}
#endif
	returnVal = doFileCopy(regPtr[1], regPtr[2], (int)PARAM(3));
#ifndef __MSDOS__
	if (returnVal != 9999)
		showMsg(valspace[VAR_LINEBUFF]);
	else
		showMsg("Buffer overflow!");
#endif
	returnVal = (returnVal == 0);
}

#if __STDC__
void clearFilter(int L)
#else
void clearFilter(L)
	int L;
#endif
{
#ifndef __MSDOS__
	if (!testOption(VAR_USEDOSPATTERNS))
		strcpy(filters[L],".");
	else
#else
		strcpy(filters[L],"*.*");
#endif
	filtering[L] = 0;
}

#if __STDC__
static void setFilter(int L, char *f)
#else
static void setFilter(L, f)
	int L;
	char *f;
#endif
{
	strcpy(filters[L], f);
	filtering[L] = 1;
}

DF(i_filter)
{
	if (regPtr[1][0]=='\0') clearFilter(l);
	else setFilter(l,regPtr[1]);
	refilter(l,INFO_NOW(l).name);
}

#if __STDC__
int stripwhitespace(char *buf)
#else
int stripwhitespace(buf)
	char *buf;
#endif
{
	int b = GRABBUF(), len;
	char *p = buf;
	while (*p && (*p==' ' || *p=='\t' || *p=='\n' || *p=='\r'))
		p++;
	strcpy(BUF[b],p);
	p = BUF[b] + strlen(BUF[b]) - 1;
	while (p>=BUF[b] && (*p==' ' || *p=='\t' || *p=='\n' || *p=='\r'))
		p--;
   p++;
	*p = '\0';
	len = p-BUF[b];
	strcpy(buf,BUF[b]);
	FREEBUF(b);
	return len;
}

DF(i_findarea)
{
	returnVal = 0;
	if (catchFP)
		CloseFile(catchFP);
	catchFP = OpenFile(regPtr[1],"r");
	STRUPR(regPtr[2]);
	returnVal = seekToArea(catchFP, regPtr[2]);
}

DF(i_function)
{
	int i;
	short numP, numV;
	callStack[callStkLvl].name = PARAM(1);
	numP = callStack[callStkLvl].np = PARAM(2);
	numV = callStack[callStkLvl].nv = PARAM(3);

	/* clear the local variables */

	for (i=0; i<MAX_LOCVARS ; i++)
		callStack[callStkLvl].locvar[i] = NULL;

	/* set up the name pointers */

	for (i=0; i<numP; i++)
		callStack[callStkLvl].pnames[i] = PARAM(4+i);
	for (i=0; i<numV; i++)
		callStack[callStkLvl].vnames[i] = PARAM(4+numP+i);

	/* initialise the loop stack */

	callStack[callStkLvl].loopP = 0;

	/* and increment the pointers */

	lookupLevel = ++callStkLvl;
	ip += numP+numV;
}

DF(i_goto)	{ ip = PARAM(1); 				}
DF(i_hborder)	{ hborder = PARAM(1);	}

DF(i_head)
{
	short len, L = (short)strlen(regPtr[3]);
	if (PARAM(1)==0)
	{
		int b = GRABBUF();
		len = (short)atoi(lookupVar(PARAM(2), b));
		FREEBUF(b);
	}
	else len = PARAM(2);
	if (len>=0) regPtr[3][min(len,L)]='\0';
        else regPtr[3][L-min(-len,L)] = '\0';
	assign2var(PARAM(3), regPtr[3]);
}

DF(i_help)
{
	showMsg(KeyTbl[PARAM(1)].hlp);
}

DF(i_helpkey)
{
	int i = GetKeyBinding(my_getch());
	if (i>=0 && KeyTbl[i].ip && KeyTbl[i].hlp) showMsg(KeyTbl[i].hlp);
	else showMsg("No help for this key");
}

static char *CmdEdName[] =
{
	"Show hypertext help",
	"Insert file name at cursor position",
	"Move the cursor to the start of the line",
	"Move the cursor to the end of the line",
	"Move the cursor left one position",
	"Move the cursor right one position",
	"Delete the character under the cursor",
	"Delete the character left of the cursor",
	"Toggle insert/overstrike mode",
	"Clear the whole line",
	"Erase from the start of the line to the cursor",
	"Erase from the cursor to the end of the line",
	"Delete from start of previous word to the cursor",
	"Delete from the cursor to the start of next word",
	"Move cursor to the start of the previous word",
	"Move cursor to start of the next word",
	"Recall the previous command in history starting with same text",
	"Recall the next command in history starting with same text"
};

#if __STDC__
static int showHelp(int idx, int *row)
#else
static int showHelp(idx, row)
	int idx;
	int *row;
#endif
{
	char line[80];
	if (*row==0)
	{
		if (idx>=(MAX_KEYS-NUM_CED_KEYS))
			ScreenMsg("COMMAND LINE EDITOR");
		ScreenMsg("Key     Action");
		ScreenMsg("===     ============================================================");
		*row = 1;
	}
	if (idx>=(MAX_KEYS-NUM_CED_KEYS))
	{
		(*row)++;
		sprintf(line,"%-8.8s %-70.70s", KeyName(idx),
			CmdEdName[idx-(MAX_KEYS-NUM_CED_KEYS)]);
		ScreenMsg(line);
	}
	else if (KeyTbl[idx].ip && KeyTbl[idx].hlp!=NULL)
	{
		(*row)++;
		if (idx>=221)
			sprintf(line,"Esc-%4.4s %-70.70s",
				KeyName(idx),KeyTbl[idx].hlp);
		else
			sprintf(line,"%-8.8s %-70.70s",
				KeyName(idx),KeyTbl[idx].hlp);
		ScreenMsg(line);
	}
	if ((*row)>(Lines-5))
	{
		int rtn;
		*row = 0;
		rtn = promptForKey();
	    	clrscr();
		return rtn;
	}
	else return 0;
}

#if __STDC__
static void showAllHelp(void)
#else
static void showAllHelp()
#endif
{
	int i=0, r=0, k = 0;
	clrscr();
	while (i<(MAX_KEYS-NUM_CED_KEYS))
	{ 
		if (showHelp(i,&r)==27) return;
		i++;
	}
	if (r>1)
		k = promptForKey();
	clrscr();
	if (k!=27)
	{
		r = 0;
		while (i<MAX_KEYS)
		{ 
			if (showHelp(i,&r)==27) return;
			i++;
		}
		if (r>1) promptForKey();
		clrscr();
	}
}

DF(i_helpall)	{ showAllHelp(); invalidateWindows();	}
DF(i_home)		{ cursorAction(6); 					}

DF(i_iassign)
{
	int b = GRABBUF(), o;
	if (PARAM(2)<0) /* RHS index */
	{
		o = atoi(lookupVar(-PARAM(2), b));
		assign2var(PARAM(1), lookupVar(PARAM(3)+o, b));
	}
	else
	{
		o = atoi(lookupVar(PARAM(2), b));
		assign2var(PARAM(1)+o, lookupVar(PARAM(3),b));
	}
	FREEBUF(b);
}

DF(i_if)			{ if (!returnVal) ip = PARAM(1); }

DF(i_inpath)
{
	char *n = searchPath(regPtr[1]);
	if (n)
	{
 		if (PARAM(2)) assign2var(PARAM(2),n);
		returnVal = 1;
	} else 	returnVal = 0;
}

DF(i_invalidate)	{ invalidateWindows(); 	}

DF(i_keycode)
{
    SetKeyCode(PARAM(1), (PARAM(2)<<16) + PARAM(3));
}

DF(i_length)	{ returnVal = (int)strlen(regPtr[1]);		}

DF(i_locate)
{
	sWinLocate((short)atoi(regPtr[1]), (short)atoi(regPtr[2]));
}

DF(i_lookup)
{
	char *v = getenv(regPtr[2]);
	assign2var(PARAM(1),v?v:"");
}

DF(i_loaditem)
{
	int d = GRABBUF();
	STRUPR(regPtr[1]);
	returnVal = loadItem(catchFP,regPtr[1],BUF[d]);
	assign2var(PARAM(2),BUF[d]);
	FREEBUF(d);
}

DF(i_loadrules)
{
	readRules(catchFP);
}

DF(i_loopall)
{
	short L = callStkLvl-1;
	short lp = callStack[L].loopP++;
	callStack[L].loopStack[lp].v=PARAM(1);
	callStack[L].loopStack[lp].s=PARAM(2);
	callStack[L].loopStack[lp].e=PARAM(3);
	callStack[L].loopStack[lp].idx=PARAM(4);
	if (PARAM(1))
		assign2var(PARAM(1),INFO(l,0).name);
	callStack[L].loopStack[lp].pos=1;
}

DF(i_makepath)
{
	int b = GRABBUF(), b2 = GRABBUF();
	/* as fixPath uses makePath's static return area, we must copy out
		before calling... */
	strcpy(BUF[b2], makePath(regPtr[2], regPtr[3]));
	fixPath(BUF[b2], BUF[b]);
	assign2var(PARAM(1), BUF[b]);
	FREEBUF(b);
	FREEBUF(b2);
}

#if __STDC__
static void checkMark(char *fname)
#else
static void checkMark(fname)
	char *fname;
#endif
{
	int i;
	returnVal = (findFile(l,0,fname,&i)>=0) && (fInfo[l][i].flag & F_SELECTED);
}

DF(i_marked)	{ checkMark(regPtr[1]);				}

#if __STDC__
static void setMark(char *mask, int is_pat, int v)
#else
static void setMark(mask, is_pat, v)
	int v, is_pat;
	char *mask;
#endif
{
	int i=-1, ii, idx;
	returnVal = 0;
	if (is_pat)
	{
		while (ii=i, (i = findMatch(i,mask,&idx))>ii)
		{
			nselect(l,idx,v);
			returnVal = 1;
		}
	}
	else if (findFile(l,0,mask,&idx)>=0)
	{
		nselect(l,idx,v);
		returnVal = 1;
	}
}

DF(i_patmark)	{ setMark(regPtr[1],1,1);			}
DF(i_strmark)	{ setMark(regPtr[1],0,1);			}
DF(i_patunmark)	{ setMark(regPtr[1],1,0);			}
DF(i_strunmark)	{ setMark(regPtr[1],0,0);			}

#if __STDC__
static void chop(char *str, short cnt)
#else
static void chop(str, cnt)
	char *str;
	short cnt;
#endif
{
	if (cnt<0)
	{
		/* Copy -cnt chars from end of string to beginning */
		int p, L = strlen(str);
		cnt = -cnt;
		if (cnt>L) cnt=L;
		L -= cnt;
		for (p=0;p<cnt;p++) str[p] = str[p+L];
		str[p]=0;
	}
	else if (cnt>0) str[cnt]=0; /* truncate at cnt'th char */
}

#if __STDC__
int doesMatch(char *pat, char *str, int len)
#else
int doesMatch(pat, str, len)
	char *pat, *str;
	int len;
#endif
{
	if (prepareRE(pat)==0)
	{
		chop(str,len);
		if ((*matchRegexp)(str)==1)
				return 1;
	}
	return 0;
}

DF(i_patmatch)
{
	returnVal=doesMatch(regPtr[2], regPtr[1], (int)PARAM(3));
}

DF(i_strmatch) {
	chop(regPtr[1], PARAM(3));
#ifdef __MSDOS__
	STRUPR(regPtr[1]);
	STRUPR(regPtr[2]);
#endif
	returnVal = !strcmp(regPtr[1], regPtr[2]);
}

DF(i_metakey)	{ metaKey = (PARAM(1)<<16) + PARAM(2); }

DF(i_mode)
{
#ifdef __MSDOS__
	char *s = regPtr[1];
	if (!isdigit(s[0]))
		returnVal = 0;
	else
		returnVal = setVideoMode(atoi(s));
#endif
}

#if __STDC__
int readNextLine(FILE *fp)
#else
int readNextLine(fp)
	FILE *fp;
#endif
{
	int rtn = 0;
	assign2var(VAR_LINEBUFF,"");
	if (fp)
	{
		int buf = GRABBUF();
		if (fgets(BUF[buf],BUFFER_SIZE,fp))
		{
			if (BUF[buf][strlen(BUF[buf])-1]!='\n')
			{
				while (!feof(fp))
				{
					if (fgetc(fp)=='\n') break;
				}
			}
			assign2var(VAR_LINEBUFF, BUF[buf]);
			rtn = 1;
		}
		FREEBUF(buf);
	}
	return rtn;
}

DF(i_nextline)
{
	returnVal = readNextLine(catchFP);
}

DF(i_not)	{ returnVal = !returnVal; }

DF(i_open)
{
	if (catchFP)
	{
		CloseFile(catchFP);
		unlink(tempName);
	}
	if ((catchFP = OpenFile(regPtr[1],"r"))!=NULL)
	{
		returnVal = 1;
		i_nextline();
	}
	else returnVal = 0;
}

DF(i_paint)	{ repaintScreen(); }
DF(i_pgdn)	{ cursorAction(5); }
DF(i_pgup)	{ cursorAction(4); }

DF(i_playmac)
{
	playMacro(atoi(regPtr[1]));
}

DF(i_radio)
{
	returnVal = !sWinRadio(PARAM(1),PARAM(2));
}

DF(i_read)
{
	returnVal = getInput(regPtr[1],PARAM(2),1);
	assign2var(PARAM(1),regPtr[1]);
	clearCmdWin();
}

DF(i_recmac)
{
	StartRecordingMacro(atoi(regPtr[1]));
}

DF(i_rescan)
{
	int arg = atoi(regPtr[1]);
	if (arg&1)
		winInvalid[l] = winInvalid[l] ? 2 : 1;
	if (arg&2)
		winInvalid[1-l] = winInvalid[1-l] ? 2 : 1;
}

DF(i_restore)
{
#ifdef __MSDOS__
	returnVal = restoreScreen();
	invalidateWindows();
#else
	returnVal = 0;
#endif
}

#if __STDC__
static void doReturn(void)
#else
static void doReturn()
#endif
{
	int i;
	ip = callStack[lookupLevel = --callStkLvl].ip;
	/* free up any local vars */
	for (i=0;i<MAX_LOCVARS;i++)
	{
		if (callStack[callStkLvl].locvar[i])
		{
			free(callStack[callStkLvl].locvar[i]);
			callStack[callStkLvl].locvar[i] = NULL;
		}
	}
}

DF(i_return)
{
	if (PARAM(1)>=0)
		returnVal = PARAM(1);
	doReturn();
}

DF(i_retvar)
{
	char *b = regPtr[1];
	if (isdigit(b[0]))
		returnVal = atoi(b);
	else returnVal = (b[0]=='Y' || b[0]=='y');
	doReturn();
}

#if __STDC__
static void move2match(int i)
#else
static void move2match(i)
	int i;
#endif
{
	if (i>=0)
	{
		highlight[l] = i;
		normalise(l);
		returnVal = 1;
	}
	else returnVal = 0;
}

DF(i_search)
{
	move2match(findFile(l,0, regPtr[1],NULL));
}

DF(i_patsearch)
{
	move2match(findMatch(highlight[l]-1, regPtr[1], NULL));
}

DF(i_procwin)
{
	short rtn = PARAM(1);
	assign2var(rtn, sWinProcess());
}

DF(i_selattrib)
{
	sWinSelAttrib(atoi(regPtr[1]));
}

DF(i_setattrib)
{
	if (PARAM(2))
		setAttrEntry(atoi(regPtr[1]),atoi(regPtr[2]),atoi(regPtr[3]));
	else
		setAttrEntry(atoi(regPtr[1]),-1,atoi(regPtr[3]));
}

DF(i_setexpr)
{
	int buf = GRABBUF();
	sprintf(BUF[buf],"%d",returnVal);
	assign2var(PARAM(1),BUF[buf]);
	FREEBUF(buf);
}

DF(i_setmatch)
{
	int s=-1, i, buf = GRABBUF(), L = 1;
	BUF[buf][0] = '\0';
	if (prepareRE(regPtr[1])==0)
	{
		for (;;)
		{
			i = findMatch(s,NULL,NULL);
			if (i>s)
			{
				char *n = INFO(l,i).name;
				s = i;
				L += strlen(n)+1;
				if (L>=BUFFER_SIZE) {
					showMsg("Buffer overflow!");
					break;
				}
				if (BUF[buf][0]) strcat(BUF[buf]," ");
				strcat(BUF[buf],n);
			}
			else break;
		}
	}
	assign2var(PARAM(1),BUF[buf]);
	returnVal = (int)BUF[buf][0];
	FREEBUF(buf);
}

DF(i_show)
{
	/* Specifies what must be displayed */
	switch (info2show = PARAM(1))
	{
	case 0: /* None */
		fnameLen = 31; break;
	case 1: /* size */
	case 2: /* owner */
	case 3: /* group */
		fnameLen = 22; break; /* size/owner/group are 8 plus 1 space */
	case 4: /* perms */
#ifdef __MSDOS__
		fnameLen = 24; break;
#else
		fnameLen = 20; break;
#endif
	case 5: /* mtime */
	case 6: /* atime */
		fnameLen = 13; break;
	}
}

DF(i_sleep)
{
	sleep((unsigned)PARAM(1));
}

DF(i_sort)
{
	short key, dir;
	if (PARAM(1))
	{
		key = PARAM(2);
		dir = (regPtr[1][0]=='1') ? 1 : -1;
	}
	else if (PARAM(2)<0)
	{
		key = -PARAM(2);
		dir = -1;
	}
	else
	{
		key = -PARAM(2);
		dir = 1;
	}
	resortList(l, dir,  key);
	normalise(l);
}

#if __STDC__
static char *strsrch(char *p, char *cset, int clen)
#else
static char *strsrch(p, cset, clen)
	char *p, *cset;
	int clen;
#endif
{
	int i=0, j=0;
	while (p[i])
	{
		for (j=0;j<clen;j++)
			if (p[i] == cset[j]) return p+i;
		i++;
	}
	return NULL;
}

#if __STDC__
static char *strrsrch(char *p, char *cset, int clen)
#else
static char *strrsrch(p, cset, clen)
	char *p, *cset;
	int clen;
#endif
{
	int L = strlen(p), j;
	while (--L >= 0)
	{
		for (j=0;j<clen;j++)
			if (p[L] == cset[j]) return p+L;
	}
	return NULL;
}

DF(i_split)
{
	short skip = PARAM(1);
	char *t = NULL, *u = regPtr[2];
	int L = strlen(u);

	if (regPtr[3][0]==0)
	{
		returnVal = 0;
		goto end;
 	}
#ifdef DEBUG
	if (testOption(VAR_DEBUGGING))
	{
		fprintf(debug,"in i_split(%d,%s,%s)\n",skip,u,regPtr[3]);
	}
#endif
	if (skip>=0) /* search forward */
	{
		t = regPtr[3]-1;
		while (skip>0)
		{
			if ((t=strsrch(t+1,u,L))==NULL)
			{
				returnVal = 0;
				goto end;
			}
			else skip--;
		}
		*t = 0;
	}
	else /* search back from end */
	{
		char *tp=NULL, c = 0;
		while (skip<0)
		{
			if ((t=strrsrch(regPtr[3],u,L))==NULL)
			{
				returnVal = 0;
				goto end;
			}
			else
			{
				if (tp) *tp = c;
				skip++;
				c = *t;
				*(tp = t--) = '\0';
			}
		}
		t++;
	}
	assign2var(PARAM(3),regPtr[3]);
	if (PARAM(4)!=PARAM(3)) assign2var(PARAM(4),t+1);
	returnVal = 1;
#ifdef DEBUG
	if (testOption(VAR_DEBUGGING))
		fprintf(debug,"After split, returnVal=%d, head=%s, tail=%s\n",
			returnVal,regPtr[3],t+1);
#endif
end:;
}

DF(i_strip)
{
	(void)stripwhitespace(regPtr[1]);
	assign2var(PARAM(1),regPtr[1]);
}

DF(i_swap)
{
	short v = PARAM(1), L;
	if (v==1) L=0; /* left */
	else if (v==2) L=1; /* right */
	else L = 1-l; /* toggle */
	if (listVis[L])
	{
		l = L;
		returnVal = 1;
		if (testOption(VAR_VIEWER))
		{
			if (winState[l] == W_VIEW)
			{
				if (viewingDir)
				{
					assign2var(VAR_VIEWER,"0");
					showMsg("Viewer disabled");
					CHDIR(paths[l]);
				} else
				{
					showMsg("Viewer navigation enabled");
					return; /* No CHDIR in this case */
				}
			}
			else
				showMsg("Directory browser navigation enabled");
		}
		CHDIR(paths[l]);
	} else
	{
		returnVal = 0;
	}
}

DF(i_sync)	{ syncScreen(); 				}

DF(i_tail)
{
	int buf = GRABBUF();
	int L = (int)strlen(regPtr[3]);
	short cnt = PARAM(2);
	if (PARAM(1)==0) 
		cnt = (short)atoi(lookupArg(PARAM(2),buf));
	if (cnt<0)
		strcpy(BUF[buf],regPtr[3]+min(-cnt,L)); /* All but first -cnt chars */
	else strcpy(BUF[buf],regPtr[3]+L-min(cnt,L));
	assign2var(PARAM(3),BUF[buf]);
	FREEBUF(buf);
}

DF(i_test)
{
	char *tmp, *t1, *t2;
	int pos = PARAM(3);
	t2 = expand(regPtr[2],regBuf[2]);
	t1=regPtr[1];
	if (t1[0]==0) returnVal = (t2[0]==0);
	else
	{
		if (pos>0) tmp=(char *)STRCHR(t2, t1[pos-1]);
		else tmp=(char *)STRCHR(t2, t1[strlen(t1)+pos]);
		if (tmp) returnVal = (tmp-t2)+1;
		else returnVal = 0;
	}
}

DF(i_text)
{
	returnVal = !sWinText(PARAM(1));
}

DF(i_toggle)
{
	returnVal = !sWinToggle(PARAM(1),PARAM(2));
}

DF(i_typeof)
{
	returnVal = 0;
	if (access(regPtr[1],0)==0)
	{
		if (PARAM(2) & 128)
		{
			int ex = GRABBUF(), ne = GRABBUF();
			int typ = splitPath(regPtr[1], BUF[ex], BUF[ne]);
			returnVal = (typ>=0);
			FREEBUF(ex);
			FREEBUF(ne);
		}
		if (!returnVal)
			returnVal = checkType(getFileMode(regPtr[1]),PARAM(2));
	}
}

DF(i_unbind) { KeyTbl[PARAM(1)].ip = 0; }

#if __STDC__
static void unlinkFail(char *name, int err)
#else
static void unlinkFail(name, err)
	char *name;
	int err;
#endif
{
	int buf = GRABBUF();
#ifdef NO_STRERROR
	sprintf(BUF[buf],"Failed to unlink %.20s",name);
#else
	sprintf(BUF[buf],"Failed to unlink %.20s: %.30s",name,
				(char *)strerror(err));
#endif
	showMsg(BUF[buf]);
	FREEBUF(buf);
}

#if __STDC__
static void doUnlink(int match, char *re)
#else
static void doUnlink(match, re)
	int match;
	char *re;
#endif
{
	int i=-1, ii;
	returnVal = 1;
	if (!match) /* Just a single file */
	{
		if (unlinkFile(re))
		{
			unlinkFail(re,errno);
			returnVal = 0;
		}
	}
	else if (prepareRE(re)==0)
	{
		for (;;)
		{
			int idx;
			if (ii=i, (i = findMatch(i,NULL,&idx))>ii)
			{
				if (unlinkFile(fInfo[l][idx].name))
					unlinkFail(fInfo[l][idx].name,errno);
			}
			else break;
		}
	}
	else returnVal = 0;
}

DF(i_unlink)
{
	int buf = GRABBUF();
	doUnlink((int)PARAM(1),
		(PARAM(1))?lookupArg(PARAM(2),buf):lookupArg(PARAM(2),buf));
	FREEBUF(buf);
}

DF(i_up)
{
	cursorAction(0);
}

DF(i_vborder)
{
	vborder = PARAM(1);
}

DF(i_view)
{
	strcpy(file2view,regPtr[1]);
  if (winState[0]==W_VIEW)
		winInvalid[0] = 2;
  if (winState[1]==W_VIEW)
		winInvalid[1] = 2;
}

DF(i_window)
{
	returnVal = !sWinMake((short)atoi(regPtr[1]), (short)atoi(regPtr[2]),
					(short)atoi(regPtr[3]), (short)atoi(regPtr[4]));
}

DF(i_write)
{
	writeMsg(regPtr[1]);
}

DF(i_yntoggle)
{
	returnVal = !sWinYNToggle(PARAM(1),PARAM(2));
}

/********************************************************/

#if	__STDC__
int Interpret(short IPtr, int ch, short stackBot)
#else
int Interpret(IPtr, ch, stackBot)
	short IPtr, stackBot;
	int ch;
#endif
{
	short op, numP, p;
	for (p = 1;p<MAX_REGS;p++) regBuf[p] = -1;
	if ((ip = IPtr) == defaultEntryPoint)
	{
		char buf[2];
		buf[0] = (char)ch;
		buf[1] = '\0';
		assign2var(VAR_CH, buf);
	}
	for (;;)
	{
		op = codespace[ip];
#ifdef DEBUG
		if (testOption(VAR_DEBUGGING))
		{
			fprintf(debug,"========================================================\n");
			(void)dumpOp(debug,ip,1);
			fflush(debug);
		}
#endif
		old_ip = ip;
		if (writeIP)
		{
#ifdef __MSDOS__
			FILE *fp = OpenFile("\\gc3.ip","at");
#else
			FILE *fp = OpenFile("/tmp/gc3.ip","a");
#endif
			(void)dumpOp(fp,ip,1);
			CloseFile(fp);
		}
		ip += (numP = ops[op].cnt);
		if (ops[op].handler)
		{
			/* Set up the `registers' */
			for (p = 1;p<numP;p++)
			{
				char code = ops[op].link[p];
				assert(regBuf[p]==-1);
				if (code=='a' || code=='r' || code=='v') /* exclude procwin */
				{
					regBuf[p] = GRABBUF();
					regPtr[p] = lookupArg(PARAM(p),regBuf[p]);
				}
			}
			(*(ops[op].handler))();
			/* Free up the buffers */
			for (p = 1;p<numP;p++)
			{
				if (regBuf[p]>=0)
				{
					FREEBUF(regBuf[p]);
					regBuf[p] = -1;
				}
			}
			if (showIP)
				displayIP(ip);
			if (op==(short)OP_RETURN || op==(short)OP_RETVAR)
			{
				if (callStkLvl<=stackBot) return 0;
			}
		}
		else if (op==(short)OP_QUIT)
		{
			exitCurses(1);
#ifdef DEBUG
			fprintf(debug,"Maximum buffers used: %d\n", maxBufsUsed);
#endif
			break;
		}
	}
	return 1;
}

/* Fake a re-entrant call to interpreter, for use by
	action entries in script-defined windows
*/

#if __STDC__
void subExecute(short IPtr)
#else
void subExecute(IPtr)
	short IPtr;
#endif
{
	short oldIP = ip, oldStk = callStkLvl++;
	if (Interpret(IPtr,0,callStkLvl))
		Shutdown();
	ip = oldIP;
	callStkLvl = oldStk;
}

