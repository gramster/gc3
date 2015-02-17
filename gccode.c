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
#ifdef GC3
#include "gc3.h"
#define PTR(n)	n
#else
#include "gcboth.h"
#define PTR(n)	NULL
#endif
#include "gclib.h"
#include "gcerr.h"
#include "gcops.h"
#include "gccc.h"

/*
 * The codes for the linker and disassembler are:
 *
 *	n	- +ve string index
 *	i	- integer; do nothing
 *	s	- negative string index; do nothing
 *	z	- colour name; do nothing
 *	Z	- ASCII char; do nothing
 *	k	- keytable index; do nothing
 *	v	- call fixarg (variable)
 *	V	- call fixarg (writeonly variable for procwin hack)
 *	a	- call fixarg (variable or string literal)
 *	c	- call fixcall
 *	C	- call fixcall if arg is positive
 *	l	- call fixlabel
 *	o	- option number
 *	P	- if prev arg is 0, do fixArg(arg) (else it is a regexp)
 *	.	- do nothing
 *	r	- regexp, do nothing
 *	=	- comparison key; do nothing
 *	N	- NOT flag; do nothing
 *	T	- `typeof' type; do nothing
 */

char		codepath[256] = { (char)0 };
short		startupEntryPoint=0;
short		initialiseEntryPoint=0;
short		defaultEntryPoint=0;
short		afterKeyEntryPoint=0;
short		idents=0;
short		varcnt=0;
short		namemap[MAX_IDS];
objectrecord	objects[MAX_IDS];
int		lineno = 1;
short		stringp=0, codep=0;
short		linked = 0;

/*******************************************************/

gcop_t ops[] = {
 { OP_ACCESS	, 3, "Access"	,".aa",	PTR(i_access)	},
 { OP_ACTION	, 3, "Action"	,".al",	PTR(i_action)	},
 { OP_APPEND	, 3, "Append"	,".aa",	PTR(i_append)	},
 { OP_ASSIGN	, 3, "Assign"	,".aa",	PTR(i_assign)	},
 { OP_BEEP	, 1, "Beep"	,".",	PTR(i_beep)	},
 { OP_BIND	, 4, "Bind"	,".kl",	PTR(i_bind)	},
 { OP_BOX		, 1, "Box"		,".",	PTR(i_box)	},
 { OP_BREAK	, 2, "Break"	,".l",	PTR(i_break)	},
 { OP_BROWSE	, 3, "Browse"	,".aa",	PTR(i_browse)	},
 { OP_CAT	, 7, "Cat"	,".aiaaaa",PTR(i_cat)	},
 { OP_CALL	, 3, "Call"	,".ciaaaaaaaa",PTR(i_call)},
 { OP_CD	, 2, "Cd"	,".a",	PTR(i_cd)	},
 { OP_CLEAR	, 1, "Clear"	,".",	PTR(i_clear)	},
 { OP_COMPARE	, 4, "Compare"	,".=aa",PTR(i_compare)	},
 { OP_CONTAINS	, 3, "Contains"	,".aa",	PTR(i_contains)	},
 { OP_DOWN	, 1, "Down"	,".",	PTR(i_down)	},
 { OP_DUMP	, 1, "Dump"	,".",	PTR(i_dump)	},
 { OP_ECHO	, 2, "Echo"	,".a",	PTR(i_echo)	},
 { OP_EDITTEXT, 3, "EditText",".ai",	PTR(i_edittext)},
 { OP_END	, 1, "End"	,".",	PTR(i_end)	},
 { OP_ENDLOOP	, 1, "EndLoop"	,".",	PTR(i_endloop)	},
 { OP_ENDREC	, 1, "EndRec"	,".",	PTR(i_endrec)	},
 { OP_EVAL	, 3, "Eval"	,".Na",	PTR(i_eval)	},
 { OP_EXEC	, 2, "Exec"	,".a",	PTR(i_exec)	},
 { OP_EXECCATCH,2, "ExecCatch",".a",	PTR(i_execcatch)},
 { OP_EXITTEXT, 2, "ExitText",".a",	PTR(i_exittext)},
 { OP_EXPAND	, 2, "Expand"	,".v",	PTR(i_expand)	},
 { OP_FCOPY	, 4, "Fcopy"	,".aai",PTR(i_fcopy)	},
 { OP_FILTER	, 2, "Filter"	,".r",	PTR(i_filter)	},
 { OP_FINDAREA	, 3, "FindArea"	,".aa",	PTR(i_findarea)	},
 { OP_FUNCTION	, 4, "Function"	,".nii",PTR(i_function)	},
 { OP_GOTO	, 2, "Goto"	,".l",	PTR(i_goto)	},
 { OP_HBORDER	, 2, "HBorder"	,".Z",	PTR(i_hborder)	},
 { OP_HEAD	, 4, "Head"	,".iPa",PTR(i_head)	},
 { OP_HELP	, 2, "Help"	,".k",	PTR(i_help)	},
 { OP_HELPALL	, 1, "HelpAll"	,".",	PTR(i_helpall)	},
 { OP_HELPKEY	, 1, "HelpKey"	,".",	PTR(i_helpkey)	},
 { OP_HOME	, 1, "Home"	,".",	PTR(i_home)	},
 { OP_IASSIGN	, 4, "IAssign"	,".iii",	PTR(i_iassign)	},
 { OP_IF	, 2, "If"	,".l",	PTR(i_if)	},
 { OP_INPATH	, 3, "InPath"	,".aa",	PTR(i_inpath)	},
 { OP_INVALIDATE	, 1, "Invalidate"	,".",	PTR(i_invalidate)	},
 { OP_KEYCODE	, 4, "Keycode"	,".kii",PTR(i_keycode)	},
 { OP_LABEL	, 2, "Label"	,".i",	PTR(NULL)	},
 { OP_LENGTH	, 2, "Length"	,".a",	PTR(i_length)	},
 { OP_LOADITEM	, 3, "LoadItem"	,".av",	PTR(i_loaditem)	},
 { OP_LOADRULES, 1, "LoadRules"	,".",	PTR(i_loadrules)	},
 { OP_LOCATE	, 3, "Locate"	,".aa",	PTR(i_locate)	},
 { OP_LOOKUP	, 3, "Lookup"	,".va",	PTR(i_lookup)	},
 { OP_LOOPALL	, 4, "LoopAll"	,".all",PTR(i_loopall)	},
 { OP_MAKEPATH	, 4, "MakePath",".vaa",	PTR(i_makepath)	},
 { OP_MARKED	, 2, "Marked"	,".a",	PTR(i_marked)	},
 { OP_METAKEY	, 3, "Metakey"	,".ii",	PTR(i_metakey)	},
 { OP_MODE	, 2, "Mode"	,".a",	PTR(i_mode)	},
 { OP_NEXTLINE	, 1, "NextLine"	,".",	PTR(i_nextline)	},
 { OP_NOT	, 1, "Not"	,".",	PTR(i_not)	},
 { OP_OPEN	, 2, "Open"	,".a",	PTR(i_open)	},
 { OP_PAINT	, 1, "Paint"	,".",	PTR(i_paint)	},
 { OP_PATMARK	, 2, "PatMark"	,".r",	PTR(i_patmark)	},
 { OP_PATMATCH	, 4, "PatMatch",".ari",	PTR(i_patmatch)	},
 { OP_PATSEARCH	, 2, "PatSearch",".r",	PTR(i_patsearch)},
 { OP_PATUNMARK	, 2, "PatUnmark",".r",	PTR(i_patunmark)},
 { OP_PGDN	, 1, "PgDn"	,".",	PTR(i_pgdn)	},
 { OP_PGUP	, 1, "PgUp"	,".",	PTR(i_pgup)	},
 { OP_PLAYMAC	, 2, "PlayMac"	,".a",	PTR(i_playmac)	},
 { OP_PROCWIN	, 2, "ProcWin"	,".V",	PTR(i_procwin)	},
 { OP_QUIT	, 1, "Quit"	,".",	PTR(NULL)	},
 { OP_RADIO	, 3, "Radio",".vi",	PTR(i_radio)},
 { OP_READ	, 3, "Read"	,".ai",	PTR(i_read)	},
 { OP_RECMAC	, 2, "RecMac"	,".a",	PTR(i_recmac)	},
 { OP_RESCAN	, 2, "Rescan"	,".a",	PTR(i_rescan)	},
 { OP_RESTORE	, 1, "Restore"	,".",	PTR(i_restore)	},
 { OP_RETURN	, 2, "Return"	,".i",	PTR(i_return)	},
 { OP_RETVAR	, 2, "RetVar",	".a",	PTR(i_retvar)	},
 { OP_SEARCH	, 2, "Search"	,".a",	PTR(i_search)	},
 { OP_SELATTRIB, 2, "SelAttrib"	,".a", PTR(i_selattrib)	},
 { OP_SETATTRIB, 4, "SetAttrib"	,".aaa", PTR(i_setattrib)	},
 { OP_SETEXPR	, 2, "SetExpr"	,".a",	PTR(i_setexpr)	},
 { OP_SETMATCH	, 3, "SetMatch"	,".r",	PTR(i_setmatch)	},
 { OP_SHOW	, 2, "Show"	,".=",	PTR(i_show)	},
 { OP_SLEEP	, 2, "Sleep"	,".i",	PTR(i_sleep)	},
 { OP_SORT	, 3, "Sort"	,".ai",	PTR(i_sort)	},
 { OP_SPLIT	, 5, "Split"	,".iavv",PTR(i_split)	},
 { OP_STRIP	, 2, "Strip"	,".v",	PTR(i_strip)	},
 { OP_STRMARK	, 2, "StrMark"	,".a",	PTR(i_strmark)	},
 { OP_STRMATCH	, 4, "StrMatch",".aai",	PTR(i_strmatch)	},
 { OP_STRUNMARK	, 2, "StrUnmark",".a",	PTR(i_strunmark)},
 { OP_SWAP	, 2, "Swap"	,".i",	PTR(i_swap)	},
 { OP_SYNC		, 1, "Sync"	,".",	PTR(i_sync)	},
 { OP_TAIL	, 4, "Tail"	,".iPa",PTR(i_tail)	},
 { OP_TEST	, 4, "Test"	,".aai",PTR(i_test)	},
 { OP_TEXT	, 2, "Text",".a",	PTR(i_text)},
 { OP_TOGGLE	, 3, "Toggle",".va",	PTR(i_toggle)},
 { OP_TYPEOF	, 3, "Typeof"	,".aT",	PTR(i_typeof)	},
 { OP_UNBIND	, 2, "Unbind"	,".k",	PTR(i_unbind)	},
 { OP_UNLINK	, 3, "Unlink"	,".iP",	PTR(i_unlink)	},
 { OP_UP	, 1, "Up" 	,".",	PTR(i_up)	},
 { OP_VBORDER	, 2, "VBorder"	,".Z",	PTR(i_vborder)	},
 { OP_VIEW	, 2, "View"	,".a",	PTR(i_view)	},
 { OP_WINDOW	, 5, "Window"	,".aaaa",	PTR(i_window)	},
 { OP_WRITE	, 2, "Write"	,".a",	PTR(i_write)	},
 { OP_YNTOGGLE, 3, "YNToggle",".vi",	PTR(i_yntoggle)}
};

static char *colorNames[] =
{
	COLOURS
};

static char *typeNames[] =
{
	"Dir", "Pipe", "CDev", "BDev", "Reg", "Exec", "Link"
};

#if __STDC__
void printArg(FILE *f, short v)
#else
void printArg(f, v)
	short v;
	FILE *f;
#endif
{
	if (v==0)
		fprintf(f,"0");
	else if (IS_LITERAL(v))
		fprintf(f,"\"%s\"",STRING(v));
	else if (IS_GLOBAL(v))
		fprintf(f,"$%s",ID_NAME(v));
	else if (IS_PARAM(v))
		fprintf(f,"$%d",FROM_PARAM(v));	/* parameter	*/
	else
		fprintf(f,"$_%d", FROM_LOCAL(v)); 	/* local var	*/
}

#if __STDC__
static void printLabel(FILE *f, short v)
#else
static void printLabel(f, v)
	short v;
	FILE *f;
#endif
{
	int j;
	for (j=0;j<MAX_IDS;j++)
	{
		if (objects[j].type&FUNC_TYPE && objects[j].p==v)
		{
			fprintf(f,"%s",ID_NAME(j));
			break;
		}
	}
	if (j>=MAX_IDS) fprintf(f,"L_%d",v);
}

#define printRegexp(f,v)	fprintf(f,"/%s/",REGEXP(v))

#if __STDC__
short dumpOp(FILE *fp, short Ip, int symbolic)
#else
short dumpOp(fp, Ip, symbolic)
	FILE *fp;
	short Ip;
	int symbolic;
#endif
{
	short A, a, o, i, t, u;
	char *typ;
	fprintf(fp,"\t%-4d %-15s ",Ip, ops[o = codespace[Ip]].name);
	a = A = ops[o].cnt;
	typ = ops[o].link;
	assert(ops[o].op == o);
	if (o==OP_CALL)
	{
		a+=codespace[Ip+2];
		A=a;
	}
	else if (o==OP_FUNCTION)
		a+=codespace[Ip+2] + codespace[Ip+3];
	for (i=1;i<a;)
	{
		short v = codespace[Ip+i];
		if (!symbolic) fprintf(fp,"%5d", v);
		else if (i>=A)
		{
			/* param or local var name in OP_FUNCTION */
			fprintf(fp,"$%s",LITERAL(v));
		}
		else
		{
			switch(typ[i])
			{
			case 'T':
				fprintf(fp,"(");
				t=1; u = 0;
				while (t<=64)
				{
					if (t&v) fprintf(fp," %s",typeNames[u]);
					u++; t*=2;
				}
				fprintf(fp," )");
				break;
			case 'n':
				fprintf(fp,"%s",LITERAL(v));
				break;
			case 'N':
				fprintf(fp,"%s",v?"NOT":"");
				break;
			case 'k':
				fprintf(fp,"[#%d]",v);
				break;
			case 'Z':
				fprintf(fp,"`%c'",v);
				break;
			case 'z':
				fprintf(fp,"%s",colorNames[v]);
				break;
			case 's':
			case 'v':
			case 'V':
			case 'a':
				printArg(fp, v);
				break;
			case 'C':
				if (v<0) goto numeric;
				else
			case 'c':
				printLabel(fp, v);
				break;
			case 'l':
				printLabel(fp, v);
				break;
			case 'P':
				if (codespace[Ip+i-1]==0) 
					printArg(fp, v);
				else
			case 'r':
				printRegexp(fp, v);
			numeric:
			case 'i':
			default:
				fprintf(fp,"%5d", v);
			}
		}
		if (++i<a) fprintf(fp,", ");
	}
	fprintf(fp,"\n");
	return Ip+a;
}

#if	__STDC__
int ReadCode(void)
#else
int ReadCode()
#endif
{
	FILE *codefile = NULL;
	int retried=0;
retry:
#ifdef __MSDOS__
	if (findReadPath(CODNAME, "$GCPATH\\.$PATH", NULL, codepath))
		codefile = fopen(codepath,"rb");
#else
	if (findReadPath(CODNAME, "$GCPATH$HOME.$PATH/", NULL, codepath))
		codefile = fopen(codepath,"r");
#endif
	if (codefile)
	{
		short vernum = 0;
		(void)fread(&vernum,(unsigned)1,sizeof(short),codefile);
		if (vernum != VERCODE)
		{
			fclose(codefile);
			fprintf(stderr,"\n\rWarning: code file is out of date (%X instead of %X)!\n\r",vernum,VERCODE);
			fprintf(stderr,"\tCannot use this code file - please recompile the script file.\r\n");
			if (!retried)
			{
				fprintf(stderr,"Attempting to recover...\r\n");
				retried = 1;
				CompileScript(NULL, 0, 0);
				goto retry;
			}
			sleep(2);
			return -1;
		}
		(void)fread(&linked,(unsigned)1,sizeof(short),codefile);
		(void)fread(&startupEntryPoint,(unsigned)1,sizeof(short),codefile);
		(void)fread(&initialiseEntryPoint,(unsigned)1,sizeof(short),codefile);
		(void)fread(&defaultEntryPoint,(unsigned)1,sizeof(short),codefile);
		(void)fread(&afterKeyEntryPoint,(unsigned)1,sizeof(short),codefile);
		(void)fread(&codep,(unsigned)1,sizeof(short),codefile);
		(void)fread(codespace,(unsigned)codep,sizeof(short),codefile);
		(void)fread(&stringp,(unsigned)1,sizeof(short),codefile);
		(void)fread(stringspace,(unsigned)stringp,sizeof(char),codefile);
		(void)fread(&idents,(unsigned)1,sizeof(short),codefile);
		(void)fread(&varcnt,(unsigned)1,sizeof(short),codefile);
		(void)fread(namemap,(unsigned)idents,sizeof(namemap[0]),codefile);
		(void)fread(objects,(unsigned)idents,sizeof(objects[0]),codefile);
		fclose(codefile);
		codepath[strlen(codepath)-strlen(CODNAME)] = '\0';
		return 0;
	}
	else
	{
		if (!retried)
		{
			fprintf(stderr,"\n\rWarning: cannot open a code file\n\r");
			fprintf(stderr,"Attempting to recover...\n\r");
			retried = 1;
			CompileScript(NULL, 0, 0);
			goto retry;
		}
		sleep(2);
		return -1;
	}
}

#if __STDC__
void writecodefile(void)
#else
void writecodefile()
#endif
{
	FILE *codefile = NULL;
#ifdef __MSDOS__
	if (findWritePath(CODNAME, forceCurrent ? "." : "$GCPATH\\", codepath))
		codefile = fopen(codepath,"wb");
#else
	if (findWritePath(CODNAME, forceCurrent ? "." : "$GCPATH$HOME", codepath))
		codefile = fopen(codepath,"w");
#endif
	if (codefile)
	{
		short vernum = VERCODE;
		fprintf(stderr,"Writing code file %s\n",codepath);
		(void)fwrite(&vernum,(unsigned)1,sizeof(short),codefile);
		(void)fwrite(&linked,(unsigned)1,sizeof(short),codefile);
		(void)fwrite(&startupEntryPoint,(unsigned)1,sizeof(short),codefile);
		(void)fwrite(&initialiseEntryPoint,(unsigned)1,sizeof(short),codefile);
		(void)fwrite(&defaultEntryPoint,(unsigned)1,sizeof(short),codefile);
		(void)fwrite(&afterKeyEntryPoint,(unsigned)1,sizeof(short),codefile);
		(void)fwrite(&codep,(unsigned)1,sizeof(short),codefile);
		(void)fwrite(codespace,(unsigned)codep,sizeof(short),codefile);
		(void)fwrite(&stringp,(unsigned)1,sizeof(short), codefile);
		(void)fwrite(stringspace,(unsigned)stringp,sizeof(char),codefile);
		(void)fwrite(&idents,(unsigned)1,sizeof(short),codefile);
		(void)fwrite(&varcnt,(unsigned)1,sizeof(short),codefile);
		(void)fwrite(namemap,(unsigned)idents,sizeof(namemap[0]),codefile);
		(void)fwrite(objects,(unsigned)idents,sizeof(objects[0]),codefile);
		fclose(codefile);
	}
	else fprintf(stderr,"Cannot open code file for output!\n");
}

/*******************************************
	CODE DISASSEMBLER
********************************************/

static short symbolicDump = 0;

#if __STDC__
static void DumpStrings(void)
#else
static void DumpStrings()
#endif
{
	int i = 0;
	fprintf(stdout,"\n\nSTRING SPACE\n====== =====\n\n");
	while (i<stringp)
	{
		fprintf(stdout,"%5d: %s\n",i,LITERAL(i));
		i += strlen(LITERAL(i))+1;
	}
	fprintf(stdout,"\n\n");
}

#if __STDC__
static void DumpSymbols(void)
#else
static void DumpSymbols()
#endif
{
	int i;
	fprintf(stdout,"\n\nSYMBOL SPACE\n====== =====\n\n");
	for (i=0;i<idents;i++)
	{
		if (objects[i].type&FUNC_TYPE)
			fprintf(stdout,"%3d %15s function  %4d (params %d)\n",
				i,ID_NAME(i),
				(int)objects[i].p,
				(int)objects[i].np);
		else fprintf(stdout,"%3d %15s variable   %d\n",
			i,ID_NAME(i),
			(int)objects[i].p);
	}
}

#if __STDC__
static int GetNextFunction(int p)
#else
static int GetNextFunction(p)
	int p;
#endif
{
	int fp, rtn=0, ep=codep;
	for (fp=0;fp<idents;fp++)
	{
		if (objects[fp].type & FUNC_TYPE)
		{
			if ((objects[fp].p > p) && (objects[fp].p <ep ))
			{
				ep = objects[fp].p;
				rtn = fp;
			}
		}
	}
	return rtn;
}

#if __STDC__
void DumpCode(void)
#else
void DumpCode()
#endif
{
	short i = 1, j;
	int fp = GetNextFunction(0);
	fprintf(stdout,"\n\nCODE SPACE\n====== =====\n\n");
	while (i<codep)
	{
		if (i==(int)objects[fp].p)
		{
			fprintf(stdout,"\n%s:\n",ID_NAME(fp));
			fp = GetNextFunction(i);
		}
		else if (i==initialiseEntryPoint)
			fprintf(stdout,"\nGCInit:\n");
		else if (i==startupEntryPoint)
			fprintf(stdout,"\nGCStart:\n");
		else if (i==defaultEntryPoint)
			fprintf(stdout,"\nGCDefault:\n");
		else if (i==afterKeyEntryPoint)
			fprintf(stdout,"\nGCAfterKey:\n");
		if ((j = dumpOp(stdout,i,symbolicDump-1))==i) break;
		else i = j;
	}
	fprintf(stdout,"\nStartup entry point:    %d\n",startupEntryPoint);
	fprintf(stdout,"\nInitialise entry point: %d\n",initialiseEntryPoint);
	fprintf(stdout,"\nDefault entry point:    %d\n",defaultEntryPoint);
	fprintf(stdout,"\nAfter key entry point:  %d\n",afterKeyEntryPoint);
	fprintf(stdout,"\nCode size: %d\n",codep);
	fprintf(stdout,"\nString size: %d\n",stringp);
	fprintf(stdout,"\nSymbols: %d\n",idents);
}

/************************************************************
		SCRIPT COMPILER INTERFACE
*************************************************************/

int verbose = 0;

#if __STDC__
int CompileScript(char *script, short symdump, short dontLink)
#else
int CompileScript(script, symdump, dontLink)
	char *script;
	short symdump, dontLink;
#endif
{
	if (script == NULL) script = "gc3.key";
	symbolicDump = symdump;
#ifdef __MSDOS__
	Define("DOS");
#else
	Define("UNIX");
#endif
	Define("ANY");
#ifdef __MSDOS__
	if (findReadPath(script, "$GCPATH\\.$PATH", NULL, codepath))
#else
	if (findReadPath(script, "$GCPATH$HOME.$PATH/", NULL, codepath))
#endif
		keydeffile = fopen(codepath, "r");
	if (keydeffile)
	{
		fprintf(stderr,"Opened file: %s for reading\n", codepath);
		config_file();
		fclose(keydeffile);
		if (verbose)
			fprintf(stderr,"Compiled %d lines to %d bytes\n",lineno,codep);
		if (dontLink) ;
		else if (gclink()==0)
		{
			if (!initialiseEntryPoint)
				warn(ER_NOINIT);
			if (!defaultEntryPoint)
				warn(ER_NODEFAULT);
			if (!startupEntryPoint)
				warn(ER_NOSTART);
			writecodefile();
		}
		else error(ER_LINKER);
	}
	else error(ER_KEYFILE);
	if (verbose && !dontLink)
			fprintf(stderr,"After link: code %d strings %d\n",codep,stringp);
	fprintf(stderr,"Compilation successful!\n");
	if (symbolicDump)
	{
		lineno = -1;
		DumpStrings();
		DumpSymbols();
		DumpCode();
	}
	freeStab();
	return 0;
}

/********************************/
/* Allocate and free code space */
/********************************/

short *codespace = NULL;

#if __STDC__
void AllocateCodeSpace(void)
#else
void AllocateCodeSpace()
#endif
{
	assert(codespace == NULL);
	codespace = (short *)calloc(CODESPACE,sizeof(short));
	assert(codespace);
}

#if __STDC__
void FreeCodeSpace(void)
#else
void FreeCodeSpace()
#endif
{
	assert(codespace);
	free(codespace);
	codespace = NULL;
}

