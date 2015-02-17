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


#ifndef _GCERR_H
#define _GCERR_H

enum errort {
	ER_REDEFINE, ER_UNDEFINED, ER_NOLOOP, ER_BADRETURN, ER_BADCMD,
	ER_RELOP, ER_STRING, ER_FKEY, ER_CTRLKEY, ER_METAKEY, 
	ER_CHARKEY, ER_KEYSPEC, ER_INTEGER, ER_NOSEMICOLON, ER_NORPAR,
	ER_NOIDENT, ER_NOLPAR, ER_NOCOMMA, ER_NOLBRAK, ER_NORBRAK,
	ER_NOOF, ER_SYNTAX, ER_NAMESTORE, ER_MALLOC, ER_VARS, 
	ER_KEYFILE, ER_CODEFILE, ER_SHOWWHAT, ER_LABELS, ER_LOOPTYPE,
	ER_VAREXP, ER_FUNCEXP, ER_SEARCH, ER_NOSTRING, ER_COLEXP,
	ER_DIGSPEC, ER_EXPOPT, ER_BADIFUNC, ER_EVALEXPR, ER_LINKER,
	ER_BIND, ER_CODESPACE, ER_NOKEY, ER_COMPTYPE, ER_COMPEVALEXP,
	ER_PARAMS, ER_NUMPARMS, ER_REFPARM, ER_DEFREDEF, ER_STARTREDEF,
	ER_INITREDEF, ER_NOINIT, ER_NODEFAULT, ER_NOSTART, ER_ONOFF,
	ER_NOPARAM, ER_MAXPARAMS, ER_LOCVARS, ER_IDS, ER_AFTERREDEF,
	ER_INTERNAL, ER_TYPELIST, ER_COMBOKEY, ER_EDITKEY, ER_MONOATTRIB,
	ER_INDEX
};

#if __STDC__
void error(enum errort typ);
#else
void error();
#endif

#endif
