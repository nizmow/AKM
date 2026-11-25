/*********************************************************************
*
* Dieses Modul enthält die Routinen für den Dialog
* "Datei/Ordner-Info anzeigen"
*
*********************************************************************/

#include "K.H"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define INFO_FILE_NOBJS (FI_SIZER + 1)
#define ALIAS_MAXLEN 255

typedef struct _ifd
{
	struct _ifd *next;	/* Verkettungszeiger */
	DIALOG *dialog;		/* Zeiger auf DIALOG oder NULL (kein Fenster) */
	WORD whdl;			/* Handle des Fensters oder -1 */
	OBJECT *tree;		/* Objektbaum */

	int isfolder;		/* Diese Instanz wird für "Folder Info" verwendet. */
	int isdrive;		/* Diese Instanz wird für einen "Drive Info" verwendet. */
	int isdeskalias;	/* Desktop-Icon (Datei oder Ordner) */
	MYDTA f;			/* alle Infos über die Datei */
	char old_name[65];	/* alter Dateiname */
	char new_name[65];	/* neuer Dateiname */
	XTED fname_xted;	/* Für den Dateinamen */
	TEDINFO fname_ted;

	char old_alias[ALIAS_MAXLEN + 1];
	char new_alias[ALIAS_MAXLEN + 1];
	XTED alias_xted;
	TEDINFO alias_ted;

	WORD drv;			/* tatsächliches Laufwerk */
	char path[258];		/* logischer Pfad */
	int is_8_3;
	char extended_view;

	CICONBLK cic;		/* Für das Icon rechts oben */
	OBJECT _tree[INFO_FILE_NOBJS];

	char _varstrings[0];	/* Alle nicht editierbaren Zeichenketten */
} INFO_FILE_DATA;


static INFO_FILE_DATA *info_file_dialogs;
static int n_info_file_dialogs;

static int _info_file(INFO_FILE_DATA *ifd, int further);
static WORD cdecl hdl_info_file(struct HNDL_OBJ_args args);

/*
 IDs der festen Zeichenketten im Info-Dialog, die je nach Instanz
 geändert werden, z.B. "Anzahl Bytes" oder Dateiname.
*/
static const int variable_strings[] =
{
	FI_SIZE,
	FI_DATUM,
	FI_ZEIT,
	OI_N_DAT,
	OI_N_ORD,
	OI_N_VDA,
	OI_B_VDA,
	DI_BTOTAL,
	DI_BUSED,
	DI_BFREE,
	DI_CLUST,
	DI_SECCL,
	DI_BYSEC,
	0
};

/****************************************************************
*
* Diese Routine wird in der evnt_multi-Schleife aufgerufen
* und behandelt alle Fensterdialoge
*
****************************************************************/

void do_info_file_dialogs(EVNT *w_ev)
{
	INFO_FILE_DATA *ifd, **prev;

	for	(ifd = info_file_dialogs, prev = &info_file_dialogs;
			(ifd); prev = &ifd->next, ifd = ifd->next)
	{
		if (!wdlg_evnt(ifd->dialog, w_ev))
		{
			wdlg_close(ifd->dialog, NULL, NULL);
			wdlg_delete(ifd->dialog);
			*prev = ifd->next;	/* ausklinken */
			n_info_file_dialogs--;
			free(ifd);
		}
	} 
}


/****************************************************************
*
* Informationen zu einer Datei anzeigen.
* <further> zeigt, ob danach noch weitere Dateien angezeigt
* werden. Genau: Die Anzahl der Dateien, die noch folgt.
* <number> ist die Nummer (0, 1, ...).
* <drv> ist das tatsächliche Laufwerk.
* <path> ist der logische Pfad (kann Aliase enthalten)
*
****************************************************************/

int info_file(char *path, int drv, MYDTA *f,
			int isfolder, int isdrive, int isdeskalias,
			int further, int number)
{
	INFO_FILE_DATA *ifd;	/* das Objekt, das die gesamte Dialog-Instanz enthält */
	int ret;
	int x,y;
	int title_rsc_id;
	char *wintitle;
	static long len_of_varstrings = 0;
	const int *obx;
	register char *_varstrings;
	register long l;



	/* Beim ersten Aufruf die Länge aller */
	/* variablen G_STRINGs addieren       */
	/* ---------------------------------- */

	if (!len_of_varstrings)
	{
		for	(obx = variable_strings; *obx; obx++)
			len_of_varstrings += strlen(tree_datinf[*obx].ob_spec.free_string) + 1;
	}

	/* Dialog-Info-Struktur erstellen */
	/* ------------------------------ */

	ifd = malloc(sizeof(INFO_FILE_DATA) + len_of_varstrings);
	if (!ifd)			/* zuwenig Speicher */
	{
		err_alert(ENSMEM);
		return 1;
	}

	/* Baum kopieren */
	/* ------------- */

	memcpy(ifd->_tree, tree_datinf, INFO_FILE_NOBJS * sizeof(OBJECT));
	ifd->tree = ifd->_tree;

	/* TEDINFOs (Text-Eingabefelder) erstellen */
	/* --------------------------------------- */

	memcpy(&ifd->fname_ted, tree_datinf[FI_FILENAME].ob_spec.tedinfo, sizeof(TEDINFO));
	ifd->tree[FI_FILENAME].ob_spec.tedinfo = &ifd->fname_ted;

	memcpy(&ifd->alias_ted, tree_datinf[FI_ALIAS].ob_spec.tedinfo, sizeof(TEDINFO));
	ifd->tree[FI_ALIAS].ob_spec.tedinfo = &ifd->alias_ted;

	/* Zu verändernde "free_string"s (feste Zeichenketten, nicht editierbar) erstellen */
	/* ------------------------------------------------------------------------------- */

	for	(obx = variable_strings, _varstrings = ifd->_varstrings;
		 *obx;
		 obx++)
	{
		/* Die Länge entspricht der im ursprünglichen Baum, festgelegt in der .RSC-Datei. */
		l = strlen(ifd->tree[*obx].ob_spec.free_string) + 1;	/* +1 wegen end-of-string */
		/* Wir biegen den Zeiger in unserer Kopie des Objektbaums um. */
		ifd->tree[*obx].ob_spec.free_string = _varstrings;
		/* Zur Sicherheit, falls etwas schiefläuft, initialisieren wir den Text mit einem Marker. */
		_varstrings[0] = '-';
		_varstrings[1] = '\0';
		_varstrings += l;
	}
	
	/* versch. */
	/* ------- */

	if (f)
	{
		ifd->f = *f;
		strcpy(ifd->old_name, f->filename);
	}
	else
	{
		ifd->old_name[0] = EOS;
	}

	ifd->drv = drv;
	ifd->isfolder = isfolder;
	ifd->isdrive = isdrive;
	ifd->isdeskalias = isdeskalias;
	strcpy(ifd->path, path);
	if (isdrive)
		ifd->f.is_alias = FALSE;

/*
if (n_info_file_dialogs > 4)
	goto fixed_dialog;
*/
	ifd->dialog = wdlg_create(
			hdl_info_file,
			ifd->tree,
			ifd,
			0,
			NULL,
			0);

	if (!ifd->dialog)
	{
		fixed_dialog:
		ret = _info_file(ifd, further);
		free(ifd);
		return ret;
	}

	x = 100 + (number << 2);
	if ((desk_g.g_w >= 800) && (number & 1))
	{
		x += ifd->tree->ob_width + 32;
	}
	y = desk_g.g_y + (number << 2) + 20;
	if (desk_g.g_h >= 460)
	{
		y += 80;
	}

	if (isfolder)
	{
		title_rsc_id = (isdrive) ? STR_TIT_DLG_DRIV : STR_TIT_DLG_FOLD;
	}
	else
	{
		title_rsc_id = STR_TIT_DLG_FILE;
	}
	wintitle = Rgetstring(title_rsc_id);

	ifd->whdl = wdlg_open(ifd->dialog, wintitle,
					NAME + MOVER + CLOSER, x, y, 0, NULL);
	if (!ifd->whdl)
	{
		wdlg_delete(ifd->dialog);
		ifd->dialog = NULL;
		goto fixed_dialog;
	}

	/* Struktur einketten */
	/* ------------------ */

	ifd->next = info_file_dialogs;
	info_file_dialogs = ifd;
	n_info_file_dialogs++;
	return 0;
}


/****************************************************************
*
* Größenanpassung für den Dialog für "Datei anzeigen"
*
****************************************************************/

static void resize_info_file_tree(INFO_FILE_DATA *ifd)
{
	OBJECT *tree;
	DIALOG *d;
	OBJECT *o,*o2;
	int vislen;
	XTED *xted;
	TEDINFO *t;
	int editob;
	short cursorpos;


	d = ifd->dialog;
	editob = wdlg_get_edit(d, &cursorpos);
	if (editob)
	{
		wdlg_set_edit(d, 0);		/* Cursor abmelden */
		wdlg_set_edit(d, editob);	/* Cursor anmelden */
	}

	subobj_wdraw(d, FI_SIZER, 0, 0);
	subobj_wdraw(d, FI_ICON, 0, 0);
	subobj_wdraw(d, FI_CAN, 0, 0);
	subobj_wdraw(d, FI_OK, 0, 0);

	tree = ifd->tree;

	o = tree + FI_SIZER;
	o->ob_x = tree->ob_width - o->ob_width;
	o->ob_y = tree->ob_height - o->ob_height;

	o = tree + FI_ICON;
	o->ob_x = tree->ob_width - o->ob_width - 2 * gl_hwchar;

	o = tree + FI_CAN;
	o->ob_x = tree->ob_width - o->ob_width - 2 * gl_hwchar;

	o2 = tree + FI_OK;
	o2->ob_x = o->ob_x - o2->ob_width - 2 * gl_hwchar;

	if (!ifd->is_8_3)
	{
		o = tree + FI_FILENAME;
		t = &ifd->fname_ted;
		xted = &ifd->fname_xted;
		vislen = ((tree->ob_width - o->ob_x) / gl_hwchar) - 2;
		if (vislen > t->te_txtlen - 1)
			vislen = t->te_txtlen - 1;

		if (xted->xte_vislen != vislen)
		{
			subobj_wdraw(d, FI_FILENAME, 0, 0);
			xted->xte_vislen = vislen;
			tree[FI_FILENAME].ob_width = vislen*gl_hwchar;
			subobj_wdraw(d, FI_FILENAME, 0, 1);
		}
	}

	o = tree+FI_ALIAS;
	if (!(o->ob_flags & HIDETREE))
	{
		subobj_wdraw(d, FI_ALIAS, 0, 0);
		xted = &ifd->alias_xted;
		vislen = (tree->ob_width / gl_hwchar) - 4;
		xted->xte_vislen = vislen;
		tree[FI_ALIAS].ob_width = vislen*gl_hwchar;
		subobj_wdraw(d, FI_ALIAS, 0, 1);
	}

	subobj_wdraw(d, FI_SIZER, 0, 1);
	subobj_wdraw(d, FI_ICON, 0, 1);
	subobj_wdraw(d, FI_CAN, 0, 1);
	subobj_wdraw(d, FI_OK, 0, 1);

	wdlg_set_size(d, (GRECT *) &tree->ob_x);

	if (editob)
	{
		wdlg_set_edit(d, editob);	/* Cursor anmelden */
	}
}


/****************************************************************
*
* Initialisiert den Dialog für "Datei anzeigen"
*
****************************************************************/

static void cati(char *s, int i)
{
	s += strlen(s);
	sprintf(s, "%lu", (unsigned long)(unsigned int)i);
}

static int init_info_file_tree(INFO_FILE_DATA *ifd, int further)
{
	long err;
	OBJECT *o;
	ULONG bytes[2];		/* 64 Bit */
	OBJECT *tree;
	int ob;


	tree = ifd->tree;
	if (ifd->isdrive)
	{
		objs_hide(tree, FI_FILEATTR, FI_TIMEDATE, 0);
	}
	else
	{
		objs_hide(tree, FI_DRIVEATTR, FI_DISKLETTER, 0);
		tree[FI_FLDATTR].ob_y += gl_hhchar;
	}

	ob = (ifd->isfolder) ? FI_FILEATTR : FI_FLDATTR;
	objs_hide(tree, ob, 0);

	if (ifd->dialog)	/* Dialog im Fenster -> kein Titel */
		objs_hide(tree, FI_TITLE, 0);

	if (ifd->isdrive)
	{
		DISKINFO diskinfo;
		int lw;
		int offs;


		ifd->cic = *diskname_to_iconblk(ifd->path[0], NULL);
		err = Dreadlabel(ifd->path, ifd->old_name, 65);
		if ((err != E_OK) && (err != EFILNF) && (err != ENMFIL))
		{
			 fehler:
/*Cconws("HALLO1\r\n");*/
			err_alert(err);
			return -1;
		}

		lw = ifd->path[0] - 'A';
		err = Dfree(&diskinfo, lw + 1);
		if (err != E_OK)
			goto fehler;
	
		/* Anzahl von Clustern auf dem Laufwerk: */
		sprintf(tree[DI_CLUST].ob_spec.free_string, "%lu", (unsigned long)diskinfo.b_total);
		/* Sektoren pro Cluster: */
		sprintf(tree[DI_SECCL].ob_spec.free_string, "%lu", (unsigned long)diskinfo.b_clsiz);
		/* Bytes pro Sektor: */
		sprintf(tree[DI_BYSEC].ob_spec.free_string, "%lu", (unsigned long)diskinfo.b_secsiz);
		/* Bytes pro Cluster: */
		diskinfo.b_secsiz *= diskinfo.b_clsiz;
	
		/* Benutzte Bytes auf Laufwerk: total - frei */
		ullmul(diskinfo.b_total - diskinfo.b_free, diskinfo.b_secsiz, bytes);
		print_ull(bytes, tree[DI_BUSED].ob_spec.free_string);
	
		/* freie Bytes auf Laufwerk: */
		ullmul(diskinfo.b_free, diskinfo.b_secsiz, bytes);
		print_ull(bytes, tree[DI_BFREE].ob_spec.free_string);

		/* Gesamt Bytes auf Laufwerk: */
		ullmul(diskinfo.b_total, diskinfo.b_secsiz, bytes);
		print_ull(bytes, tree[DI_BTOTAL].ob_spec.free_string);

		offs = 4*gl_hhchar;

		if (lw < 2)		/* A: oder B: */
		{
			long ser;
			short bps, spc, res, nfats, ndirs, nsects, media, spf, spt, nsides, nhid, exec;
			char *flp_info;


			offs += gl_hhchar;
			err = read_bootsec(lw, &ser,&bps,&spc,&res,&nfats,&ndirs,&nsects,
							   &media,&spf,&spt,&nsides,&nhid,&exec);
			if (err != E_OK)
				goto fehler;

			if (nsides <= 0 || spt <= 0 || nsects <= 0)
			{
				err = EMEDIA;
				goto fehler;
			}

			flp_info = ifd->old_alias;
			tree[DI_FLOP].ob_spec.free_string = flp_info;
			flp_info[0] = EOS;
			cati(flp_info, nsides);
			strcat(flp_info, Rgetstring(STR_SIDES));
			cati(flp_info, nsects/(nsides*spt));
			strcat(flp_info, Rgetstring(STR_TRACKS));
			cati(flp_info, spt);
			strcat(flp_info, Rgetstring(STR_SECTORS));
			objs_unhide(tree, DI_FLOP, 0);
			if (exec)
				objs_unhide(tree, DI_EXEC, 0);
			else
				objs_hide(tree, DI_EXEC, 0);
		}
		else
		{
			objs_hide(tree, DI_FLOP, DI_EXEC, 0);
		}

		ifd->extended_view = settings.disk_extinfo;
		if (ifd->extended_view)
		{
			tree->ob_height += offs;
			tree[FI_CONT].ob_y += offs;
			tree[FI_OK].ob_y += offs;
			tree[FI_CAN].ob_y += offs;
		}
		else
		{
			objs_hide(tree, FI_XDRIVEATTR, 0);
			tree[FI_EXP].ob_spec.obspec.character = 3;	/* Pfeil rechts */
		}
	}
	else
	{
		ifd->cic = ifd->f.ciconblk;
	}

	if (init_xted(tree + FI_FILENAME, ifd->path, &ifd->fname_xted,
				ifd->new_name,
				ifd->old_name,
				&ifd->is_8_3, 22))
	{
		return -1;
	}

	ifd->cic.monoblk.ib_ptext = "";
	ifd->cic.monoblk.ib_xicon = ifd->cic.monoblk.ib_yicon =
		ifd->cic.monoblk.ib_wtext = 0;

	o = tree + FI_SIZER;
	o->ob_x = tree->ob_width - o->ob_width;
	o->ob_y = tree->ob_height - o->ob_height;

	o = tree + FI_ICON;
	o->ob_type = G_CICON;
	o->ob_spec.ciconblk = &ifd->cic;
	o->ob_width = ifd->cic.monoblk.ib_wicon;
	o->ob_height = ifd->cic.monoblk.ib_hicon;
	o->ob_x = tree->ob_width - o->ob_width - 2*gl_hwchar;

	if (further)
		tree[FI_CONT].ob_flags &= ~HIDETREE;
	else
		tree[FI_CONT].ob_flags |= HIDETREE;

	/* Datei ist ein Alias oder ein Desktop-Alias */
	/* ------------------------------------------ */

	if ((ifd->f.is_alias) || (ifd->isdeskalias))
	{
		register TEDINFO *t;
		XTED *xted;
		int vislen;

		objs_hide(tree, (ifd->isdeskalias) ? FI_ISALI : FI_ISDESKALI, 0);
		if (ifd->isdeskalias)
		{
			objs_uneditable(tree, FI_FILENAME, FI_ALIAS, 0);
			objs_disable(tree, FI_FILENAME, FI_ALIAS, 0);
			strcpy(ifd->old_alias, ifd->path);
		}
		else
		{
			err = Freadlink(256, ifd->old_alias, ifd->path);
			if (err)
				*(ifd->old_alias) = EOS;
		}

/*		objs_editable(tree, FI_ALIAS, 0);			*/
/*		objs_unhide(tree, FI_ISALI, FI_ALIAS, 0);	*/

		strcpy(ifd->new_alias, ifd->old_alias);
		t = tree[FI_ALIAS].ob_spec.tedinfo;
		xted = &ifd->alias_xted;

		t->te_ptext = ifd->new_alias;
		t->te_tmplen = t->te_txtlen = ALIAS_MAXLEN + 1;
		xted->xte_pvalid = "m";
		xted->xte_scroll = 0;
		xted->xte_ptmplt =
				"________________________________"
				"________________________________"
				"________________________________"
				"________________________________"
				"________________________________"
				"________________________________"
				"________________________________"
				"________________________________";
		t->te_ptmplt = NULL;
		t->te_pvalid = (void *) xted;
		vislen = (tree->ob_width / gl_hwchar) - 4;
		xted->xte_vislen = vislen;
		tree[FI_ALIAS].ob_width = vislen * gl_hwchar;
	}
	else
	{
		objs_hide(tree, FI_ISALI, FI_ALIAS, FI_ISDESKALI, FI_FINDORIGINAL, 0);
		objs_uneditable(tree, FI_ALIAS, 0);
	}

	time_to_str(tree[FI_ZEIT].ob_spec.free_string, ifd->f.time);
	date_to_str(tree[FI_DATUM].ob_spec.free_string, ifd->f.date);

	if (ifd->isfolder)
	{
		char *endp;
		int  n_dat,n_vda,n_ord;
		long b_dat,b_vda;


		tree[FI_SIZELAB].ob_spec.free_string = tree[FI_CONTSTR].ob_spec.free_string;
		if (ifd->isdrive)
				tree[FI_NUMFILES].ob_spec.free_string = tree[FI_DISKLETTER].ob_spec.free_string;
		endp = ifd->path + strlen(ifd->path);
		strcat(ifd->path, "\\");
		Mgraf_mouse(HOURGLASS);
		err = walk_path(ifd->path, &b_dat, &b_vda, &n_ord, &n_dat, &n_vda);
		*endp = EOS;
		Mgraf_mouse(ARROW);

		bytes[0] = 0L;
		if (err != E_OK)
		{
/*Cconws("HALLO2\r\n");*/
			err_alert(err);
			bytes[1] = 0;
		}
		else
		{
			sprintf(tree[OI_N_ORD].ob_spec.free_string, "%u", (unsigned int)n_ord);
			sprintf(tree[OI_N_DAT].ob_spec.free_string, "%u", (unsigned int)n_dat);
			sprintf(tree[OI_N_VDA].ob_spec.free_string, "%u", (unsigned int)n_vda);
			sprintf(tree[OI_B_VDA].ob_spec.free_string, "%lu", (unsigned long)b_vda);
	
			bytes[1] = b_dat + b_vda;
		}
	}
	else
	{
		bytes[0] = 0L;
		bytes[1] = ifd->f.filesize;

		ob_dsel(tree, FI_SETDA);
		ob_sel_dsel(tree, FI_RDONL, (ifd->f.attrib) & F_RDONLY);
		ob_sel_dsel(tree, FI_SYSTE, (ifd->f.attrib) & F_SYSTEM);
		ob_sel_dsel(tree, FI_HIDDE, (ifd->f.attrib) & F_HIDDEN);
		ob_sel_dsel(tree, FI_ARCHI, (ifd->f.attrib) & F_ARCHIVE);
	}

	print_ull(bytes, tree[FI_SIZE].ob_spec.free_string);
	strcat(tree[FI_SIZE].ob_spec.free_string, " Bytes");

	return 0;
}


/****************************************************************
*
* Wird beim Beenden des Dialogs aufgerufen, wenn OK
* angewählt wurde.
*
****************************************************************/

static long exit_info_file_tree(INFO_FILE_DATA *ifd)
{
	long err;
	DOSTIME dostime;
	unsigned char new_attr;


	if (ifd->is_8_3)
	{
		char s[20];
		fname_ext(ifd->new_name, s);
		strcpy(ifd->new_name,s);
	}

	if (!ifd->isfolder)
	{
		new_attr  = selected(ifd->tree, FI_RDONL) * F_RDONLY;
		new_attr += selected(ifd->tree, FI_SYSTE) * F_SYSTEM;
		new_attr += selected(ifd->tree, FI_HIDDE) * F_HIDDEN;
		new_attr += selected(ifd->tree, FI_ARCHI) * F_ARCHIVE;
	}
	
	Mgraf_mouse(HOURGLASS);
	err = 0x1234L;

	/* Uhrzeit und Datum der Datei aktualisieren */
	/* ----------------------------------------- */

	if ((!ifd->isfolder) && selected(ifd->tree, FI_SETDA))
	{
		int hdl;

		err = Fopen(ifd->path, RMODE_WR);
		if (err >= E_OK)
		{
			hdl = (int) err;
			dostime.time = Tgettime();
			dostime.date = Tgetdate();
			err = Fdatime(&dostime, hdl, TRUE);
			Fclose(hdl);
			set_dirty(err, ifd->path, ifd->drv, 2);
		}
	}

	/* Attribute ändern */
	/* ---------------- */

	if ((!ifd->isfolder) && err >= E_OK && new_attr != ifd->f.attrib)
	{
		err = Fattrib(ifd->path, TRUE, (int) new_attr);
		set_dirty(err, ifd->path, ifd->drv, 2);
	}

	/* Alias ändern */
	/* ------------ */

	if (err >= E_OK && (ifd->f.is_alias) && !(ifd->isdeskalias) &&
				strcmp(ifd->new_alias, ifd->old_alias))
	{
		err = Fdelete(ifd->path);
		if (!err)
			err = Fsymlink(ifd->new_alias, ifd->path);
		set_dirty(err, ifd->path, ifd->drv, 2);
	}

	/* Laufwerknamen ändern */
	/* -------------------- */

	if ((err >= E_OK) && (ifd->isdrive) &&
		 (strcmp(ifd->new_name, ifd->old_name)))
	{
		err = Dwritelabel(ifd->path, ifd->new_name);
		if (!err)
		{
			err = Dreadlabel(ifd->path, ifd->new_name, 65);
			if (err == EFILNF)
				err = E_OK;
			if (!err)
				set_dname(ifd->drv, ifd->new_name);
		}
	}

	/* Dateinamen ändern */
	/* ----------------- */

	if ((err >= E_OK) && !(ifd->isdrive) &&
		 (strcmp(ifd->new_name, ifd->f.filename)))
	{
		char flag;
		char new_path[256 + 2];

		flag = ((!ifd->isfolder) && (new_attr & F_RDONLY) &&
				!(ifd->f.attrib & F_RDONLY));
		if (flag)
		{
			err = Fattrib(ifd->path, TRUE, (int) ifd->f.attrib);
			if (err < E_OK)
				goto atterr;
		}

		strcpy(new_path, ifd->path);
		*(get_name(new_path)) = EOS;
		strcat(new_path, ifd->new_name);
		err = Frename(0, ifd->path, new_path);
		if (flag && !err)
			err = Fattrib(new_path, TRUE, (int) new_attr);

		atterr:
		set_dirty(err, ifd->path, ifd->drv, 2);
	}

	Mgraf_mouse(ARROW);
	if (err < E_OK)
	{
/*Cconws("HALLO3\r\n");*/
		err_alert(err);
	}

	/* Status für erweiterte Informationen merken */
	/* ------------------------------------------ */

	settings.disk_extinfo = ifd->extended_view;
	return err;
}


/****************************************************************
*
* Informationen zu einer Datei anzeigen. Dialog.
* <further> zeigt, ob danach noch weitere Dateien angezeigt
* werden.
*
****************************************************************/

static int _info_file(INFO_FILE_DATA *ifd, int further)
{
	int ret;


	if (init_info_file_tree(ifd, further))
		return 0;

	objs_hide(ifd->tree, FI_SIZER, 0);

	ret = do_dialog(ifd->tree);
	if (ret == FI_CAN)
		return -1;		/* Abbruch */
	if (ret == FI_CONT)
		return 0;		/* Weiter */

	exit_info_file_tree(ifd);
	return 0;
}


/*********************************************************************
*
* Behandelt die Exit- Objekte des Dialogs.
*
*********************************************************************/

#pragma warn -par
static WORD cdecl hdl_info_file(struct HNDL_OBJ_args args)
{
	DIALOG *d = args.dialog;
	WORD obj = args.obj;
	void *data = args.data;
	INFO_FILE_DATA *ifd;
	OBJECT *tree;
	WORD dummy;


	if (obj < 0)					/* Nachricht? */
	{
		if (obj == HNDL_INIT)
		{
			ifd = (INFO_FILE_DATA *) wdlg_get_udata(d);
			ifd->dialog = d;
			if (init_info_file_tree(ifd, FALSE))
				return 0;
			return 1;
		}

		if (obj == HNDL_CLSD)		/* Dialog geschlossen? */
		{
			return 0;
		}
	}
	else
	{
		ifd = (INFO_FILE_DATA *) data;
		tree = ifd->tree;

		if (obj == FI_SIZER)		/* Größe ändern */
		{
			graf_rubbox(
						tree->ob_x, tree->ob_y,
						tree_datinf->ob_width, tree_datinf->ob_height,
						&tree->ob_width, &dummy
					);
			resize_info_file_tree(ifd);
			return 1;
		}

		if (obj == FI_EXP)
		{
			int handle = wdlg_get_handle(d);
			GRECT g;
			int ydiff = 4 * gl_hhchar;
			char c;

			if (ifd->drv < 2)
				ydiff += gl_hhchar;

			ifd->extended_view = !ifd->extended_view;
			wind_get_grect(handle, WF_CURRXYWH, &g);
			if (ifd->extended_view)
			{
				objs_unhide(tree, FI_XDRIVEATTR, 0);
				objs_hide(tree, FI_OK, FI_CAN, FI_SIZER, 0);
				subobj_wdraw(d, FI_OK, ROOT, MAX_DEPTH);
				subobj_wdraw(d, FI_CAN, ROOT, MAX_DEPTH);
				subobj_wdraw(d, FI_SIZER, ROOT, MAX_DEPTH);
				subobj_wdraw(d, FI_XDRIVEATTR, ROOT, MAX_DEPTH);
				c = 2;	/* Pfeil nach unten */
			}
			else
			{
				objs_hide(tree, FI_XDRIVEATTR, 0);
				subobj_wdraw(d, FI_XDRIVEATTR, ROOT, MAX_DEPTH);
				ydiff = -ydiff;
				c = 3;	/* Pfeil nach rechts */
			}

			tree->ob_height += ydiff;
			g.g_h += ydiff;
			tree[FI_OK].ob_y += ydiff;
			tree[FI_CAN].ob_y += ydiff;
			tree[FI_SIZER].ob_y += ydiff;
			wind_set_grect(handle, WF_CURRXYWH, &g);
			if (ifd->extended_view)
			{
				objs_unhide(tree, FI_OK, FI_CAN, FI_SIZER, 0);
			}
			else
			{
				subobj_wdraw(d, FI_OK, ROOT, MAX_DEPTH);
				subobj_wdraw(d, FI_CAN, ROOT, MAX_DEPTH);
				subobj_wdraw(d, FI_SIZER, ROOT, MAX_DEPTH);
			}

			tree[FI_EXP].ob_spec.obspec.character = c;
			ob_dsel(tree, obj);
			subobj_wdraw(d, obj, 0, MAX_DEPTH);
			return 1;
		}

		if (obj == FI_FINDORIGINAL)
		{
			char path[128];
			char *name;
			char c;

			name = get_name(ifd->old_alias);
			c = *name;
			*name = EOS;
			strcpy(path, ifd->old_alias);
			*name = c;
			open_window(path,					/* path */
					  "*",					/* mask */
					  name,					/* selmask */
					  FALSE);					/* new */

			ob_dsel(tree, obj);
			subobj_wdraw(d, obj, 0, 1);
			return 1;
		}
		if (obj == FI_CAN)		/* Abbruch */
		{
			return 0;
		}
		if (obj == FI_OK)		/* OK */
		{
			exit_info_file_tree(ifd);
			return 0;
		}

		return 0;
	}

	return 1;	
}
#pragma warn +par
