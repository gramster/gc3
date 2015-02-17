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


#ifndef _GCKEY_H
#define _GCKEY_H

#define MAX_KEYS	256
#define NUM_PRIM_KEYS	8	/* Primitive cursor ops		*/
#define NUM_CED_KEYS	18	/* Command line editing keys	*/
#define NUM_META_KEYS	16	/* Meta-key combos		*/
#define START_META_KEYS	(MAX_KEYS-NUM_CED_KEYS-NUM_META_KEYS)
#define START_CED_KEYS	(MAX_KEYS-NUM_CED_KEYS)

typedef struct {
	short ip;
	char  *hlp;
} keydef_t;

#if __MSDOS__

#define KEY_DOWN	0x5000   
#define KEY_UP	0x4800   
#define KEY_LEFT	0x4B00
#define KEY_RIGHT	0x4D00
#define KEY_PGUP	0x4900
#define KEY_PGDN	0x5100
#define KEY_PPAGE	0x4900	/* page up */
#define KEY_NPAGE	0x5100	/* page down */
#define KEY_SF		0x7600  /* ctrl-pgdn */
#define KEY_SR		0x8400	/* ctrl-pgup */
#define KEY_HOME	0x4700
#define KEY_END		0x4F00
#define KEY_HELP	0x5400	/* shift F1 */
#define KEY_DEL		0x5300
#define KEY_INS		0x5200
#define KEY_CTRL_HOME	0x7700
#define KEY_CTRL_END	0x7500
#define KEY_CTRL_LEFT	0x7300
#define KEY_CTRL_RIGHT	0x7400
#define KEY_CTRL_PGUP	0x8400
#define KEY_CTRL_PGDN	0x7600
#define KEY_BACKSPC	0x0008
#define KEY_F1		0x3B00
#define KEY_F2		0x3C00
#define KEY_F3		0x3D00
#define KEY_F4		0x3E00
#define KEY_F5		0x3F00
#define KEY_F6		0x4000
#define KEY_F7		0x4100
#define KEY_F8		0x4200
#define KEY_F9		0x4300
#define KEY_F0		0x4400
#define KEY_ALT_A	0x1E00
#define KEY_ALT_B	0x3000
#define KEY_ALT_C	0x2E00
#define KEY_ALT_D	0x2000
#define KEY_ALT_E	0x1200
#define KEY_ALT_F	0x2100
#define KEY_ALT_G	0x2200
#define KEY_ALT_H	0x2300
#define KEY_ALT_I	0x1700
#define KEY_ALT_J	0x2400
#define KEY_ALT_K	0x2500
#define KEY_ALT_L	0x2600
#define KEY_ALT_M	0x3200
#define KEY_ALT_N	0x3100
#define KEY_ALT_O	0x1800
#define KEY_ALT_P	0x1900
#define KEY_ALT_Q	0x1000
#define KEY_ALT_R	0x1300
#define KEY_ALT_S	0x1F00
#define KEY_ALT_T	0x1400
#define KEY_ALT_U	0x1600
#define KEY_ALT_V	0x2F00
#define KEY_ALT_W	0x1100
#define KEY_ALT_X	0x2D00
#define KEY_ALT_Y	0x1500
#define KEY_ALT_Z	0x2C00
#define KEY_ALT_0	0x8100
#define KEY_ALT_1	0x7800
#define KEY_ALT_2	0x7900
#define KEY_ALT_3	0x7A00
#define KEY_ALT_4	0x7B00
#define KEY_ALT_5	0x7C00
#define KEY_ALT_6	0x7D00
#define KEY_ALT_7	0x7E00
#define KEY_ALT_8	0x7F00
#define KEY_ALT_9	0x8000

#else /* not __MSDOS__ */
/*
 * Lots of curses key defs, for those whose curses doesn't define them.
 * You may want to modify the values if your curses is different; otherwise
 * you shouldn't need to change these.
 */

#ifndef KEY_DOWN
#define KEY_DOWN 0402	/* Sent by terminal left arrow key */
#endif

#ifndef KEY_UP
#define KEY_UP 0403	/* Sent by terminal left arrow key */
#endif

#ifndef KEY_LEFT
#define KEY_LEFT 0404	/* Sent by terminal left arrow key */
#endif

#ifndef KEY_RIGHT
#define KEY_RIGHT 0405	/* Sent by terminal right arrow key */
#endif

#ifndef KEY_PPAGE
#define KEY_PPAGE 0523	/* Previous page */
#endif

#ifndef KEY_NPAGE
#define KEY_NPAGE 0522	/* Next page */
#endif

#ifndef KEY_SF
#define KEY_SF 0520
#endif

#ifndef KEY_SR
#define KEY_SR 0521
#endif

#ifndef KEY_HOME
#define KEY_HOME 0406
#endif

#ifndef KEY_END
#define KEY_END	0550
#endif

#ifndef KEY_HELP
#define KEY_HELP 0553
#endif

#ifndef KEY_F0
#define KEY_F0 0410
#endif
#ifndef KEY_F1
#define KEY_F1 0411
#endif
#ifndef KEY_F2
#define KEY_F2 0412
#endif
#ifndef KEY_F3
#define KEY_F3 0413
#endif
#ifndef KEY_F4
#define KEY_F4 0414
#endif
#ifndef KEY_F5
#define KEY_F5 0415
#endif
#ifndef KEY_F6
#define KEY_F6 0416
#endif
#ifndef KEY_F7
#define KEY_F7 0417
#endif
#ifndef KEY_F8
#define KEY_F8 0420
#endif
#ifndef KEY_F9
#define KEY_F9 0421
#endif

#ifndef KEY_DEL
#define KEY_DEL	127
#endif

#ifndef KEY_INS
#define KEY_INS	0513
#endif

#ifndef KEY_PGDN
#define KEY_PGDN 0522
#endif

#ifndef KEY_PGUP
#define KEY_PGUP 0523
#endif

#endif /* not __MSDOS__ */

/* Command line editor key indices */

#define HELP_HOOK	238

#define CED_INSFNAME	239
#define CED_HOME	240
#define CED_END		241
#define CED_LEFT	242
#define CED_RIGHT	243
#define CED_DEL		244
#define CED_BKSPC	245
#define CED_MODE	246
#define CED_CLEAR	247
#define CED_DELLEFT	248
#define CED_DELRIGHT	249
#define CED_DELLWORD	250
#define CED_DELRWORD	251
#define CED_LWORD	252
#define CED_RWORD	253
#define CED_PREV	254
#define CED_NEXT	255

extern keydef_t KeyTbl[MAX_KEYS];

#if __STDC__

void InitialiseKeyTable(void);
char *getPrevCmd(char *prefix);
char *getNextCmd(char *prefix);
char *KeyName(int idx);
void	SetKeyCode(short key, short code);
short	GetKeyCode(short key);
int	my_getch(void);
int	EditString(int c, char *start, char **end, char **now,
			int *insmode, int maxlen);
void	cursorAction(int key);
int	GetKeyBinding(int key);
void	playMacro(short MNum);
void	StartRecordingMacro(short MNum);
void	StopRecordingMacro(void);
void	saveHist(char *b);
int	ExecuteKey(int i);

#else

void InitialiseKeyTable();
char *getPrevCmd();
char *getNextCmd();
char *KeyName();
void  SetKeyCode();
short GetKeyCode();
int	my_getch();
int	EditString();
void	cursorAction();
int	GetKeyBinding();
void	playMacro();
void	StartRecordingMacro();
void	StopRecordingMacro();
void	saveHist();
int	ExecuteKey();

#endif

#endif


