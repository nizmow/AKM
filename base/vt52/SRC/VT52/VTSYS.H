/*
*
* Diese Datei beschreibt den Aufbau der Übergabestruktur,
* mit der sich VT52.PRG ins System einhängt.
* VT52 muß die Adresse 0x98c auf NULL lassen, damit es
* keinen Konflikt mit der alten Version gibt. VT52 hängt seine
* Struktur in Adresse 0x990 ein.
*
*	<sout_cooked> gibt eine Zeichenkette aus, unter
*  Berücksichtigung von ^C.
*
*  <cin_cooked> liest ein Zeichen von der Tastatur ein, unter
*  Berücksichtigung von ^C.
*
*  <inherit> vererbt ein Terminalfenster von einem Task an
*  einen anderen (i.a. mehrere Threads und ein Prozeß).
*
*  <uninherit> meldet den Task für sein Terminalfenster wieder
*  ab, wenn z.B. der Task bzw. der Signalhandler terminiert.
*
*  <bg> schickt einen Task in den Hintergrund, d.h. er bekommt
*  keine Tastatureingaben mehr.
*
*  <fg> legt den Task wieder in den Vordergrund.
*
*/

struct vtsys
{
	LONG	version;			/* Versionsnummer der Struktur */
	/* Native MagiC register entries, not C stack-callable functions. */
	LONG	(*getVDIESC)(void);
	LONG	(*sout_cooked)(void);
	LONG	(*cin_cooked)(void);
	LONG	(*inherit)(void);
	LONG	(*uninherit)(void);
	LONG	(*bg)(void);
	LONG	(*fg)(void);
};

_Static_assert(sizeof(struct vtsys) == 32, "MagiC VTSYS table layout");
