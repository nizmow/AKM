/*
*
* Dieses Programm erzeugt eine Tastaturtabelle "KEYTABLS.SYS".
* Die Datei muû in gemsys/magic/xtension liegen.
*
* FÅr die DE-ISO-Tastatur
*
* Neue Version mit "dead keys"
*
*/

#include <mint/osbind.h>

static char tab_unshift[128] = {
 0,		/* 0x00 (  0): nicht belegt */
 0x1b,	/* 0x01 (  1): Esc */
 '1',	/* 0x02 (  2): '1' */
 '2',	/* 0x03 (  3): '2' */
 '3',	/* 0x04 (  4): '3' */
 '4',
 '5',
 '6',
 '7',
 '8',
 '9',
 '0',	/* 0x0b ( 11): '0' */
 0x9e,	/* 0x0c ( 12): 'û' */
 0xba,	/* 0x0d ( 13): '''     MF-2: acute accent */
 0x08,	/* 0x0e ( 14): Backspace */
 0x09,	/* 0x0f ( 15): Tabulator */
 'q',	/* 0x10 ( 16): 'q' */
 'w',
 'e',
 'r',
 't',
 'z',
 'u',
 'i',
 'o',
 'p',
 0x81,	/* 0x1a ( 26): 'Å' */
 '+',	/* 0x1b ( 27): '+' */
 0x0d,	/* 0x1c ( 28): Return */
 0,		/* 0x1d ( 29): Control */
 'a',
 's',
 'd',	/* 0x20 ( 32): d */
 'f',
 'g',
 'h',
 'j',	/* 0x24 ( 36): j */
 'k',	/* 0x25 ( 37): k */
 'l',	/* 0x26 ( 38): 'l' */
 0x94,	/* 0x27 ( 39): 'î' */
 0x84,	/* 0x28 ( 40): 'é' */
 '^',	/* 0x29 ( 41): '#  ^'			MF-2: '^ ¯' */
 0,		/* 0x2a ( 42): ShiftLinks */
 '#',	/* 0x2b ( 43): '~  |' 			MF-2: '# '' */
 'y',	/* 0x2c ( 44): 'y' */
 'x',	/* 0x2d ( 45): 'x' */
 'c',
 'v',
 'b',
 'n',
 'm',
 ',',	/* 0x33 ( 51): ', ;' */
 '.',	/* 0x34 ( 52): '. :' */
 '-',	/* 0x35 ( 53): '- _' */
 0,		/* 0x36 ( 54): ShiftRechts */
 0,		/* 0x37 ( 55): <unbelegt> */
 0,		/* 0x38 ( 56): Alternate */
 ' ',	/* 0x39 ( 57): SPACE */
 0,		/* 0x3a ( 58): CapsLock */
 0,		/* 0x3b ( 59): F1 */
 0,
 0,
 0,
 0,
 0,
 0,
 0,
 0,
 0,		/* 0x44 ( 68): F10 */
 0,		/* 0x45 ( 69): 				MF-2: F11 */
 0,		/* 0x46 ( 70):				MF-2: F12 */
 0,		/* 0x47 ( 71): Clr/Home */
 0,		/* 0x48 ( 72): CursorHoch */
 0,		/* 0x49 ( 73):				MF-2: BildHoch */
 '-',	/* 0x4a ( 74): Num-'-' */
 0,		/* 0x4b ( 75): CursorLinks */
 0,		/* 0x4c ( 76):				MF-2: AltGr */
 0,		/* 0x4d ( 77): CursorRechts */
 '+',	/* 0x4e ( 78): Num-'+' */
 0,		/* 0x4f ( 79):				MF-2: Ende */
 0,		/* 0x50 ( 80): CursorRunter */
 0,		/* 0x51 ( 81):				MF-2: BildRunter */
 0,		/* 0x52 ( 82): Insert			MF-2: Einfg */
 0x7f,	/* 0x53 ( 83): Delete			MF-2: Entf */
 0,		/* 0x54 ( 84): Shift-F1 */
 0,
 0,
 0,
 0,
 0,
 0,
 0,
 0,
 0,		/* 0x5d ( 93): Shift-F10 */
 0,		/* 0x5e ( 94):				MF-2: Shift-F11 */
 0,		/* 0x5f ( 95):				MF-2: Shift-F12 */
 '<',	/* 0x60 ( 96): '<  >' */
 0,		/* 0x61 ( 97): Undo */
 0,		/* 0x62 ( 98): Help */
 '(',	/* 0x63 ( 99): Num-'(' */
 ')',	/* 0x64 (100): Num-')' */
 '/',	/* 101: Num-'/' */
 '*',	/* 102: Num-'*' */
 '7',	/* 103: Num-'7' */
 '8',	/* 104: Num-'8' */
 '9',	/* 105: Num-'9' */
 '4',	/* 106: Num-'4' */
 '5',	/* 107: Num-'5' */
 '6',	/* 108: Num-'6' */
 '1',	/* 109: Num-'1' */
 '2',	/* 110: Num-'2' */
 '3',	/* 111: Num-'3' */
 '0',	/* 112: Num-'0' */
 '.',	/* 113: Num-'.'		MF-2: Num-',' */
 0x0d,	/* 0x72 (114): Num-Enter */
 0,		/* 115: Ctrl-CursorLinks */
 0,		/* 116: Ctrl-CursorRechts */
 0,		/* 117: <unbelegt> */
 0,		/* 118: <unbelegt> */
 0,		/* 119: Ctrl-Home */
 0,		/* 120: Alt-1 */
 0,		/* 121: Alt-2 */
 0,		/* 122: Alt-3 */
 0,		/* 123: Alt-4 */
 0,		/* 124: Alt-5 */
 0,		/* 125: Alt-6 */
 0,		/* 126: Alt-7 */
 0		/* 127: Alt-8 */
};

static const char tab_shift[128] = {
 0,
 0x1b,
 '!',
 '"',
 0xdd,	/*   4: › */
 '$',	/*   5: $ */
 '%',	/*   6: % */
 '&',	/*   7: & */
 '/',	/*   8: / */
 '(',	/*   9: ( */
 ')',	/*  10: ) */
 '=',	/*  11: = */
 '?',	/*  12: ? */
 0x60,	/*  13: ` */
 0x08,	/*  14: Backspace */
 0x09,	/*  15: Tab */
 'Q','W','E','R','T',
 'Z','U','I','O','P',
 0x9a,	/* 0x1a ( 26): ö */
 '*',	/* 0x1b ( 27): * */
 0x0d,	/* 0x1c ( 28): Return */
 0,		/* 0x1d ( 29): Control */
 'A','S','D','F','G','H','J','K','L',
 0x99,	/* 0x27 ( 39): 'ô' */
 0x8e,	/* 0x28 ( 40): 'é' */
 '¯',	/* 0x29 ( 41): '#  ^'			MF-2: '^ ¯' */
 0,		/* 0x2a ( 42): ShiftLinks */
 0x27,	/* 0x2b ( 43): '~  |' 			MF-2: '# '' */
 'Y','X','C','V','B','N',
 'M',	/* 0x32 ( 50): M */
 ';',	/* 0x33 ( 51): ', ;' */
 ':',	/* 0x34 ( 52): '. :' */
 '_',	/* 0x35 ( 53): '- _' */
 0,		/* 0x36 ( 54): ShiftRechts */
 0,		/* 0x37 ( 55): <unbelegt> */
 0,		/* 0x38 ( 56): Alternate */
 ' ',	/* 0x39 ( 57): SPACE */
 0,		/* 0x3a ( 58): CapsLock */
 0,		/* 0x3b ( 59): F1 */
 0,0,0,0,0,0,0,0,
 0,		/* 0x44 ( 68): F10 */
 0,		/* 0x45 ( 69): 				MF-2: F11 */
 0,		/* 0x46 ( 70):				MF-2: F12 */
 '7',	/* 0x47 ( 71): Clr/Home */
 '8',	/* 0x48 ( 72): CursorHoch */
 0,		/* 0x49 ( 73):				MF-2: BildHoch */
 '-',	/* 0x4a ( 74): Num-'-' */
 '4',	/* 0x4b ( 75): CursorLinks */
 0,		/* 0x4c ( 76):				MF-2: AltGr */
 '6',	/* 0x4d ( 77): CursorRechts */
 '+',	/* 0x4e ( 78): Num-'+' */
 0,		/* 0x4f ( 79):				MF-2: Ende */
 '2',	/* 0x50 ( 80): CursorRunter */
 0,		/* 0x51 ( 81):				MF-2: BildRunter */
 '0',	/* 0x52 ( 82): Insert			MF-2: Einfg */
 0x7f,	/* 0x53 ( 83): Delete			MF-2: Entf */
 0,		/* 0x54 ( 84): Shift-F1 */
 0,0,0,0,0,0,0,0,
 0,		/* 0x5d ( 93): Shift-F10 */
 0,		/* 0x5e ( 94):				MF-2: Shift-F11 */
 0,		/* 0x5f ( 95):				MF-2: Shift-F12 */
 '>',	/* 0x60 ( 96): '<  >' */
 0,		/* 0x61 ( 97): Undo */
 0,		/* 0x62 ( 98): Help */
 '(',')','/','*','7','8','9','4','5','6','1','2','3','0','.',
 0x0d,	/* 0x72 (114): Num-Enter */
 0,0,0,0,0,0,0,0,0,0,0,0,0
};

static const char tab_caps[128] = {
 0,		/* 0x00 (  0): nicht belegt */
 0x1b,	/* 0x01 (  1): Esc */
 '1',	/* 0x02 (  2): '1' */
 '2',	/* 0x03 (  3): '2' */
 '3',	/* 0x04 (  4): '3' */
 '4',
 '5',
 '6',
 '7',
 '8',
 '9',
 '0',	/* 0x0b ( 11): '0' */
 0x9e,	/* 0x0c ( 12): 'û' */
 0x27,	/* 0x0d ( 13): ''' */
 0x08,	/* 0x0e ( 14): Backspace */
 0x09,	/* 0x0f ( 15): Tabulator */
 'Q','W','E','R','T',
 'Z','U','I','O','P',
 0x9a,	/* 0x1a ( 26): ö */
 '+',	/* 0x1b ( 27): '+' */
 0x0d,	/* 0x1c ( 28): Return */
 0,		/* 0x1d ( 29): Control */
 'A','S','D','F','G','H','J','K','L',
 0x99,	/* 0x27 ( 39): 'ô' */
 0x8e,	/* 0x28 ( 40): 'é' */
 '^',	/* 0x29 ( 41): '#  ^'			MF-2: '^ ¯' */
 0,		/* 0x2a ( 42): ShiftLinks */
 '#',	/* 0x2b ( 43): '~  |' 			MF-2: '# '' */
 'Y','X','C','V','B','N','M',
 ',',	/* 0x33 ( 51): ', ;' */
 '.',	/* 0x34 ( 52): '. :' */
 '-',	/* 0x35 ( 53): '- _' */
 0,		/* 0x36 ( 54): ShiftRechts */
 0,		/* 0x37 ( 55): <unbelegt> */
 0,		/* 0x38 ( 56): Alternate */
 ' ',	/* 0x39 ( 57): SPACE */
 0,		/* 0x3a ( 58): CapsLock */
 0,		/* 0x3b ( 59): F1 */
 0,
 0,
 0,
 0,
 0,
 0,
 0,
 0,
 0,		/* 0x44 ( 68): F10 */
 0,		/* 0x45 ( 69): 				MF-2: F11 */
 0,		/* 0x46 ( 70):				MF-2: F12 */
 0,		/* 0x47 ( 71): Clr/Home */
 0,		/* 0x48 ( 72): CursorHoch */
 0,		/* 0x49 ( 73):				MF-2: BildHoch */
 '-',	/* 0x4a ( 74): Num-'-' */
 0,		/* 0x4b ( 75): CursorLinks */
 0,		/* 0x4c ( 76):				MF-2: AltGr */
 0,		/* 0x4d ( 77): CursorRechts */
 '+',	/* 0x4e ( 78): Num-'+' */
 0,		/* 0x4f ( 79):				MF-2: Ende */
 0,		/* 0x50 ( 80): CursorRunter */
 0,		/* 0x51 ( 81):				MF-2: BildRunter */
 0,		/* 0x52 ( 82): Insert			MF-2: Einfg */
 0x7f,	/* 0x53 ( 83): Delete			MF-2: Entf */
 0,		/* 0x54 ( 84): Shift-F1 */
 0,
 0,
 0,
 0,
 0,
 0,
 0,
 0,
 0,		/* 0x5d ( 93): Shift-F10 */
 0,		/* 0x5e ( 94):				MF-2: Shift-F11 */
 0,		/* 0x5f ( 95):				MF-2: Shift-F12 */
 '<',	/* 0x60 ( 96): '<  >' */
 0,		/* 0x61 ( 97): Undo */
 0,		/* 0x62 ( 98): Help */
 '(',	/* 0x63 ( 99): Num-'(' */
 ')',	/* 0x64 (100): Num-')' */
 '/',	/* 101: Num-'/' */
 '*',	/* 102: Num-'*' */
 '7',	/* 103: Num-'7' */
 '8',	/* 104: Num-'8' */
 '9',	/* 105: Num-'9' */
 '4',	/* 106: Num-'4' */
 '5',	/* 107: Num-'5' */
 '6',	/* 108: Num-'6' */
 '1',	/* 109: Num-'1' */
 '2',	/* 110: Num-'2' */
 '3',	/* 111: Num-'3' */
 '0',	/* 112: Num-'0' */
 '.',	/* 113: Num-'.'		MF-2: Num-',' */
 0x0d,	/* 0x72 (114): Num-Enter */
 0,		/* 115: Ctrl-CursorLinks */
 0,		/* 116: Ctrl-CursorRechts */
 0,		/* 117: <unbelegt> */
 0,		/* 118: <unbelegt> */
 0,		/* 119: Ctrl-Home */
 0,		/* 120: Alt-1 */
 0,		/* 121: Alt-2 */
 0,		/* 122: Alt-3 */
 0,		/* 123: Alt-4 */
 0,		/* 124: Alt-5 */
 0,		/* 125: Alt-6 */
 0,		/* 126: Alt-7 */
 0		/* 127: Alt-8 */
};

#define	XXX	0x20		/* darf nicht belegt werden */
#define	YYY	XXX		/* ist nicht belegt */

static const char tab_altgr[128] = {
     0, YYY,0xfc,0xfd,0xfe,0xfb,0xae,0xee,0x7b, '[', ']',0x7d,'\\',0xf4, YYY, YYY,
   '@',0x8a,0x82,   0,   0,0x97,0xa3,0xa1,0xa2,0x95,0xb1,0x7e, YYY, XXX,0xa0,0x85,
  0xb0,0x86,0xa6,   0,0xc0,0x8d,0xb3,0xb4,0x91,0xf0, XXX,0xf3,0x98,   0,0x87,   0,
     0,0xa4,0xe6,0xa9,0xfa,0xff, XXX,   0, XXX, YYY, XXX,0xc2,0xc3,0xc4,0xc5,0xc6,
  0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd, YYY, YYY,   0,0xbf, YYY,   0, YYY,0xf1,   0,
   YYY,   0, YYY, YYY,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0x7c,0xdb,0xda,   0,   0,0xf6,0xf8,0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,
     0,0xbd, YYY,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

static const char tab_shaltgr[128] = {
     0, YYY,0xad,0xab,0xbb,0xac,0xaf,0xef,0x9b,0x9c,0x9d,0xdf,0xa8,0xf5,   0,   0,
  0x89,0x88,0x90,   0,   0,0x96,0,0x8b,0xa7,0x93,0xb8,0xb9, YYY, XXX,0x83,0xb6,
  0xb7,0x8f,   0,   0,0xc1,0x8c,0xb2,0xb5,0x92,0xf7, XXX,0xf2,   0,   0,0x80,   0,
     0,0xa5,   0,0xaa,0x7f,0x9f, XXX,   0, XXX, YYY, XXX,0xce,0xcf,0xd0,0xd1,0xd2,
  0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9, YYY, YYY,0xbc,   0, YYY,   0, YYY,   0,   0,
   YYY,   0, YYY, YYY,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,0xdc,   0,   0,   0,0xf9,0xe9,0xea,0xeb,0xec,0xed,   0,   0,   0,   0,
     0,0xbe, YYY,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

static const char tab_alt[128] = {
  0,0,0,0,0,0,0,  0,  0,0,  0,0,0,0,0,0,
  0,0,0,0,0,0,0,  0,  0,0,'@',0,0,0,0,0,
  0,0,0,0,0,0,0,'[',']',0,  0,0,0,0,0,0,
  0,0,0,0,0,0,0,  0,  0,0,  0,0,0,0,0,0,
  0,0,0,0,0,0,0,  0,  0,0,  0,0,0,0,0,0,
  0,0,0,0,0,0,0,  0,  0,0,  0,0,0,0,0,0,
  0,0,0,0,0,0,0,  0,  0,0,  0,0,0,0,0,0,
  0,0,0,0,0,0,0,  0,  0,0,  0,0,0,0,0,0
};

static const char tab_shalt[128] = {
  0,0,0,0,0,0,0,  0,  0,0,   0,0,0,0,0,0,
  0,0,0,0,0,0,0,  0,  0,0,'\\',0,0,0,0,0,
  0,0,0,0,0,0,0,'{','}',0,   0,0,0,0,0,0,
  0,0,0,0,0,0,0,  0,  0,0,   0,0,0,0,0,0,
  0,0,0,0,0,0,0,  0,  0,0,   0,0,0,0,0,0,
  0,0,0,0,0,0,0,  0,  0,0,   0,0,0,0,0,0,
  0,0,0,0,0,0,0,  0,  0,0,   0,0,0,0,0,0,
  0,0,0,0,0,0,0,  0,  0,0,   0,0,0,0,0,0
};

#if 1

/* mit dead keys: */
static const char tab_dead[] = {
 '∫','`','^',0,
 'a','†','e','Ç','E','ê','i','°','o','¢','u','£',0,
 'a','Ö','A','∂','e','ä','i','ç','o','ï','u','ó',0,
 'a','É','e','à','i','å','o','ì','u','ñ',0,0
};

#else

/* ohne dead keys: */
static const char tab_dead[] = {
  0,0
};

#endif


int main( void )
{
	long ret;
	int hdl;

	ret = Fcreate("KEYTABLS.SYS", 0);
	hdl = (int) ret;
	if	(ret < 0)
		return(hdl);

	ret = Fwrite(hdl, 128L, tab_unshift);
	if	(ret != 128L)
		{
		 wr_err:
		Fclose(hdl);
		Fdelete("KEYTABLS.SYS");
		return(-1);
		}
	ret = Fwrite(hdl, 128L, tab_shift);
	if	(ret != 128L)
		goto wr_err;
	ret = Fwrite(hdl, 128L, tab_caps);
	if	(ret != 128L)
		goto wr_err;
	ret = Fwrite(hdl, 128L, tab_altgr);
	if	(ret != 128L)
		goto wr_err;
	ret = Fwrite(hdl, 128L, tab_shaltgr);
	if	(ret != 128L)
		goto wr_err;
	ret = Fwrite(hdl, 128L, tab_altgr);
	if	(ret != 128L)
		goto wr_err;
	ret = Fwrite(hdl, 128L, tab_alt);
	if	(ret != 128L)
		goto wr_err;
	ret = Fwrite(hdl, 128L, tab_shalt);
	if	(ret != 128L)
		goto wr_err;
	ret = Fwrite(hdl, 128L, tab_alt);
	if	(ret != 128L)
		goto wr_err;
	ret = Fwrite(hdl, sizeof(tab_dead), tab_dead);
	if	(ret != sizeof(tab_dead))
		goto wr_err;

	Fclose(hdl);
	return(0);
}
