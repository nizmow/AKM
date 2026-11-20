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
	LONG	(*getVDIESC)( APPL *app);
	LONG	(*sout_cooked)( APPL *app, char *text, LONG len);
	LONG	(*cin_cooked)( APPL *app );
	/* neu: */
	LONG	(*inherit)( WORD dst_apid, WORD src_apid);
	LONG	(*uninherit)( WORD apid );
	LONG	(*bg)( WORD apid );
	LONG	(*fg)( WORD apid );
};
