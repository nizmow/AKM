/*
	Tabulatorweite: 3
	Kommentare ab: Spalte 60											*Spalte 60*
*/

typedef struct _window
{
	struct _window	*next; 												/* Zeiger auf nächste Struktur */

	WORD	handle;															/* Fensterhandle */
	WORD	kind;																/* Fensterkomponenten */

	GRECT	border;															/* Fensteraußenfläche */
	GRECT	workarea;														/* Fensterinnenfläche */
	GRECT	saved_border;													/* bei Iconify gesicherte Fensteraußenfläche */

	BYTE	*name;															/* Zeiger auf den Fensternamen oder 0L */
	BYTE	*iconified_name;												/* Fenstername für ikonifiziertes Fenster oder 0L */
	OBJECT iconified_tree[2];											/* Objektbaum für ikonifiziertes Fenster */
	short dirty;		/* redraw notwendig */

	struct
	{
		unsigned           	:	7;										/* reserviert */
		unsigned hide_cursor	:	1;										/* 0: Cursor nicht ausschalten (wird von der Redraw-Routine erledigt) 1: Cursor vor Redraw ausschalten */
		unsigned	snap_width	:	1;										/* Breite auf Vielfache von dx anpassen */
		unsigned	snap_height	:	1;										/* Höhe auf Vielfache von dy anpassen */
		unsigned	snap_x		:	1;										/* x-Koordinate auf Vielfache von dx anpassen */
		unsigned	snap_y		:	1;										/* y-Koordinate auf Vielfache von dy anpassen */
		unsigned	smart_size	:	1;										/* Smart Redraw fürs Sizen */ 
		unsigned	limit_wsize	:	1;										/* maximale Fenstergröße begrenzen */
		unsigned	fuller		:	1;										/* Fenster hat maximale Größe */
		unsigned	iconified	:	1;										/* Fenster ist ikonifiziert */
	} wflags;

	void	(*redraw)( struct _window *window, GRECT *area );	/* Zeiger auf eine Redraw-Routine */
	
	LONG	interior_flags;												/* Art des Inhalts */
	void	*interior;														/* Zeiger auf den Inhalt */

	LONG	x;																	/* x-Koordinate der linken oberen Ecke */
	LONG	y;																	/* y-Koordinate der linken oberen Ecke */
	LONG	w;																	/* Breite der Arbeitsfläche */
	LONG	h;																	/* Höhe der Arbeitsfläche */

	WORD	dx;																/* Breite einer Scrollspalte in Pixeln */
	WORD	dy;																/* Höhe einer Scrollzeile in Pixeln */
	WORD	snap_dx;															/* Vielfaches für horizontale Fensterposition */
	WORD	snap_dy															/* Vielfaches für vertikale Fensterposition */;							
	WORD	snap_dw;															/* Vielfaches der Fensterbreite */
	WORD	snap_dh;															/* Vielfaches der Fensterhöhe */
	WORD	limit_w;															/* maximal anzuzeigende Breite der Arbeitsfläche oder 0 */
	WORD	limit_h;															/* maximal anzuzeigende Höhe der Arbeitsfläche oder 0 */

	WORD	hslide;															/* Position des horizontalen Sliders in Promille */
	WORD	vslide;															/* Position des vertikalen Sliders in Promille */
	WORD	hslsize;															/* Größe des horizontalen Sliders in Promille */
	WORD	vslsize;															/* Größe des vertikalen Sliders in Promille */

} WINDOW;
