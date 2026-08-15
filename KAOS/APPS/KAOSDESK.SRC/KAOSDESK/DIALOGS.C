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

#if		COUNTRY==FRG
	strcpy(alrts, "[2][Arbeit sichern nach|");
	strcat(alrts, inf_name);
	strcat(alrts, " ?][OK|Abbruch]");
#elif	COUNTRY==USA
	strcpy(alrts, "[2][Save status to file|");
	strcat(alrts, inf_name);
	strcat(alrts, " ?][OK|Cancel]");
#endif

	if (1 == form_alert(1, alrts))
		save_status(TRUE);
}


/****************************************************************
*
* Der Dialog "Programme".
*
****************************************************************/

void dial_prgrms(void)
{
	register int i;
	char oldstrings[ANZPROGRAMS][41];
	register char *quel,*ziel;


	for	(i = 0; i < ANZPROGRAMS; i++)
	{
		quel = ((adr_prgrms+i+PG_1)->ob_spec.tedinfo)->te_ptext;
		ziel = oldstrings[i];
		strcpy(ziel, quel);
	}

	if 	(PG_OK != do_dialog(adr_prgrms))
	{
		for	(i = 0; i < ANZPROGRAMS; i++)
		{
			ziel = ((adr_prgrms+i+PG_1)->ob_spec.tedinfo)->te_ptext;
			quel = oldstrings[i];
			strcpy(ziel, quel);
		}
	}
	else
	{
		for	(i = PG_2; i <= PG_3; i++)
		{
			ziel = ((adr_prgrms+i)->ob_spec.tedinfo)->te_ptext;
			while (*ziel && *ziel != ' ')
			{
				*ziel = toupper(*ziel);
				ziel++;
			}
		}
	}
}


/****************************************************************
*
* Der Eintrag "Åber KAOSDesk..." ist angewÑhlt worden.
*
****************************************************************/

void dial_about(void)
{
#if	MAGIXDESK == 0
	static char mems[] = "                    ";
	char *ziel,*quelle;


	ziel = ((adr_about+O_SPEICH)->ob_spec.tedinfo)->te_ptext;
	ltoa(memsize(), mems+10, 10);
	quelle = mems+strlen(mems)-7;
	strncpy(ziel+11, quelle, strlen(quelle));
	ltoa((long) Malloc(-1L), mems+10, 10);
	quelle = mems+strlen(mems)-7;
	strncpy(ziel+36, quelle, strlen(quelle));
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
#if		COUNTRY==FRG
			alrts = "[1][PAPIERKORB-INFORMATION| |"
				   	  "Schiebe Dateien, Ordner und|"
				   	  "Disks hier hinein,|"
				   	  "um sie zu lîschen.]"
				   "[  OK  ]";
				   break;
#elif	COUNTRY==USA
			alrts = "[1][TRASH INFORMATION| |"
				   	  "Drag files, folders and|"
				   	  "disks into this icon|"
				   	  "for deleting them.]"
				   "[  OK  ]";
				   break;
#endif
		case 'D_':
#if		COUNTRY==FRG
			alrts = "[1][DRUCKER-INFORMATION| |"
					  "Schiebe Dateien hier hinein,|"
				   	  "um sie auszudrucken.]"
				   "[  OK  ]";
				   break;
#elif	COUNTRY==USA
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
* Informationen Åber selektierte Diskette oder Festplatte anzeigen
*
****************************************************************/

void info_disk(char *path, int typ)
{
	int lw;
	long dummy;
	long err;
	int n_dat,n_vda,n_ord;
	long b_dat,b_vda;
	DISKINFO frei;
	OBJECT *adr_dskinf;
	register char *ziel;
	char altname[13];
	char neuname[13];


	Mgraf_mouse(HOURGLASS);
	lw = path[0] - 'A';
	rsrc_gaddr(0, T_DSKINF, &adr_dskinf);
	drv_to_str(((adr_dskinf+DSKINF_T)->ob_spec.tedinfo)->te_ptmplt, path[0]);
	ziel = ((adr_dskinf+DSKINF_T)->ob_spec.tedinfo)->te_ptext;
	err = read_dname(lw, altname);
	if (err != E_OK && err != EFILNF && err != ENMFIL)
	{
		fehler:
		Mgraf_mouse(ARROW);
		err_alert(err);
		return;
	}

	if (err == E_OK)
		fname_int(altname, ziel);
	else
		*ziel = EOS;

	err = Dfree(&frei, lw + 1);
	if (err != E_OK)
		goto fehler;

	ultoa(frei.b_total, (adr_dskinf+DI_CLUST)->ob_spec.free_string, 10);
	ultoa(frei.b_clsiz, (adr_dskinf+DI_SECCL)->ob_spec.free_string, 10);
	ultoa(frei.b_secsiz,(adr_dskinf+DI_BYSEC)->ob_spec.free_string, 10);
	frei.b_secsiz *= frei.b_clsiz;
	frei.b_total  *= frei.b_secsiz;
	frei.b_free   *= frei.b_secsiz;
	ultoa(frei.b_total, (adr_dskinf+DI_BTOTL)->ob_spec.free_string, 10);
	ultoa(frei.b_total - frei.b_free, (adr_dskinf+DI_BUSED)->ob_spec.free_string, 10);
	ultoa(frei.b_free, (adr_dskinf+DI_BFREE)->ob_spec.free_string, 10);

	err = walk_path(path, &dummy, &b_dat, &b_vda, &n_ord, &n_dat, &n_vda,
				 0x37, 0L, 0L, 0L);
	if (err != E_OK)
		goto fehler;
	ultoa(n_ord, (adr_dskinf+DI_N_ORD)->ob_spec.free_string, 10);
	ultoa(n_dat, (adr_dskinf+DI_N_DAT)->ob_spec.free_string, 10);
	ultoa(n_vda, (adr_dskinf+DI_N_VDA)->ob_spec.free_string, 10);
/*
	ultoa(b_dat, (adr_dskinf+DI_B_DAT)->ob_spec.free_string, 10);
	ultoa(b_vda, (adr_dskinf+DI_B_VDA)->ob_spec.free_string, 10);
*/

	if (typ == 'FD')
	{
		long ser;
		int bps,spc,res,nfats,ndirs,nsects,media,spf,spt,nsides,nhid,exec;

		err = read_bootsec(lw, &ser,&bps,&spc,&res,&nfats,&ndirs,&nsects,
						   &media,&spf,&spt,&nsides,&nhid,&exec);
		if (err != E_OK)
			goto fehler;

		if (nsides <= 0 || spt <= 0 || nsects <= 0)
		{
			err = EMEDIA;
			goto fehler;
		}

		ultoa(nsides, (adr_dskinf+DI_SIDES)->ob_spec.free_string, 10);
		ultoa(nsects/(nsides*spt), (adr_dskinf+DI_TRSID)->ob_spec.free_string, 10);
		ultoa(spt, (adr_dskinf+DI_SECTR)->ob_spec.free_string, 10);
		objs_unhide(adr_dskinf, DI_K1, DI_K2, DI_K3,
							DI_SIDES, DI_TRSID, DI_SECTR, 0);
		if (exec)
			objs_unhide(adr_dskinf, DI_EXEC, 0);
		else
			objs_hide(adr_dskinf, DI_EXEC, 0);
	}
	else
		objs_hide(adr_dskinf, DI_K1, DI_K2, DI_K3,
							DI_SIDES, DI_TRSID, DI_SECTR, DI_EXEC, 0);

	Mgraf_mouse(ARROW);
	if (DSKIN_OK != do_dialog(adr_dskinf))
		return;

	fname_ext(ziel, neuname);
	if (err == E_OK && !strcmp(neuname, altname))
		return;

	Mgraf_mouse(HOURGLASS);
	err = label(lw, neuname);
	Mgraf_mouse(ARROW);
	if (err != EWRPRO)
		dirty_drives[lw] = 2;

	err_alert(err);
}

void info_file(char *path, MYDTA *f)
{
	long err;
	DOSTIME dostime;
	register char *ziel;
	char neuname[13];
	unsigned char neuattr;


	ziel = ((adr_datinf+DATINF_T)->ob_spec.tedinfo)->te_ptext;

	fname_int(f->filename, ziel);
	time_to_str((adr_datinf+FI_ZEIT )->ob_spec.free_string, f->time);
	date_to_str((adr_datinf+FI_DATUM)->ob_spec.free_string, f->date);
	ultoa(f->filesize, (adr_datinf+FI_SIZE)->ob_spec.free_string, 10);
	ob_dsel(adr_datinf, FI_SETDA);
	ob_sel_dsel(adr_datinf, FI_RDONL, (f->attrib) & F_RDONLY);
	ob_sel_dsel(adr_datinf, FI_SYSTE, (f->attrib) & F_SYSTEM);
	ob_sel_dsel(adr_datinf, FI_HIDDE, (f->attrib) & F_HIDDEN);
	ob_sel_dsel(adr_datinf, FI_ARCHI, (f->attrib) & F_ARCHIVE);

	if (FI_OK != do_dialog(adr_datinf))
		return;
	fname_ext(ziel, neuname);
	neuattr  = selected(adr_datinf, FI_RDONL) * F_RDONLY;
	neuattr += selected(adr_datinf, FI_SYSTE) * F_SYSTEM;
	neuattr += selected(adr_datinf, FI_HIDDE) * F_HIDDEN;
	neuattr += selected(adr_datinf, FI_ARCHI) * F_ARCHIVE;
	
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

	if (err >= E_OK && neuattr != f->attrib)
	{
		err = Fattrib(path, TRUE, (int) neuattr);
		set_dirty(err, path, 2);
	}

	if (err >= E_OK && strcmp(neuname, f->filename))
	{
		char flag;
		char	neupath[128];

		flag = (neuattr & F_RDONLY && !(f->attrib & F_RDONLY));
		if (flag)
		{
			err = Fattrib(path, TRUE, (int) f->attrib);
			if (err < E_OK)
				goto atterr;
			}
		strcpy(neupath, path);
		*(get_name(neupath)) = EOS;
		strcat(neupath, neuname);
		err = Frename(0, path, neupath);
		if (flag && !err)
			err = Fattrib(neupath, TRUE, (int) neuattr);
		atterr:
		set_dirty(err, path, 2);
	}

	Mgraf_mouse(ARROW);
	if (err < E_OK)
		err_alert(err);
}

void info_folder(char *path, MYDTA *f)
{
	long err;
	long dummy;
	char *endp;
	int n_dat,n_vda,n_ord;
	long b_dat,b_vda;
	register char *ziel;
	char neuname[13];


	ziel = ((adr_ordinf+ORDINF_T)->ob_spec.tedinfo)->te_ptext;

	fname_int(f->filename, ziel);
	time_to_str((adr_ordinf+OI_ZEIT )->ob_spec.free_string, f->time);
	date_to_str((adr_ordinf+OI_DATUM)->ob_spec.free_string, f->date);

	endp = path + strlen(path);
	strcat(path, "\\");
	Mgraf_mouse(HOURGLASS);
	err = walk_path(path, &dummy, &b_dat, &b_vda, &n_ord, &n_dat, &n_vda,
				 0x37, 0L, 0L, 0L);
	*endp = EOS;
	if (err != E_OK)
		goto fehler;

	ultoa(n_ord, (adr_ordinf+OI_N_ORD)->ob_spec.free_string, 10);
	ultoa(n_dat, (adr_ordinf+OI_N_DAT)->ob_spec.free_string, 10);
	ultoa(n_vda, (adr_ordinf+OI_N_VDA)->ob_spec.free_string, 10);
	ultoa(b_vda, (adr_ordinf+OI_B_VDA)->ob_spec.free_string, 10);
	ultoa(b_dat + n_vda, (adr_ordinf+OI_BYTES)->ob_spec.free_string, 10);

	Mgraf_mouse(ARROW);
	if (OI_OK != do_dialog(adr_ordinf))
		return;
	fname_ext(ziel, neuname);

	Mgraf_mouse(HOURGLASS);
	if (strcmp(neuname, f->filename))
	{
		char neupath[128];

		strcpy(neupath, path);
		*(get_name(neupath)) = EOS;
		strcat(neupath, neuname);
		err = Frename(0, path, neupath);
		set_dirty(err, path, 2);
	}

	fehler:
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
	int wnr,obj,typ,ret;
	char path[128];
	long err;
	MYDTA *file;
	MYDTA g;
	DTA dta;


	if (1 != icsel(&wnr, &obj))
		return;

	ret = obj_typ(wnr, obj, path, &file, &typ);
	if (ret == -1)
	{
		info_spec(typ);
		return;
	}

	if ((typ & 0xff) == 'D')
	{
		info_disk(path, typ);
		return;
	}

	if ((typ & 0xff) == 'O')
		path[strlen(path) - 1] = EOS;

	if (file == NULL && ((typ & 0xff) == 'O' || ret == 0))
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

	if ((typ & 0xff) == 'O')
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
* Der Eintrag "Maske setzen..." ist angewÑhlt worden.
*
****************************************************************/

void dial_maske(void)
{
	OBJECT *adr_maske;
	char oldmask[20];
	register char *quel,*ziel;
	int wnr,ret;


	if (0 >= (wnr = top_wnr()))
		return;

	rsrc_gaddr(0, T_MASKE, &adr_maske);
	ziel = ((adr_maske+MASKE_TX)->ob_spec.tedinfo)->te_ptext;
	quel = fenster[wnr].maske;
	fname_int(quel, ziel);		/* in GEM- Format wandeln */
	ret = do_dialog(adr_maske);

	if (ret != MASKE_OK && ret != MASKE_AL)
		return;

	strcpy(oldmask, quel);

	if (ret == MASKE_AL)
		strcpy(quel, "*.*");
	else
		fname_ext(ziel, quel);

	if (strcmp(oldmask, quel))
		upd_mask(wnr);
}


/****************************************************************
*
* Der Eintrag "Neuer Ordner..." ist angewÑhlt worden.
*
****************************************************************/

void dial_neuord(void)
{
	OBJECT *adr_neuord;
	register char *eingabe;
	char name[20];
	int wnr,ret;
	long doserr;


	if (0 >= (wnr = top_wnr()))
	{
		return;
	}

	rsrc_gaddr(0, T_NEUORD, &adr_neuord);
	eingabe = ((adr_neuord + NEUORD_T)->ob_spec.tedinfo)->te_ptext;
	*eingabe = EOS;

	ret = do_dialog(adr_neuord);
	if (ret != NEORD_OK || !*eingabe)
	{
		return;
	}

	fname_ext(eingabe, name);
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
* Der Dialog "Ordner existiert schon".
* RÅckgabe  1, wenn OK geklickt.
*		   -1, wenn Abbruch
*           0, wenn Benutzen
*
****************************************************************/

static int chk_ordexi(OBJECT *tree, int exitbutton)
{
	register char *eingabe;
	register char *alt;

	if (exitbutton == ORDEX_OK)
	{
		eingabe = ((tree+ORDEXI_N)->ob_spec.tedinfo)->te_ptext;
		alt = ((tree+ORDEXI_O)->ob_spec.tedinfo)->te_ptext;
		if (*eingabe == EOS || !strcmp(eingabe, alt))
			return FALSE;		/* Fehler */
	}

	return TRUE;				/* ok */
}

int dial_ordexi(char *name, int *do_redraw)
{
	OBJECT *adr_ordexi;
	register char *alt,*neu;
	int ret;


	rsrc_gaddr(0, T_ORDEXI, &adr_ordexi);
	alt = ((adr_ordexi+ORDEXI_O)->ob_spec.tedinfo)->te_ptext;
	neu = ((adr_ordexi+ORDEXI_N)->ob_spec.tedinfo)->te_ptext;
	fname_int(name, alt);
	*neu = EOS;
	Mgraf_mouse(ARROW);
	ret = do_exdialog(adr_ordexi, chk_ordexi, do_redraw);
	Mgraf_mouse(HOURGLASS);

	if (ret == ORDEX_BN)
		return 0;

	if (ret == ORDEX_OK)
	{
		fname_ext(neu, name);
		return 1;
	}

	return -1;
}


/****************************************************************
*
* Der Dialog "Datei existiert schon".
* RÅckgabe 1, wenn OK geklickt.
*		  -1, wenn Abbruch
*          0, wenn öberspringen
*
****************************************************************/

static int chk_datexi(OBJECT *tree, int exitbutton)
{
	register char *eingabe;

	if (exitbutton == DATEX_OK)
	{
		eingabe = ((tree+ORDEXI_N)->ob_spec.tedinfo)->te_ptext;
		if (*eingabe == EOS)
			return FALSE;		/* Fehler */
		}
	return TRUE;				/* ok */
}

int dial_datexi(char *name, int move_flag, int *do_redraw)
{
	OBJECT *adr_datexi;
	register char *alt,*neu,*titel;
	int ret;


	rsrc_gaddr(0, T_DATEXI, &adr_datexi);
	titel = (adr_datexi+DATEXI_T)->ob_spec.free_string;
#if		COUNTRY==FRG
	strcpy(titel+20, (move_flag) ? "Verschieben" : "Kopieren");
#elif	COUNTRY==USA
	strcpy(titel+20, (move_flag) ? "moving" : "copying");
#endif
	alt = ((adr_datexi+DATEXI_O)->ob_spec.tedinfo)->te_ptext;
	neu = ((adr_datexi+DATEXI_N)->ob_spec.tedinfo)->te_ptext;
	fname_int(name, alt);
	fname_int(name, neu);
	Mgraf_mouse(ARROW);
	ret = do_exdialog(adr_datexi, chk_datexi, do_redraw);
	Mgraf_mouse(HOURGLASS);

	if (ret == DATEX_SK)
		return 0;

	if (ret == DATEX_OK)
	{
		fname_ext(neu, name);
		return 1;
	}

	return -1;
}


/****************************************************************
*
* Der Dialog "Formatieren".
*
****************************************************************/

static int devno;
static int do_options;
static char _inter,_spurver,_seitenver,_clsize;

static void up_cnt(int n)
{
	subobj_draw(adr_format, FORMT_DT, n, NULL);
}

static int chk_format(OBJECT *tree, int exitbutton)
{
	long doserr;
	char *t,*s;
	char *d;
	char *dname;
	char ename[20];
	register int i,obj;


	s = (tree + SEC_NUM)->ob_spec.free_string;
	t = (tree + TRK_NUM)->ob_spec.free_string;
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

	dname = ((tree+FORMAT_T)->ob_spec.tedinfo)->te_ptext;
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

static int chk_fmtopt(OBJECT *tree, int exitbutton)
{
	char *inter,*spurv,*seitv,*clust;
	char *d;
	register int i,obj;


	inter = (tree + INT_NUM)->ob_spec.free_string;
	spurv = (tree + SPV_NUM)->ob_spec.free_string;
	seitv = (tree + SEV_NUM)->ob_spec.free_string;
	clust = (tree + CLU_NUM)->ob_spec.free_string;
	_inter     = atoi(inter);
	_spurver   = atoi(spurv);
	_seitenver = atoi(seitv);
	_clsize	 = atoi(clust);
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
		d = &_spurver;
		i = (exitbutton == SPV_MINU) ? -1 : 1;
	}

	if (exitbutton == SEV_MINU || exitbutton == SEV_PLUS)
	{
		obj = SEV_NUM;
		d = &_seitenver;
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
		fmtstat.trkincr	= _spurver;
		fmtstat.sidincr	= _seitenver;
		fmtstat.clustsize	= _clsize;
	}

	return TRUE;					/* Ende */
}

void dial_format(void)
{
	int wnr,obj,typ;
	register int ret;
	char *s;
	char	path[128];
#if		COUNTRY==FRG
	static char alerts[] = "[2][Alle Daten auf Laufwerk X:|werden gelîscht!]"
						"[OK|Abbruch]";
	#define _DRV 28
#elif	COUNTRY==USA
	static char alerts[] = "[2][All data at drive X:|will be deleted!]"
						"[OK|Cancel]";
	#define _DRV 22
#endif


	/* Feststellen, welches Laufwerk formatiert werden soll */
	/* ---------------------------------------------------- */

	if (1 != icsel(&wnr, &obj))
		return;

	ret = obj_typ(wnr, obj, path, NULL, &typ);
	if (ret != 1 || (typ & 0xff) != '\0D')	/* nur, wenn Disk- Icon */
		return;

	devno = path[0] - 'A';

	if (typ == 'FD')
		(adr_format + FORMT_OK)->ob_state &= ~DISABLED;
	else
		(adr_format + FORMT_OK)->ob_state |= DISABLED;

	if (typ == 'HD')
	{
		alerts[_DRV] = devno + 'A';
		if (2 == form_alert(2, alerts))
			return;
	}

	drv_to_str(((adr_format+FORMAT_T)->ob_spec.tedinfo)->te_ptmplt, path[0]);
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

	s = (adr_format+TRK_NUM)->ob_spec.free_string;
	ultoa(fmtstat.tracks, s, 10);
	s = (adr_format+SEC_NUM)->ob_spec.free_string;
	ultoa(fmtstat.sectors, s, 10);

	do
	{
		do_options = FALSE;
		objs_hide(adr_format, FORMT_H1, FORMT_DT, 0);
		do_exdialog(adr_format, chk_format, NULL);
		if (do_options)
		{
			s = (adr_fmtopt+INT_NUM)->ob_spec.free_string;
			ultoa(fmtstat.interlv, s, 10);
			s = (adr_fmtopt+SPV_NUM)->ob_spec.free_string;
			ultoa(fmtstat.trkincr, s, 10);
			s = (adr_fmtopt+SEV_NUM)->ob_spec.free_string;
			ultoa(fmtstat.sidincr, s, 10);
			s = (adr_fmtopt+CLU_NUM)->ob_spec.free_string;
			ultoa(fmtstat.clustsize, s, 10);

			do_exdialog(adr_fmtopt, chk_fmtopt, NULL);
		}
	}
	while (do_options);
}


/****************************************************************
*
* Der Dialog "Diskette kopieren".
*
****************************************************************/

static int slw,dlw;

static void fmt_feedb(char *action, int n)
{
	subobj_draw(adr_cpydsk, CPYDS_DO, -1, action);
	subobj_draw(adr_cpydsk, CPYDS_DT, n, NULL);
}

static int chk_cpydsk(OBJECT *tree, int exitbutton)
{
	char newlw[2];
	long doserr;
	int tmplw;
	long ser;
	int bps,spc,res,nfats,ndirs,nsects,media,spf,spt,nsides,nhid,exec;
	int tracks;


	slw = *((tree + CPYDS_QU)->ob_spec.free_string) - 'A';
	dlw = *((tree + CPYDS_ZI)->ob_spec.free_string) - 'A';

	if (exitbutton == CPYDS_ZI)			/* Ziel- Laufwerk */
	{
		lw_anzeig:

		do
		{
			dlw++;				/* nÑchstes Laufwerk */
			dlw %= 32;			/* maximal 32 Laufwerke */
		}
		while (dlw != 0 && disk_type(dlw) != 'FD');

		newlw[0] = dlw + 'A';
		newlw[1] = EOS;
		subobj_draw(tree, exitbutton, -1, newlw);
		return FALSE;
	}
		
	if (exitbutton == CPYDS_QU)			/* Quell- Laufwerk */
	{
		dlw = slw;
		goto lw_anzeig;
	}

	if (exitbutton == CPYDS_CN)			/* Abbruch */
		return TRUE;

	tmplw = ((tree+CPYDS_TM)->ob_spec.tedinfo)->te_ptext[0];
	fmtstat.tmpdrv = tmplw;		/* TemporÑrlaufwerk merken */
	if (tmplw == 0 || tmplw == '@')
		tmplw = -1;			/* Kein TemporÑrlaufwerk */
	else
	{
		tmplw -= 'A';
		if ((tmplw == slw) || (tmplw == dlw) || 
			 (tmplw < 0) || (tmplw > 15) ||
			 (0 == (Drvmap() & (1 << tmplw)))
			)
		{
			doserr = EDRIVE;
			fehler:
			Mgraf_mouse(ARROW);
			err_alert(doserr);
			return FALSE;
		}
	}
#if		COUNTRY==FRG
	chg_msg[17] = slw + 'A';
	strncpy(chg_msg+4, "Quell", 5);
#elif	COUNTRY==USA
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
	doserr = diskcopy(slw, dlw, tmplw, nsides, tracks, spt, 
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


void dial_cpydsk(void)
{
	
	objs_hide(adr_cpydsk, CPYDS_SI, CPYDS_TR, CPYDS_SC, CPYDS_DO, CPYDS_DT,
					  CPYDS_H1, CPYDS_H2, CPYDS_H3, CPYDS_H4, 0);
	((adr_cpydsk+CPYDS_TM)->ob_spec.tedinfo)->te_ptext[0] = fmtstat.tmpdrv;
	do_exdialog(adr_cpydsk, chk_cpydsk, NULL);
}


/****************************************************************
*
* Der Dialog "Laufwerk anmelden".
*
****************************************************************/

static int chk_laufwe(OBJECT *tree, int exitbutton)
{
	register char *lw;

	lw = ((tree+ICNCHG_L)->ob_spec.tedinfo)->te_ptext;
	/* *lw &= 0x5f; toupper nicht nîtig wegen p_valid = 'A' */
	if (exitbutton >= ICNCHG_F && exitbutton <= ICNCHG_R)
	{
		if (*lw == EOS)
		{
			err_alert(EDRIVE);
			return FALSE;		/* Fehler */
		}
	}
	else
	if (exitbutton == ICNCHG_D || exitbutton == ICNCHG_P)
		*lw = EOS;

	return TRUE;				/* ok */
}

void dial_laufwe(void)
{
	int ret,wnr,obj,x,y;
	register char *lw;


	lw = ((adr_laufwe+ICNCHG_L)->ob_spec.tedinfo)->te_ptext;
	if (1 == icsel(&wnr, &obj) && wnr == 0 && icon[obj-1].icontyp < I_ORDNER)
	{
		lw[0] = icon[obj-1].isdisk;
		lw[1] = EOS;
		(adr_laufwe+ICNCH_DL)->ob_state &= ~DISABLED;
		(adr_laufwe+ICNCH_OK)->ob_state &= ~DISABLED;
	}
	else
	{
		obj = 0;
		lw[0] = EOS;
		(adr_laufwe+ICNCH_DL)->ob_state |= DISABLED;
		(adr_laufwe+ICNCH_OK)->ob_state |= DISABLED;
	}

	ob_sel_dsel (adr_laufwe, ICNCH_OK, obj);
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
					(adr_laufwe+ret)->ob_width,
					(adr_laufwe+ret)->ob_height,
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
	e1 = ((adr_anwndg+ANWND_E1)->ob_spec.tedinfo)->te_ptext;
	e2 = ((adr_anwndg+ANWND_E2)->ob_spec.tedinfo)->te_ptext;
	strcpy((adr_anwndg + ANWNDG_T) -> ob_spec.free_string, get_name(path));

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
		(adr_anwndg + ANWND_DL) -> ob_state &= ~DISABLED;
	}
	else
	{
		*e1 = *e2 = EOS;
		typ = suffixtyp(path);
		(adr_anwndg + ANWND_DL) -> ob_state |= DISABLED;
	}

#if	MAGIXDESK
	ob_sel_dsel(adr_anwndg, ANWND_SI,((typ & 0xff) == 'p'));
#endif
	typ &= 0xff5f;						/* Groûschrift */

	switch(typ)
	{
		case 'GP': ob_sel(adr_anwndg, ANWNDG_1);
				 break;
		case '_P': ob_sel(adr_anwndg, ANWNDG_2);
				 break;
		case 'PP': ob_sel(adr_anwndg, ANWNDG_3);
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
#if		COUNTRY==FRG
		form_alert(1, "[1][Alle EintrÑge belegt!][Abbruch]");
#elif	COUNTRY==USA
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
	char *txtheights,txtheight;
#if MAGIXDESK
	POPINFO pop_device;


	rsrc_gaddr(0, T_DEVICE, &pop_device.tree);
	pop_device.obnum = vdi_device + DEVICE_1 - 1;
	((adr_einste+EINST_DV)->ob_spec.free_string) = (char *) &pop_device;
	(adr_einste+EINST_DV)->ob_type = G_POPUP;
#else
	register int i;

	for	(i = EINST_G1; i <= EINST_GL; i++)
		ob_sel_dsel(adr_einste, i, (i - EINST_G1 + 1) == vdi_device);
#endif
	txtheights = ((adr_einste+EINST_TX)->ob_spec.tedinfo)->te_ptext;
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

		status.check_free	= selected(adr_einste, EINST_KF);
		status.use_fldl	= selected(adr_einste, EINST_BD);
		status.resident	= selected(adr_einste, EINST_RS);
		status.show_all	= selected(adr_einste, EINST_VE);
		status.clock		= selected(adr_einste, EINST_UR);
		status.use_cache	= selected(adr_einste, EINST_CA);
		status.dnam_init	= selected(adr_einste, EINST_DN);

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
	register char *ziel1,*ziel2;


	ziel1 = (adr_ttppar+TTPPAR_T)->ob_spec.free_string;
	strcpy(ziel1, get_name(pgm));

	ziel1 = ((adr_ttppar+TTPPAR_1)->ob_spec.tedinfo)->te_ptext;
	ziel2 = ((adr_ttppar+TTPPAR_2)->ob_spec.tedinfo)->te_ptext;
	if (*par)
	{
		strncpy(ziel1, par, TTPLEN);
		if (strlen(par) >= TTPLEN)
		{
			ziel1[TTPLEN] = EOS;
			strcpy(ziel2, par + TTPLEN);
		}
	}

	if 	(TTPPA_OK != do_dialog(adr_ttppar))
		return 0;

	strcpy(par, ziel1);
	strcat(par, ziel2);

	return 1;
}
