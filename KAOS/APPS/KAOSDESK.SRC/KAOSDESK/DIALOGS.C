/*********************************************************************
*
* Dieses Modul enthÑlt alle Bearbeitung von Dialogen
*
*********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mgx_dos.h>	/* include before tos.h */
#include <tos.h>
#include <tosdefs.h>
#include "k.h"


/****************************************************************
*
* Dialog ausfÅhren
*
****************************************************************/

static int do_dialog(OBJECT *dialog)
{
	GRECT g;
	int exitbutton, dummy;
	void *flyinf;
	void **p_flyinf;


	flyinf = NULL;
	p_flyinf = (status.use_fldl) ? & flyinf : NULL;
	form_center(dialog, &g);
	form_xdial(FMD_START, NULL, &g, p_flyinf);
	objc_draw(dialog, ROOT, MAX_DEPTH, &g);
	exitbutton = 0x7f & form_xdo(dialog, 0, &dummy, NULL, flyinf);
	form_xdial(FMD_FINISH, NULL, &g, p_flyinf);
	ob_dsel(dialog, exitbutton);
	return exitbutton;
}


/****************************************************************
*
* erweiterten Dialog ausfÅhren
*
****************************************************************/

static int do_exdialog(OBJECT *dialog,
			 int (*check)(OBJECT *dialog, int exitbutton),
			 int *was_redraw)
{
	GRECT g;
	int exitbutton,dummy;
	void *flyinf;
	void **p_flyinf;


	flyinf = NULL;
	p_flyinf = (status.use_fldl) ? &flyinf : NULL;
	form_center(dialog, &g);
	form_xdial(FMD_START, NULL, &g, p_flyinf);
	objc_draw(dialog, ROOT, MAX_DEPTH, &g);

	for (;;)
	{
		exitbutton = 0x7f & form_xdo(dialog, 0, &dummy, NULL, flyinf);
		ob_dsel(dialog, exitbutton);
		if ((*check)(dialog, exitbutton))
			break;
		objc_draw(dialog, exitbutton, 1, &g);
	}

	form_xdial(FMD_FINISH, NULL, &g, p_flyinf);

	if (was_redraw != NULL)
	{
		*was_redraw = (flyinf == NULL);	/* RÅckgabe: Bildschirm zerstîrt */
	}

	return exitbutton;
}


/****************************************************************
*
* Der Dialog "Arbeit sichern".
*
****************************************************************/

void dial_arbsic(void)
{
	char alrts[200];

#if LANGUAGE == LANG_GER
	strcpy(alrts, "[2][Arbeit sichern nach|");
	strcat(alrts, inf_name);
	strcat(alrts, " ?][OK|Abbruch]");
#elif LANGUAGE == LANG_ENG
	strcpy(alrts, "[2][Save status to file|");
	strcat(alrts, inf_name);
	strcat(alrts, " ?][OK|Cancel]");
#endif

	if (1 == form_alert(1, alrts))
	{
		save_status(TRUE);
	}
}


/****************************************************************
*
* Der Dialog "Programme".
*
****************************************************************/

void dial_prgrms(void)
{
	register int i;
	char oldstrings[NUM_PROGRAMS][41];
	register char *src, *dst;


	for	(i = 0; i < NUM_PROGRAMS; i++)
	{
		src = adr_prgrms[PG_1 + i].ob_spec.tedinfo->te_ptext;
		dst = oldstrings[i];
		strcpy(dst, src);
	}

	if 	(PG_OK != do_dialog(adr_prgrms))
	{
		for	(i = 0; i < NUM_PROGRAMS; i++)
		{
			dst = adr_prgrms[PG_1 + i].ob_spec.tedinfo->te_ptext;
			src = oldstrings[i];
			strcpy(dst, src);
		}
	}
	else
	{
		for	(i = PG_2; i <= PG_3; i++)
		{
			dst = adr_prgrms[i].ob_spec.tedinfo->te_ptext;
			while (*dst && *dst != ' ')
			{
				*dst = toupper(*dst);
				dst++;
			}
		}
	}
}


/****************************************************************
*
* Der Dialog "Åber KAOSDesk..."
*
****************************************************************/

void dial_about(void)
{
#if	MAGIXDESK == 0
	static char mems[] = "                    ";
	char *dst, *src;


	dst = adr_about[O_SPEICH].ob_spec.tedinfo->te_ptext;
	ltoa(memsize(), mems + 10, 10);
	src = mems + strlen(mems) - 7;
	strncpy(dst + 11, src, strlen(src));
	ltoa((long) Malloc(-1L), mems + 10, 10);
	src = mems + strlen(mems) - 7;
	strncpy(dst+36, src, strlen(src));
#endif
	do_dialog(adr_about);
}


/****************************************************************
*
* Der Eintrag "Informationen..." ist angewÑhlt worden.
*
****************************************************************/

void info_spec(int typ)
{
	register char *alrts;

	switch(typ)
	{
		case 'P_':
#if LANGUAGE == LANG_GER
			alrts = "[1][PAPIERKORB-INFORMATION| |"
				   	  "Schiebe Dateien, Ordner und|"
				   	  "Disks hier hinein,|"
				   	  "um sie zu lîschen.]"
				   "[  OK  ]";
				   break;
#elif LANGUAGE == LANG_ENG
			alrts = "[1][TRASH INFORMATION| |"
				   	  "Drag files, folders and|"
				   	  "disks into this icon|"
				   	  "for deleting them.]"
				   "[  OK  ]";
				   break;
#endif
		case 'D_':
#if LANGUAGE == LANG_GER
			alrts = "[1][DRUCKER-INFORMATION| |"
					  "Schiebe Dateien hier hinein,|"
				   	  "um sie auszudrucken.]"
				   "[  OK  ]";
				   break;
#elif LANGUAGE == LANG_ENG
			alrts = "[1][PRINTER INFORMATION| |"
					  "Drag files into this icon|"
				   	  "for printing them.]"
				   "[  OK  ]";
				   break;
#endif
		default:
			err_alert(EINTRN);
				   return;
	}

	form_alert(1, alrts);
}


/****************************************************************
*
* print large numbers, factor is always a power of two
*
****************************************************************/

static void num2text(unsigned long num, unsigned long factor, char *text)
{
	static const char *units[] =
	{
		"",
		"k",
		"M",
		"G",
		"T"
	};

	if (num == 0xffffffffL)
	{
		*text++ = '-';
		*text = '\0';
	}
	else
	{
		unsigned unit = 0;

		while(factor > 1024)
		{
			factor >>= 10;
			unit++;
		}

		if (factor == 512)
		{
			factor = 1;
			num >>= 1;
			unit++;
		}

		if (num > 10000000L)
		{
			num >>= 10;
			unit++;
		}

		num *= factor;		/* hopefully not overflow */

		ultoa(num, text, /* decimal */ 10);
		strcat(text, units[unit]);
	}
}


/****************************************************************
*
* Informationen Åber selektierte Diskette oder Festplatte anzeigen
*
****************************************************************/

void info_disk(char *path, int typ)
{
	int lw;
	long dummy;
	unsigned long cluster_bytes;
	long err;
	int n_files = 0, n_hidden_files = 0, n_folders = 0;
	long bytes_files = 0, bytes_hidden_files = 0;
	DISKINFO info_free;
	OBJECT *adr_dskinf;
	register char *dst;
	char prev_name[13];
	char new_name[13];


	Mgraf_mouse(HOURGLASS);
	lw = path[0] - 'A';
	rsrc_gaddr(0, T_DSKINF, &adr_dskinf);
	drv_to_str(adr_dskinf[DSKINF_T].ob_spec.tedinfo->te_ptmplt, path[0]);
	dst = adr_dskinf[DSKINF_T].ob_spec.tedinfo->te_ptext;

	/* get volume name */

	err = read_dname(lw, prev_name);
	if (err != E_OK && err != EFILNF && err != ENMFIL)
	{
		error:
		Mgraf_mouse(ARROW);
		err_alert(err);
		return;
	}

	/* show missing volume name as empty string */

	if (err == E_OK)
		fname_int(prev_name, dst);
	else
		*dst = EOS;

	/* get free/used information */

	err = Dfree(&info_free, lw + 1);
	if (err != E_OK)
		goto error;

	/* total number of blocks (up to 4294967295) */
	ultoa(info_free.b_total,  adr_dskinf[DI_CLUST].ob_spec.free_string, 10);
	/* sectors per block, usually 1 or 2 */
	ultoa(info_free.b_clsiz,  adr_dskinf[DI_SECCL].ob_spec.free_string, 10);
	/* sector size in bytes, usually 512 .. 4096 */ 
	ultoa(info_free.b_secsiz, adr_dskinf[DI_BYSEC].ob_spec.free_string, 10);

	/* bytes per block (cluster) */
	cluster_bytes = info_free.b_secsiz * info_free.b_clsiz;
	
	/* total bytes */
	num2text(info_free.b_total, cluster_bytes, adr_dskinf[DI_BTOTL].ob_spec.free_string);

	/* used bytes */
	num2text(info_free.b_total - info_free.b_free, cluster_bytes, adr_dskinf[DI_BUSED].ob_spec.free_string);

	/* free bytes */
	num2text(info_free.b_free, cluster_bytes, adr_dskinf[DI_BFREE].ob_spec.free_string);

	err = walk_path(path, &dummy,
					&bytes_files, &bytes_hidden_files,
					&n_folders, &n_files, &n_hidden_files,
				 	0x37, 0L, 0L, 0L);
	#if 0	/* just continue in case of an error deep in the tree */
	if (err != E_OK)
		goto error;
	#endif

	ultoa(n_folders, adr_dskinf[DI_N_ORD].ob_spec.free_string, 10);
	ultoa(n_files, adr_dskinf[DI_N_DAT].ob_spec.free_string, 10);
	ultoa(n_hidden_files, adr_dskinf[DI_N_VDA].ob_spec.free_string, 10);

	num2text(bytes_files, 1, adr_dskinf[DI_B_DAT].ob_spec.free_string);
	num2text(bytes_hidden_files, 1, adr_dskinf[DI_B_VDA].ob_spec.free_string);

	if (typ == 'FD')
	{
		long ser;
		int bps,spc,res,nfats,ndirs,nsects,media,spf,spt,nsides,nhid,exec;
		int trsid;

		err = read_bootsec(lw, &ser,&bps,&spc,&res,&nfats,&ndirs,&nsects,
						   &media,&spf,&spt,&nsides,&nhid,&exec);
		if (err != E_OK)
		{
			bps = -1;
			spc = -1;
			res = -1;
			nfats = -1;
			ndirs = -1;
			nsects = -1;
			media = -1;
			spf = -1;
			spt = -1;
			nsides = -1;
			nhid = -1;
			exec = 0;
		}
		else
		if (nsides <= 0 || spt <= 0 || nsects <= 0)
		{
			err = EMEDIA;
		}

		/* number of sides, floppy disks have 1 or 2 */
		num2text(nsides, 1, adr_dskinf[DI_SIDES].ob_spec.free_string);

		/* tracks per side, floppy disks usually have 80 */
		trsid = ((nsects > 0) && (nsides > 0) && (spt > 0)) ? (nsects/(nsides*spt)) : -1;
		num2text(trsid, 1, adr_dskinf[DI_TRSID].ob_spec.free_string);

		/* sectors per track, floppy disks usually have 9 */
		num2text(spt, 1, adr_dskinf[DI_SECTR].ob_spec.free_string);

		objs_unhide(adr_dskinf, DI_K1, DI_K2, DI_K3,
							DI_SIDES, DI_TRSID, DI_SECTR, 0);
		if (exec)
			objs_unhide(adr_dskinf, DI_EXEC, 0);
		else
			objs_hide(adr_dskinf, DI_EXEC, 0);
	}
	else
	{
		objs_hide(adr_dskinf, DI_K1, DI_K2, DI_K3,
							DI_SIDES, DI_TRSID, DI_SECTR, DI_EXEC, 0);
	}

	Mgraf_mouse(ARROW);
	if (DSKIN_OK != do_dialog(adr_dskinf))
		return;

	fname_ext(dst, new_name);
	if (err == E_OK && !strcmp(new_name, prev_name))
		return;

	Mgraf_mouse(HOURGLASS);
	err = label(lw, new_name);
	Mgraf_mouse(ARROW);
	if (err != EWRPRO)
		dirty_drives[lw] = 2;

	err_alert(err);
}


/****************************************************************
*
* Informationen Åber selektierte Datei anzeigen
*
****************************************************************/

void info_file(char *path, MYDTA *f)
{
	long err;
	DOSTIME dostime;
	register char *dst;
	char new_name[13];
	unsigned char new_attr;


	dst = adr_datinf[DATINF_T].ob_spec.tedinfo->te_ptext;

	fname_int(f->filename, dst);
	time_to_str(adr_datinf[FI_ZEIT].ob_spec.free_string, f->time);
	date_to_str(adr_datinf[FI_DATUM].ob_spec.free_string, f->date);
	ultoa(f->filesize, adr_datinf[FI_SIZE].ob_spec.free_string, 10);
	ob_dsel(adr_datinf, FI_SETDA);
	ob_sel_dsel(adr_datinf, FI_RDONL, (f->attrib) & F_RDONLY);
	ob_sel_dsel(adr_datinf, FI_SYSTE, (f->attrib) & F_SYSTEM);
	ob_sel_dsel(adr_datinf, FI_HIDDE, (f->attrib) & F_HIDDEN);
	ob_sel_dsel(adr_datinf, FI_ARCHI, (f->attrib) & F_ARCHIVE);

	if (FI_OK != do_dialog(adr_datinf))
		return;

	fname_ext(dst, new_name);
	new_attr  = selected(adr_datinf, FI_RDONL) * F_RDONLY;
	new_attr += selected(adr_datinf, FI_SYSTE) * F_SYSTEM;
	new_attr += selected(adr_datinf, FI_HIDDE) * F_HIDDEN;
	new_attr += selected(adr_datinf, FI_ARCHI) * F_ARCHIVE;
	
	Mgraf_mouse(HOURGLASS);
	err = 0x1234L;
	if (selected(adr_datinf, FI_SETDA))
	{
		int hdl;

		err = Fopen(path, RMODE_WR);
		if (err >= E_OK)
		{
			hdl = (int) err;
			dostime.time = Tgettime();
			dostime.date = Tgetdate();
			err = Fdatime(&dostime, hdl, TRUE);
			Fclose(hdl);
			set_dirty(err, path, 2);
		}
	}

	if (err >= E_OK && new_attr != f->attrib)
	{
		err = Fattrib(path, TRUE, (int) new_attr);
		set_dirty(err, path, 2);
	}

	if (err >= E_OK && strcmp(new_name, f->filename))
	{
		char flag;
		char new_path[128];

		flag = (new_attr & F_RDONLY && !(f->attrib & F_RDONLY));
		if (flag)
		{
			err = Fattrib(path, TRUE, (int) f->attrib);
			if (err < E_OK)
				goto atterr;
			}
		strcpy(new_path, path);
		*(get_name(new_path)) = EOS;
		strcat(new_path, new_name);
		err = Frename(0, path, new_path);
		if (flag && !err)
			err = Fattrib(new_path, TRUE, (int) new_attr);
		atterr:
		set_dirty(err, path, 2);
	}

	Mgraf_mouse(ARROW);
	if (err < E_OK)
		err_alert(err);
}


/****************************************************************
*
* Informationen Åber selektierten Ordner anzeigen
*
****************************************************************/

void info_folder(char *path, MYDTA *f)
{
	long err;
	long dummy;
	char *endp;
	int n_files, n_hidden_files, n_folders;
	long bytes_files, bytes_hidden_files;
	register char *dst;
	char new_name[13];


	dst = adr_ordinf[ORDINF_T].ob_spec.tedinfo->te_ptext;

	fname_int(f->filename, dst);
	time_to_str(adr_ordinf[OI_ZEIT].ob_spec.free_string, f->time);
	date_to_str(adr_ordinf[OI_DATUM].ob_spec.free_string, f->date);

	endp = path + strlen(path);
	strcat(path, "\\");
	Mgraf_mouse(HOURGLASS);
	err = walk_path(path, &dummy, &bytes_files, &bytes_hidden_files, &n_folders, &n_files, &n_hidden_files,
				 0x37, 0L, 0L, 0L);
	*endp = EOS;
	#if 0	/* just continue in case of an error deep in the tree */
	if (err != E_OK)
		goto fehler;
	#endif

	ultoa(n_folders, adr_ordinf[OI_N_ORD].ob_spec.free_string, 10);
	ultoa(n_files, adr_ordinf[OI_N_DAT].ob_spec.free_string, 10);
	ultoa(n_hidden_files, adr_ordinf[OI_N_VDA].ob_spec.free_string, 10);
	ultoa(bytes_hidden_files, adr_ordinf[OI_B_VDA].ob_spec.free_string, 10);
	ultoa(bytes_files + n_hidden_files, adr_ordinf[OI_BYTES].ob_spec.free_string, 10);

	Mgraf_mouse(ARROW);
	if (OI_OK != do_dialog(adr_ordinf))
		return;
	fname_ext(dst, new_name);

	Mgraf_mouse(HOURGLASS);
	if (strcmp(new_name, f->filename))
	{
		char neupath[128];

		strcpy(neupath, path);
		*(get_name(neupath)) = EOS;
		strcat(neupath, new_name);
		err = Frename(0, path, neupath);
		set_dirty(err, path, 2);
	}

	Mgraf_mouse(ARROW);
	if (err < E_OK)
		err_alert(err);
}


/****************************************************************
*
* Informationen Åber selektiertes Objekt anzeigen
*
****************************************************************/

void dial_info(void)
{
	int wnr,obj,obj_type,ret;
	char path[128];
	long err;
	MYDTA *file;
	MYDTA g;
	DTA dta;


	if (1 != icsel(&wnr, &obj))
		return;

	ret = obj_typ(wnr, obj, path, &file, &obj_type);
	if (ret == -1)
	{
		info_spec(obj_type);
		return;
	}

	if ((obj_type & 0xff) == 'D')
	{
		info_disk(path, obj_type);
		return;
	}

	if ((obj_type & 0xff) == 'O')
	{
		path[strlen(path) - 1] = EOS;
	}

	if (file == NULL && ((obj_type & 0xff) == 'O' || ret == 0))
	{
		Mgraf_mouse(HOURGLASS);
		Fsetdta(&dta);
		err = Fsfirst(path, 0x37);
		Mgraf_mouse(ARROW);
		if (err != E_OK)
		{
			err_alert(err);
			return;
		}

		file = &g;
		g.time = dta.d_time;
		g.date = dta.d_date;
		g.filesize = dta.d_length;
		strcpy(g.filename, dta.d_fname);
		g.attrib = dta.d_attrib;
	}

	if ((obj_type & 0xff) == 'O')
	{
		info_folder(path, file);
		return;
	}

	if (ret == 0)
	{
		info_file(path, file);
		return;
	}
}


/****************************************************************
*
* Der Dialog "Maske setzen..."
*
****************************************************************/

void dial_maske(void)
{
	OBJECT *adr_maske;
	char oldmask[20];
	register char *src,*dst;
	int wnr, ret;


	if (0 >= (wnr = top_wnr()))
		return;

	rsrc_gaddr(0, T_MASKE, &adr_maske);
	dst = adr_maske[MASKE_TX].ob_spec.tedinfo->te_ptext;
	src = fenster[wnr].maske;
	fname_int(src, dst);		/* in GEM- Format wandeln */
	ret = do_dialog(adr_maske);

	if (ret != MASKE_OK && ret != MASKE_AL)
		return;

	strcpy(oldmask, src);

	if (ret == MASKE_AL)
		strcpy(src, "*.*");
	else
		fname_ext(dst, src);

	if (strcmp(oldmask, src))
	{
		upd_mask(wnr);
	}
}


/****************************************************************
*
* Der Eintrag "Neuer Ordner..." ist angewÑhlt worden.
*
****************************************************************/

void dial_neuord(void)
{
	OBJECT *adr_neuord;
	register char *input;
	char name[20];
	int wnr,ret;
	long doserr;


	if (0 >= (wnr = top_wnr()))
	{
		return;
	}

	rsrc_gaddr(0, T_NEUORD, &adr_neuord);
	input = adr_neuord[NEUORD_T].ob_spec.tedinfo->te_ptext;
	*input = EOS;

	ret = do_dialog(adr_neuord);
	if (ret != NEORD_OK || !*input)
	{
		return;
	}

	fname_ext(input, name);
	Mgraf_mouse(HOURGLASS);
	doserr = GDcreate(fenster[wnr].path, name);
	Mgraf_mouse(ARROW);

	if (doserr == EPTHNF)
	{
		fenster[wnr].path[3] = EOS;	/* zurÅck zur Root */
		upd_path(wnr, FALSE);
	}
}


/****************************************************************
*
* Callback fÅr dial_ordexi()
*
****************************************************************/

static int chk_ordexi(OBJECT *tree, int exitbutton)
{
	register char *input;
	register char *old;

	if (exitbutton == ORDEX_OK)
	{
		input = tree[ORDEXI_N].ob_spec.tedinfo->te_ptext;
		old   = tree[ORDEXI_O].ob_spec.tedinfo->te_ptext;
		if (*input == EOS || !strcmp(input, old))
			return FALSE;		/* Fehler */
	}

	return TRUE;				/* ok */
}


/****************************************************************
*
* Der Dialog "Ordner existiert schon".
* RÅckgabe  1, wenn OK geklickt.
*		   -1, wenn Abbruch
*           0, wenn benutzen
*
****************************************************************/

int dial_ordexi(char *name, int *do_redraw)
{
	OBJECT *adr_ordexi;
	register char *old, *new;
	int ret;


	rsrc_gaddr(0, T_ORDEXI, &adr_ordexi);
	old = adr_ordexi[ORDEXI_O].ob_spec.tedinfo->te_ptext;
	new = adr_ordexi[ORDEXI_N].ob_spec.tedinfo->te_ptext;
	fname_int(name, old);
	*new = EOS;
	Mgraf_mouse(ARROW);
	ret = do_exdialog(adr_ordexi, chk_ordexi, do_redraw);
	Mgraf_mouse(HOURGLASS);

	if (ret == ORDEX_BN)
		return 0;

	if (ret == ORDEX_OK)
	{
		fname_ext(new, name);
		return 1;
	}

	return -1;
}


/****************************************************************
*
* Callback fÅr dial_datexi()
*
****************************************************************/

static int chk_datexi(OBJECT *tree, int exitbutton)
{
	register char *input;

	if (exitbutton == DATEX_OK)
	{
		input = tree[ORDEXI_N].ob_spec.tedinfo->te_ptext;
		if (*input == EOS)
			return FALSE;		/* Fehler */
	}
	return TRUE;				/* ok */
}


/****************************************************************
*
* Der Dialog "Datei existiert schon".
* RÅckgabe 1, wenn OK geklickt.
*		  -1, wenn Abbruch
*          0, wenn öberspringen
*
****************************************************************/

int dial_datexi(char *name, int move_flag, int *do_redraw)
{
	OBJECT *adr_datexi;
	register char *old, *new, *title;
	int ret;


	rsrc_gaddr(0, T_DATEXI, &adr_datexi);
	title = adr_datexi[DATEXI_T].ob_spec.free_string;
#if LANGUAGE == LANG_GER
	strcpy(title + 20, (move_flag) ? "Verschieben" : "Kopieren");
#elif LANGUAGE == LANG_ENG
	strcpy(title + 20, (move_flag) ? "moving" : "copying");
#endif
	old = adr_datexi[DATEXI_O].ob_spec.tedinfo->te_ptext;
	new = adr_datexi[DATEXI_N].ob_spec.tedinfo->te_ptext;
	fname_int(name, old);
	fname_int(name, new);
	Mgraf_mouse(ARROW);
	ret = do_exdialog(adr_datexi, chk_datexi, do_redraw);
	Mgraf_mouse(HOURGLASS);

	if (ret == DATEX_SK)
		return 0;

	if (ret == DATEX_OK)
	{
		fname_ext(new, name);
		return 1;
	}

	return -1;
}


/****************************************************************
*
* Statische Variablen fÅr dial_format()
*
****************************************************************/

static int devno;
static int do_options;
static char _inter,_trk_incr,_side_incr,_clsize;


/****************************************************************
*
* Hilfsfunktion fÅr chk_format()
*
****************************************************************/

static void up_cnt(int n)
{
	subobj_draw(adr_format, FORMT_DT, n, NULL);
}


/****************************************************************
*
* Callback fÅr dial_format()
*
****************************************************************/

static int chk_format(OBJECT *tree, int exitbutton)
{
	long doserr;
	char *t,*s;
	char *d;
	char *dname;
	char ename[20];
	register int i,obj;


	s = tree[SEC_NUM].ob_spec.free_string;
	t = tree[TRK_NUM].ob_spec.free_string;
	fmtstat.tracks = atoi(t);
	fmtstat.sectors = atoi(s);
	obj = 0;

	if ((exitbutton == FORMT_DD) || (exitbutton == FORMT_HD))
	{
		int dummy;

		fmtstat.sides	= 2;
		if (selected(tree, FORMT_S1))
			form_button(tree, FORMT_S2, 1, &dummy);
		fmtstat.tracks = 80;
		fmtstat.sectors = (exitbutton == FORMT_HD) ? 18 : 9;
		subobj_draw(tree, TRK_NUM, fmtstat.tracks, NULL);
		subobj_draw(tree, SEC_NUM, fmtstat.sectors, NULL);
		return FALSE;
	}

	if (exitbutton == TRK_PLUS)
	{
		obj = TRK_NUM;
		d = &fmtstat.tracks;
		i = 1;
	}

	if (exitbutton == TRK_MINU)
	{
		obj = TRK_NUM;
		d = &fmtstat.tracks;
		i = -1;
	}

	if (exitbutton == SEC_MINU)
	{
		obj = SEC_NUM;
		d = &fmtstat.sectors;
		i = -1;
	}

	if (exitbutton == SEC_PLUS)
	{
		obj = SEC_NUM;
		d = &fmtstat.sectors;
		i = 1;
	}

	if (obj)
	{
		if (*d + i >= 5 && *d + i <= 99)
		{
			*d += i;
			subobj_draw(tree, obj, *d, NULL);
		}
		return FALSE;
	}

	fmtstat.sides = (selected(tree, FORMT_S1)) ? 1 : 2;

	if (exitbutton == FORMT_EX)
		do_options = TRUE;

	if (exitbutton != FORMT_OK && exitbutton != FORMT_IN)
		return TRUE;					/* Ende */

	dname = tree[FORMAT_T].ob_spec.tedinfo->te_ptext;
	fname_ext(dname, ename);
	Mgraf_mouse(HOURGLASS);
	objs_unhide(tree, FORMT_H1, FORMT_DT, 0);
	subobj_draw(tree, FORMT_H1, -1, NULL);
	doserr = format_floppy(devno, fmtstat.sides, fmtstat.tracks,
				 fmtstat.sectors, fmtstat.clustsize,
				 fmtstat.interlv, fmtstat.trkincr, fmtstat.sidincr,
				 up_cnt, (exitbutton == FORMT_IN), ename);
	objs_hide(tree, FORMT_H1, FORMT_DT, 0);
	subobj_draw(tree, FORMT_H1, -1, NULL);
	subobj_draw(tree, FORMT_DT, -1, NULL);
	Mgraf_mouse(ARROW);
	dirty_drives[devno] = TRUE;
	err_alert(doserr);
	return FALSE;						/* weiter */
}


/****************************************************************
*
* Hilfsfunktion fÅr dial_format()
*
****************************************************************/

static int chk_fmtopt(OBJECT *tree, int exitbutton)
{
	char *inter,*trk_incr,*side_incr,*clust;
	char *d;
	register int i,obj;


	inter     = tree[INT_NUM].ob_spec.free_string;
	trk_incr  = tree[SPV_NUM].ob_spec.free_string;
	side_incr = tree[SEV_NUM].ob_spec.free_string;
	clust     = tree[CLU_NUM].ob_spec.free_string;
	_inter     = atoi(inter);
	_trk_incr  = atoi(trk_incr);
	_side_incr = atoi(side_incr);
	_clsize	   = atoi(clust);
	obj = 0;

	if (exitbutton == INT_MINU || exitbutton == INT_PLUS)
	{
		obj = INT_NUM;
		d = &_inter;
		i = (exitbutton == INT_MINU) ? -1 : 1;
	}

	if (exitbutton == SPV_MINU || exitbutton == SPV_PLUS)
	{
		obj = SPV_NUM;
		d = &_trk_incr;
		i = (exitbutton == SPV_MINU) ? -1 : 1;
	}

	if (exitbutton == SEV_MINU || exitbutton == SEV_PLUS)
	{
		obj = SEV_NUM;
		d = &_side_incr;
		i = (exitbutton == SEV_MINU) ? -1 : 1;
	}

	if (exitbutton == CLU_MINU || exitbutton == CLU_PLUS)
	{
		obj = CLU_NUM;
		d = &_clsize;
		i = (exitbutton == CLU_MINU) ? -(_clsize >> 1) : _clsize;
	}

	if (obj)
	{
		if (*d + i >= 0 && *d + i <= 99)
		{
			*d += i;
			subobj_draw(tree, obj, *d, NULL);
		}
		return FALSE;		/* weitermachen */
	}

	if (exitbutton == FMOPT_OK)
	{
		fmtstat.interlv	= _inter;
		fmtstat.trkincr	= _trk_incr;
		fmtstat.sidincr	= _side_incr;
		fmtstat.clustsize = _clsize;
	}

	return TRUE;					/* Ende */
}


/****************************************************************
*
* Der Dialog "Formatieren".
*
****************************************************************/

void dial_format(void)
{
	int wnr,obj,obj_type;
	register int ret;
	char *s;
	char path[128];
#if LANGUAGE == LANG_GER
	static char alerts[] = "[2][Alle Daten auf Laufwerk X:|werden gelîscht!]"
						"[OK|Abbruch]";
	#define _DRV 28
#elif LANGUAGE == LANG_ENG
	static char alerts[] = "[2][All data at drive X:|will be deleted!]"
						"[OK|Cancel]";
	#define _DRV 22
#endif


	/* Feststellen, welches Laufwerk formatiert werden soll */
	/* ---------------------------------------------------- */

	if (1 != icsel(&wnr, &obj))
		return;

	ret = obj_typ(wnr, obj, path, NULL, &obj_type);
	if (ret != 1 || (obj_type & 0xff) != '\0D')	/* nur, wenn Disk- Icon */
		return;

	devno = path[0] - 'A';

	if (obj_type == 'FD')
		adr_format[FORMT_OK].ob_state &= ~DISABLED;
	else
		adr_format[FORMT_OK].ob_state |= DISABLED;

	if (obj_type == 'HD')
	{
		alerts[_DRV] = devno + 'A';
		if (2 == form_alert(2, alerts))
			return;
	}

	drv_to_str(adr_format[FORMAT_T].ob_spec.tedinfo->te_ptmplt, path[0]);
	if (fmtstat.sides == 2)
	{
		ob_dsel(adr_format, FORMT_S1);
		ob_sel(adr_format, FORMT_S2);
	}
	else
	{
		ob_dsel(adr_format, FORMT_S2);
		ob_sel (adr_format, FORMT_S1);
	}

	s = adr_format[TRK_NUM].ob_spec.free_string;
	ultoa(fmtstat.tracks, s, 10);
	s = adr_format[SEC_NUM].ob_spec.free_string;
	ultoa(fmtstat.sectors, s, 10);

	do
	{
		do_options = FALSE;
		objs_hide(adr_format, FORMT_H1, FORMT_DT, 0);
		do_exdialog(adr_format, chk_format, NULL);
		if (do_options)
		{
			s = adr_fmtopt[INT_NUM].ob_spec.free_string;
			ultoa(fmtstat.interlv, s, 10);
			s = adr_fmtopt[SPV_NUM].ob_spec.free_string;
			ultoa(fmtstat.trkincr, s, 10);
			s = adr_fmtopt[SEV_NUM].ob_spec.free_string;
			ultoa(fmtstat.sidincr, s, 10);
			s = adr_fmtopt[CLU_NUM].ob_spec.free_string;
			ultoa(fmtstat.clustsize, s, 10);

			do_exdialog(adr_fmtopt, chk_fmtopt, NULL);
		}
	}
	while (do_options);
}


/****************************************************************
*
* Statische Variablen fÅr dial_cpydsk()
*
****************************************************************/

static int slw,dlw;


/****************************************************************
*
* Hilfsfunktion fÅr chk_cpydsk()
*
****************************************************************/

static void fmt_feedb(char *action, int n)
{
	subobj_draw(adr_cpydsk, CPYDS_DO, -1, action);
	subobj_draw(adr_cpydsk, CPYDS_DT, n, NULL);
}


/****************************************************************
*
* Callback fÅr dial_cpydsk()
*
****************************************************************/

static int chk_cpydsk(OBJECT *tree, int exitbutton)
{
	char new_drv[2];
	long doserr;
	int tmp_drv;
	long ser;
	int bps,spc,res,nfats,ndirs,nsects,media,spf,spt,nsides,nhid,exec;
	int tracks;


	slw = *(tree[CPYDS_QU].ob_spec.free_string) - 'A';
	dlw = *(tree[CPYDS_ZI].ob_spec.free_string) - 'A';

	if (exitbutton == CPYDS_ZI)			/* Ziel- Laufwerk */
	{
		lw_anzeig:

		do
		{
			dlw++;				/* nÑchstes Laufwerk */
			dlw %= 32;			/* maximal 32 Laufwerke */
		}
		while (dlw != 0 && disk_type(dlw) != 'FD');

		new_drv[0] = dlw + 'A';
		new_drv[1] = EOS;
		subobj_draw(tree, exitbutton, -1, new_drv);
		return FALSE;
	}
		
	if (exitbutton == CPYDS_QU)			/* Quell- Laufwerk */
	{
		dlw = slw;
		goto lw_anzeig;
	}

	if (exitbutton == CPYDS_CN)			/* Abbruch */
		return TRUE;

	tmp_drv = tree[CPYDS_TM].ob_spec.tedinfo->te_ptext[0];
	fmtstat.tmpdrv = tmp_drv;		/* TemporÑrlaufwerk merken */
	if (tmp_drv == 0 || tmp_drv == '@')
		tmp_drv = -1;			/* Kein TemporÑrlaufwerk */
	else
	{
		tmp_drv -= 'A';
		if ((tmp_drv == slw) || (tmp_drv == dlw) || 
			 (tmp_drv < 0) || (tmp_drv > 15) ||
			 (0 == (Drvmap() & (1 << tmp_drv)))
			)
		{
			doserr = EDRIVE;
			fehler:
			Mgraf_mouse(ARROW);
			err_alert(doserr);
			return FALSE;
		}
	}
#if LANGUAGE == LANG_GER
	chg_msg[17] = slw + 'A';
	strncpy(chg_msg+4, "Quell", 5);
#elif LANGUAGE == LANG_ENG
	chg_msg[28] = slw + 'A';
	strncpy(chg_msg+11, "source", 6);
#endif
	if (slw == dlw && 2 == form_alert(1, chg_msg))
		return FALSE;
	Mgraf_mouse(HOURGLASS);
	doserr = read_bootsec(slw, &ser,&bps,&spc,&res,&nfats,&ndirs,&nsects,
					   &media,&spf,&spt,&nsides,&nhid,&exec);
	if (doserr != E_OK)
		goto fehler;
	if (nsides <= 0 || spt <= 0 || nsects <= 0)
	{
		doserr = EMEDIA;
		goto fehler;
	}

	objs_unhide(tree, CPYDS_SI, CPYDS_TR, CPYDS_SC, CPYDS_DO, CPYDS_DT,
				   CPYDS_H1, CPYDS_H2, CPYDS_H3, CPYDS_H4, 0);
	tracks = nsects/(nsides*spt);
/*	subobj_draw(tree, CPYDS_DO, -1, NULL);
	subobj_draw(tree, CPYDS_DT, -1, NULL); */
	subobj_draw(tree, CPYDS_H1, -1, NULL);
	subobj_draw(tree, CPYDS_H2, -1, NULL);
	subobj_draw(tree, CPYDS_H3, -1, NULL);
	subobj_draw(tree, CPYDS_H4, -1, NULL);
	subobj_draw(tree, CPYDS_SI, nsides, NULL);
	subobj_draw(tree, CPYDS_TR, tracks, NULL);
	subobj_draw(tree, CPYDS_SC, spt, NULL);
	doserr = diskcopy(slw, dlw, tmp_drv, nsides, tracks, spt, 
				   selected(tree, CPYDS_FM), fmt_feedb);
	Mgraf_mouse(ARROW);
	if (doserr != EWRPRO)
		dirty_drives[dlw] = TRUE;
	err_alert(doserr);
	objs_hide(tree, CPYDS_SI, CPYDS_TR, CPYDS_SC, CPYDS_DO, CPYDS_DT,
		           CPYDS_H1, CPYDS_H2, CPYDS_H3, CPYDS_H4, 0);
	subobj_draw(tree, CPYDS_SI, -1, NULL);
	subobj_draw(tree, CPYDS_TR, -1, NULL);
	subobj_draw(tree, CPYDS_SC, -1, NULL);
	subobj_draw(tree, CPYDS_DO, -1, NULL);
	subobj_draw(tree, CPYDS_DT, -1, NULL);
	subobj_draw(tree, CPYDS_H1, -1, NULL);
	subobj_draw(tree, CPYDS_H2, -1, NULL);
	subobj_draw(tree, CPYDS_H3, -1, NULL);
	subobj_draw(tree, CPYDS_H4, -1, NULL);
	return FALSE;						/* weiter */
}


/****************************************************************
*
* Der Dialog "Diskette kopieren".
*
****************************************************************/

void dial_cpydsk(void)
{
	
	objs_hide(adr_cpydsk, CPYDS_SI, CPYDS_TR, CPYDS_SC, CPYDS_DO, CPYDS_DT,
					  CPYDS_H1, CPYDS_H2, CPYDS_H3, CPYDS_H4, 0);
	adr_cpydsk[CPYDS_TM].ob_spec.tedinfo->te_ptext[0] = fmtstat.tmpdrv;
	do_exdialog(adr_cpydsk, chk_cpydsk, NULL);
}


/****************************************************************
*
* Callback fÅr dial_laufwe()
*
****************************************************************/

static int chk_laufwe(OBJECT *tree, int exitbutton)
{
	register char *textfield;

	textfield = tree[ICNCHG_L].ob_spec.tedinfo->te_ptext;
	/* *textfield &= 0x5f; toupper nicht nîtig wegen p_valid = 'A' */
	if ((exitbutton >= ICNCHG_F) && (exitbutton <= ICNCHG_R))
	{
		if (*textfield == EOS)
		{
			err_alert(EDRIVE);
			return FALSE;		/* Fehler */
		}
	}
	else
	if (exitbutton == ICNCHG_D || exitbutton == ICNCHG_P)
	{
		*textfield = EOS;
	}

	return TRUE;				/* ok */
}


/****************************************************************
*
* Der Dialog "Laufwerk anmelden".
*
****************************************************************/

void dial_laufwe(void)
{
	int ret,wnr,obj,x,y;
	register char *lw;


	lw = adr_laufwe[ICNCHG_L].ob_spec.tedinfo->te_ptext;
	if (1 == icsel(&wnr, &obj) && wnr == 0 && icon[obj - 1].icontyp < I_ORDNER)
	{
		lw[0] = icon[obj - 1].isdisk;
		lw[1] = EOS;
		adr_laufwe[ICNCH_DL].ob_state &= ~DISABLED;
		adr_laufwe[ICNCH_OK].ob_state &= ~DISABLED;
	}
	else
	{
		obj = 0;
		lw[0] = EOS;
		adr_laufwe[ICNCH_DL].ob_state |= DISABLED;
		adr_laufwe[ICNCH_OK].ob_state |= DISABLED;
	}

	ob_sel_dsel(adr_laufwe, ICNCH_OK, obj);
	ret = do_exdialog(adr_laufwe, chk_laufwe, NULL);

	if (ret == ICNCH_DL && obj)				/* "Entfernen */
	{
		kill_icon(obj);
		return;
	}

	if (ret < ICNCHG_F || ret > ICNCHG_P)		/* "Abbruch" */
		return;

	if (selected(adr_laufwe, ICNCH_OK))		/* "éndern" */
	{
		OBJECT *ob;

		ob = fenster[0].pobj+obj;
		x = ob->ob_x + fenster[0].pobj->ob_x;
		y = ob->ob_y + fenster[0].pobj->ob_y;
		kill_icon(obj);
	}
	else
	{
		objc_offset(adr_laufwe, ret, &x, &y);	/* "Neu" */
		graf_dragbox(
					adr_laufwe[ret].ob_width,
					adr_laufwe[ret].ob_height,
					x,
					y,
					&fenster[0].in,
					&x,
					&y);
	}

	make_icon(ret - ICNCHG_F + 1, lw[0], NULL, NULL, x, y);
}


/****************************************************************
*
* Der Dialog "Anwendung anmelden".
*
****************************************************************/

void dial_anwndg(void)
{
	char path[128];
	char *e1,*e2;
	OBJECT *adr_anwndg;
	int wnr,obj,typ,ret;
	APPLICATION *a;


	if (1 != icsel(&wnr, &obj))
		return;
	if (0 != (ret = obj_to_path(wnr, obj, path, NULL)))
		return;
	a = file_to_defapp(path);

	/* Wenn eine (angemeldete) Datei angewÑhlt wurde, wird ihre */
	/* Applikation angezeigt								*/
	/* -------------------------------------------------------- */

	if (a)
		strcpy(path, a -> path);

	rsrc_gaddr(0, T_ANWNDG, &adr_anwndg);
	e1 = adr_anwndg[ANWND_E1].ob_spec.tedinfo->te_ptext;
	e2 = adr_anwndg[ANWND_E2].ob_spec.tedinfo->te_ptext;
	strcpy(adr_anwndg[ANWNDG_T].ob_spec.free_string, get_name(path));

	/* Es wird geprÅft, ob die Applikation bereits angemeldet ist */
	/* ---------------------------------------------------------- */

	ob_dsel(adr_anwndg, ANWNDG_1);
	ob_dsel(adr_anwndg, ANWNDG_2);
	ob_dsel(adr_anwndg, ANWNDG_3);
	a = find_defapp(path);

	if (a)
	{
		strcpy(e1, a -> ext1);
		strcpy(e2, a -> ext2);
		typ = a -> typ;
		adr_anwndg[ANWND_DL].ob_state &= ~DISABLED;
	}
	else
	{
		*e1 = *e2 = EOS;
		typ = suffixtyp(path);
		adr_anwndg[ANWND_DL].ob_state |= DISABLED;
	}

#if	MAGIXDESK
	ob_sel_dsel(adr_anwndg, ANWND_SI, ((typ & 0xff) == 'p'));
#endif
	typ &= 0xff5f;						/* Groûschrift */

	switch(typ)
	{
		case 'GP':
			ob_sel(adr_anwndg, ANWNDG_1);
			break;

		case '_P':
			ob_sel(adr_anwndg, ANWNDG_2);
			break;

		case 'PP':
			ob_sel(adr_anwndg, ANWNDG_3);
			 break;
	}
	
	if (ANWND_CN == (ret = do_dialog(adr_anwndg)))
		return;

	if (ret == ANWND_DL)
	{
		if (a)
			a -> path[0] = EOS;
		return;
	}

	if (!a)
		a = find_defapp("");		/* freien Eintrag suchen */

	if (!a)
	{
#if LANGUAGE == LANG_GER
		form_alert(1, "[1][Alle EintrÑge belegt!][Abbruch]");
#elif LANGUAGE == LANG_ENG
		form_alert(1, "[1][All entries used!][Cancel]");
#endif
		return;
	}

	if (strlen(path) > 79)
	{
		err_alert(EPTHOV);
		return;
	}

	strcpy(a -> path, path);
	strcpy(a -> ext1, e1);
	strcpy(a -> ext2, e2);
	if (selected(adr_anwndg, ANWNDG_1))
		typ = 'GP';
	if (selected(adr_anwndg, ANWNDG_2))
		typ = '_P';
	if (selected(adr_anwndg, ANWNDG_3))
		typ = 'PP';
#if	MAGIXDESK
	if (selected(adr_anwndg, ANWND_SI))
		typ |= 0x20;			/* 'P' -> 'p' */
#endif
	a -> typ = typ;
}


/****************************************************************
*
* Der Dialog "Einstellungen Ñndern".
* Es wird hier undokumentiert davon ausgegangen, daû der VDI-
* treiber die Nummer der Auflîsung + 2 hat.
*
****************************************************************/

void dial_einste(void)
{
	register int ndrvr;
	int ret;
	char *txtheights, txtheight;
#if MAGIXDESK
	POPINFO pop_device;


	rsrc_gaddr(0, T_DEVICE, &pop_device.tree);
	pop_device.obnum = vdi_device + DEVICE_1 - 1;
	(adr_einste[EINST_DV].ob_spec.free_string) = (char *) &pop_device;
	adr_einste[EINST_DV].ob_type = G_POPUP;
#else
	register int i;

	for	(i = EINST_G1; i <= EINST_GL; i++)
		ob_sel_dsel(adr_einste, i, (i - EINST_G1 + 1) == vdi_device);
#endif
	txtheights = adr_einste[EINST_TX].ob_spec.tedinfo->te_ptext;
	txtheights[0] = '0';
	txtheights[1] = EOS;

	ob_sel_dsel(adr_einste, EINST_BL, status.cnfm_del);
	ob_sel_dsel(adr_einste, EINST_BK, status.cnfm_copy);

	ob_sel_dsel(adr_einste, EINST_KU, status.mode_ovwr == OVERWRITE);
	ob_sel_dsel(adr_einste, EINST_KA, status.mode_ovwr == BACKUP);
	ob_sel_dsel(adr_einste, EINST_KB, status.mode_ovwr == CONFIRM);

	ob_sel_dsel(adr_einste, EINST_KF, status.check_free);

	ob_sel_dsel(adr_einste, EINST_BD, status.use_fldl);
	ob_sel_dsel(adr_einste, EINST_RS, status.resident);
	ob_sel_dsel(adr_einste, EINST_VE, status.show_all);
	ob_sel_dsel(adr_einste, EINST_UR, status.clock);
	ob_sel_dsel(adr_einste, EINST_CA, status.use_cache);
	ob_sel_dsel(adr_einste, EINST_DN, status.dnam_init);

	ret = do_dialog(adr_einste);

	if (ret == EINST_LD)
	{
#if MAGIXDESK
		reload_status(-1);
#else
		reload_status(TRUE, -1);
#endif
		return;
	}

	if (ret == EINST_OK)		/* OK */
	{
		txtheight = atoi(txtheights);
		status.cnfm_del  = selected(adr_einste, EINST_BL);
		status.cnfm_copy = selected(adr_einste, EINST_BK);

		if (selected(adr_einste, EINST_KB))
			status.mode_ovwr = CONFIRM;
		else
		if (selected(adr_einste, EINST_KA))
			status.mode_ovwr = BACKUP;
		else
			status.mode_ovwr = OVERWRITE;

		status.check_free = selected(adr_einste, EINST_KF);
		status.use_fldl   = selected(adr_einste, EINST_BD);
		status.resident   = selected(adr_einste, EINST_RS);
		status.show_all   = selected(adr_einste, EINST_VE);
		status.clock      = selected(adr_einste, EINST_UR);
		status.use_cache  = selected(adr_einste, EINST_CA);
		status.dnam_init  = selected(adr_einste, EINST_DN);

#if MAGIXDESK
		ndrvr = pop_device.obnum - DEVICE_1 + 1;
#else
		ndrvr = vdi_device;
		for	(i = EINST_G1; i <= EINST_GL; i++)
		{
			if (selected(adr_einste, i))
				ndrvr = i - EINST_G1 + 1;
		}
#endif
		if (ndrvr == vdi_device && txtheight == 0)
			return;
#if	MAGIXDESK
		change_res(ndrvr, txtheight);
#else
		if (change_res(ndrvr, txtheight))
			_ende(2);
#endif
	}
}


/****************************************************************
*
* Der Dialog "Parameter eingeben".
*
****************************************************************/

int dial_ttppar(char *pgm, char *par)
{
	register char *dst1, *dst2;


	dst1 = adr_ttppar[TTPPAR_T].ob_spec.free_string;
	strcpy(dst1, get_name(pgm));

	dst1 = adr_ttppar[TTPPAR_1].ob_spec.tedinfo->te_ptext;
	dst2 = adr_ttppar[TTPPAR_2].ob_spec.tedinfo->te_ptext;
	if (*par)
	{
		strncpy(dst1, par, TTPLEN);
		if (strlen(par) >= TTPLEN)
		{
			dst1[TTPLEN] = EOS;
			strcpy(dst2, par + TTPLEN);
		}
	}

	if 	(TTPPA_OK != do_dialog(adr_ttppar))
		return 0;

	strcpy(par, dst1);
	strcat(par, dst2);

	return 1;
}
