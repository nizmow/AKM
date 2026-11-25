/*
	Tabulatorweite: 3
	Kommentare ab: Spalte 60											*Spalte 60*

*/

#ifndef	__PRN_DIALOGS__
#define	__PRN_DIALOGS__

typedef struct _prn_tray												/* Beschreibung eines Einzugs/Auswurfs */
{
	struct _prn_tray	*next;
	
	int32_t	tray_id;															/* Nummer des Einzugs oder Auswurfs */
	int8_t	name[32];														/* Name des Schachts */
} PRN_TRAY;

typedef struct _media_size												/* Beschreibung eines Papierformats */
{
	struct _media_size	*next;

	int32_t			size_id;													/* Kennung des Papierformats */
	int8_t			name[32];												/* Name des Papierformats */
} MEDIA_SIZE;

typedef struct _media_type												/* Beschreibung eines Papiertyps/Druckmediums */
{
	struct _media_type	*next;

	int32_t			type_id;													/* Kennung des Papierformats */
	int8_t			name[32];												/* Name des Papierformats */
} MEDIA_TYPE;

typedef struct _prn_mode												/* Beschreibung eines Druckermodus */
{
	struct _prn_mode *next;												/* Zeiger auf den nÑchsten Druckermodus */

	int32_t			mode_id;													/* Moduskennung (Index innerhalb der Datei) */
	int16_t			hdpi;														/* horizontale Auflîsung in dpi */
	int16_t			vdpi;														/* vertikale Auflîsung in dpi */
	int32_t			mode_capabilities;									/* Moduseigenschaften */

	int32_t			color_capabilities;									/* einstellbare Farbmodi */
	int32_t			dither_flags;											/* Flags, die angeben, ob der korrespondierende Farbmodus mit oder ohne Dithern ansprechbar ist */
	MEDIA_TYPE	*paper_types;											/* geeignete Papiertypen */
	int32_t			reserved;

	int8_t			name[32];												/* Modusname */
} PRN_MODE;

/* old_printer kann auch 0L sein */
typedef int32_t (cdecl *PRN_SWITCH) (struct _drv_entry *drivers, struct _prn_settings *settings, struct _prn_entry *old_printer, struct _prn_entry *new_printer);

/* sub_flags */
#define	PRN_STD_SUBS	0x0001										/* Standard-Unterdialoge fÅr NVDI-Drucker */
#define	PRN_FSM_SUBS	0x0002										/* Standard-Unterdialoge fÅr FSM-Drucker */
#define	PRN_QD_SUBS		0x0004										/* Standard-Unterdialoge fÅr QuickDraw-Drucker */

typedef struct _prn_entry												/* GerÑtebeschreibung */
{
	struct _prn_entry	*next;											/* Zeiger auf die die nÑchste GerÑtebeschreibung */
	int32_t			length;													/* StrukturlÑnge */
	int32_t			format;													/* Datenformat */
	int32_t			reserved;												/* reserviert */
	
	int16_t			driver_id;												/* Treiberkennung */
	int16_t			driver_type;											/* Treibertyp */
	int32_t			printer_id;												/* Druckerkennung */
	int32_t			printer_capabilities;								/* Druckereigenschaften */
	int32_t			reserved1;

	int32_t			sub_flags;												/* Flags fÅr die Unterdialoge */
	struct _pdlg_sub	*sub_dialogs;									/* Zeiger auf die Liste der Unterdialoge fÅr diesen Drucker */
	PRN_SWITCH	setup_panel;											/* Unterdialog bei Druckerwechsel initialisieren */
	PRN_SWITCH	close_panel;											/* Unterdialog bei Druckerwechsel schlieûen */

	PRN_MODE		*modes;													/* Liste der vorhandenen Auflîsungen */
	MEDIA_SIZE	*papers;													/* Liste der vorhandenen Papierformate */
	PRN_TRAY		*input_trays;											/* Liste der EinzÅge */
	PRN_TRAY		*output_trays;											/* Liste der AuswÅrfe */

	int8_t			name[32];												/* Name des Druckers */
} PRN_ENTRY;

typedef struct	_dither_mode
{
	struct _dither_mode	*next;
	int32_t			length;													/* StrukturlÑnge */
	int32_t			format;													/* Datenformat */
	int32_t			reserved;												/* reserviert */

	int32_t			dither_id;												/* Kennung */
	int32_t			color_modes;											/* unterstÅtzte Farbtiefen */
	int32_t			reserved1;
	int32_t			reserved2;

	int8_t			name[32];												/* Name des Rasterverfahrens */
} DITHER_MODE;


typedef struct
{
	int32_t			magic;													/* 'pdnf' */
	int32_t			length;													/* StrukturlÑnge */
	int32_t			format;													/* Datenformat */
	int32_t			reserved;												/* reserviert */

	int16_t			driver_id;												/* Treibernummer fÅrs VDI */
	int16_t			driver_type;											/* Treibertyp */
	int32_t			reserved1;
	int32_t			reserved2;
	int32_t			reserved3;

	PRN_ENTRY	*printers;												/* Liste der zum Treiber gehîrenden Drucker */
	DITHER_MODE	*dither_modes;											/* Liste der vom Treiber unterstÅtzten Rasterverfahren */
	int32_t			reserved4;
	int32_t			reserved5;

	int32_t			reserved6;
	int32_t			reserved7;
	int32_t			reserved8;
	int32_t			reserved9;
	
	int8_t			device[128];											/* Ausgabedatei des Druckertreibers */
} DRV_INFO;

typedef struct _drv_entry
{
	struct _drv_entry	*next;
} DRV_ENTRY;

#define	PDLG_CHG_SUB	0x80000000L
#define	PDLG_IS_BUTTON	0x40000000L

#define	PDLG_PREBUTTON	0x20000000L
#define	PDLG_PB_OK		1
#define	PDLG_PB_CANCEL	2
#define	PDLG_PB_DEVICE	3

#define	PDLG_BUT_OK		( PDLG_PREBUTTON + PDLG_PB_OK )
#define	PDLG_BUT_CNCL	( PDLG_PREBUTTON + PDLG_PB_CANCEL )
#define	PDLG_BUT_DEV	( PDLG_PREBUTTON + PDLG_PB_DEVICE )

typedef	int32_t (cdecl *PDLG_INIT)(struct _prn_settings *settings, struct _pdlg_sub *sub);
typedef	int32_t	(cdecl *PDLG_HNDL)(struct _prn_settings *settings, struct _pdlg_sub *sub, int16_t exit_obj);
typedef	int32_t	(cdecl *PDLG_RESET)(struct _prn_settings *settings, struct _pdlg_sub *sub);

typedef struct _pdlg_sub												/* Unterdialog fÅr GerÑteeinstellung */
{
	struct _pdlg_sub *next;												/* Zeiger auf den Nachfolger in der Liste */
	int32_t			length;													/* StrukturlÑnge */
	int32_t			format;													/* Datenformat */
	int32_t			reserved;												/* reserviert */
	
	void			*drivers;												/* nur fÅr interne Dialoge */
	int16_t			option_flags;											/* Flags, u.a. PDLG_PRINTING, PDLG_PREFS */
	int16_t			sub_id;													/* Kennung des Unterdialogs, wird fÅr globale Unterdialoge von pdlg_add() eingetragen */
	DIALOG		*dialog;													/* Zeiger auf die Struktur des Fensterdialogs oder 0L */
	OBJECT		*tree;													/* Zeiger auf den zusammengesetzen Objektbaum */
	int16_t			index_offset;											/* Indexverschiebung des Unterdialogs */
	int16_t			reserved1;
	int32_t			reserved2;
	int32_t			reserved3;
	int32_t			reserved4;

	PDLG_INIT	init_dlg;												/* Initialisierungsfunktion */
	PDLG_HNDL	do_dlg;													/* Behandlungsfunktion */
	PDLG_RESET	reset_dlg;												/* ZurÅcksetzfunktion */
	int32_t			reserved5;

	OBJECT		*sub_icon;												/* Zeiger auf das Icon fÅr die Listbox */
	OBJECT		*sub_tree;												/* Zeiger auf den Objektbaum des Unterdialogs */
	int32_t			reserved6;
	int32_t			reserved7;

	int32_t			private1;												/* dialogeigene Informationen */
	int32_t			private2;
	int32_t			private3;
	int32_t			private4;
} PDLG_SUB;

#endif