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


#include "gcsite.h"
#include "gcport.h"
#include "gccc.h"
#include "gckey.h"

static short	labelcnt = 1, innerloop = 0;
static short	loopstack[MAX_LOOPS][2];
static int		inDecl;
static short	pnames[MAX_PARAMS],
					vnames[MAX_LOCVARS];	/* indexes of their names */

static short trueString;

static int EM=1;

/******************************************
	PARSER TABLE - this table stipulates
	how to parse the simpler commands. If
	a symbol has an entry in this table, the
	opcode is used to determine the link info
	from the ops[] table, which is then used
	to determine how to do the parse as well.
******************************************/

static ParserRule_t ParseRules[] =
{
	{ SYM_APPEND, OP_APPEND },
	{ SYM_BOX, OP_BOX },
	{ SYM_BEEP, OP_BEEP },
	{ SYM_CD, OP_CD },
	{ SYM_CLEAR, OP_CLEAR },
	{ SYM_DOWN, OP_DOWN },
	{ SYM_DUMP, OP_DUMP },
	{ SYM_ECHO, OP_ECHO },
	{ SYM_EDITTEXT, OP_EDITTEXT },
	{ SYM_END, OP_END },
	{ SYM_ENDREC, OP_ENDREC },
	{ SYM_EXEC, OP_EXEC },
	{ SYM_EXECCATCH, OP_EXECCATCH },
	{ SYM_EXITTEXT, OP_EXITTEXT },
	{ SYM_EXPAND, OP_EXPAND },
	{ SYM_FILTER, OP_FILTER },
	{ SYM_FINDAREA, OP_FINDAREA },
	{ SYM_HBORDER, OP_HBORDER },
	{ SYM_HOME, OP_HOME },
	{ SYM_INVALIDATE, OP_INVALIDATE },
	{ SYM_LOADITEM, OP_LOADITEM },
	{ SYM_LOADRULES, OP_LOADRULES },	
	{ SYM_LOCATE, OP_LOCATE },
	{ SYM_LOOKUP, OP_LOOKUP },
	{ SYM_MAKEPATH, OP_MAKEPATH },
	{ SYM_MODE, OP_MODE },
	{ SYM_NEXTLINE, OP_NEXTLINE },
	{ SYM_OPEN, OP_OPEN },
	{ SYM_PAINT, OP_PAINT },
	{ SYM_PGDN, OP_PGDN },
	{ SYM_PGUP, OP_PGUP },
	{ SYM_PLAYMAC, OP_PLAYMAC },
	{ SYM_QUIT, OP_QUIT },
	{ SYM_RECMAC, OP_RECMAC },
	{ SYM_RESCAN, OP_RESCAN },
	{ SYM_RESTORE, OP_RESTORE },
	{ SYM_SLEEP, OP_SLEEP },
	{ SYM_SPLIT, OP_SPLIT },
	{ SYM_STRIP, OP_STRIP },
	{ SYM_SYNC, OP_SYNC },
	{ SYM_TEXT, OP_TEXT },
	{ SYM_TOGGLE, OP_TOGGLE },
	{ SYM_UP, OP_UP },
	{ SYM_VBORDER, OP_VBORDER },
	{ SYM_VIEW, OP_VIEW },
	{ SYM_WINDOW, OP_WINDOW },
	{ SYM_WRITE, OP_WRITE },
	{ 0, 0 } /* end marker */
};

/********************************************
	PROTOTYPES OF LOCALS
*********************************************/

#if __STDC__

static void initvars(void);
static short savestring(char *);
static short getArg(int no_funcs_allowed);
static int function(void);
static void command_list(void);
static void command(void);
static short string(void);
static void call(short f);
static void do_cmd(void);
static void stringcmd(enum codeops op);
static short regexp(void);
static void if_cmd(void);
static void loop_cmd(void);
static void search_expr(void);
static void expression(void);
static void show_cmd(void);
static void sort_cmd(void);
static void unlink_func(void);
static void while_cmd(void);
static void primary(void);
static short type_list(void);
static void read_cmd(void);

#else

static void initvars();
static short savestring();
static short getArg();
static int function();
static void command_list();
static void command();
static void stringcmd();
static void unlink_func();
static void loop_cmd();
static void while_cmd();
static void do_cmd();
static void if_cmd();
static void search_expr();
static void call();
static void show_cmd();
static void sort_cmd();
static void expression();
static short string();
static short regexp();
static void primary();
static short type_list();
static void read_cmd();

#endif


/********************************************
		ERRORS AND WARNINGS
********************************************/

static char *(errormsg[]) = {
	"Identifier redefined",
	"Undefined identifier",
	"Not in a loop",
	"Bad RETURN command",
	"Bad command",

	"Bad relational operator",
	"Bad string",
	"Bad function key specifier",
	"Bad control key specifier",
	"Bad meta key specifier",

	"Bad character key specifier",
	"Bad key specifier",
	"Bad integer",
	"`;' expected",
	"`)' expected",

	"Identifier expected",
	"`(' expected",
	"`,' expected",
	"`[' expected",
	"`]' expected",

	"`OF' expected",
	"Syntax error",
	"Namestore overflow",
	"Malloc failure",
	"Too many variables",

	"Cannot open key definition file",
	"Cannot open key code compilation file",
	"Bad SHOW command argument",
	"Too many labels",
	"MARKED, ALL or a regular expression expected",

	"Variable expected",
	"Function name expected",
	"Variable or regular expression expected",
	"String expected",
	"Colour expected",

	"Numeric keyspec expected",
	"Option expected",
	"Invalid internal function in bind",
	"Error in expression",
	"Link unsuccessfull",

	"Cannot bind/unbind this keyspec",
	"Out of code space!",
	"Sort key expected",
	"Compare key expected",
	"`compare' or `eval' expected",

	"Too many parameters in call (max 10)",
	"Wrong number of parameters in call",
	"String used where a variable parameter is required",
	"gcdefault function already defined",
	"gcstartup function already defined",

	"gcinit function already defined",
	"No gcinit function found",
	"No gcdefault function found",
	"No gcstartup function found",
	"ON or OFF expected",

	"This function may not take parameters",
	"Too many parameters",
	"Too many local variables",
	"Too many functions and global variables",
	"gcafterkey function already defined",

	"Internal error",
	"Bad type list in typeof expression",
	"Bad meta-combo key specifier",
	"Bad command line editor key specifier",
	"Bad monochrome attribute",

	"Bad variable index"
};

#if __STDC__
void error(enum errort typ) {
#else
void error(typ)
	enum errort typ;
{
#endif
	if (lineno>=0)
		fprintf(stderr,"ERROR: %s (line %d)\n",errormsg[(int)typ],lineno);
	else fprintf(stderr,"ERROR: %s\n",errormsg[(int)typ]);
	exit(0);
}

#if __STDC__
void warn(enum errort typ) {
#else
void warn(typ)
	enum errort typ;
{
#endif
	if (lineno>=0)
		fprintf(stderr,"WARNING: %s (line %d)\n",errormsg[(int)typ],lineno);
	else fprintf(stderr,"WARNING: %s\n",errormsg[(int)typ]);
}

/*******************************************
		SYMBOL TABLE
********************************************/

#define MAXKEY			631

typedef	struct namerecord	*namerecordp;

struct namerecord {
        namerecordp	nextname;
        short		IDindex;	/* Identifier index */
        short		refcnt;
};

static namerecordp 	*hash;

#if __STDC__
static short hashkey(char *keytext) {
#else
static short hashkey(keytext)
	char *keytext;
{
#endif
	short sum=0;
	while (*keytext) {
	        sum += *keytext++;
		sum %= 32641 		/* 32768-127 */;
        }
	return sum % MAXKEY;
}

#if __STDC__
static void insert(char *name, short IDindex, short type)
#else
static void insert(name, IDindex, type)
	char *name;
	short IDindex, type;
#endif
{
	namerecordp nrp;
	short keyno = hashkey(name);
	namemap[IDindex] 	= savestring(name);
	objects[IDindex].p	= (type&FUNC_TYPE)?-1:(varcnt++);
	objects[IDindex].type	= type;
	if (varcnt>MAX_VARS) error(ER_VARS);
	if ((nrp=(namerecordp)calloc(1,sizeof(struct namerecord)))!=NULL) {
		nrp->nextname	= hash[keyno];
		nrp->IDindex	= IDindex;
		nrp->refcnt		= 0;
		hash[keyno]	= nrp;
	} else error(ER_MALLOC);
}

#if __STDC__
void freeStab(void)
#else
void freeStab()
#endif
{
	int i;
	for (i=0; i<MAXKEY; i++) {
		while (hash[i]) {
			namerecordp tmp = hash[i];
			if (verbose && tmp->IDindex > LASTPREDEFVAR)
				fprintf(stdout,"%3d references to identifier %s\n",
							tmp->refcnt, ID_NAME(tmp->IDindex));
			hash[i] = hash[i]->nextname;
			free((char *)tmp);
		}
	}
	free(hash);
}

#if __STDC__
static namerecordp lookup(char *name, short type)
#else
static namerecordp lookup(name, type)
	char *name;
	short type;
#endif
{
	namerecordp nrp;
	short keyno = hashkey(name);
	for (nrp = hash[keyno] ; nrp ; nrp = nrp->nextname) {
	fflush(stdout);
   if (objects[nrp->IDindex].type&type
		&& strcmp(name,ID_NAME(nrp->IDindex))==0) 
				break;
	}
	return nrp;
}

#if __STDC__
static short search(char *name, short type)
#else
static short search(name, type) 
	char *name;
	short type;
#endif
{
	namerecordp nrp;
	int i;
	if (type & VAR_TYPE) {
		/* First look at local vars and params */
		for (i=0;i<np;i++) 
			if (strcmp(LITERAL(pnames[i]),name)==0)
				return TO_PARAM(i);
		for (i=0;i<nv;i++) 
			if (strcmp(LITERAL(vnames[i]),name)==0)
				return TO_LOCAL(i);
	}
	/* OK, try the symbol table */
	if ((nrp = lookup(name,type)) == NULL) {
		if (idents > MAX_IDS) error(ER_IDS);
		insert(name,idents,type);
		return idents++;
   } else
	{
		nrp->refcnt++;
		return nrp->IDindex;
	}
}

#define initVar(n)	insert(n,idents++,VAR_TYPE)

#if __STDC__
static void initvars(void) {
#else
static void initvars() {
#endif
			hash = (namerecordp *)calloc(sizeof(namerecordp), MAXKEY);
			if (hash == NULL) error(ER_MALLOC);
/* 0 */	initVar("GC3"); /* dummy - don't want var zero */
/* 1 */	initVar("PFILE");
/* 2 */	initVar("SFILE");
/* 3 */	initVar("PFILES");
/* 4 */	initVar("SFILES");
/* 5 */	initVar("PPATH");
/* 6 */	initVar("SPATH");
/* 7 */	initVar("PSEL");
/* 8 */	initVar("SSEL");
/* 9 */	initVar("PSELSIZE");
/* 10*/	initVar("SSELSIZE");
/* 11*/	initVar("PSELCNT");
/* 12*/	initVar("SSELCNT");
/* 13*/	initVar("PSIZE");
/* 14*/	initVar("SSIZE");
/* 15*/	initVar("PMTIME");
/* 16*/	initVar("SMTIME");
/* 17*/	initVar("PATIME");
/* 18*/	initVar("SATIME");
/* 19*/	initVar("POWNER");
/* 20*/	initVar("SOWNER");
/* 21*/	initVar("PGROUP");
/* 22*/	initVar("SGROUP");
/* 23*/	initVar("PPERMS");
/* 24*/	initVar("SPERMS");
/* 25*/	initVar("USERNAME"); /* login name */
/* 26*/	initVar("TEMPNAME");
/* 27*/	initVar("TEMPFILE");
/* 28*/	initVar("TEMPPATH");
/* 29*/	initVar("DATE");
/* 30*/	initVar("TIME");
/* 31*/	initVar("RANDOM");
/* 32*/	initVar("PREVCMD");
/* 33*/	initVar("NEXTCMD");
/* 34*/ initVar("DISKFREE");
/* 35*/ initVar("DISKUSED");
/* 36*/ initVar("VERSION");
/* 37*/ initVar("PATCHLEVEL");
/* 38*/ initVar("CODEPATH");
/* 39*/ initVar("VIEWLINE");
/* 40*/ initVar("VIEWFILE");
/* 41*/ initVar("WINDOW");
/* 42*/ initVar("DIRSELECT");
/* 43*/ initVar("DIRSFIRST");
/* 44*/ initVar("SHOWHIDDEN");
/* 45*/ initVar("FOLLOWLINKS");
/* 46*/ initVar("DOSPATTERNS");
/* 47*/ initVar("VIEWER");
/* 48*/ initVar("DEBUG");
/* 49*/ initVar("EXPAND");
/* 50*/ initVar("NUMBERLINES");
/* 51*/ initVar("DISPLAY");
/* 52*/ initVar("TREE");
/* 53*/ initVar("CONFIRM");
/* 54*/ initVar("WAIT");
/* 55*/ initVar("LINEBUFF");
/* 56*/ initVar("MESSAGE");
/* 57*/ initVar("CH");
/* 58*/ initVar("WIDTH");
/* 59*/ initVar("HEIGHT");
/* 60*/ initVar("LEFTMARGIN");
/* 61*/ initVar("TOPMARGIN");
/* 62*/ initVar("SHOWVIEW");
/* 63*/ initVar("SHOWSTATUS");
/* 64*/ initVar("SHELL");
/* 65*/ initVar("HELPFILE");
/* 66*/ initVar("HELPENTRY");
/* 67*/ initVar("ISCOLOR");
}	 

/********************************************
		LEXICAL ANALYSER
*********************************************/

static char symtext[512];
static int newID = 0;
FILE *keydeffile;
unsigned ch = ' ';
static int symvalue;
static enum symboltype symbol;
static char *(cursorKeyNames[]) =
{
	"Up", "Down", "Left", "Right",
	"PgUp", "PgDn", "Home", "End",
	"CtlLeft", "CtlRight", "CtlUp", "CtlDown",
	"CtlPgUp", "CtlPgDn", "CtlHome", "CtlEnd",
	"Ins", "Del", "Bkspc"
};

static char *reservedWords[] = {
	COLOURS,
	"ACCESS", "ACTION", "ALL", "AND", "APPEND", "ASCENDING", "ATIME",	"ATTRIBUTE",
	"BEEP", "BDEV", "BIND", "BOX", "BREAK", "BROWSE",
	"CAT", "CD", "CDEV", "CLEAR", "COMPARE",
	"CONTAINER", "CONTAINS", "CONTENTS", "CONTINUE", "COPY",
	"DESCENDING", "DIR", "DO", "DOWN", "DUMP", 
	"ECHO", "EDITTEXT", "ELSE", "END", "ENDREC", "EVAL",
	"EXEC", "EXECCATCH", "EXITTEXT", "EXPAND", "EXTENS",
	"FILTER", "FINDAREA", "FROM",
	"GCAFTERKEY", "GCDEFAULT", "GCINIT", "GCSTART", "GROUP",
	"HBORDER", "HEAD", "HELP", "HOME",
	"IF", "IN", "INPATH", "INVALIDATE", "IS",
	"KEYCODE", 
	"LEFT", "LENGTH", "LINK", "LOADITEM", "LOADRULES",
	"LOCAL", "LOCATE", "LOCK", "LOOKUP", "LOOP", 
	"MAKEPATH", "MARK", "MARKED", "MATCHES", "METAKEY", "MODE", "MOVE", "MTIME", 
	"NAME", "NEXTLINE", "NONE", "NOT",
	"ON", "OPEN", "OR", "OWNER",
	"PAINT", "PERMS", "PGDN", "PGUP", "PIPE", "PLAYMACRO", "PROCWIN",
	"QUIT", 
	"RADIO", "READ", "RECMACRO", "REG", 
	"RESCAN", "RESTORE", "RETURN", "RIGHT",
	"SEARCH", "SHOW",
	"SIZE", "SLEEP", "SORTBY", "SPLIT", "STRIP", "SWAP", "SYNC",
	"TAIL", "TEXT", "TO", "TOGGLE", "TYPEOF",
	"UNBIND", "UNLINK", "UNMARK", "UP", 
	"VBORDER", "VIEW",
	"WHILE", "WINDOW", "WRITE",
	"YNTOGGLE"
};

#if __STDC__
static int isReserved(char *name)
#else
static int isReserved(name)
	char *name;
#endif
{
	int i;
	for (i=0;i<(sizeof(reservedWords)/sizeof(reservedWords[0]));i++)
		if (strcmp(name,reservedWords[i])==0)
			return i;
	return -1;
}

#if __STDC__
static short savestring(char *s)
#else
static short savestring(s)
	char *s;
#endif
{
	short rtn = stringp, l = strlen(s), i;
	/* First see if we have a copy */
	i = 0;
	while (i<stringp) {
		if (strcmp(stringspace+i,s)==0) return i;
		i+=strlen(stringspace+i)+1;
	}
	stringp += l+1;
	if (stringp >= STRINGSPACE) error(ER_NAMESTORE);
	strcpy(stringspace+rtn,s);
	return rtn;
}

static char defined[MAX_DEFS][16];
static int numdefs = 0;

#if __STDC__
void Define(char *name)
#else
void Define(name)
	char *name;
#endif
{
	if (numdefs<MAX_DEFS) strcpy((char *)defined[numdefs++],name);
	else fprintf(stderr,"Too many IF_ names - can't define <%s>!\n",name);
}

#if __STDC__
static int isDefined(char *name)
#else
static int isDefined(name)
	char *name;
#endif
{
	int i;
	for (i=0;i<MAX_DEFS;i++)
	{
		if (strcmp(name,defined[i])==0)
			return i;
	}
	return -1;
}

#if __STDC__
static void nextchar(void)
#else
static void nextchar()
#endif
{
	if (feof(keydeffile)) ch=ETX;
	else do
	{
		ch = fgetc(keydeffile);
	} while (!feof(keydeffile) && (ch<32 || ch>127)
		&& ch!='\n' && ch!='\t');
}

#if __STDC__
void nextsymbol(void)
#else
void nextsymbol()
#endif
{
	short length, IDindex, digit, var_ref;
	char *tokenstring, tch;
	int t;
	symbol = SYM_UNDEF;
	newID = 0;
	while (symbol==SYM_UNDEF || !EM)
	{
		tokenstring = symtext;
		*tokenstring = '\0';
		var_ref = 0;
	        switch (ch) {
		case ETX:	symbol = SYM_EOF;
				break;
		case '\t':
		case ' ': 	nextchar();
				break;
		case '\n':	nextchar();
				fprintf(stderr,"%d\r",++lineno);
				break;
		case '#': 	while (ch!='\n') nextchar();
				break;
		case '(':	nextchar();
              symbol = SYM_LPARENTHESIS;
             	break;
      case ')':	symbol = SYM_RPARENTHESIS;
				nextchar();
				break;
		case '/':	symbol = SYM_REGEXP;
				goto doString;
		case '\'':	
		case '"':
             	symbol = SYM_STRING;
		doString:
				tch = ch;
				t = 0;
				nextchar();
             	if (ch>=32 && ch<=126)
				{
					for (;;)
					{
						if (ch<32 || ch>126) error(ER_STRING);
						if (ch=='#')
						{
							int oldch = ch;
							nextchar();
							if (ch!=tch)
							{
								*tokenstring++ = (char)oldch;
								t++;
							}
						}
						else if (ch==tch)
							break;
						*tokenstring++ = (char)ch;
						t++;
                    	nextchar();
                	}
					nextchar();
				}
				else error(ER_STRING);
				*tokenstring = '\0';
				if (tch=='\'')
				{
					if (t==1)
					{
						symbol = SYM_INTEGER;
						symvalue = symtext[0];
					}
					else if (t==2 && symtext[0]=='#')
					{
						symbol = SYM_INTEGER;
						switch (symtext[1])
						{
						case 'n': symvalue = '\n';
							break;
						case 'r': symvalue = '\r'; 
							break;
						case 't': symvalue = '\t';
							break;
						default:
							symvalue = symtext[1];
						}
					}
				}
             	break;
		case ';':
				symbol = SYM_SEMICOLON;
				nextchar();
				break;
      case ':':
				symbol = SYM_COLON;
				nextchar();
				break;
      case '{':
				symbol = SYM_LBRACE;
				nextchar();
				break;
      case '}':
				symbol = SYM_RBRACE;
				nextchar();
				break;
		case '[':
				nextchar();
				if (ch=='F')
				{
					nextchar();
					if (ch<'0' || ch>'9')
						error(ER_FKEY);
					if (ch=='0') ch=9;
					else ch -= '1';
					symvalue = 20+ch;
					nextchar();
				}
				else if (ch=='^')
				{
					nextchar();
					if (ch>='a' && ch<='z')
						ch -= ('a'-'A');
					else if (ch<'A' || ch>'Z')
						error(ER_CTRLKEY);
					symvalue = 30+(ch-'A');
					nextchar();
				}
				else if (ch=='+') /* meta-combo */
				{
					symvalue = 0;
					nextchar();
					if (ch>='0' && ch<='9')
					{
						symvalue = ch - '0';
						nextchar();
						if (ch>='0' && ch<='9')
						{
							symvalue = symvalue * 10 + ch - '0';
							nextchar();
						}
						symvalue += START_META_KEYS;
					}
					else error(ER_COMBOKEY);
				}
				else if (ch=='*') /* cmdln editor key */
				{
					symvalue = 0;
					nextchar();
					if (ch>='0' && ch<='9')
					{
						symvalue = ch - '0';
						nextchar();
						if (ch>='0' && ch<='9')
						{
							symvalue = symvalue * 10 + ch - '0';
							nextchar();
						}
						symvalue += START_CED_KEYS;
					}
					else error(ER_EDITKEY);
				}
				else if (ch=='M' || ch=='m')
				{
					nextchar();
					if (ch>='0' && ch<='9')
						ch -= '0';
					else if (ch>='a' && ch<='z')
						ch -= 'a'-10;
					else if (ch>='A' && ch<='Z')
						ch -= 'A'-10;
					else error(ER_METAKEY);
					symvalue = 60+ch;
					nextchar();
				}
				else if (ch=='\'')
				{
					nextchar();
					symvalue = ch-' '+100;
					nextchar();
					if (ch != '\'')
						error(ER_CHARKEY);
					nextchar();
				}
				else if (ch=='#')
				{
					/* Numeric keyspec */
					int k=0;
					nextchar();
					if (EM && !isdigit(ch))
						error(ER_DIGSPEC);
					while (isdigit(ch))
					{
						k = k*10 + ch-'0';
						nextchar();
					}
					symvalue = k;
				}
				else
				{
					char name[20], n2[20];
					int i=0;
					if (EM && !isalpha(ch))
						error(ER_KEYSPEC);
					while (isalpha(ch) && i<18)
					{
						name[i++] = ch;
						nextchar();
					}
					name[i] = '\0';
					STRUPR(name);
					for (i=0;i<19;i++)
					{
						strcpy(n2,cursorKeyNames[i]);
						STRUPR(n2);
						if (strcmp(name,n2)==0)
						{
							symvalue = i;
							break;
						}
					}
					if (i>=19) error(ER_KEYSPEC);
				}
				if (ch!=']') error(ER_KEYSPEC);
				nextchar();
				symbol = SYM_KEYSPEC;
				break;
      case '=':
				nextchar();
				if (ch=='=')
				{
					nextchar();
					symbol = SYM_EQUALS;
				}
				else 	symbol = SYM_ASSIGN;
				break;
		case '+':
				symbol = SYM_PLUS;
				nextchar();
				break;
      case ',':
				symbol = SYM_COMMA;
				nextchar();
				break;
      case '$':
				nextchar();
				var_ref = 1;
      default:
				if (!var_ref && (ch=='-' || isdigit(ch)))
				{
					short sgn = 1, base=10;
					if (ch=='-')
					{
						sgn = -1;
						nextchar();
                      if (!isdigit(ch))
							error(ER_INTEGER);
					}
            		symvalue = 0;
					if (ch=='0')
					{
						nextchar();
						if (ch=='x')
						{
							base = 16;
							nextchar();
						}
						else base = 8; /* not supported */
					}
                  while (isdigit(ch) || (base==16 && isxdigit(ch)))
					{
                		if (base==16 && ch>'9')
						{
							if (ch>'Z') digit = ch-'a'+10;
							else digit = ch-'A'+10;
						}
						else digit = ch-'0';
						if (symvalue<=(MAXINT-digit)/base)
						{
							symvalue = base*symvalue+digit;
		                    nextchar();
	                    }
						else error(ER_INTEGER);
					}
					symvalue *= sgn;
		            symbol = SYM_INTEGER;
	            }
				else if (isalpha(ch) || ch=='_')
				{
					int oldid = idents-1;
                  length = 0;
					while (isalnum(ch) || ch=='_')
					{
						ch = tolower(ch);
                 		symtext[length++] = ch;
		            	nextchar();
        	      	}
					symtext[length]='\0';
					STRUPR(symtext);
					if (inDecl) symbol = SYM_DECL;
					else if (!var_ref && strncmp(symtext,"IF_",3)==0)
					{
						EM = (isDefined(symtext+3)>=0);
						symbol = SYM_UNDEF;
						continue;
					}
					else if (!var_ref && (t=isReserved(symtext))>=0)
					{
						symbol = (enum symboltype)(t);
					}
					else if (EM)
					{
						if ((IDindex = search(symtext,
							var_ref?VAR_TYPE:FUNC_TYPE)) > oldid)
								newID = 1;
	                	symbol = SYM_IDENTIFIER;
	               		symvalue = (int)IDindex;
					}
     			}
				else
				{
					if (!isspace(ch) && !feof(keydeffile))
						fprintf(stderr,"Warning: ignoring illegal character `%c' on line %d\n",ch,lineno);
					symbol = SYM_UNDEF;
					nextchar();
				}
				break;
         }
	}
}

#if __STDC__
static void expect(enum symboltype s)
#else
static void expect(s)
	enum symboltype s;
#endif
{
	if (symbol == s) nextsymbol();
	else 	{
		register enum errort e;
		switch(s) {
		case SYM_SEMICOLON:		e = ER_NOSEMICOLON;
						break;
		case SYM_RPARENTHESIS:		e = ER_NORPAR;
						break;
		case SYM_IDENTIFIER: 		e = ER_NOIDENT;
						break;
		case SYM_LPARENTHESIS:		e = ER_NOLPAR;
						break;
		case SYM_COMMA:			e = ER_NOCOMMA;
						break;
		case SYM_LBRACKET:		e = ER_NOLBRAK;
						break;
		case SYM_RBRACKET:		e = ER_NORBRAK;
						break;
		case SYM_STRING:		e = ER_NOSTRING;
						break;
		default: 			e = ER_SYNTAX;
		}
	error(e);
	}
}

/*******************************************
			PARSER
********************************************/

#define _emit(v)		(codespace[codep++] = (v))
#define CHK(n)			if ((codep+n)>=CODESPACE) error(ER_CODESPACE)
#define EMIT(v)			{ if (EM) { CHK(1); _emit(v); }}
#define EMIT2(v1,v2)		{ if (EM) { CHK(2); _emit(v1);_emit(v2);}}
#define EMIT3(v1,v2,v3)		{ if (EM) { CHK(3); _emit(v1);_emit(v2);_emit(v3);}}
#define EMIT4(v1,v2,v3,v4)	{ if (EM) { CHK(4); _emit(v1);_emit(v2);_emit(v3);_emit(v4);}}

#define newlabel()		(labelcnt++)

static short fnow;	/* current function being parsed	*/
int	np,		/* Number of parameters of the function */
	nv;		/* Number of local vars of the function */

/**********************************************************/

#if __STDC__
void config_file(void) {
#else
void config_file() {
#endif
	initvars();
	nextsymbol();
	inDecl = 0;
	trueString = -savestring("yY123456789"); /* Truth values */
	EMIT(OP_NOT) /* Dummy so we start at 1, and thus have a `NULL
			pointer' for functions using value 0 */
	for (;;) if (function()) break;
}

#if __STDC__
static int function(void) {
#else
static int function() {
#endif
	int i, paramsAllowed = 1;
	short nm=0;
	short initLocs[MAX_LOCVARS];
	np = nv = 0;
	switch (symbol) {
	case SYM_EOF:	return 1;
	case SYM_GCDEFAULT:
		if (defaultEntryPoint) error(ER_DEFREDEF);
		EMIT2(OP_LABEL,defaultEntryPoint = newlabel())
		paramsAllowed = 0;
		nm = savestring("GCDEFAULT");
		break;
	case SYM_GCSTART:
		if (startupEntryPoint) error(ER_STARTREDEF);
		EMIT2(OP_LABEL,startupEntryPoint = newlabel())
		paramsAllowed = 0;
		nm = savestring("GCSTART");
		break;
	case SYM_GCINIT:
		if (initialiseEntryPoint) error(ER_INITREDEF);
		EMIT2(OP_LABEL,initialiseEntryPoint = newlabel())
		paramsAllowed = 0;
		nm = savestring("GCINIT");
		break;
	case SYM_GCAFTERKEY:
		if (afterKeyEntryPoint) error(ER_AFTERREDEF);
		EMIT2(OP_LABEL,afterKeyEntryPoint = newlabel())
		paramsAllowed = 0;
		nm = savestring("GCAFTERKEY");
		break;
	case SYM_IDENTIFIER:
		if (objects[symvalue].p<0) {
			EMIT2(OP_LABEL, objects[fnow = symvalue].p = newlabel())
			objects[fnow].partyp = 0;
			if (objects[symvalue].type & VAR_TYPE)
				error(ER_FUNCEXP);
			nm = namemap[fnow];
		} else error(ER_REDEFINE); /* ID redefined */
		break;
	default: error(ER_FUNCEXP);
	}
	nextsymbol();
	inDecl = 1;
	if (symbol==SYM_LPARENTHESIS) {
		if (!paramsAllowed) error(ER_NOPARAM);
		nextsymbol();
		while (symbol==SYM_DECL) {
		    next_par:
			pnames[np++] = savestring(symtext);
			nextsymbol();
			if (np>=MAX_PARAMS)
				error(ER_MAXPARAMS);
			if (symbol==SYM_COMMA) {
				nextsymbol();
				goto next_par;
			}
		}
		expect(SYM_RPARENTHESIS);
	}
	inDecl = 0;
	if (paramsAllowed)
		objects[fnow].np = np;
	expect(SYM_LBRACE);
	if (symbol==SYM_LOCAL) {
		inDecl = 1;
		nextsymbol();
		while (symbol==SYM_DECL) {
		    next_var:
			if (nv>=MAX_LOCVARS) error(ER_LOCVARS);
			vnames[nv] = savestring(symtext);
			nextsymbol();
			if (symbol==SYM_ASSIGN) {
				inDecl = 0;
				nextsymbol();
				inDecl = 1;
				initLocs[nv] = getArg(1);
			} else initLocs[nv] = 0;
			nv++;
			if (symbol==SYM_COMMA) {
				nextsymbol();
				goto next_var;
			}
		}
		inDecl = 0;
		expect(SYM_SEMICOLON);
	}
	EMIT4(OP_FUNCTION,nm,np,nv);
	for (i=0;i<np;i++) EMIT(pnames[i]);
	for (i=0;i<nv;i++) EMIT(vnames[i]);
	for (i=0;i<nv;i++) {
		if (initLocs[i])
			EMIT3(OP_ASSIGN, TO_LOCAL(i), initLocs[i]);
	}
	command_list();
	/* Default prologue */
	EMIT2(OP_RETURN, 1)
	expect(SYM_RBRACE);
	return 0;
}
		
#if __STDC__
static void command_list(void) {
#else
static void command_list() {
#endif
	while (symbol!=SYM_RBRACE) {
		command();
		if (symbol==SYM_SEMICOLON) nextsymbol();
	}
}

#if __STDC__
static short getArg(int no_funcs_allowed) {
#else
static short getArg(no_funcs_allowed)
	int no_funcs_allowed;
{
#endif
	short rtn = (short)symvalue;
	if (symbol==SYM_STRING) rtn = string();
	else if (no_funcs_allowed &&
			(symbol!=SYM_IDENTIFIER ||
	       	  ( rtn>=0 && rtn<idents && (objects[rtn].type & FUNC_TYPE))))
			error(ER_VAREXP);
	else nextsymbol();
	return rtn;
}

#if __STDC__
static short getInteger(void) {
#else
static short getInteger() {
#endif
	short rtn = (short)symvalue;
	expect(SYM_INTEGER);
	return rtn;
}

#if __STDC__
static short getVar(void) {
#else
static short getVar() {
#endif
	short rtn=(short)symvalue;
	if (symbol != SYM_IDENTIFIER || !IS_VAR(rtn))
		error(ER_VAREXP);
	nextsymbol();
	return rtn;
}

#if __STDC__
static void command(void) {
#else
static void command() {
#endif
	short t1 = 0, t2 = 0, t3 = 0, idx = 0; /* '= 0' prevents gcc warnings */
	if (symbol==SYM_IDENTIFIER || symbol==SYM_STRING)
	{
		t1 = getArg(0);
		if (symbol==SYM_COLON && IS_VAR(t1))
		{
			nextsymbol();
			idx = getArg(1);
			if (!IS_VAR(idx)) error(ER_INDEX);
		}
		if (symbol==SYM_ASSIGN && IS_VAR(t1)) {
			if (IS_PARAM(t1)) {
				/* Must be a reference parameter */
				objects[fnow].partyp |= 1 << FROM_PARAM(t1);
			}
			nextsymbol();
			if (symbol==SYM_PROCWIN)
			{
				EMIT2(OP_PROCWIN,t1)
				nextsymbol();
			}
			else if (symbol==SYM_REGEXP)
				EMIT3(OP_SETMATCH,t1,regexp())
			else if (symbol==SYM_IDENTIFIER || symbol==SYM_STRING)
			{
				if (idx)
					EMIT4(OP_IASSIGN, t1, idx, getArg(1))
				else 
				{
					t2 = getArg(1);
					if (symbol == SYM_COLON)
					{
						nextsymbol();
						idx = getArg(1);
						if (!IS_VAR(t2) || !IS_VAR(idx)) error(ER_INDEX);
						EMIT4(OP_IASSIGN, t1, -idx, t2)
					}
					else
						EMIT3(OP_ASSIGN, t1, t2)
				}
			}
			else if (symbol==SYM_CAT)
			{
				int j, i = 0, v[4];
				nextsymbol();
				while (i<4)
				{
					v[i++] = getArg(1);
					if (symbol!=SYM_PLUS) break;
					nextsymbol();
				}
				EMIT3(OP_CAT,t1,i)
				for (j=0;j<i;j++) EMIT(v[j]);
				for (;j<4;j++) EMIT(0);
			}
			else
			{
				expression();
				EMIT2(OP_SETEXPR,t1)
			}
		}
		else call(t1);
	}
	else
	{
		enum symboltype cmdSym = symbol;
		int i;
		nextsymbol();
		/* First try the parser rules */
		for (i=0;ParseRules[i].sym;i++)
			if (cmdSym == ParseRules[i].sym)
			{
				int j = 1;
				enum codeops op;
				char *args = ops[op=ParseRules[i].op].link;
				EMIT(op);
				while (args[j])
				{
					switch (args[j])
					{
						case 'a': EMIT(getArg(1));
							break;
						case 'v': EMIT(getVar());
							break;
						case 'r': EMIT(regexp());
							break;
						case 'Z':
						case 'i':
							EMIT(getInteger());
							break;
						default:
							error(ER_INTERNAL);
					}
					j++;
				}
				break;
			}
		if (ParseRules[i].sym==0) switch(cmdSym)
		{
		case SYM_ATTRIBUTE:
			t1 = getArg(1);
			if (symbol == SYM_IS)
			{
				nextsymbol();
				/* Colour spec or numeric mono spec */
				t2 = getArg(1);
				if (symbol == SYM_ON)
				{
					expect(SYM_ON);
					EMIT4(OP_SETATTRIB,t1,t2,getArg(1))
				} else
					EMIT4(OP_SETATTRIB,t1,0,t2)
			}
			else EMIT2(OP_SELATTRIB,t1)
			break;
		case SYM_ACTION:
			t1 = getArg(1);
			if (newID) error(ER_UNDEFINED);
			if (objects[t2 = (short)symvalue].type & VAR_TYPE)
				error(ER_FUNCEXP);
			objects[t2].type |= REFERENCED;
			t2 = objects[t2].p;
			if (t2<0) error(ER_UNDEFINED);
			nextsymbol();
			EMIT3(OP_ACTION,t1,t2)
			break;
		case SYM_BIND:
			t1 = (short)symvalue;
			if (t1<0 || t1>=240) error(ER_BIND);
			expect(SYM_KEYSPEC);
			expect(SYM_TO);
			if (symbol==SYM_LBRACE) {
				EMIT2(OP_GOTO,t3=newlabel())
				EMIT2(OP_LABEL,t2=newlabel())
				EMIT4(OP_FUNCTION,fnow,0,0);
				nextsymbol();
				command_list();
				expect(SYM_RBRACE);
				EMIT2(OP_RETURN, 1)
				EMIT2(OP_LABEL,t3)
			} else {
				if (newID) error(ER_UNDEFINED);
				if (objects[t2 = (short)symvalue].type & VAR_TYPE)
					error(ER_FUNCEXP);
				objects[t2].type |= REFERENCED;
				t2 = objects[t2].p;
				if (t2<0) error(ER_UNDEFINED);
				nextsymbol();
			}
			EMIT4(OP_BIND,t1,t2,string())
			break;
		case SYM_BREAK:
			if (innerloop) 
				EMIT2(OP_BREAK,loopstack[innerloop-1][1])
			else error(ER_NOLOOP);
			break;
		case SYM_BROWSE:
			t1 = getArg(1);
			if (symbol==SYM_FROM)
			{
				nextsymbol();
				t2 = getArg(1);
			}
			else t2 = 0;
			EMIT3(OP_BROWSE,t1,t2);
			break;
		case SYM_LBRACE:
			command_list();
			expect(SYM_RBRACE);
			break;
		case SYM_CONTINUE:
			if (innerloop)
				EMIT2(OP_GOTO,loopstack[innerloop-1][0])
			else error(ER_NOLOOP);
			break;
		case SYM_COPY:
			t1 = getArg(1);
			EMIT4(OP_FCOPY,t1,getArg(1),0)
			break;
		case SYM_DO:
			do_cmd();
			break;
		case SYM_HEAD:
			stringcmd(OP_HEAD);
			break;
		case SYM_HELP:
			if (symbol==SYM_ALL) {
				EMIT(OP_HELPALL)
				nextsymbol();
			} else if (symbol==SYM_KEYSPEC) {
				EMIT2(OP_HELP,(short)symvalue)
				nextsymbol();
			} else EMIT(OP_HELPKEY)
			break;
		case SYM_IF:
			if_cmd();
			break;
		case SYM_INPATH:
			t1 = getArg(1);
			t2=(short)symvalue;
			if (symbol!=SYM_IDENTIFIER || !IS_VAR(t2)) t2 = 0;
			else nextsymbol();
			EMIT3(OP_INPATH,t1,t2);
			break;
		case SYM_KEYCODE:
			t1 = (short)symvalue;
			expect(SYM_KEYSPEC);
			expect(SYM_ASSIGN);
			/* this code is broken, as we only have a short... */
			EMIT4(OP_KEYCODE,t1,symvalue>>16, symvalue&0xFFFF)
			expect(SYM_INTEGER);
			break;
		case SYM_LOOP:
			loop_cmd();
			break;
		case SYM_MARK:
			if (symbol==SYM_REGEXP) EMIT2(OP_PATMARK,regexp())
			else EMIT2(OP_STRMARK, getArg(1))
			break;
		case SYM_METAKEY:
			/* This code is broken, as symvalue is 16 bits */
			EMIT3(OP_METAKEY,symvalue>>16, symvalue&0xFFFF)
			expect(SYM_INTEGER);
			break;
		case SYM_MOVE:
			t1 = getArg(1);
			EMIT4(OP_FCOPY,t1,getArg(1),1)
			break;
		case SYM_RADIO:
			t1 = getArg(1);
			if (symbol == SYM_INTEGER)
			{
				t2 = symvalue;
				nextsymbol();
			} else t2 = -1;
			EMIT3(OP_RADIO,t1,t2)
			break;
		case SYM_READ:
			read_cmd();
			break;
		case SYM_RETURN:
			if (symbol==SYM_INTEGER) {
				EMIT2(OP_RETURN,getInteger())
			} else if (symbol==SYM_IDENTIFIER) {
				t1 = getArg(0);
				if (IS_FUNC(t1)) {
					call(t1);
					EMIT2(OP_RETURN,-1)
				} else EMIT2(OP_RETVAR,getArg(1))
			} else error(ER_BADRETURN);
			break;
		case SYM_SEARCH:
			search_expr();
			break;
		case SYM_SHOW:
			show_cmd();
			break;
		case SYM_SORTBY:
			sort_cmd();
			break;
		case SYM_SWAP: 
			if (symbol==SYM_LEFT) {
				nextsymbol();
				EMIT2(OP_SWAP,1)
			} else if (symbol==SYM_RIGHT) {
				nextsymbol();
				EMIT2(OP_SWAP,2)
			} else EMIT2(OP_SWAP,0)
			break;
		case SYM_TAIL:
			stringcmd(OP_TAIL);
			break;
		case SYM_UNBIND:
			t1 = (short)symvalue;
			if (t1<0 || t1>=240) error(ER_BIND);
			EMIT2(OP_UNBIND,(short)symvalue)
			expect(SYM_KEYSPEC);
			break;
		case SYM_UNLINK:
			unlink_func();
			break;
		case SYM_UNMARK:
			if (symbol==SYM_REGEXP) EMIT2(OP_PATUNMARK,regexp())
			else EMIT2(OP_STRUNMARK,getArg(1))
			break;
		case SYM_WHILE:
			while_cmd();
			break;
		case SYM_YNTOGGLE:
			t1 = getArg(1);
			if (symbol == SYM_INTEGER)
			{
				t2 = symvalue;
				nextsymbol();
			} else t2 = -1;
			EMIT3(OP_YNTOGGLE,t1,t2)
			break;
		default:
			error(ER_BADCMD);
		}
	}
}

#if __STDC__
static void call(short f) {
#else
static void call(f)
	short f;
{
#endif
	short i, params[10], pcnt=0;
	if (symbol==SYM_LPARENTHESIS) {
		nextsymbol();
		while (symbol!=SYM_RPARENTHESIS) {
		   nextPar:
			if (pcnt>=MAX_PARAMS) error(ER_PARAMS);
			if (symbol==SYM_STRING) {
				if (f>=0 && f<idents && (objects[f].partyp & (1<<pcnt)))
					error(ER_REFPARM);
			}
			params[pcnt++] = getArg(1);
			if (symbol==SYM_COMMA) {
				nextsymbol();
				goto nextPar;
			}
		}
		nextsymbol();
	}
	EMIT3(OP_CALL,f,pcnt);
	if (f>=0 && f<idents)
	{
		if (pcnt != objects[f].np)
			error(ER_NUMPARMS);
		objects[f].type |= REFERENCED;
	}
	for (i=0;i<pcnt;i++) EMIT(params[i]);
}

#if __STDC__
static void read_cmd(void) {
#else
static void read_cmd() {
#endif
	short t;
	t = getVar();
	EMIT3(OP_READ,t,getInteger())
}

#if __STDC__
static void search_expr(void) {
#else
static void search_expr() {
#endif
	if (symbol==SYM_REGEXP) EMIT2(OP_PATSEARCH,regexp())
	else EMIT2(OP_SEARCH,getArg(1))
}

#if __STDC__
static void stringcmd(enum codeops op) {
#else
static void stringcmd(op)
	enum codeops op;
{
#endif
	short t, u;
	if (symbol==SYM_INTEGER) {
		t = getInteger();
		u = 1;
	} else {
		t = getArg(1);
		u = 0;
	}
	EMIT4(op,u,t,getVar())
}

#if __STDC__
static void unlink_func(void) {
#else
static void unlink_func() {
#endif
	if (symbol==SYM_REGEXP) EMIT3(OP_UNLINK,1,regexp())
	else EMIT3(OP_UNLINK,0,getArg(1))
}

#if __STDC__
static void loop_cmd(void) {
#else
static void loop_cmd() {
#endif
	short s=newlabel(), e = newlabel(), c = newlabel();
	if (symbol==SYM_LPARENTHESIS) {
		nextsymbol();
		EMIT4(OP_LOOPALL,getVar(),s,e)
		expect(SYM_RPARENTHESIS);
	} else EMIT4(OP_LOOPALL,0,s,e); 
	loopstack[innerloop][0] = c;
	loopstack[innerloop][1] = e;
	innerloop++;
	EMIT2(OP_LABEL,s)
	command();
	EMIT2(OP_LABEL,c)
	EMIT(OP_ENDLOOP)
	EMIT2(OP_LABEL,e)
	innerloop--;
}

#if __STDC__
static void while_cmd(void) {
#else
static void while_cmd() {
#endif
	short s=newlabel(), e = newlabel();
	loopstack[innerloop][0] = s;
	loopstack[innerloop][1] = e;
	innerloop++;
	EMIT2(OP_LABEL,s)
	expect(SYM_LPARENTHESIS);
	expression();
	EMIT2(OP_IF,e)
	expect(SYM_RPARENTHESIS);
	command();
	EMIT2(OP_GOTO,s)
	EMIT2(OP_LABEL,e)
	innerloop--;
}

#if __STDC__
static void do_cmd(void) {
#else
static void do_cmd() {
#endif
	short s=newlabel(), e = newlabel();
	loopstack[innerloop][0] = s;
	loopstack[innerloop][1] = e;
	innerloop++;
	EMIT2(OP_LABEL,s)
	command();
	expect(SYM_WHILE);
	expect(SYM_LPARENTHESIS);
	expression();
	EMIT2(OP_IF,e)
	expect(SYM_RPARENTHESIS);
	EMIT2(OP_GOTO,s)
	EMIT2(OP_LABEL,e)
	innerloop--;
}

#if __STDC__
static void if_cmd(void) {
#else
static void if_cmd() {
#endif
	short e1 = newlabel(), e2 = newlabel();
	expect(SYM_LPARENTHESIS);
	expression();
	EMIT2(OP_IF,e1)
	expect(SYM_RPARENTHESIS);
	command();
	if (symbol==SYM_ELSE) {
		EMIT2(OP_GOTO,e2)
		EMIT2(OP_LABEL,e1)
		nextsymbol();
		command();
		EMIT2(OP_LABEL,e2)
	}
	else
		EMIT2(OP_LABEL,e1)
}

#if __STDC__
static void show_cmd(void) {
#else
static void show_cmd() {
#endif
	short t=0;
	switch (symbol) {
	case SYM_NONE:	t=0;	break;
	case SYM_SIZE:	t=1;	break;
	case SYM_OWNER: t=2;	break;
	case SYM_GROUP: t=3;	break;
	case SYM_PERMS: t=4;	break;
	case SYM_MTIME: t=5;	break;
	case SYM_ATIME: t=6;	break;
	default: error(ER_SHOWWHAT);
	}
	EMIT2(OP_SHOW,t)
	nextsymbol();
}

#if __STDC__
static void sort_cmd(void) {
#else
static void sort_cmd() {
#endif
	int dir=0, key = 1;
	if (symbol==SYM_ASCENDING) nextsymbol();
	else if (symbol==SYM_DESCENDING) {
		nextsymbol();
		key = -1;
	} else dir = getArg(1);
	switch (symbol) {
	case SYM_NAME: key *= S_NAME; break;
	case SYM_SIZE: key *= S_SIZE; break;
	case SYM_GROUP: key *= S_GROUP; break;
	case SYM_OWNER: key *= S_OWNER; break;
	case SYM_MTIME: key *= S_MTIME; break;
	case SYM_ATIME: key *= S_ATIME; break;
	case SYM_EXTENS: key *= S_EXTENS; break;
	default: error(ER_NOKEY);
	}
	EMIT3(OP_SORT,dir,key)
	nextsymbol();
}

#if __STDC__
static short type_list(void) {
#else
static short type_list() {
#endif
	short f = 0;
	expect(SYM_LBRACE);
	for (;;) {
		switch(symbol) {
		case SYM_DIR:
			f |= 1; break;
		case SYM_PIPE:
			f |= 2; break;
		case SYM_CDEV:
			f |= 4; break;
		case SYM_BDEV:
			f |= 8; break;
		case SYM_REG:
			f |= 16; break;
		case SYM_EXEC:
			f |= 32; break;
		case SYM_LINK:
			f |= 64; break;
		case SYM_CONTAINER:
			f |= 128; break;
		default:
			error(ER_TYPELIST);
		}
		nextsymbol();
		if (symbol == SYM_COMMA) nextsymbol();
		else break;
	}
	expect(SYM_RBRACE);
	return f;
}

#if __STDC__
static short string(void) {
#else
static short string() {
#endif
	short rtn = -savestring(symtext);
	expect(SYM_STRING);
	return rtn;
}

#if __STDC__
static short regexp(void) {
#else
static short regexp() {
#endif
	short rtn = -savestring(symtext);
	expect(SYM_REGEXP);
	return rtn;
}

/*************** EXPRESSIONS *******************/

#if __STDC__
static void primary(void) {
#else
static void primary() {
#endif
	short t=0, v, w, neq=0;
	if (symbol==SYM_PLUS) {
		nextsymbol();
		neq=1;
		if (symbol!=SYM_COMPARE && symbol!=SYM_EVAL)
			error(ER_COMPEVALEXP);
	}
	switch(symbol) {
	case SYM_ACCESS:
		nextsymbol();
		v = getArg(1); /* Should change this to work with a perm string */
		EMIT3(OP_ACCESS,v,getArg(1))
		break;
	case SYM_COMPARE:
		nextsymbol();
		v = 0;
		switch (symbol) {
		case SYM_SIZE:		v++;
		case SYM_CONTENTS:	v++;
		case SYM_MTIME:		v++;
		case SYM_ATIME:		v++;
		case SYM_OWNER:		v++;
		case SYM_GROUP:		v++;
		case SYM_NAME:		v++;
					break;
		default:
			error(ER_COMPTYPE);
		}
		nextsymbol();
		w = getArg(1);
		EMIT4(OP_COMPARE, neq? (v+100) : v, w, getArg(1));
		break;
	case SYM_CONTAINS:
		nextsymbol();
		v = getArg(1);
		EMIT3(OP_CONTAINS,v,getArg(1));
		break;
	case SYM_EVAL:
		nextsymbol();
		EMIT3(OP_EVAL,neq,getArg(1))
		break;
	case SYM_LENGTH:
		nextsymbol();
		EMIT2(OP_LENGTH,getArg(1))
		break;
	case SYM_MARKED:
		nextsymbol();
		EMIT2(OP_MARKED,getArg(1));
		break;
	case SYM_MATCHES:
		nextsymbol();
		t = getArg(1);
		if (symbol==SYM_REGEXP) {
			v = OP_PATMATCH;
			w = regexp();
		} else {
			v = OP_STRMATCH;
			w = getArg(1);
		}
		EMIT4(v,t,w,symbol==SYM_INTEGER? getInteger() : 0);
		break;
	case SYM_STRING:
	case SYM_IDENTIFIER:
		v=getArg(0);
		if (symbol==SYM_IN) {
			nextsymbol();
			w = getArg(1);
			if (symbol==SYM_INTEGER) t = getInteger();
			else t = 1;
			EMIT4(OP_TEST,v,w,t)
		} else if (IS_FUNC(v) || symbol==SYM_LPARENTHESIS) call(v);
		else EMIT4(OP_TEST,v,trueString,1) /* implicit test */
		break;
	case SYM_TYPEOF:
		nextsymbol();
		t = getArg(1);
		expect(SYM_IN);
		EMIT3(OP_TYPEOF,t,type_list())
		break;
	default:
		command();
		break;
	}
}

#if __STDC__
static void simple_expression(void)
#else
static void simple_expression()
#endif
{
	if (symbol==SYM_LPARENTHESIS)
	{
		nextsymbol();
		expression();
		expect(SYM_RPARENTHESIS);
	} else
	{
		short n = 0;
		if (symbol==SYM_NOT)
		{
			nextsymbol();
			n = 1;
		}
		primary();
		if (n) EMIT(OP_NOT)
	}
}

#if __STDC__
static void and_expression(void)
#else
static void and_expression()
#endif
{
	simple_expression();
	if (symbol==SYM_AND)
	{
		short n = newlabel();
		EMIT2(OP_IF,n);
		nextsymbol();
		and_expression();
		EMIT2(OP_LABEL,n);
	}
}

#if __STDC__
static void expression(void)
#else
static void expression()
#endif
{
	and_expression();
	if (symbol==SYM_OR)
	{
		short n = newlabel(), m = newlabel();
		EMIT2(OP_IF,m);
		EMIT2(OP_GOTO,n);
		EMIT2(OP_LABEL,m);
		nextsymbol();
		expression();
		EMIT2(OP_LABEL,n);
	}
}

/**********************************
			LINKER/OPTIMISER
***********************************/

#define MAXLABELS	800

static short *linkTbl;

#define fixlabel(n)	codespace[i+n] = linkTbl[codespace[i+n]]

int errs=0;
short i=0, j=0;

#if __STDC__
int numParams(int p)
#else
int numParams(p)
	int p;
#endif
{
	int o = codespace[p];
	if (o==OP_LABEL) return 0;
	else if (o==OP_CALL) return (3+codespace[p+2]);
	else if (o==OP_FUNCTION) return (4+codespace[p+2]+codespace[p+3]);
	else return ops[o].cnt;
}

#define fixarg(n)	((void)0)	/* NOP; just done for clarity */

#if __STDC__
void fixcall(short p) {
#else
void fixcall(p)
	short p;
{
#endif
	if (objects[codespace[i+p]].p==-1) {
		errs++;
		fprintf(stderr,"Call to undefined function: %s\n",ID_NAME(codespace[i+p]));
	} else {
		int ip = objects[codespace[i+p]].p;
	        if (ip<0 || ip>=MAXLABELS)
			fprintf(stderr,"Invalid call object in CALL at %d - ip = %d\n",i,ip);
		else
			codespace[i+p] = linkTbl[ip];
	}
}

#if __STDC__
int gclink(void) {
#else
int gclink() {
#endif
	short k, n, o, d, change;
	if (labelcnt>=MAXLABELS) error(ER_LABELS);
	linkTbl = (short *)calloc(MAXLABELS,sizeof(short));
	if (linkTbl==NULL) error(ER_MALLOC);
	for (i=0;i<MAXLABELS;i++) linkTbl[i] = -1;

	/* Figure out the label <==> offset mapping... */

	for (i=j=0 ; i<codep ; i+=(d==0?2:d), j+=d) {
		d = numParams(i);
		if (d==0) linkTbl[codespace[i+1]] = j;
	}

	for (i=0 ; i<codep ; i+=(d==0)?2:d) {
		o = codespace[i];
		d = numParams(i);
		if (o==OP_CALL) {
			if (codespace[i+1]>=0) fixcall(1);
			n = codespace[i+2];
			for (k=0;k<n;k++) fixarg(3+k);
		} else if (o != OP_FUNCTION) {
			char *pg = ops[o].link;
			k = 0;
			while (pg[k]) {
				switch(pg[k]) {
				case 'a':
				case 'v':
				case 'V':
						fixarg(k);
					break;
#if 0  /* not used */
				case 'C':
					if (codespace[i+k]>=0)
						/* fall through */
				case 'c':
						fixcall(k);
					break;
#endif
				case 'l':
					fixlabel(k);
					break;
				case 'P':
					if (codespace[i+k-1]==0) fixarg(k);
				default:
					break;
				}
				k++;
			}
		}
	}
	/* 3rd pass - strip the labels */
	for (i=j=0;i<codep; i+=(n==0)?2:n, j+=n) {
		n = numParams(i);
		for (k=0;k<n;k++)
			codespace[j+k]=codespace[i+k];
	}
	codep = j;
	/* 4+th pass - fold multiple GOTOs */
	do {
		change = 0;
		for (i=0;i<codep; i+=n) {
			n = numParams(i);
			if (codespace[i]==OP_GOTO) {
				short dest = codespace[i+1];
				if (codespace[dest]==OP_GOTO) {
					change = 1;
					codespace[i+1] = codespace[dest+1];
				}
			}
		}
	} while (change);

	if (defaultEntryPoint)
		defaultEntryPoint = linkTbl[defaultEntryPoint];
	if (afterKeyEntryPoint)
		afterKeyEntryPoint = linkTbl[afterKeyEntryPoint];
	if (initialiseEntryPoint)
		initialiseEntryPoint = linkTbl[initialiseEntryPoint];
	if (startupEntryPoint)
		startupEntryPoint = linkTbl[startupEntryPoint];
	/* Fix up the symbol table references */
	for (i=0;i<idents;i++)
		if (objects[i].type&FUNC_TYPE)
		{
			objects[i].p = linkTbl[objects[i].p];
			if (verbose && (objects[i].type & REFERENCED) == 0)
				fprintf(stderr,"Function %s is not referenced\n",ID_NAME(i));
		}
	linked = (errs == 0);
	free(linkTbl);
	return errs;
}



