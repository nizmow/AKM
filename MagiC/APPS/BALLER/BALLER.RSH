/* GEM Resource C Source */

#include <portab.h>
#include <aes.h>
#include "BALLER.H"

#if !defined(WHITEBAK)
#define WHITEBAK    0x0040
#endif
#if !defined(DRAW3D)
#define DRAW3D      0x0080
#endif

#define FLAGS9  0x0200
#define FLAGS10 0x0400
#define FLAGS11 0x0800
#define FLAGS12 0x1000
#define FLAGS13 0x2000
#define FLAGS14 0x4000
#define FLAGS15 0x8000
#define STATE8  0x0100
#define STATE9  0x0200
#define STATE10 0x0400
#define STATE11 0x0800
#define STATE12 0x1000
#define STATE13 0x2000
#define STATE14 0x4000
#define STATE15 0x8000

TEDINFO rs_tedinfo[] =
{ " Winkel:",
  "\0",
  "\0",
  IBM  , 0, TE_LEFT , 0x1180, 0, -1, 9, 1,
  " Pulver:",
  "\0",
  "\0",
  IBM  , 0, TE_LEFT , 0x1180, 0, -1, 9, 1,
  "von",
  "\0",
  "\0",
  SMALL, 0, TE_LEFT , 0x1180, 0, -1, 4, 1,
  "Erweiterte Version (10/89)",
  "\0",
  "\0",
  SMALL, 0, TE_CNTR , 0x1200, 0, -1, 27, 1,
  "____________________",
  "Spieler 1: ____________________",
  "nnnnnnnnnnnnnnnnnnnn",
  IBM  , 0, TE_CNTR , 0x1180, 0, -1, 21, 32,
  "____________________",
  "Spieler 2: ____________________",
  "nnnnnnnnnnnnnnnnnnnn",
  IBM  , 0, TE_CNTR , 0x1180, 0, -1, 21, 32,
  "1234567",
  "1. _______",
  "nnnnnnn",
  IBM  , 0, TE_CNTR , 0x1180, 0, -1, 8, 11,
  "1234567",
  "2. _______",
  "nnnnnnn",
  IBM  , 0, TE_CNTR , 0x1180, 0, -1, 8, 11,
  "1234567",
  "3. _______",
  "nnnnnnn",
  IBM  , 0, TE_CNTR , 0x1180, 0, -1, 8, 11,
  "1234567",
  "4. _______",
  "nnnnnnn",
  IBM  , 0, TE_CNTR , 0x1180, 0, -1, 8, 11,
  "1234567",
  "5. _______",
  "nnnnnnn",
  IBM  , 0, TE_CNTR , 0x1180, 0, -1, 8, 11,
  "1234567",
  "6. _______",
  "nnnnnnn",
  IBM  , 0, TE_CNTR , 0x1180, 0, -1, 8, 11,
  "100",
  "___",
  "999",
  IBM  , 0, TE_CNTR , 0x1180, 0, -1, 4, 4
};

OBJECT rs_object[] =
{ 
  /******** Tree 0 MENUE ****************************************************/
        -1,        1,        8, G_IBOX            ,   /* Object 0  */
  NONE, NORMAL, (LONG)0x00000000L,
  0x0000, 0x0000, 0x0050, 0x0019,
         8,        2,        2, G_BOX             ,   /* Object 1  */
  NONE, NORMAL, (LONG)0x00001100L,
  0x0000, 0x0000, 0x0050, 0x0201,
         1,        3,        7, G_IBOX            ,   /* Object 2  */
  NONE, NORMAL, (LONG)0x00000000L,
  0x0002, 0x0000, 0x0028, 0x0301,
         4,       -1,       -1, G_TITLE           ,   /* Object 3  */
  NONE, NORMAL, (LONG)" Desk ",
  0x0000, 0x0000, 0x0006, 0x0301,
         5,       -1,       -1, G_TITLE           ,   /* Object 4  */
  NONE, NORMAL, (LONG)" Spiel ",
  0x0006, 0x0000, 0x0007, 0x0301,
         6,       -1,       -1, G_TITLE           ,   /* Object 5  */
  NONE, NORMAL, (LONG)" Modus",
  0x000D, 0x0000, 0x0007, 0x0301,
         7,       -1,       -1, G_TITLE           ,   /* Object 6  */
  NONE, NORMAL, (LONG)" Regeln",
  0x0014, 0x0000, 0x0008, 0x0301,
         2,       -1,       -1, G_TITLE           ,   /* Object 7  */
  NONE, NORMAL, (LONG)" Ergebnisse ",
  0x001C, 0x0000, 0x000C, 0x0301,
         0,        9,       34, G_IBOX            ,   /* Object 8  */
  NONE, NORMAL, (LONG)0x00000000L,
  0x0000, 0x0301, 0x0050, 0x0017,
        18, MN_INFO         ,       17, G_BOX             ,   /* Object 9  */
  NONE, NORMAL, (LONG)0x00FF1100L,
  0x0002, 0x0000, 0x0014, 0x0008,
        11,       -1,       -1, G_STRING          ,   /* Object 10 MN_INFO */
  SELECTABLE, NORMAL, (LONG)"  Ballerburg Info   ",
  0x0000, 0x0000, 0x0014, 0x0001,
  MN_ACC1         ,       -1,       -1, G_STRING          ,   /* Object 11  */
  NONE, DISABLED, (LONG)"--------------------",
  0x0000, 0x0001, 0x0014, 0x0001,
        13,       -1,       -1, G_STRING          ,   /* Object 12 MN_ACC1 */
  NONE, NORMAL, (LONG)"  Desk Accessory 1  ",
  0x0000, 0x0002, 0x0014, 0x0001,
        14,       -1,       -1, G_STRING          ,   /* Object 13  */
  NONE, NORMAL, (LONG)"  Desk Accessory 2  ",
  0x0000, 0x0003, 0x0014, 0x0001,
        15,       -1,       -1, G_STRING          ,   /* Object 14  */
  NONE, NORMAL, (LONG)"  Desk Accessory 3  ",
  0x0000, 0x0004, 0x0014, 0x0001,
        16,       -1,       -1, G_STRING          ,   /* Object 15  */
  NONE, NORMAL, (LONG)"  Desk Accessory 4  ",
  0x0000, 0x0005, 0x0014, 0x0001,
        17,       -1,       -1, G_STRING          ,   /* Object 16  */
  NONE, NORMAL, (LONG)"  Desk Accessory 5  ",
  0x0000, 0x0006, 0x0014, 0x0001,
         9,       -1,       -1, G_STRING          ,   /* Object 17  */
  NONE, NORMAL, (LONG)"  Desk Accessory 6  ",
  0x0000, 0x0007, 0x0014, 0x0001,
        25, SPI_NEU         , SPI_ENDE        , G_BOX             ,   /* Object 18  */
  NONE, NORMAL, (LONG)0x00FF1100L,
  0x0008, 0x0000, 0x0017, 0x0006,
        20,       -1,       -1, G_STRING          ,   /* Object 19 SPI_NEU */
  NONE, NORMAL, (LONG)"  Neues Spiel...    ^N ",
  0x0000, 0x0000, 0x0017, 0x0001,
  SPI_NAM         ,       -1,       -1, G_STRING          ,   /* Object 20  */
  NONE, DISABLED, (LONG)"-----------------------",
  0x0000, 0x0001, 0x0017, 0x0001,
  SPI_OPT         ,       -1,       -1, G_STRING          ,   /* Object 21 SPI_NAM */
  NONE, NORMAL, (LONG)"  Namen der Spieler... ",
  0x0000, 0x0002, 0x0017, 0x0001,
        23,       -1,       -1, G_STRING          ,   /* Object 22 SPI_OPT */
  NONE, NORMAL, (LONG)"  Optionen...       \007E ",
  0x0000, 0x0003, 0x0017, 0x0001,
  SPI_ENDE        ,       -1,       -1, G_STRING          ,   /* Object 23  */
  NONE, DISABLED, (LONG)"-----------------------",
  0x0000, 0x0004, 0x0017, 0x0001,
        18,       -1,       -1, G_STRING          ,   /* Object 24 SPI_ENDE */
  NONE, NORMAL, (LONG)"  Ende              ^Q ",
  0x0000, 0x0005, 0x0017, 0x0001,
        32, M_MOD1          , M_MOD5          , G_BOX             ,   /* Object 25  */
  NONE, NORMAL, (LONG)0x00FF1100L,
  0x000F, 0x0000, 0x001B, 0x0006,
  M_MOD2          ,       -1,       -1, G_STRING          ,   /* Object 26 M_MOD1 */
  NONE, CHECKED, (LONG)"  Spieler  - Spieler    ^1 ",
  0x0000, 0x0000, 0x001B, 0x0001,
  M_MOD3          ,       -1,       -1, G_STRING          ,   /* Object 27 M_MOD2 */
  NONE, NORMAL, (LONG)"  Spieler  - Computer   ^2 ",
  0x0000, 0x0001, 0x001B, 0x0001,
  M_MOD4          ,       -1,       -1, G_STRING          ,   /* Object 28 M_MOD3 */
  NONE, NORMAL, (LONG)"  Computer - Spieler    ^3 ",
  0x0000, 0x0002, 0x001B, 0x0001,
        30,       -1,       -1, G_STRING          ,   /* Object 29 M_MOD4 */
  NONE, NORMAL, (LONG)"  Computer - Computer   ^4 ",
  0x0000, 0x0003, 0x001B, 0x0001,
  M_MOD5          ,       -1,       -1, G_STRING          ,   /* Object 30  */
  NONE, DISABLED, (LONG)"---------------------------",
  0x0000, 0x0004, 0x001B, 0x0001,
        25,       -1,       -1, G_STRING          ,   /* Object 31 M_MOD5 */
  NONE, NORMAL, (LONG)"  Computer ausw\204hlen... ^5 ",
  0x0000, 0x0005, 0x001B, 0x0001,
        34, M_HILFE         , M_HILFE         , G_BOX             ,   /* Object 32  */
  NONE, NORMAL, (LONG)0x00FF1100L,
  0x0016, 0x0000, 0x0014, 0x0001,
        32,       -1,       -1, G_STRING          ,   /* Object 33 M_HILFE */
  NONE, NORMAL, (LONG)"  Spielregeln... ^H ",
  0x0000, 0x0000, 0x0014, 0x0001,
         8, M_ERG1          , M_SHOWTB        , G_BOX             ,   /* Object 34  */
  NONE, NORMAL, (LONG)0x00FF1100L,
  0x001E, 0x0000, 0x0017, 0x0005,
  M_ERG2          ,       -1,       -1, G_STRING          ,   /* Object 35 M_ERG1 */
  NONE, NORMAL, (LONG)"  Namen eintragen...   ",
  0x0000, 0x0000, 0x0017, 0x0001,
  M_ERG3          ,       -1,       -1, G_STRING          ,   /* Object 36 M_ERG2 */
  NONE, NORMAL, (LONG)"  Tabelle laden        ",
  0x0000, 0x0001, 0x0017, 0x0001,
        38,       -1,       -1, G_STRING          ,   /* Object 37 M_ERG3 */
  NONE, NORMAL, (LONG)"  Tabelle speichern    ",
  0x0000, 0x0002, 0x0017, 0x0001,
  M_SHOWTB        ,       -1,       -1, G_STRING          ,   /* Object 38  */
  NONE, DISABLED, (LONG)"-----------------------",
  0x0000, 0x0003, 0x0017, 0x0001,
        34,       -1,       -1, G_STRING          ,   /* Object 39 M_SHOWTB */
  LASTOB, NORMAL, (LONG)"  Tabelle zeigen... ^T ",
  0x0000, 0x0004, 0x0017, 0x0001,
  
  /******** Tree 1 T_SCHUSS ****************************************************/
        -1, SOK             ,       11, G_BOX             ,   /* Object 0  */
  NONE, NORMAL, (LONG)0x00031141L,
  0x0000, 0x0000, 0x0327, 0x0A0D,
  SAB             ,       -1,       -1, G_BUTTON          ,   /* Object 1 SOK */
  SELECTABLE|DEFAULT|EXIT, NORMAL, (LONG)"OK",
  0x051D, 0x020A, 0x0008, 0x0001,
  KAST            ,       -1,       -1, G_BUTTON          ,   /* Object 2 SAB */
  SELECTABLE|EXIT, NORMAL, (LONG)"Abbruch",
  0x051D, 0x0D0B, 0x0008, 0x0001,
         5,        4,        4, G_IBOX            ,   /* Object 3 KAST */
  NONE, DISABLED, (LONG)0x00FD1080L,
  0x0102, 0x0304, 0x0719, 0x0A08,
  KAST            ,       -1,       -1, G_BOX             ,   /* Object 4  */
  NONE, NORMAL, (LONG)0x00FF1151L,
  0x0000, 0x0607, 0x0619, 0x0301,
        11, WINK            , WR2             , G_BOXTEXT         ,   /* Object 5  */
  NONE, NORMAL, (LONG)&rs_tedinfo[0],
  0x0002, 0x0800, 0x0021, 0x0801,
  WR1             ,       -1,       -1, G_BUTTON          ,   /* Object 6 WINK */
  NONE, NORMAL, (LONG)"000",
  0x0212, 0x0400, 0x0005, 0x0001,
  WL1             ,       -1,       -1, G_BOXCHAR         ,   /* Object 7 WR1 */
  TOUCHEXIT, NORMAL, (LONG) ((LONG)'\003' << 24)|0x00FF1100L,
  0x0617, 0x0400, 0x0202, 0x0001,
  WL2             ,       -1,       -1, G_BOXCHAR         ,   /* Object 8 WL1 */
  TOUCHEXIT, NORMAL, (LONG) ((LONG)'\004' << 24)|0x00FF1100L,
  0x040F, 0x0400, 0x0202, 0x0001,
  WR2             ,       -1,       -1, G_BUTTON          ,   /* Object 9 WL2 */
  TOUCHEXIT, NORMAL, (LONG)"\004\004",
  0x000C, 0x0400, 0x0003, 0x0001,
         5,       -1,       -1, G_BUTTON          ,   /* Object 10 WR2 */
  TOUCHEXIT, NORMAL, (LONG)"\003\003",
  0x041A, 0x0400, 0x0702, 0x0001,
         0, PULV            , PR2             , G_BOXTEXT         ,   /* Object 11  */
  NONE, NORMAL, (LONG)&rs_tedinfo[1],
  0x0002, 0x0502, 0x0021, 0x0801,
  PL1             ,       -1,       -1, G_BUTTON          ,   /* Object 12 PULV */
  NONE, NORMAL, (LONG)"000",
  0x0212, 0x0400, 0x0005, 0x0001,
  PR1             ,       -1,       -1, G_BOXCHAR         ,   /* Object 13 PL1 */
  TOUCHEXIT, NORMAL, (LONG) ((LONG)'\004' << 24)|0x00FF1100L,
  0x040F, 0x0400, 0x0202, 0x0001,
  PL2             ,       -1,       -1, G_BOXCHAR         ,   /* Object 14 PR1 */
  TOUCHEXIT, NORMAL, (LONG) ((LONG)'\003' << 24)|0x00FF1100L,
  0x0617, 0x0400, 0x0202, 0x0001,
  PR2             ,       -1,       -1, G_BUTTON          ,   /* Object 15 PL2 */
  TOUCHEXIT, NORMAL, (LONG)"\004\004",
  0x010C, 0x0400, 0x0702, 0x0001,
        11,       -1,       -1, G_BUTTON          ,   /* Object 16 PR2 */
  LASTOB|TOUCHEXIT, NORMAL, (LONG)"\003\003",
  0x041A, 0x0400, 0x0702, 0x0001,
  
  /******** Tree 2 T_INFO ****************************************************/
        -1,        1,       35, G_BOX             ,   /* Object 0  */
  NONE, OUTLINED, (LONG)0x65031121L,
  0x0000, 0x0000, 0x004B, 0x0015,
         8,        2,        7, G_BOX             ,   /* Object 1  */
  NONE, SHADOWED, (LONG)0x00FD1181L,
  0x0701, 0x0001, 0x001C, 0x0007,
         3,       -1,       -1, G_STRING          ,   /* Object 2  */
  NONE, NORMAL, (LONG)"B A L L E R B U R G",
  0x0304, 0x0900, 0x0013, 0x0001,
         4,       -1,       -1, G_STRING          ,   /* Object 3  */
  NONE, NORMAL, (LONG)"Eckhard Kruse",
  0x0007, 0x0403, 0x000D, 0x0001,
         5,       -1,       -1, G_STRING          ,   /* Object 4  */
  NONE, NORMAL, (LONG)"Reichenberweg 7",
  0x0007, 0x0504, 0x000F, 0x0001,
         6,       -1,       -1, G_STRING          ,   /* Object 5  */
  NONE, NORMAL, (LONG)"D-3302 Weddel",
  0x0007, 0x0705, 0x000D, 0x0001,
         7,       -1,       -1, G_TEXT            ,   /* Object 6  */
  NONE, NORMAL, (LONG)&rs_tedinfo[2],
  0x0007, 0x0B02, 0x0202, 0x0600,
         1,       -1,       -1, G_BOXTEXT         ,   /* Object 7  */
  NONE, OUTLINED, (LONG)&rs_tedinfo[3],
  0x0104, 0x0E01, 0x0513, 0x0800,
         9,       -1,       -1, G_BUTTON          ,   /* Object 8  */
  EXIT, SHADOWED, (LONG)"Dieses Programm ist frei kopierbar !",
  0x0020, 0x0001, 0x0028, 0x0001,
        10,       -1,       -1, G_STRING          ,   /* Object 9  */
  NONE, NORMAL, (LONG)"\0",
  0x0020, 0x0003, 0x0006, 0x0001,
        11,       -1,       -1, G_STRING          ,   /* Object 10  */
  NONE, NORMAL, (LONG)"Dieses Programm darf an  jeden weiterge-",
  0x0020, 0x0003, 0x0028, 0x0001,
        12,       -1,       -1, G_STRING          ,   /* Object 11  */
  NONE, NORMAL, (LONG)"geben  werden. Voraussetzung  daf\201r ist",
  0x0020, 0x0004, 0x0027, 0x0001,
        13,       -1,       -1, G_STRING          ,   /* Object 12  */
  NONE, NORMAL, (LONG)"allerdings, da\236 sich niemand damit durch",
  0x0020, 0x0005, 0x0028, 0x0001,
        14,       -1,       -1, G_STRING          ,   /* Object 13  */
  NONE, NORMAL, (LONG)"Verkauf bereichert und da\236 das Programm",
  0x0020, 0x0006, 0x0027, 0x0001,
        15,       -1,       -1, G_STRING          ,   /* Object 14  */
  NONE, NORMAL, (LONG)"unver\204ndert bleibt.",
  0x0020, 0x0007, 0x0013, 0x0001,
        16,       -1,       -1, G_STRING          ,   /* Object 15  */
  NONE, NORMAL, (LONG)"Sollte Ihnen das Programm  gefallen, so ",
  0x0002, 0x0009, 0x0028, 0x0001,
        17,       -1,       -1, G_STRING          ,   /* Object 16  */
  NONE, NORMAL, (LONG)"bitte ich Sie, mir - sozusagen",
  0x002A, 0x0009, 0x001E, 0x0001,
        18,       -1,       -1, G_STRING          ,   /* Object 17  */
  NONE, NORMAL, (LONG)"als Anerkennung - 20 DM zu schicken. Hab",
  0x0002, 0x000A, 0x0028, 0x0001,
        19,       -1,       -1, G_STRING          ,   /* Object 18  */
  NONE, NORMAL, (LONG)"en Sie ferner Interesse an dem",
  0x002A, 0x000A, 0x001E, 0x0001,
        20,       -1,       -1, G_STRING          ,   /* Object 19  */
  NONE, NORMAL, (LONG)"Sourcecode ( C ), so k\224nnen Sie einen fr",
  0x0002, 0x000B, 0x0028, 0x0001,
        21,       -1,       -1, G_STRING          ,   /* Object 20  */
  NONE, NORMAL, (LONG)"ankierten R\201ckumschlag und ei-",
  0x002A, 0x000B, 0x001E, 0x0001,
        22,       -1,       -1, G_STRING          ,   /* Object 21  */
  NONE, NORMAL, (LONG)"ne einseitig formatierte Diskette beileg",
  0x0002, 0x000C, 0x0028, 0x0001,
        23,       -1,       -1, G_STRING          ,   /* Object 22  */
  NONE, NORMAL, (LONG)"en. Auf dieser w\201rde ich Ihnen",
  0x002A, 0x000C, 0x001E, 0x0001,
        24,       -1,       -1, G_STRING          ,   /* Object 23  */
  NONE, NORMAL, (LONG)"dann folgendes speichern:",
  0x0002, 0x000D, 0x0019, 0x0001,
        25,       -1,       -1, G_STRING          ,   /* Object 24  */
  NONE, NORMAL, (LONG)"- den eigentlichen Sourcecode ( bestehen",
  0x0002, 0x000E, 0x0028, 0x0001,
        26,       -1,       -1, G_STRING          ,   /* Object 25  */
  NONE, NORMAL, (LONG)"d aus zwei Teilen )",
  0x002A, 0x000E, 0x0013, 0x0001,
        27,       -1,       -1, G_STRING          ,   /* Object 26  */
  NONE, NORMAL, (LONG)"- den Programmteil zum Laden und Abspiel",
  0x0002, 0x000F, 0x0028, 0x0001,
        28,       -1,       -1, G_STRING          ,   /* Object 27  */
  NONE, NORMAL, (LONG)"en der Musik-Editor Datei",
  0x002A, 0x000F, 0x0019, 0x0001,
        29,       -1,       -1, G_STRING          ,   /* Object 28  */
  NONE, NORMAL, (LONG)"- eine ausf\201hrlichere Spielbeschreibung",
  0x0002, 0x0010, 0x0027, 0x0001,
        30,       -1,       -1, G_STRING          ,   /* Object 29  */
  NONE, NORMAL, (LONG)"en der Musik-Editor-Datei",
  0x002A, 0x000F, 0x0019, 0x0001,
        31,       -1,       -1, G_STRING          ,   /* Object 30  */
  NONE, NORMAL, (LONG)"- Informationen, wie Sie  durch Erweiter",
  0x0002, 0x0011, 0x0028, 0x0001,
        32,       -1,       -1, G_STRING          ,   /* Object 31  */
  NONE, NORMAL, (LONG)"n  der BALLER.DAT-Datei leicht",
  0x002A, 0x0011, 0x001E, 0x0001,
        33,       -1,       -1, G_STRING          ,   /* Object 32  */
  NONE, NORMAL, (LONG)"  eigene Burgen kreieren k\224nnen",
  0x0002, 0x0012, 0x001F, 0x0001,
        34,       -1,       -1, G_STRING          ,   /* Object 33  */
  NONE, NORMAL, (LONG)"So, das w\204r\047s - und noch viel Spa\236 beim ",
  0x0002, 0x0013, 0x0028, 0x0001,
        35,       -1,       -1, G_STRING          ,   /* Object 34  */
  NONE, NORMAL, (LONG)"Spielen!",
  0x002A, 0x0013, 0x0008, 0x0001,
         0,       -1,       -1, G_BUTTON          ,   /* Object 35  */
  SELECTABLE|DEFAULT|EXIT|LASTOB, NORMAL, (LONG)"OK",
  0x0035, 0x0013, 0x0014, 0x0001,
  
  /******** Tree 3 T_BURG ****************************************************/
        -1, BSP1            ,       11, G_BOX             ,   /* Object 0  */
  NONE, OUTLINED, (LONG)0x750311C1L,
  0x0000, 0x0000, 0x0642, 0x0A0F,
  BSP2            ,       -1,       -1, G_BUTTON          ,   /* Object 1 BSP1 */
  NONE, SHADOWED, (LONG)"Spieler 1",
  0x0002, 0x0002, 0x001F, 0x0001,
  BL1             ,       -1,       -1, G_BUTTON          ,   /* Object 2 BSP2 */
  NONE, SHADOWED, (LONG)"Spieler 2",
  0x0022, 0x0002, 0x001F, 0x0001,
  BK1             ,       -1,       -1, G_BOXCHAR         ,   /* Object 3 BL1 */
  TOUCHEXIT, NORMAL, (LONG) ((LONG)'\004' << 24)|0x00FE1100L,
  0x0002, 0x000C, 0x000F, 0x0001,
  BK2             ,       -1,       -1, G_BOX             ,   /* Object 4 BK1 */
  NONE, NORMAL, (LONG)0x00FE1100L,
  0x0002, 0x0803, 0x001F, 0x0008,
  BR1             ,       -1,       -1, G_BOX             ,   /* Object 5 BK2 */
  NONE, NORMAL, (LONG)0x00FE1100L,
  0x0022, 0x0803, 0x001F, 0x0008,
  BL2             ,       -1,       -1, G_BOXCHAR         ,   /* Object 6 BR1 */
  TOUCHEXIT, NORMAL, (LONG) ((LONG)'\003' << 24)|0x00FE1100L,
  0x0012, 0x000C, 0x000F, 0x0001,
  BR2             ,       -1,       -1, G_BOXCHAR         ,   /* Object 7 BL2 */
  TOUCHEXIT, NORMAL, (LONG) ((LONG)'\004' << 24)|0x00FE1100L,
  0x0022, 0x000C, 0x000F, 0x0001,
  BAB             ,       -1,       -1, G_BOXCHAR         ,   /* Object 8 BR2 */
  TOUCHEXIT, NORMAL, (LONG) ((LONG)'\003' << 24)|0x00FE1100L,
  0x0032, 0x000C, 0x000F, 0x0001,
  BOK             ,       -1,       -1, G_BUTTON          ,   /* Object 9 BAB */
  SELECTABLE|EXIT, NORMAL, (LONG)"Abbruch",
  0x0039, 0x0F0D, 0x0008, 0x0001,
        11,       -1,       -1, G_BUTTON          ,   /* Object 10 BOK */
  SELECTABLE|DEFAULT|EXIT, NORMAL, (LONG)"OK",
  0x022F, 0x000E, 0x0008, 0x0001,
         0,       -1,       -1, G_BUTTON          ,   /* Object 11  */
  LASTOB, SHADOWED, (LONG)"Neues Spiel: W\204hlen Sie die Burgen !",
  0x060B, 0x0700, 0x032B, 0x0001,
  
  /******** Tree 4 DRAN ****************************************************/
        -1,        1, DOK             , G_BOX             ,   /* Object 0  */
  NONE, NORMAL, (LONG)0x00031141L,
  0x0000, 0x0000, 0x0717, 0x0E07,
  DWIN            , DNAM            ,        3, G_BOX             ,   /* Object 1  */
  NONE, NORMAL, (LONG)0x00FF1100L,
  0x0001, 0x0700, 0x0016, 0x0002,
         3,       -1,       -1, G_STRING          ,   /* Object 2 DNAM */
  NONE, NORMAL, (LONG)"12345678901234567890",
  0x0001, 0x0000, 0x0014, 0x0001,
         1,       -1,       -1, G_STRING          ,   /* Object 3  */
  NONE, NORMAL, (LONG)"ist am Zug...",
  0x0001, 0x0001, 0x000D, 0x0001,
         5,       -1,       -1, G_BUTTON          ,   /* Object 4 DWIN */
  NONE, NORMAL, (LONG)"\004 12 \004",
  0x0001, 0x0304, 0x0008, 0x0001,
  DWBX            ,       -1,       -1, G_BUTTON          ,   /* Object 5  */
  NONE, NORMAL, (LONG)"Wind:",
  0x0001, 0x0D02, 0x0008, 0x0001,
  DOK             ,       -1,       -1, G_BOX             ,   /* Object 6 DWBX */
  NONE, NORMAL, (LONG)0x00FF1100L,
  0x000A, 0x0D02, 0x000D, 0x0003,
         0,       -1,       -1, G_BUTTON          ,   /* Object 7 DOK */
  SELECTABLE|EXIT|LASTOB, NORMAL, (LONG)"Fertig",
  0x0700, 0x0506, 0x0116, 0x0101,
  
  /******** Tree 5 NAMEN ****************************************************/
        -1,        1,        5, G_BOX             ,   /* Object 0  */
  NONE, OUTLINED, (LONG)0x45031141L,
  0x0000, 0x0000, 0x0029, 0x0009,
  NSP1            ,       -1,       -1, G_STRING          ,   /* Object 1  */
  NONE, NORMAL, (LONG)"Geben Sie die Namen der Spieler ein:",
  0x0002, 0x0001, 0x0025, 0x0001,
  NSP2            ,       -1,       -1, G_FBOXTEXT        ,   /* Object 2 NSP1 */
  EDITABLE, NORMAL, (LONG)&rs_tedinfo[4],
  0x0002, 0x0003, 0x0025, 0x0001,
  NOK             ,       -1,       -1, G_FBOXTEXT        ,   /* Object 3 NSP2 */
  EDITABLE, NORMAL, (LONG)&rs_tedinfo[5],
  0x0002, 0x0005, 0x0025, 0x0001,
         5,       -1,       -1, G_BUTTON          ,   /* Object 4 NOK */
  SELECTABLE|DEFAULT|EXIT, NORMAL, (LONG)"OK",
  0x0015, 0x0007, 0x0008, 0x0001,
         0,       -1,       -1, G_BUTTON          ,   /* Object 5  */
  SELECTABLE|EXIT|LASTOB, NORMAL, (LONG)"Abbruch",
  0x001F, 0x0007, 0x0008, 0x0001,
  
  /******** Tree 6 STATUS ****************************************************/
        -1,        1,       14, G_BOX             ,   /* Object 0  */
  NONE, NORMAL, (LONG)0x00031141L,
  0x0000, 0x0000, 0x002D, 0x000E,
  FERTIG          ,        2, SHG             , G_BOX             ,   /* Object 1  */
  NONE, NORMAL, (LONG)0x00FF1100L,
  0x0001, 0x0001, 0x002B, 0x0005,
  ST_GELD         ,       -1,       -1, G_STRING          ,   /* Object 2  */
  NONE, NORMAL, (LONG)"Du hast:",
  0x0011, 0x0000, 0x0008, 0x0001,
  ST_FOERD        ,       -1,       -1, G_STRING          ,   /* Object 3 ST_GELD */
  NONE, NORMAL, (LONG)"Geld       10000",
  0x0002, 0x0001, 0x0010, 0x0001,
  ST_GESCH        ,       -1,       -1, G_STRING          ,   /* Object 4 ST_FOERD */
  NONE, NORMAL, (LONG)"F\224rdert\201rme   10",
  0x0019, 0x0001, 0x0010, 0x0001,
  ST_FAHNE        ,       -1,       -1, G_STRING          ,   /* Object 5 ST_GESCH */
  NONE, NORMAL, (LONG)"Gesch\201tze      5",
  0x0002, 0x0002, 0x0010, 0x0001,
  ST_PULVR        ,       -1,       -1, G_STRING          ,   /* Object 6 ST_FAHNE */
  NONE, NORMAL, (LONG)"Windfahne      1",
  0x0019, 0x0002, 0x0010, 0x0001,
  ST_KUGEL        ,       -1,       -1, G_STRING          ,   /* Object 7 ST_PULVR */
  NONE, NORMAL, (LONG)"Pulver       100",
  0x0002, 0x0003, 0x0010, 0x0001,
  ST_VOLK         ,       -1,       -1, G_STRING          ,   /* Object 8 ST_KUGEL */
  NONE, NORMAL, (LONG)"Kugeln        15",
  0x0019, 0x0003, 0x0010, 0x0001,
  ST_STEUR        ,       -1,       -1, G_STRING          ,   /* Object 9 ST_VOLK */
  NONE, NORMAL, (LONG)"Volk         200",
  0x0002, 0x0004, 0x0010, 0x0001,
  SHG             , SHK             , SHK             , G_STRING          ,   /* Object 10 ST_STEUR */
  NONE, NORMAL, (LONG)"Steuern [%]  100",
  0x0019, 0x0004, 0x0010, 0x0001,
  ST_STEUR        ,       -1,       -1, G_BOXCHAR         ,   /* Object 11 SHK */
  TOUCHEXIT, NORMAL, (LONG) ((LONG)'\002' << 24)|0x00FF1100L,
  0x070B, 0x0000, 0x0001, 0x0001,
         1,       -1,       -1, G_BOXCHAR         ,   /* Object 12 SHG */
  TOUCHEXIT, NORMAL, (LONG) ((LONG)'\001' << 24)|0x00FF1100L,
  0x0129, 0x0004, 0x0001, 0x0001,
        14,       -1,       -1, G_BUTTON          ,   /* Object 13 FERTIG */
  SELECTABLE|DEFAULT|EXIT, NORMAL, (LONG)"Fertig",
  0x0001, 0x000C, 0x002B, 0x0001,
         0,       15, MA_KUGLN        , G_BOX             ,   /* Object 14  */
  NONE, NORMAL, (LONG)0x00FF1100L,
  0x0001, 0x0007, 0x002B, 0x0004,
  MA_BAUEN        ,       -1,       -1, G_STRING          ,   /* Object 15  */
  NONE, NORMAL, (LONG)"Markt:",
  0x0012, 0x0000, 0x0006, 0x0001,
  MA_FOERD        ,       -1,       -1, G_STRING          ,   /* Object 16 MA_BAUEN */
  SELECTABLE|EXIT, NORMAL, (LONG)"Anbauen      130",
  0x0002, 0x0001, 0x0010, 0x0001,
  MA_KANON        ,       -1,       -1, G_STRING          ,   /* Object 17 MA_FOERD */
  SELECTABLE|EXIT, NORMAL, (LONG)"F\224rderturm   876",
  0x0019, 0x0001, 0x0010, 0x0001,
  MA_FAHNE        ,       -1,       -1, G_STRING          ,   /* Object 18 MA_KANON */
  SELECTABLE|EXIT, NORMAL, (LONG)"Gesch\201tz     180",
  0x0002, 0x0002, 0x0010, 0x0001,
  MA_PULVR        ,       -1,       -1, G_STRING          ,   /* Object 19 MA_FAHNE */
  SELECTABLE|EXIT, DISABLED, (LONG)"Windfahne    150",
  0x0019, 0x0002, 0x0010, 0x0001,
  MA_KUGLN        ,       -1,       -1, G_STRING          ,   /* Object 20 MA_PULVR */
  SELECTABLE|EXIT, NORMAL, (LONG)"30 Pulver     90",
  0x0002, 0x0003, 0x0010, 0x0001,
        14,       -1,       -1, G_STRING          ,   /* Object 21 MA_KUGLN */
  SELECTABLE|EXIT|LASTOB, NORMAL, (LONG)"2 Kugeln      30",
  0x0019, 0x0003, 0x0010, 0x0001,
  
  /******** Tree 7 SIEGER ****************************************************/
        -1,        1,        2, G_BOX             ,   /* Object 0  */
  NONE, NORMAL, (LONG)0x00FD1111L,
  0x0000, 0x0000, 0x0242, 0x0E0C,
         2,       -1,       -1, G_BOX             ,   /* Object 1  */
  NONE, NORMAL, (LONG)0x00001121L,
  0x0101, 0x0A00, 0x073F, 0x090B,
         0,        3,        3, G_BOX             ,   /* Object 2  */
  NONE, NORMAL, (LONG)0x00001131L,
  0x0402, 0x0601, 0x013D, 0x030A,
         2,        4,        4, G_BOX             ,   /* Object 3  */
  NONE, NORMAL, (LONG)0x00001141L,
  0x0201, 0x0900, 0x043A, 0x0F08,
         3,        5,        5, G_BOX             ,   /* Object 4  */
  NONE, NORMAL, (LONG)0x00001151L,
  0x0201, 0x0A00, 0x0737, 0x0807,
         4, SG1             ,        7, G_BOX             ,   /* Object 5  */
  NONE, NORMAL, (LONG)0x00001161L,
  0x0301, 0x0C00, 0x0135, 0x0006,
         7,       -1,       -1, G_BUTTON          ,   /* Object 6 SG1 */
  DEFAULT|EXIT, NORMAL, (LONG)"!!",
  0x0301, 0x0D00, 0x0332, 0x0501,
         5, SG2             , SG3             , G_BOX             ,   /* Object 7  */
  NONE, NORMAL, (LONG)0x00FD1100L,
  0x0301, 0x0C02, 0x0332, 0x0902,
  SG3             ,       -1,       -1, G_STRING          ,   /* Object 8 SG2 */
  NONE, NORMAL, (LONG)"(",
  0x0600, 0x0400, 0x0031, 0x0F00,
         7,       -1,       -1, G_STRING          ,   /* Object 9 SG3 */
  LASTOB, NORMAL, (LONG)")",
  0x0600, 0x0501, 0x0131, 0x0F00,
  
  /******** Tree 8 COMPUTER ****************************************************/
        -1,        1,       28, G_BOX             ,   /* Object 0  */
  NONE, NORMAL, (LONG)0x00031141L,
  0x0000, 0x0000, 0x0020, 0x000F,
         2,       -1,       -1, G_BUTTON          ,   /* Object 1  */
  EXIT, NORMAL, (LONG)"Computer 1      Computer 2",
  0x0002, 0x0001, 0x001C, 0x0001,
        10, CN1             ,        9, G_BOX             ,   /* Object 2  */
  NONE, NORMAL, (LONG)0x00FF1100L,
  0x0701, 0x0802, 0x000D, 0x0E07,
         4,       -1,       -1, G_BUTTON          ,   /* Object 3 CN1 */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"T\224lpel ",
  0x0100, 0x0100, 0x060C, 0x0001,
         5,       -1,       -1, G_BUTTON          ,   /* Object 4  */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"Dummel ",
  0x0100, 0x0301, 0x060C, 0x0001,
         6,       -1,       -1, G_BUTTON          ,   /* Object 5  */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"Brubbel",
  0x0100, 0x0502, 0x060C, 0x0001,
         7,       -1,       -1, G_BUTTON          ,   /* Object 6  */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"Wusel  ",
  0x0100, 0x0703, 0x060C, 0x0001,
         8,       -1,       -1, G_BUTTON          ,   /* Object 7  */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"Br\224sel ",
  0x0100, 0x0904, 0x060C, 0x0001,
         9,       -1,       -1, G_BUTTON          ,   /* Object 8  */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"Toffel ",
  0x0100, 0x0B05, 0x060C, 0x0001,
         2,       -1,       -1, G_BUTTON          ,   /* Object 9  */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"R\201pel  ",
  0x0100, 0x0D06, 0x060C, 0x0001,
        18, CN2             ,       17, G_BOX             ,   /* Object 10  */
  NONE, NORMAL, (LONG)0x00FF1100L,
  0x0111, 0x0802, 0x000D, 0x0E07,
        12,       -1,       -1, G_BUTTON          ,   /* Object 11 CN2 */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"T\224lpel ",
  0x0100, 0x0100, 0x060C, 0x0001,
        13,       -1,       -1, G_BUTTON          ,   /* Object 12  */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"Dummel ",
  0x0100, 0x0301, 0x060C, 0x0001,
        14,       -1,       -1, G_BUTTON          ,   /* Object 13  */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"Brubbel",
  0x0100, 0x0502, 0x060C, 0x0001,
        15,       -1,       -1, G_BUTTON          ,   /* Object 14  */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"Wusel  ",
  0x0100, 0x0703, 0x060C, 0x0001,
        16,       -1,       -1, G_BUTTON          ,   /* Object 15  */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"Br\224sel ",
  0x0100, 0x0904, 0x060C, 0x0001,
        17,       -1,       -1, G_BUTTON          ,   /* Object 16  */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"Toffel ",
  0x0100, 0x0B05, 0x060C, 0x0001,
        10,       -1,       -1, G_BUTTON          ,   /* Object 17  */
  SELECTABLE|RBUTTON, NORMAL, (LONG)"R\201pel  ",
  0x0100, 0x0D06, 0x060C, 0x0001,
        23, CTS1            ,       22, G_BOX             ,   /* Object 18  */
  NONE, NORMAL, (LONG)0x00FF1100L,
  0x0701, 0x0E0A, 0x000D, 0x0201,
        20,       -1,       -1, G_BOXCHAR         ,   /* Object 19 CTS1 */
  SELECTABLE|RBUTTON, NORMAL, (LONG) ((LONG)'1' << 24)|0x00FF1100L,
  0x0100, 0x0100, 0x0003, 0x0001,
        21,       -1,       -1, G_BOXCHAR         ,   /* Object 20  */
  SELECTABLE|RBUTTON, NORMAL, (LONG) ((LONG)'2' << 24)|0x00FF1100L,
  0x0303, 0x0100, 0x0003, 0x0001,
        22,       -1,       -1, G_BOXCHAR         ,   /* Object 21  */
  SELECTABLE|RBUTTON, NORMAL, (LONG) ((LONG)'3' << 24)|0x00FF1100L,
  0x0506, 0x0100, 0x0003, 0x0001,
        18,       -1,       -1, G_BOXCHAR         ,   /* Object 22  */
  SELECTABLE|RBUTTON, NORMAL, (LONG) ((LONG)'4' << 24)|0x00FF1100L,
  0x0709, 0x0100, 0x0003, 0x0001,
        28, CTS2            ,       27, G_BOX             ,   /* Object 23  */
  NONE, NORMAL, (LONG)0x00FF1100L,
  0x0111, 0x0E0A, 0x000D, 0x0201,
        25,       -1,       -1, G_BOXCHAR         ,   /* Object 24 CTS2 */
  SELECTABLE|RBUTTON, NORMAL, (LONG) ((LONG)'1' << 24)|0x00FF1100L,
  0x0100, 0x0100, 0x0003, 0x0001,
        26,       -1,       -1, G_BOXCHAR         ,   /* Object 25  */
  SELECTABLE|RBUTTON, NORMAL, (LONG) ((LONG)'2' << 24)|0x00FF1100L,
  0x0303, 0x0100, 0x0003, 0x0001,
        27,       -1,       -1, G_BOXCHAR         ,   /* Object 26  */
  SELECTABLE|RBUTTON, NORMAL, (LONG) ((LONG)'3' << 24)|0x00FF1100L,
  0x0506, 0x0100, 0x0003, 0x0001,
        23,       -1,       -1, G_BOXCHAR         ,   /* Object 27  */
  SELECTABLE|RBUTTON, NORMAL, (LONG) ((LONG)'4' << 24)|0x00FF1100L,
  0x0709, 0x0100, 0x0003, 0x0001,
         0,       -1,       -1, G_BUTTON          ,   /* Object 28  */
  SELECTABLE|DEFAULT|EXIT|LASTOB, NORMAL, (LONG)"OK",
  0x0002, 0x000D, 0x001C, 0x0001,
  
  /******** Tree 9 REGEL1 ****************************************************/
        -1,        1, R12             , G_BOX             ,   /* Object 0  */
  NONE, OUTLINED, (LONG)0x00031101L,
  0x0000, 0x0000, 0x004D, 0x0015,
         2,       -1,       -1, G_BUTTON          ,   /* Object 1  */
  NONE, NORMAL, (LONG)"Die Spielregeln f\201r Ballerburg",
  0x0014, 0x0001, 0x0023, 0x0001,
         3,       -1,       -1, G_STRING          ,   /* Object 2  */
  NONE, NORMAL, (LONG)"( Seite 1 )",
  0x003C, 0x0001, 0x000B, 0x0001,
         4,       -1,       -1, G_STRING          ,   /* Object 3  */
  NONE, NORMAL, (LONG)"w\204hlen.",
  0x0002, 0x0010, 0x0007, 0x0001,
         5,       -1,       -1, G_STRING          ,   /* Object 4  */
  NONE, NORMAL, (LONG)"Bei  dem  Spiel  Ballerburg geht es daru",
  0x0002, 0x0003, 0x0028, 0x0001,
         6,       -1,       -1, G_STRING          ,   /* Object 5  */
  NONE, NORMAL, (LONG)"jeweils die Burg des Gegners zu zerst\224re",
  0x0002, 0x0004, 0x0028, 0x0001,
         7,       -1,       -1, G_STRING          ,   /* Object 6  */
  NONE, NORMAL, (LONG)"da\236 die Spieler durch den Computer erset",
  0x0002, 0x0005, 0x0028, 0x0001,
         8,       -1,       -1, G_STRING          ,   /* Object 7  */
  NONE, NORMAL, (LONG)"man entweder kein Volk mehr hat,  der K\224",
  0x0002, 0x0006, 0x0028, 0x0001,
         9,       -1,       -1, G_STRING          ,   /* Object 8  */
  NONE, NORMAL, (LONG)"so aussichtslos ist, da\236 sich der K\224nig ",
  0x0002, 0x0007, 0x0028, 0x0001,
        10,       -1,       -1, G_STRING          ,   /* Object 9  */
  NONE, NORMAL, (LONG)"Vor Spielbeginn sollte man zun\204chst die ",
  0x0002, 0x0008, 0x0028, 0x0001,
        11,       -1,       -1, G_STRING          ,   /* Object 10  */
  NONE, NORMAL, (LONG)"den Modus festlegen,  d.h. ob Sie zu zw",
  0x0002, 0x0009, 0x0027, 0x0001,
        12,       -1,       -1, G_STRING          ,   /* Object 11  */
  NONE, NORMAL, (LONG)"den  Computer  ( Hier k\224nnen Sie auch di",
  0x0002, 0x000A, 0x0028, 0x0001,
        13,       -1,       -1, G_STRING          ,   /* Object 12  */
  NONE, NORMAL, (LONG)"spielen  wollen. )  oder,  ob  Sie nur z",
  0x0002, 0x000B, 0x0028, 0x0001,
        14,       -1,       -1, G_STRING          ,   /* Object 13  */
  NONE, NORMAL, (LONG)"gegeneinander   spielen   lassen.    Au\236",
  0x0002, 0x000C, 0x0028, 0x0001,
        15,       -1,       -1, G_STRING          ,   /* Object 14  */
  NONE, NORMAL, (LONG)"Computerspieler  w\204hlen.  Dazu  ist  zu ",
  0x0002, 0x000D, 0x0028, 0x0001,
        16,       -1,       -1, G_STRING          ,   /* Object 15  */
  NONE, NORMAL, (LONG)"Strategie verfolgt,  die mehr oder wenig",
  0x0002, 0x000E, 0x0028, 0x0001,
        17,       -1,       -1, G_STRING          ,   /* Object 16  */
  NONE, NORMAL, (LONG)"sich noch die Treffsicherheit durch Wahl",
  0x0002, 0x000F, 0x0028, 0x0001,
        18,       -1,       -1, G_STRING          ,   /* Object 17  */
  NONE, NORMAL, (LONG)"m,  da\236 zwei  Spieler  versuchen,",
  0x002A, 0x0003, 0x0021, 0x0001,
        19,       -1,       -1, G_STRING          ,   /* Object 18  */
  NONE, NORMAL, (LONG)"zt werden. Verloren hat man, wenn",
  0x002A, 0x0005, 0x0021, 0x0001,
        20,       -1,       -1, G_STRING          ,   /* Object 19  */
  NONE, NORMAL, (LONG)"n. Dabei besteht die M\224glichkeit,",
  0x002A, 0x0004, 0x0021, 0x0001,
        21,       -1,       -1, G_STRING          ,   /* Object 20  */
  NONE, NORMAL, (LONG)"e Seite bestimmen,  auf  der  Sie",
  0x002A, 0x000A, 0x0021, 0x0001,
        22,       -1,       -1, G_STRING          ,   /* Object 21  */
  NONE, NORMAL, (LONG)"nig getroffen wurde oder die Lage",
  0x002A, 0x0006, 0x0021, 0x0001,
        23,       -1,       -1, G_STRING          ,   /* Object 22  */
  NONE, NORMAL, (LONG)"ergibt.",
  0x002A, 0x0007, 0x0007, 0x0001,
        24,       -1,       -1, G_STRING          ,   /* Object 23  */
  NONE, NORMAL, (LONG)"Namen der Spieler eingeben  sowie",
  0x002A, 0x0008, 0x0021, 0x0001,
        25,       -1,       -1, G_STRING          ,   /* Object 24  */
  NONE, NORMAL, (LONG)"eit spielen wollen, allein gegen",
  0x0029, 0x0009, 0x0020, 0x0001,
        26,       -1,       -1, G_STRING          ,   /* Object 25  */
  NONE, NORMAL, (LONG)"uschauen wollen und zwei Computer",
  0x002A, 0x000B, 0x0021, 0x0001,
        27,       -1,       -1, G_STRING          ,   /* Object 26  */
  NONE, NORMAL, (LONG)"er effektiv ist.  Zus\204tzlich l\204\236t",
  0x002A, 0x000E, 0x0021, 0x0001,
        28,       -1,       -1, G_STRING          ,   /* Object 27  */
  NONE, NORMAL, (LONG)"erdem   k\224nnen   Sie   noch   die",
  0x002A, 0x000C, 0x0021, 0x0001,
        29,       -1,       -1, G_STRING          ,   /* Object 28  */
  NONE, NORMAL, (LONG)" sagen,  da\236  jeder  eine  eigene",
  0x002A, 0x000D, 0x0021, 0x0001,
  R1F             ,       -1,       -1, G_STRING          ,   /* Object 29  */
  NONE, NORMAL, (LONG)" der Zahl ( 4 ist am st\204rksten. )",
  0x002A, 0x000F, 0x0021, 0x0001,
  R13             ,       -1,       -1, G_BUTTON          ,   /* Object 30 R1F */
  SELECTABLE|EXIT, NORMAL, (LONG)"Fertig",
  0x003C, 0x0013, 0x000E, 0x0001,
  R12             ,       -1,       -1, G_BUTTON          ,   /* Object 31 R13 */
  SELECTABLE|EXIT, NORMAL, (LONG)"Seite 3",
  0x002C, 0x0013, 0x000E, 0x0001,
         0,       -1,       -1, G_BUTTON          ,   /* Object 32 R12 */
  SELECTABLE|DEFAULT|EXIT|LASTOB, NORMAL, (LONG)"Seite 2",
  0x001C, 0x0013, 0x000E, 0x0001,
  
  /******** Tree 10 REGEL2 ****************************************************/
        -1,        1, R21             , G_BOX             ,   /* Object 0  */
  NONE, OUTLINED, (LONG)0x00031101L,
  0x0000, 0x0000, 0x004D, 0x0015,
         2,       -1,       -1, G_BUTTON          ,   /* Object 1  */
  NONE, NORMAL, (LONG)"Die Spielregeln f\201r Ballerburg",
  0x0014, 0x0001, 0x0023, 0x0001,
         3,       -1,       -1, G_STRING          ,   /* Object 2  */
  NONE, NORMAL, (LONG)"( Seite 2 )",
  0x003C, 0x0001, 0x000B, 0x0001,
         4,       -1,       -1, G_STRING          ,   /* Object 3  */
  NONE, NORMAL, (LONG)"obwohl Sie schon eine haben.",
  0x0002, 0x0010, 0x001C, 0x0001,
         5,       -1,       -1, G_STRING          ,   /* Object 4  */
  NONE, NORMAL, (LONG)"Nach  Anklicken  von \047Neues Spiel\047  k\224nn",
  0x0002, 0x0003, 0x0028, 0x0001,
         6,       -1,       -1, G_STRING          ,   /* Object 5  */
  NONE, NORMAL, (LONG)"ausw\204hlen. Nun zum Ablauf des Spieles:",
  0x0002, 0x0004, 0x0026, 0x0001,
         7,       -1,       -1, G_STRING          ,   /* Object 6  */
  NONE, NORMAL, (LONG)"Der  Spieler,  der  gerade an der Reihe ",
  0x0002, 0x0005, 0x0028, 0x0001,
         8,       -1,       -1, G_STRING          ,   /* Object 7  */
  NONE, NORMAL, (LONG)"durchf\201hren:  Durch  Anklicken der Lager",
  0x0002, 0x0006, 0x0028, 0x0001,
         9,       -1,       -1, G_STRING          ,   /* Object 8  */
  NONE, NORMAL, (LONG)"das  Geld  (  symbolisiert durch F\204sser,",
  0x0002, 0x0007, 0x0028, 0x0001,
        10,       -1,       -1, G_STRING          ,   /* Object 9  */
  NONE, NORMAL, (LONG)"Informationen \201ber seinen momentanen Bes",
  0x0002, 0x0008, 0x0028, 0x0001,
        11,       -1,       -1, G_STRING          ,   /* Object 10  */
  NONE, NORMAL, (LONG)"von M\224glichkeiten:",
  0x0002, 0x0009, 0x0012, 0x0001,
        12,       -1,       -1, G_STRING          ,   /* Object 11  */
  NONE, NORMAL, (LONG)"Man  kann  die  Steuern  \204ndern:  Bei ho",
  0x0002, 0x000A, 0x0028, 0x0001,
        13,       -1,       -1, G_STRING          ,   /* Object 12  */
  NONE, NORMAL, (LONG)"Einnahmen,  es  besteht  jedoch  die Gef",
  0x0002, 0x000B, 0x0028, 0x0001,
        14,       -1,       -1, G_STRING          ,   /* Object 13  */
  NONE, NORMAL, (LONG)"auswandern. F\201r niedrige Steuern gilt da",
  0x0002, 0x000C, 0x0028, 0x0001,
        15,       -1,       -1, G_STRING          ,   /* Object 14  */
  NONE, NORMAL, (LONG)"Die  Punkte unter \047Markt\047 lassen sich an",
  0x0002, 0x000D, 0x0028, 0x0001,
        16,       -1,       -1, G_STRING          ,   /* Object 15  */
  NONE, NORMAL, (LONG)"werden,  k\224nnen Sie sie nicht anw\204hlen. ",
  0x0002, 0x000E, 0x0028, 0x0001,
        17,       -1,       -1, G_STRING          ,   /* Object 16  */
  NONE, NORMAL, (LONG)"Beispiel  Ihr Geld nicht reicht,  oder S",
  0x0002, 0x000F, 0x0028, 0x0001,
        18,       -1,       -1, G_STRING          ,   /* Object 17  */
  NONE, NORMAL, (LONG)"en  Sie  schlie\236lich  die  Burgen",
  0x002A, 0x0003, 0x0021, 0x0001,
        19,       -1,       -1, G_STRING          ,   /* Object 18  */
  NONE, NORMAL, (LONG)"ist,  kann verschiedene  Aktionen",
  0x002A, 0x0005, 0x0021, 0x0001,
        20,       -1,       -1, G_STRING          ,   /* Object 19  */
  NONE, NORMAL, (LONG)"hen  Steuern  macht  man  gr\224\236ere",
  0x002A, 0x000A, 0x0021, 0x0001,
        21,       -1,       -1, G_STRING          ,   /* Object 20  */
  NONE, NORMAL, (LONG)" f\201r das Pulver,  die Kugeln oder",
  0x002A, 0x0006, 0x0021, 0x0001,
        22,       -1,       -1, G_STRING          ,   /* Object 21  */
  NONE, NORMAL, (LONG)"  Kugeln und S\204cke )  erh\204lt  man",
  0x002A, 0x0007, 0x0021, 0x0001,
        23,       -1,       -1, G_STRING          ,   /* Object 22  */
  NONE, NORMAL, (LONG)"itz.  Hier hat man nun eine Reihe",
  0x002A, 0x0008, 0x0021, 0x0001,
        24,       -1,       -1, G_STRING          ,   /* Object 23  */
  NONE, NORMAL, (LONG)"ahr,  da\236 Teile  der  Bev\224lkerung",
  0x002A, 0x000B, 0x0021, 0x0001,
        25,       -1,       -1, G_STRING          ,   /* Object 24  */
  NONE, NORMAL, (LONG)" Dies ist dann der Fall, wenn zum",
  0x002A, 0x000E, 0x0021, 0x0001,
        26,       -1,       -1, G_STRING          ,   /* Object 25  */
  NONE, NORMAL, (LONG)"s Gegenteil.",
  0x002A, 0x000C, 0x000C, 0x0001,
        27,       -1,       -1, G_STRING          ,   /* Object 26  */
  NONE, NORMAL, (LONG)"klicken.  Wenn sie grau angezeigt",
  0x002A, 0x000D, 0x0021, 0x0001,
  R2F             ,       -1,       -1, G_STRING          ,   /* Object 27  */
  NONE, NORMAL, (LONG)"ie eine Windfahne kaufen  wollen,",
  0x002A, 0x000F, 0x0021, 0x0001,
  R23             ,       -1,       -1, G_BUTTON          ,   /* Object 28 R2F */
  SELECTABLE|EXIT, NORMAL, (LONG)"Fertig",
  0x003C, 0x0013, 0x000E, 0x0001,
  R21             ,       -1,       -1, G_BUTTON          ,   /* Object 29 R23 */
  SELECTABLE|DEFAULT|EXIT, NORMAL, (LONG)"Seite 3",
  0x001C, 0x0013, 0x000E, 0x0001,
         0,       -1,       -1, G_BUTTON          ,   /* Object 30 R21 */
  SELECTABLE|EXIT|LASTOB, NORMAL, (LONG)"Seite 1",
  0x002C, 0x0013, 0x000E, 0x0001,
  
  /******** Tree 11 REGEL3 ****************************************************/
        -1,        1,       33, G_BOX             ,   /* Object 0  */
  NONE, OUTLINED, (LONG)0x00031101L,
  0x0000, 0x0000, 0x004D, 0x0015,
         2,       -1,       -1, G_BUTTON          ,   /* Object 1  */
  NONE, NORMAL, (LONG)"Die Spielregeln f\201r Ballerburg",
  0x0014, 0x0001, 0x0023, 0x0001,
         3,       -1,       -1, G_STRING          ,   /* Object 2  */
  NONE, NORMAL, (LONG)"( Seite 3 )",
  0x003C, 0x0001, 0x000B, 0x0001,
         4,       -1,       -1, G_STRING          ,   /* Object 3  */
  NONE, NORMAL, (LONG)"Sollten Sie nicht schie\236en wollen oder",
  0x0002, 0x0010, 0x0026, 0x0001,
         5,       -1,       -1, G_STRING          ,   /* Object 4  */
  NONE, NORMAL, (LONG)"Anbauen gibt Ihnen die M\224glichkeit, Ihre",
  0x0002, 0x0003, 0x0028, 0x0001,
         6,       -1,       -1, G_STRING          ,   /* Object 5  */
  NONE, NORMAL, (LONG)"Durch  den  Erwerb von F\224rdert\201rmen k\224",
  0x0002, 0x0004, 0x0026, 0x0001,
         7,       -1,       -1, G_STRING          ,   /* Object 6  */
  NONE, NORMAL, (LONG)"Einnahmen machen.",
  0x0002, 0x0005, 0x0011, 0x0001,
         8,       -1,       -1, G_STRING          ,   /* Object 7  */
  NONE, NORMAL, (LONG)"Sollten Ihre Gesch\201tze zerst\224rt sein, so",
  0x0002, 0x0006, 0x0028, 0x0001,
         9,       -1,       -1, G_STRING          ,   /* Object 8  */
  NONE, NORMAL, (LONG)"Eine  Windfahne wird ben\224tigt, um die ge",
  0x0002, 0x0007, 0x0028, 0x0001,
        10,       -1,       -1, G_STRING          ,   /* Object 9  */
  NONE, NORMAL, (LONG)"beeinflu\236t die Bahn der Kugel. ) zu sehe",
  0x0002, 0x0008, 0x0028, 0x0001,
        11,       -1,       -1, G_STRING          ,   /* Object 10  */
  NONE, NORMAL, (LONG)"Pulver und Kugeln werden Sie regelm\204\236ig ",
  0x0002, 0x0009, 0x0028, 0x0001,
        12,       -1,       -1, G_STRING          ,   /* Object 11  */
  NONE, NORMAL, (LONG)"Mit \047Fertig\047 verlassen Sie wieder dieses",
  0x0002, 0x000A, 0x0028, 0x0001,
        13,       -1,       -1, G_STRING          ,   /* Object 12  */
  NONE, NORMAL, (LONG)"Desweiteren k\224nnen Sie durch Anklicken d",
  0x0002, 0x000B, 0x0028, 0x0001,
        14,       -1,       -1, G_STRING          ,   /* Object 13  */
  NONE, NORMAL, (LONG)"Leistungen beurteilt.",
  0x0002, 0x000C, 0x0015, 0x0001,
        15,       -1,       -1, G_STRING          ,   /* Object 14  */
  NONE, NORMAL, (LONG)"Sie  schlie\236en Ihren Zug ab,  indem Sie ",
  0x0002, 0x000D, 0x0028, 0x0001,
        16,       -1,       -1, G_STRING          ,   /* Object 15  */
  NONE, NORMAL, (LONG)"wollen,  anw\204hlen.  Sie  m\201ssen nun noch",
  0x0002, 0x000E, 0x0028, 0x0001,
        17,       -1,       -1, G_STRING          ,   /* Object 16  */
  NONE, NORMAL, (LONG)"k\224nnen dann durch \047OK\047 den Schu\236 ausl\224se",
  0x0002, 0x000F, 0x0028, 0x0001,
        18,       -1,       -1, G_STRING          ,   /* Object 17  */
  NONE, NORMAL, (LONG)" Burg zu reparieren.",
  0x002A, 0x0003, 0x0014, 0x0001,
        19,       -1,       -1, G_STRING          ,   /* Object 18  */
  NONE, NORMAL, (LONG)"nnen Sie jede  Runde  zus\204tzliche",
  0x0028, 0x0004, 0x0021, 0x0001,
        20,       -1,       -1, G_STRING          ,   /* Object 19  */
  NONE, NORMAL, (LONG)"nachkaufen m\201ssen.",
  0x002A, 0x0009, 0x0012, 0x0001,
        21,       -1,       -1, G_STRING          ,   /* Object 20  */
  NONE, NORMAL, (LONG)" k\224nnen Sie hier neue kaufen.",
  0x002A, 0x0006, 0x001D, 0x0001,
        22,       -1,       -1, G_STRING          ,   /* Object 21  */
  NONE, NORMAL, (LONG)"naue  Windgeschwindigkeit  (  Sie",
  0x002A, 0x0007, 0x0021, 0x0001,
        23,       -1,       -1, G_STRING          ,   /* Object 22  */
  NONE, NORMAL, (LONG)"n.",
  0x002A, 0x0008, 0x0002, 0x0001,
        24,       -1,       -1, G_STRING          ,   /* Object 23  */
  NONE, NORMAL, (LONG)"es K\224nigs h\224ren,  wie dieser Ihre",
  0x002A, 0x000B, 0x0021, 0x0001,
        25,       -1,       -1, G_STRING          ,   /* Object 24  */
  NONE, NORMAL, (LONG)" Pulver und Winkel einstellen und",
  0x002A, 0x000E, 0x0021, 0x0001,
        26,       -1,       -1, G_STRING          ,   /* Object 25  */
  NONE, NORMAL, (LONG)"die Kanone,  mit der Sie schie\236en",
  0x002A, 0x000D, 0x0021, 0x0001,
        27,       -1,       -1, G_STRING          ,   /* Object 26  */
  NONE, NORMAL, (LONG)" Men\201.",
  0x002A, 0x000A, 0x0006, 0x0001,
  R3F             ,       -1,       -1, G_STRING          ,   /* Object 27  */
  NONE, NORMAL, (LONG)"n.",
  0x002A, 0x000F, 0x0002, 0x0001,
  R32             ,       -1,       -1, G_BUTTON          ,   /* Object 28 R3F */
  SELECTABLE|DEFAULT|EXIT, NORMAL, (LONG)"Fertig",
  0x003C, 0x0013, 0x000E, 0x0001,
  R31             ,       -1,       -1, G_BUTTON          ,   /* Object 29 R32 */
  SELECTABLE|EXIT, NORMAL, (LONG)"Seite 2",
  0x002C, 0x0013, 0x000E, 0x0001,
        31,       -1,       -1, G_BUTTON          ,   /* Object 30 R31 */
  SELECTABLE|EXIT, NORMAL, (LONG)"Seite 1",
  0x001C, 0x0013, 0x000E, 0x0001,
        32,       -1,       -1, G_STRING          ,   /* Object 31  */
  NONE, NORMAL, (LONG)" im Moment keine Kanone haben, so",
  0x0028, 0x0010, 0x0021, 0x0001,
        33,       -1,       -1, G_STRING          ,   /* Object 32  */
  NONE, NORMAL, (LONG)"klicken Sie zum Abschlu\236 Ihres Zuges auf",
  0x0002, 0x0011, 0x0028, 0x0001,
         0,       -1,       -1, G_STRING          ,   /* Object 33  */
  LASTOB, NORMAL, (LONG)" \047Fertig\047.",
  0x002A, 0x0011, 0x000A, 0x0001,
  
  /******** Tree 12 EINTRAG ****************************************************/
        -1, ER_OK           ,       11, G_BOX             ,   /* Object 0  */
  NONE, OUTLINED, (LONG)0x00031141L,
  0x0000, 0x0000, 0x0027, 0x0010,
         2,       -1,       -1, G_BUTTON          ,   /* Object 1 ER_OK */
  SELECTABLE|DEFAULT|EXIT, NORMAL, (LONG)"OK",
  0x0012, 0x000E, 0x0008, 0x0001,
         3,       -1,       -1, G_BUTTON          ,   /* Object 2  */
  SELECTABLE|EXIT, NORMAL, (LONG)"Abbruch",
  0x001C, 0x000E, 0x0008, 0x0001,
         4,       -1,       -1, G_STRING          ,   /* Object 3  */
  NONE, NORMAL, (LONG)"Spielernamen f\201r die Tabelle:",
  0x0005, 0x0800, 0x001D, 0x0001,
        11, EI_N1           , EI_N6           , G_BOX             ,   /* Object 4  */
  NONE, NORMAL, (LONG)0x00FD1121L,
  0x0003, 0x0F01, 0x0021, 0x0007,
  EI_N2           ,       -1,       -1, G_FBOXTEXT        ,   /* Object 5 EI_N1 */
  EDITABLE, SHADOWED, (LONG)&rs_tedinfo[6],
  0x0002, 0x0001, 0x000D, 0x0001,
  EI_N3           ,       -1,       -1, G_FBOXTEXT        ,   /* Object 6 EI_N2 */
  EDITABLE, SHADOWED, (LONG)&rs_tedinfo[7],
  0x0012, 0x0001, 0x000D, 0x0001,
  EI_N4           ,       -1,       -1, G_FBOXTEXT        ,   /* Object 7 EI_N3 */
  EDITABLE, SHADOWED, (LONG)&rs_tedinfo[8],
  0x0002, 0x0003, 0x000D, 0x0001,
  EI_N5           ,       -1,       -1, G_FBOXTEXT        ,   /* Object 8 EI_N4 */
  EDITABLE, SHADOWED, (LONG)&rs_tedinfo[9],
  0x0012, 0x0003, 0x000D, 0x0001,
  EI_N6           ,       -1,       -1, G_FBOXTEXT        ,   /* Object 9 EI_N5 */
  EDITABLE, SHADOWED, (LONG)&rs_tedinfo[10],
  0x0002, 0x0005, 0x000D, 0x0001,
         4,       -1,       -1, G_FBOXTEXT        ,   /* Object 10 EI_N6 */
  EDITABLE, SHADOWED, (LONG)&rs_tedinfo[11],
  0x0012, 0x0005, 0x000D, 0x0001,
         0, ER_LOE          , ER_BLE          , G_BOX             ,   /* Object 11  */
  NONE, NORMAL, (LONG)0x00FD1121L,
  0x0003, 0x0909, 0x0021, 0x0C03,
  ER_BLE          ,       -1,       -1, G_BUTTON          ,   /* Object 12 ER_LOE */
  SELECTABLE|RBUTTON, SHADOWED, (LONG)"Ergebnisse l\224schen",
  0x0002, 0x0A00, 0x001D, 0x0001,
        11,       -1,       -1, G_BUTTON          ,   /* Object 13 ER_BLE */
  SELECTABLE|RBUTTON|LASTOB, SHADOWED, (LONG)"Ergebnisse bleiben",
  0x0002, 0x0302, 0x001D, 0x0001,
  
  /******** Tree 13 OPTIONEN ****************************************************/
        -1,        1,       14, G_BOX             ,   /* Object 0  */
  NONE, OUTLINED, (LONG)0x00031141L,
  0x0000, 0x0000, 0x002A, 0x0010,
         7,        2, MAX_XX          , G_BOX             ,   /* Object 1  */
  NONE, NORMAL, (LONG)0x00FD1121L,
  0x0002, 0x0E05, 0x0026, 0x0304,
  MAX_1           ,       -1,       -1, G_STRING          ,   /* Object 2  */
  NONE, NORMAL, (LONG)"Maximale Rundenzahl:",
  0x0001, 0x0800, 0x0014, 0x0001,
  MAX_2           ,       -1,       -1, G_BUTTON          ,   /* Object 3 MAX_1 */
  SELECTABLE|RBUTTON, SHADOWED, (LONG)"20",
  0x0001, 0x0402, 0x0007, 0x0001,
  MAX_NE          ,       -1,       -1, G_BUTTON          ,   /* Object 4 MAX_2 */
  SELECTABLE|RBUTTON, SHADOWED, (LONG)"50",
  0x0009, 0x0402, 0x0007, 0x0001,
  MAX_XX          ,       -1,       -1, G_BUTTON          ,   /* Object 5 MAX_NE */
  SELECTABLE|RBUTTON, SHADOWED, (LONG)"unbegrenzt",
  0x0019, 0x0402, 0x000C, 0x0001,
         1,       -1,       -1, G_FBOXTEXT        ,   /* Object 6 MAX_XX */
  SELECTABLE|EDITABLE|RBUTTON, SHADOWED, (LONG)&rs_tedinfo[12],
  0x0011, 0x0402, 0x0007, 0x0001,
  OP_OK           ,        8, A_VER           , G_BOX             ,   /* Object 7  */
  NONE, NORMAL, (LONG)0x00FD1121L,
  0x0002, 0x0002, 0x0026, 0x0003,
  A_ERL           ,       -1,       -1, G_STRING          ,   /* Object 8  */
  NONE, NORMAL, (LONG)"Anbauen ist",
  0x0001, 0x0001, 0x000B, 0x0001,
  A_VER           ,       -1,       -1, G_BUTTON          ,   /* Object 9 A_ERL */
  SELECTABLE|RBUTTON, SHADOWED, (LONG)"erlaubt",
  0x000E, 0x0001, 0x000A, 0x0001,
         7,       -1,       -1, G_BUTTON          ,   /* Object 10 A_VER */
  SELECTABLE|RBUTTON, SHADOWED, (LONG)"verboten",
  0x0019, 0x0001, 0x000B, 0x0001,
        12,       -1,       -1, G_BUTTON          ,   /* Object 11 OP_OK */
  SELECTABLE|DEFAULT|EXIT, NORMAL, (LONG)"OK",
  0x0016, 0x000E, 0x0008, 0x0001,
        13,       -1,       -1, G_BUTTON          ,   /* Object 12  */
  SELECTABLE|EXIT, NORMAL, (LONG)"Abbruch",
  0x0020, 0x000E, 0x0008, 0x0001,
        14,       -1,       -1, G_STRING          ,   /* Object 13  */
  NONE, NORMAL, (LONG)"Optionen:",
  0x0510, 0x0900, 0x0009, 0x0001,
         0, A_KAP           , A_KAP           , G_BOX             ,   /* Object 14  */
  NONE, NORMAL, (LONG)0x00FD1121L,
  0x0002, 0x0F0A, 0x0026, 0x0202,
        14,       -1,       -1, G_BUTTON          ,   /* Object 15 A_KAP */
  SELECTABLE|LASTOB, SHADOWED, (LONG)"K\224nig kann kapitulieren",
  0x0001, 0x0900, 0x0024, 0x0001,
  
  /******** Tree 14 T_TABELL ****************************************************/
        -1,       -1,       -1, G_BOX             ,   /* Object 0  */
  LASTOB, NORMAL, (LONG)0x00001100L,
  0x0000, 0x0000, 0x0044, 0x0012
};

OBJECT *rs_trindex[] =
{ &rs_object[0],   /* Tree  0 MENUE            */
  &rs_object[40],   /* Tree  1 T_SCHUSS         */
  &rs_object[57],   /* Tree  2 T_INFO           */
  &rs_object[93],   /* Tree  3 T_BURG           */
  &rs_object[105],   /* Tree  4 DRAN             */
  &rs_object[113],   /* Tree  5 NAMEN            */
  &rs_object[119],   /* Tree  6 STATUS           */
  &rs_object[141],   /* Tree  7 SIEGER           */
  &rs_object[151],   /* Tree  8 COMPUTER         */
  &rs_object[180],   /* Tree  9 REGEL1           */
  &rs_object[213],   /* Tree 10 REGEL2           */
  &rs_object[244],   /* Tree 11 REGEL3           */
  &rs_object[278],   /* Tree 12 EINTRAG          */
  &rs_object[292],   /* Tree 13 OPTIONEN         */
  &rs_object[308]    /* Tree 14 T_TABELL         */
};
