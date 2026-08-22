/*********************************************************************
*
* Dieses Modul enthÑlt die Bearbeitung aller Dateioperationen, also
*  Lîschen, Kopieren, Verschieben, Umbenennen.
* (mit BedienerfÅhrung Åber Dialogboxen)
*
*********************************************************************/

#include <string.h>
#include <stdlib.h>
#include <mgx_dos.h>	/* include before tos.h */
#include <tos.h>
#include <tosdefs.h>
#include "k.h"

static int _move_flag;
static long _bsize;			/* Puffergrîûe fÅrs Kopieren */
static char *_dest_path;
static void down_cnt(int ord, char *aktion, char *path);


/****************************************************************
*
* Gibt den Diskettentyp zurÅck.
*
****************************************************************/

int disk_type(int drvnr)
{
	register int i;

	drvnr += 'A';
	for	(i = 0; i < n_icons; i++)
	{
		if (icon[i].isdisk == drvnr)
		{
			switch (icon[i].icon_type)
			{
				case I_FLPDSK: return 'FD';
				case I_HRDDSK: return 'HD';
				case I_RAMDSK: return 'RD';
			}
		}
	}
	return '\0\0';		/* undefiniert */
}


/*********************************************************************
*
* Durch GedrÅckthalten beider Umschalttasten kann ein Vorgang
* nach Nachfrage abgebrochen werden.
*
*********************************************************************/

int qbreak(void)
{
	if ((Kbshift(-1) & (K_LSHIFT | K_RSHIFT)) == (K_LSHIFT | K_RSHIFT))
	{
#if LANGUAGE == LANG_GER
		if (1 == form_alert(1, "[1][Vorgang abbrechen ?][  JA  | NEIN ]"))
#elif LANGUAGE == LANG_ENG
		if (1 == form_alert(1, "[1][Stop process ?][  YES  | NO ]"))
#endif
			return TRUE;
	}

	return FALSE;
}


/*********************************************************************
*
* Setzt das interne Dirty-Flag von Laufwerk <lw>
*
*********************************************************************/

void set_dirty(long err, char *path, char val)
{
	if ((err != EWRPRO) && (err != EDRIVE) && (err != EFILNF) &&
	    (err != EPTHNF) && (err != EACCDN))
	{
	 	if (dirty_drives[path[0] - 'A'] == 1)
	 	{
	 		return;		/* schon "dirty" */
	 	}

		dirty_drives[path[0] - 'A'] = val;
	}
}


/*********************************************************************
*
* Ermittelt den Volume-Namen von Laufwerk <lw>
*
* Sichert die aktuelle DTA.
* Funktioniert nur fÅr das klassische 8+3-Dateinamenformat
*
*********************************************************************/

long read_dname(int lw, char *name)
{
	DTA dta;
	DTA *old_dta;
	static char path[] = "_:\\*.*";
	long doserr;

	old_dta = Fgetdta();
	Fsetdta(&dta);

	path[0] = lw + 'A';
	doserr = Fsfirst(path, F_VOLUME);
	if (doserr != E_OK)
		*name = EOS;
	else
		strcpy(name, dta.d_fname);

	Fsetdta(old_dta);
	return doserr;
}


/*********************************************************************
*
* Lîscht eine Datei
*
*********************************************************************/

long GFdelete(char *path, char *name)
{
	char all[128];
	long doserr;


	strcpy(all,path);
	if (name)
	{
		strcat(all, name);
	}

#if LANGUAGE == LANG_GER
	down_cnt(2, "lîsche Datei", all);
#elif LANGUAGE == LANG_ENG
	down_cnt(2, "deleting file", all);
#endif

	doserr = Fdelete(all);
	set_dirty(doserr, path, 1);
	err_file = all;

	return err_alert(doserr);
}


/*********************************************************************
*
* Lîscht einen Ordner
*
*********************************************************************/

long GDdelete(char *path, char *name)
{
	char all[128];
	long doserr;


	strcpy(all,path);
	if (name)
	{
		strcat(all, name);
	}
#if LANGUAGE == LANG_GER
	down_cnt(2, "lîsche Ordner", all);
#elif LANGUAGE == LANG_ENG
	down_cnt(2, "deleting folder", all);
#endif
	doserr = Ddelete(all);
	if (doserr == EACCDN || doserr == ENSMEM)
	{
		free_drive(path[0] - 'A');	/* GEMDOS- Speicher freigeben */
		doserr = Ddelete(all);		/* und nochmal versuchen */
	}

	if (doserr == ENSMEM)			/* Fehler von KAOS 1.2 */
		doserr = EACCDN;

	set_dirty(doserr, path, 1);
	err_file = all;

	return err_alert(doserr);
}


/*********************************************************************
*
* PrÅft, ob die Datei schon existiert bzw. Åberschrieben werden darf.
*
* RÅckgabe:	EFILNF		Datei darf Åberschrieben werden
*			EACCDN		Datei darf nicht Åberschrieben werden
*			< 0			Diskettenfehler
*
*********************************************************************/

static long does_exist(char *all, DTA *file)
{
	DTA	dta;
	long doserr;


	if (settings.mode_ovwr == OVERWRITE)
		return EFILNF;

	Fsetdta(&dta);
	doserr = Fsfirst(all, 0x37);
	if (doserr == E_OK && settings.mode_ovwr == BACKUP)
	{
		if ((file->d_date > dta.d_date) ||
			 ((file->d_date == dta.d_date) && (file->d_time > dta.d_time)))
		doserr = EFILNF;	/* Ñltere Datei: Åberbraten */
	}

	if (doserr == E_OK)
		doserr = EACCDN;

	return doserr;
}

/*********************************************************************
*
* Erstellt eine Datei <file->d_fname> in <path>.
* Ggf. wird andere Datei erzeugt und <file->d_fname> verÑndert.
*
* RÅckgabe: E_OK		Åberspringen
*		  EBREAK		Abbruch
*		  > 0		Dateihandle
*		  < 0		Fehlercode
*
*********************************************************************/

long GFcreate(char *path, DTA *file)
{
	char all[128], oldname[14];
	long doserr;
	unsigned int attr;
	int ret, do_redraw;
	char query = (settings.mode_ovwr != OVERWRITE);


	attr = file->d_attrib;
	attr &= ~F_RDONLY;
	do
	{
		strcpy(all, path);
		strcat(all, file->d_fname);
		if (query)
		{
			query = FALSE;
			/* prÅfen, ob Datei schon existiert */
			doserr = does_exist(all, file);
			/* ernste Diskettenfehler: sofort abbrechen */
			if (doserr != EACCDN && doserr != EFILNF)
			{
				err_file = all;
				return err_alert(doserr);
			}

			/* Datei ist schon gesichert, Åberspringen */
			if (doserr == EACCDN && settings.mode_ovwr == BACKUP)
				return E_OK;

			if (doserr == EFILNF)
				doserr = Fcreate(all, attr);
		}
		else
			doserr = Fcreate(all, attr);

		if (doserr == EACCDN)
		{
			strcpy(oldname, file->d_fname);
			ret = dial_file_exists(file->d_fname, _move_flag, &do_redraw);
			if (strcmp(file->d_fname, oldname))
				query = TRUE;
			if (do_redraw)
				down_cnt(-1, NULL, NULL);
			if (ret == 0)		/* öberspringen */
				return E_OK;
			if (ret < 0)			/* Abbruch */
				return EBREAK;
		}
	}
	while (doserr == EACCDN);

	set_dirty(doserr, path, 1);
	if (doserr < E_OK)
	{
		err_file = all;
		err_alert(doserr);
	}

	return doserr;
}


/*********************************************************************
*
* Verschiebt eine Datei <name> in <path> nach <zpath>.
* Ggf. wird andere Datei erzeugt und <name> verÑndert.
*
*********************************************************************/

long GFrename(char *path, DTA *srcfile, char *zpath)
{
	char alls[128],alld[128],oldname[14];
	long doserr;
	int ret, do_redraw;
	char query = settings.mode_ovwr;


	strcpy(alls, path);
	strcat(alls, srcfile->d_fname);
#if LANGUAGE == LANG_GER
	down_cnt(2, "verschiebe Datei", alls);
#elif LANGUAGE == LANG_ENG
	down_cnt(2, "moving file", alls);
#endif
	do
	{
		strcpy(alld, zpath);
		strcat(alld, srcfile->d_fname);

		doserr = Frename(0, alls, alld);
		if (doserr == EACCDN && query != OVERWRITE)
		{
			doserr = does_exist(alld, srcfile);
			/* Datei schon gesichert: nur lîschen */
			if (doserr == EACCDN && settings.mode_ovwr == BACKUP)
			{
				return Fdelete(alls);
			}

			if (doserr == EFILNF)
			{
				doserr = EACCDN;
				query = OVERWRITE;
			}
		}

		if ((doserr == EACCDN) && (query == OVERWRITE))
		{
			doserr = Fdelete(alld);
			set_dirty(doserr, alld, 1);
			if (doserr != E_OK && doserr != EACCDN)
			{
				err_file = alld;
				return err_alert(doserr);
			}

			if (doserr == E_OK)
				doserr = Frename(0, alls, alld);
			set_dirty(doserr, alld, 1);
		}
		else
			set_dirty(doserr, alld, 2);

		query = FALSE;
		if (doserr == EACCDN)
		{
			strcpy(oldname, srcfile->d_fname);
			ret = dial_file_exists(srcfile->d_fname, TRUE, &do_redraw);
			if (strcmp(srcfile->d_fname, oldname))
				query = TRUE;
			if (do_redraw)
				down_cnt(-1, NULL, NULL);
			if (ret == 0)		/* öberspringen */
				return E_OK;
			if (ret < 0)			/* Abbruch */
				return doserr;
		}
	}
	while (doserr == EACCDN);

	err_file = alls;
	return err_alert(doserr);
}


/*********************************************************************
*
* Kopiert eine Datei <file->d_fname> in <path> nach <zpath>.
* Ggf. wird andere Datei erzeugt und <file->d_fname> verÑndert.
*
*********************************************************************/

long GFcopy(char *path, DTA *srcfile, char *zpath)
{
	char alls[128], alld[128];
	long doserr;
	int shdl,dhdl;
	long bsize,lbytes;
	char *buf;


	/* Speicher holen, Quellpfad zusammenstellen */
	/* ----------------------------------------- */

	if (qbreak())
		return EBREAK;

	bsize = _bsize;
	if (bsize <= 0L)
		bsize = (long) Malloc(-1L) - 4096L;

	if (bsize > 65536L && settings.use_fldl)
		bsize -= 32768L;		/* Platz fÅr FlyDials */

	if (bsize < 4096L || (NULL == (buf = Malloc(bsize))))
		return err_alert(ENSMEM);

	dhdl = shdl = -1;			/* Dateien sind noch nicht geîffnet */

	strcpy(alls, path);
	strcat(alls, srcfile->d_fname);

	/* Kopierschleife */
	/* -------------- */

	do
	{
		if (qbreak())
		{
			abbruch:
			doserr = EBREAK;
			break;
		}

		/* Im Backup-Modus wird erst die Zieldatei geîffnet */
		/* ------------------------------------------------ */

		if (dhdl < 0 && settings.mode_ovwr == BACKUP)
			lbytes = 0L;

		/* Ansonsten die Quelldatei gelesen und ggf. geîffnet */
		/* -------------------------------------------------  */

		else
		{
#if LANGUAGE == LANG_GER
			down_cnt(2, "lese Datei", alls);
#elif LANGUAGE == LANG_ENG
			down_cnt(2, "reading file", alls);
#endif
			if (shdl < 0)
			{
				doserr = Fopen(alls, RMODE_RD);
				if (((int) doserr) == -4)	/* NUL: */
					doserr = EACCDN;
				if (doserr < E_OK)
					goto source_error;
				shdl = (int) doserr;
			}
			doserr = Fread(shdl, bsize, buf);
			if (doserr < E_OK)
			{
				source_error:
				err_file = alls;
				break;			/* Fehler beim Lesen */
			}
			if (doserr >= E_OK && doserr < bsize)
			{
				Fclose(shdl);
				shdl = -2;			/* Flag fÅr das Ende */
			}
			if (qbreak())
				goto abbruch;
			lbytes = doserr;			/* soviele Bytes sind gelesen */
		}

		/* Die Zieldatei wird geîffnet */
		/* --------------------------- */

		if (dhdl < 0)
		{
			strcpy(alld, zpath);
			strcat(alld, srcfile->d_fname);
#if LANGUAGE == LANG_GER
			down_cnt(2, "schreibe Datei", alld);
#elif LANGUAGE == LANG_ENG
			down_cnt(2, "writing file", alld);
#endif
			doserr = GFcreate(zpath, srcfile);
			strcpy(alld, zpath);
			strcat(alld, srcfile->d_fname);	/* Name ggf. geÑndert */
			if (doserr <= E_OK)	/* < 0: Fehler oder Abbruch */
				break;			/* = 0: öberspringen 	   */
			dhdl = (int) doserr;
		}

		/* <lbytes> gelesene Bytes werden geschrieben */
		/* ------------------------------------------ */

		if (lbytes)
		{
#if LANGUAGE == LANG_GER
			down_cnt(2, "schreibe Datei", alld);
#elif LANGUAGE == LANG_ENG
			down_cnt(2, "writing file", alld);
#endif
			doserr = Fwrite(dhdl, lbytes, buf);
			if (doserr < E_OK)
				err_file = alld;
			if (doserr >= 0L && doserr < lbytes)
			{
#if LANGUAGE == LANG_GER
				form_alert(1, "[3][Zieldisk voll!][Abbruch]");
#elif LANGUAGE == LANG_ENG
				form_alert(1, "[3][Destination disk full!][Abort]");
#endif
				doserr = -1L;
			}
		}
	}
	while (shdl != -2 && doserr >= E_OK);

	if (doserr > E_OK)
		doserr = E_OK;
	if (shdl > 0)
		Fclose(shdl);
	if (dhdl > 0)
	{
		Fdatime((DOSTIME *) &(srcfile->d_time), dhdl, RMODE_WR);
		Fclose(dhdl);
	}
	if (dhdl > 0 && doserr != E_OK && doserr != EWRPRO && doserr != EDRVNR)
		Fdelete(alld);
	Mfree(buf);
	if (err_file)
		err_alert(doserr);
	return doserr;
}


/*********************************************************************
*
* Erstellt einen Ordner <name> in <path>.
* Ggf. wird anderer Ordner erzeugt und <name> verÑndert.
*
*********************************************************************/

long GDcreate(char *path, char *name)
{
	char all[128];
	long doserr;
	int ret, do_redraw;


	do
	{
		strcpy(all, path);
		strcat(all, name);
#if LANGUAGE == LANG_GER
		down_cnt(2, "erstelle Ordner", all);
#elif LANGUAGE == LANG_ENG
		down_cnt(2, "creating folder", all);
#endif
		doserr = Dcreate(all);
		if (doserr == EACCDN)
		{
			if (settings.mode_ovwr == BACKUP)
				return E_OK;		/* Bei BACKUP immer benutzen */

			ret = dial_folder_exists(name, &do_redraw);
			if (do_redraw)
			{
				down_cnt(-1, NULL, NULL);
			}

			if (ret == 0)		/* Benutzen */
				return E_OK;

			if (ret < 0)			/* Abbruch */
				return EBREAK;
		}
	}
	while (doserr == EACCDN);

	set_dirty(doserr, path, 1);
	err_file = all;
	return err_alert(doserr);
}


/*********************************************************************
*
* Hilfsfunktion fÅr walk_path()
*
*********************************************************************/

static long _first_error;	/* avoid a large number of error messages */
static long _nbytes,_hbytes;
static int _nfiles,_hfiles,_folders;
static long (*_before_cb)(char *path, DTA *file);
static long (*_after_cb) (char *path, DTA *file);
static int depth, _attr_msk;
static long _clsize, _clusters;
static char *_path;
static char *_mask;


static long _walk_path(void)
{
	DTA	dta;
	int dirlen;			/* Anzahl EintrÑge in diesem Verzeichnis */
	long errcode;
	char *endp;


	errcode = strlen(_path);
	endp = _path + errcode;
	depth++;
	if (depth > 8 || errcode > 116)
	{
		depth--;
		errcode = EPTHOV;
		goto error;
	}

	if (qbreak())
	{
		return EBREAK;
	}

	Fsetdta(&dta);
	strcpy(endp, _mask);
	dirlen = 0;
	errcode = Fsfirst(_path, _attr_msk);
	while (errcode == E_OK)
	{
		/* user break */

		if (qbreak())
		{
			return EBREAK;
		}

		/* skip "." and ".." entries */

		if (dta.d_fname[0] == '.')
		{
			if (dta.d_fname[1] == '.' || dta.d_fname[1] == EOS)
				goto next;
		}

		/* callback BEFORE operation */

		if (_before_cb != NULL)
		{
			*endp = EOS;
			if (E_OK != (errcode = _before_cb(_path, &dta)))
			{
				return errcode;
			}
		}

		/* folder: recursion, otherwise regular file */

		if (dta.d_attrib & F_SUBDIR)
		{
			_folders++;
			strcpy(endp, dta.d_fname);
			strcat(endp, "\\");
			errcode = _walk_path();	/* REKURSION */
			*endp = EOS;
			if (errcode)
				return errcode;
		}
		else
		{
			if (dta.d_attrib & (F_HIDDEN | F_SYSTEM))
			{
				_hfiles++;
				_hbytes += dta.d_length;
			}
			else
			{
				_nfiles++;
				_nbytes += dta.d_length;
			}

			if (_clsize)
			{
				_clusters += dta.d_length/_clsize;
				if (dta.d_length % _clsize)
				{
					_clusters++;	/* auf ganze Cluster aufrunden */
				}
			}
		}

		/* callback AFTER operation */

		if ((errcode == E_OK) && (_after_cb != NULL))
		{
			*endp = EOS;
			if (E_OK != (errcode = _after_cb(_path, &dta)))
				return errcode;
		}

		next:
		dirlen++;

		/* user break */
		if (qbreak())
			return EBREAK;

		if (errcode == E_OK)
		{
			Fsetdta(&dta);
			errcode = Fsnext();
		}
	} /* ENDWHILE */

	depth--;

	if (_clsize && (endp > _path + 3))		/* nicht Root */
	{
		long dirn;

		dirn = dirlen << 5;			/* 32 Bytes pro Eintrag */
		_clusters += dirn/_clsize;
		if (dirn % _clsize)
		{
			_clusters++;
		}
	}

	if (errcode == EFILNF || errcode == ENMFIL)
	{
		errcode = E_OK;
	}

	#if 0
	if (errcode)
	{
		Cconws(_path);
		Cconws("\r\n");
	}
	#endif

	error:
	if (_first_error == 0)
	{
		_first_error = errcode;
		err_alert(_first_error);
	}

	if (errcode == EPTHOV)
	{
		errcode = E_OK;
	}

	return errcode;
}


/*********************************************************************
*
* DurchlÑuft einen Pfad und fÅhrt bei jeder gefundenen Datei
* vorher ein Programm <before>, hinterher <after> aus.
* Sind <before> und <after> == NULL, werden nur Informationen gesammelt.
* gibt ggf. DOS- Fehlercode zurÅck.
*
*********************************************************************/

long walk_path
(
	char *path, long *clusters, long *nbytes,
	long *hbytes, int *folders,
	int *nfiles, int *hfiles,
	int attr_msk, long clsize,
	long (*before_cb)(char *path, DTA *file),
	long (*after_cb) (char *path, DTA *file)
)
{
	long errcode;


	_first_error = 0L;
	_clusters = 0L;
	_nbytes = _hbytes = _nfiles = _hfiles = _folders = depth = 0;
	_before_cb = before_cb;
	_after_cb = after_cb;
	_attr_msk = attr_msk;
	_clsize = clsize;
	_path = path;
	_mask = "*.*";
	cache_on(path);
	errcode = _walk_path();
	cache_off(path);
	*nbytes   = _nbytes;		/* Bytes in normalen Dateien */
	*hbytes   = _hbytes;		/* Bytes in versteckten Dateien */
	*clusters = _clusters;		/* benîtigte Cluster */
	*folders  = _folders;		/* Anzahl Ordner */
	*nfiles   = _nfiles;		/* Anzahl normale Dateien */
	*hfiles   = _hfiles;		/* Anzahl versteckte Dateien */
	return errcode;
}


/*********************************************************************
*
* FÅhrt den Dialog Åber die Anzahl zu lîschender/kopierender/
*  verschiebender Dateien.
*
* down_cnt:
*  ord = -1: Die ganze Dialogbox wird neu aufgebaut
*  ord =  2: Aktion anzeigen
*  ord =  0: Dateien herunterzÑhlen
*  ord =  1: Dateien heraufzÑhlen
*
*********************************************************************/

static long err;
static long n_clu,n_siz;
static int  n_dat,n_ord;
static int  is_cpmvdl = FALSE;
static GRECT cg;
static void *cnf_flyinf;

void end_confirm(void)
{
	if (is_cpmvdl)
	{
		form_xdial(FMD_FINISH, NULL, &cg, &cnf_flyinf);
		is_cpmvdl = FALSE;
	}
}

static void down_cnt(int ord, char *aktion, char *path)
{
	int  obj, cnt;


	if (!is_cpmvdl)
		return;
	if (ord == -1)
	{
		objc_draw(tree_file_ops, ROOT, MAX_DEPTH, &cg);
		return;
	}
	if (ord == 2)
	{
		char *s,*t;

		s = tree_file_ops[CPMVD_DT].ob_spec.free_string;
		t = get_name(path);
		if (*path != *s || s[1] != ':' || strcmp(s+2, t))
		{
			*s++ = *path;			/* Laufwerk */
			*s++ = ':';
			strcpy(s, t);
			subobj_draw(tree_file_ops, CPMVD_DT, -1, NULL);
		}
		s = tree_file_ops[CPMVD_AK].ob_spec.free_string;
		if (strcmp(s, aktion))
			subobj_draw(tree_file_ops, CPMVD_AK, -1, aktion);
		return;
	}

	if (ord >= 0)
	{
		if (ord)
		{
			obj = CPMVDL_O;
			cnt = --n_ord;
		}
		else
		{
			obj = CPMVDL_D;
			cnt = --n_dat;
		}

		subobj_draw(tree_file_ops, obj, cnt, NULL);
	}
}


/* #include <stdio.h> */
static int info_obj(int wnr, int obj)
{
	char path[128];
	MYDTA *f;
	DTA	dta;
	long clusters;
	long dummy;
	int folders,nfiles,hfiles,ret;


	if (0 > (ret = obj_to_path(wnr, obj, path, &f)))
		return 0;

	if (ret == 0)		/* Datei */
	{
		n_dat++;
		if (n_siz)
		{
			if (f)			/* Datei im Fenster */
			{
				clusters = f->filesize / n_siz;
				if (f->filesize % n_siz)
					clusters++;
			}
			else
			{			/* Datei auf dem Desktop */
				Mgraf_mouse(HOURGLASS);
				Fsetdta(&dta);
				err = Fsfirst(path, 0x37);
				Mgraf_mouse(ARROW);

				if (err != E_OK)
				{
					err_alert(err);
					return (int) err;
				}

				clusters = dta.d_length / n_siz;
				if (dta.d_length % n_siz)
					clusters++;
			}
		}
	}
	else
	{
		if (path[3])		/* Ordner */
			n_ord++;
		err = walk_path(path, &clusters, &dummy, &dummy,&folders,
					 &nfiles, &hfiles, 0x37, n_siz, 0L, 0L);
		n_dat += nfiles+hfiles;
		n_ord += folders;
	}
/*	printf("%ld ",clusters); */
	/* Verschieben auf demselben Laufwerk benîtigt keinen Speicher */
	if ((!_move_flag) || (path[0] != _dest_path[0]))
		n_clu += clusters;
	return (int) err;
}


/*********************************************************************
*
* Fragt nach, ob Dateien zu lîschen/kopieren/verschieben sind.
*
*  function == 0 : lîschen
*  function == 1 : kopieren
*  function == 2 : verschieben
*
*  ja == 0 : nicht bestÑtigen, immer E_OK zurÅckgeben
*
*  RÅckgabe == E_OK : Funktion ausfÅhren
*
* nach Aufruf ist immer anschlieûend end_confirm() aufzurufen, um
* die Dialogbox wieder abzubauen!
*
*********************************************************************/

long confirm(int wnr, int function, int yes, char *dest_path)
{
	int exitbutton,dst_drive,dummy;
	long st;
	char *titel,*titelobj;


	if (!yes)
	{
		is_cpmvdl = FALSE;
		return E_OK;
	}

	dst_drive = dest_path[0] - 'A';

	/* Nur Speicherplatz prÅfen, wenn gewÅnscht */

	if (!settings.check_free)
		dst_drive = -1;

	/* Bytes pro Cluster merken */
	if (dst_drive >= 0)
	{
		if (!dinfo[dst_drive].b_clsiz || Mediach(dst_drive))
		{
			Mgraf_mouse(HOURGLASS);
			st = Dfree(&dinfo[dst_drive], dst_drive+1);
			Mgraf_mouse(ARROW);
			if (st)
				return st;
		}
		n_siz = dinfo[dst_drive].b_secsiz * dinfo[dst_drive].b_clsiz;
	}
	else
		n_siz = 0L;

	is_cpmvdl = TRUE;
	_dest_path = dest_path;
	_move_flag = FALSE;
	switch (function)
	{
		case 0:
#if LANGUAGE == LANG_GER
			titel = "Lîsche";
#elif LANGUAGE == LANG_ENG
			titel = "Delete";
#endif
			objs_hide(tree_file_ops, CPMVD_MD, 0);
			break;

		case 1:
#if LANGUAGE == LANG_GER
			titel = "Kopiere";
#elif LANGUAGE == LANG_ENG
			titel = "Copy";
#endif
			goto unhide;

		case 2:
#if LANGUAGE == LANG_GER
			titel = "Verschiebe";
#elif LANGUAGE == LANG_ENG
			titel = "Move";
#endif
			_move_flag = TRUE;
			unhide:
			objs_unhide(tree_file_ops, CPMVD_MD, 0);
			ob_sel_dsel(tree_file_ops, CPMVD_KB, settings.mode_ovwr == CONFIRM);
			ob_sel_dsel(tree_file_ops, CPMVD_KA, settings.mode_ovwr == BACKUP);
			ob_sel_dsel(tree_file_ops, CPMVD_KU, settings.mode_ovwr == OVERWRITE);
			break;
	}

	tree_file_ops[CPMVD_AK].ob_spec.free_string[0] = EOS;
	tree_file_ops[CPMVD_DT].ob_spec.free_string[0] = EOS;
	tree_file_ops[CPMVDL_B].ob_spec.free_string[0] = EOS;

	n_dat = n_ord = 0;
	n_clu = 0L;
	Mgraf_mouse(HOURGLASS);
	err = E_OK;
	walk_wind_sel_objs(wnr, info_obj);			/* Dateien zusammensuchen */
	Mgraf_mouse(ARROW);

	if (err)
	{
		is_cpmvdl = FALSE;
		return err;
	}

	if (n_dat == 0 && n_ord == 0)
	{
		is_cpmvdl = FALSE;
		return 1;
	}

	ultoa(n_dat, tree_file_ops[CPMVDL_D].ob_spec.free_string, 10);
	ultoa(n_ord, tree_file_ops[CPMVDL_O].ob_spec.free_string, 10);

	if (n_siz)
		ultoa((n_clu*n_siz)/1024L, tree_file_ops[CPMVDL_B].ob_spec.free_string, 10);

	titelobj = tree_file_ops[CPMVDL_T].ob_spec.free_string;
	strcpy(titelobj, titel);
#if LANGUAGE == LANG_GER
	strcat(titelobj, " Dateien");
#elif LANGUAGE == LANG_ENG
	strcat(titelobj, " Files");
#endif
	form_center(tree_file_ops, &cg);
	cnf_flyinf = NULL;
	form_xdial(FMD_START, NULL, &cg, (settings.use_fldl) ? &cnf_flyinf : NULL);
	objc_draw(tree_file_ops, ROOT, MAX_DEPTH, &cg);
	exitbutton = 0x7f & form_xdo(tree_file_ops, 0, &dummy, NULL, cnf_flyinf);
	ob_dsel(tree_file_ops, exitbutton);

	if (exitbutton != CPMVD_OK)
		return 1;				/* Abbruch */

	if (function == 1 || function == 2)
	{
		if (selected(tree_file_ops, CPMVD_KB))
			settings.mode_ovwr = CONFIRM;
		else
		if (selected(tree_file_ops, CPMVD_KA))
			settings.mode_ovwr = BACKUP;
		else
			settings.mode_ovwr = OVERWRITE;
	}

	if (function > 0 && dst_drive >= 0 &&
		 n_clu > dinfo[dst_drive].b_free && settings.mode_ovwr != BACKUP)
	{
#if LANGUAGE == LANG_GER
		if (2 == form_alert(2, "[2][Zuwenig Platz auf dem|Ziellaufwerk!]"
						"[OK|Abbruch]"))
#elif LANGUAGE == LANG_ENG
		if (2 == form_alert(2, "[2][Insufficient free space at|destination disk!]"
						"[OK|Cancel]"))
#endif
			err = EBREAK;
	}

	return err;
}


/*********************************************************************
*
* Lîscht eine Datei oder einen Ordner
* "after"- Funktion bei "Datei lîschen" fÅr walk_path
*
*********************************************************************/

static long after_delete(char *path, DTA *file)
{
	long doserr;


	if ((file->d_attrib) & F_SUBDIR)
	{
		doserr = GDdelete(path, file->d_fname);
		down_cnt(1, NULL, NULL);
	}
	else
	{
		doserr = GFdelete(path, file->d_fname);
		down_cnt(0, NULL, NULL);
	}
	return doserr;
}


/*********************************************************************
*
* Lîscht alle Dateien auf <path>.
* gibt Anzahl gelîschter Dateien zurÅck, < 0 bei Fehler
*
*********************************************************************/

int delete_files(char *path, int ispath)
{
	long dummy, doserr;
	int idummy;
	char *endp = path + strlen(path);

	if (ispath)
	{
		doserr = walk_path(path, &dummy, &dummy, &dummy,&idummy,
					 &idummy, &idummy, 0x37, 0L, 0L, after_delete);
		if (doserr || !path[3])
			return (int) doserr;

		endp[-1] = EOS;
		doserr = GDdelete(path, NULL);
		down_cnt(1, NULL, NULL);
	}
	else
	{
		doserr = GFdelete(path, NULL);
		down_cnt(0, NULL, NULL);
	}

	return (int) doserr;
}


/*********************************************************************
*
* Kopiert/Verschiebt eine Datei oder einen Ordner.
* (Bei Rekursion VOR Eintritt in einen Ordner aufrufen!)
*
*********************************************************************/

static long before_cpmv(char *path, DTA *file)
{
	long doserr;
	char all[128];
	char old_mode_ovwr;


	if (qbreak())
		return EBREAK;

	if ((file->d_attrib) & F_SUBDIR)
	{
		doserr = GDcreate(_dest_path, file->d_fname);
		strcat(_dest_path, file->d_fname);
		strcat(_dest_path, "\\");
		down_cnt(1, NULL, NULL);
		return doserr;
	}

	strcpy(all, path);
	strcat(all, file->d_fname);

	/* Wenn eine Datei in sich selbst kopiert oder verschoben werden */
	/* soll, wird die Sicherheitsabfrage automatisch eingeschaltet	*/
	/* ------------------------------------------------------------- */

	old_mode_ovwr = settings.mode_ovwr;
	if (!strcmp(path, _dest_path))
		settings.mode_ovwr = CONFIRM;

	if (_move_flag && _dest_path[0] == path[0])
		doserr = GFrename(path, file, _dest_path);
	else
		doserr = GFcopy(path, file, _dest_path);
	settings.mode_ovwr = old_mode_ovwr;
	if (doserr)
		return doserr;

	if (_move_flag && _dest_path[0] != path[0])
	{
		doserr = Fdelete(all);
		set_dirty(doserr, all, 1);
	}

	down_cnt(0, NULL, NULL);
	if (doserr != E_OK)
		err_file = all;

	return err_alert(doserr);
}


/*********************************************************************
*
* Geht im Zielpfad um eine Position in Richtung Root.
* Bei "Verschieben" wird der Ordner gelîscht.
* (Bei Rekursion NACH Eintritt und wieder Austritt aus einem Ordner
* aufrufen!)
*
*********************************************************************/

static long after_cpmv(char *path, DTA *file)
{
	long doserr;
	register char *endp = path + strlen(path);


	if (qbreak())
		return EBREAK;

	if (!(file->d_attrib & F_SUBDIR))
		return E_OK;

	if (_move_flag)
		doserr = GDdelete(path, file->d_fname);
	else
		doserr = E_OK;

	endp = _dest_path + strlen(_dest_path);
	*(--endp) = EOS;
	endp = strrchr(_dest_path, '\\');
	*(++endp) = EOS;		/* Ordner entfernen */

	return doserr;
}


/*********************************************************************
*
* Kopiert/Verschiebt alle Dateien von <src_path> nach <dst_path>.
* gibt Anzahl kopierter Dateien+Ordner zurÅck.
* oder negativ bei Fehler.
*
*********************************************************************/

int copy_move_files(char *src_path, char *dst_path, int move_flag, int ispath)
{
	long dummy,doserr;
	int idummy;
	char *endp = src_path + strlen(src_path);
	char *lastb = strrchr(src_path, '\\');
	char *zendp = dst_path + strlen(dst_path);


	_move_flag = move_flag;
	_dest_path = dst_path;

	/* Wenn eines der beteiligten Speichermedien eine RAMDisk ist,	*/
	/* werden nur 20k Kopierpuffer verwendet, sonst alles			*/
	/* ------------------------------------------------------------- */

	_bsize = -1L;			/* Pufferspeicher ganz */
	if (('RD' == disk_type(src_path[0] - 'A')) ||
		 ('RD' == disk_type(dst_path[0] - 'A')))
			_bsize = 20480L;

	/* 1. Fall: Pfad, also Ordner oder Root */
	/* ------------------------------------ */

	if (ispath)
	{
		if (dst_path == strstr(dst_path, src_path))
		{
#if LANGUAGE == LANG_GER
			form_alert(1, "[3][UngÅltige Kopieranweisung!][Abbruch]");
#elif LANGUAGE == LANG_ENG
			form_alert(1, "[3][Invalid copy operation!][Cancel]");
#endif
			return 0;
		}

		/* 1.1 Fall: Ordner, also Ordner auf Zielpfad erstellen */
		/* ---------------------------------------------------- */

		if (src_path[3])
		{
			char name[14];

			endp[-1] = EOS;
			strcpy(name, strrchr(src_path, '\\') + 1);
			endp[-1] = '\\';
			doserr = GDcreate(dst_path, name);
			if (doserr)
				return (int) doserr;

			down_cnt(1, NULL, NULL);
			strcat(dst_path, name);
			strcat(dst_path, "\\");
		}

		doserr = walk_path(src_path, &dummy, &dummy, &dummy, &idummy,
					 &idummy, &idummy, 0x37, 0L, before_cpmv, after_cpmv);
		if (doserr || !src_path[3])
		{
			*zendp = EOS;
			return (int) doserr;
		}

		endp[-1] = EOS;
		if (move_flag)
			doserr = GDdelete(src_path, NULL);
	}

	/* 2. Fall: Nur eine Datei */
	/* ----------------------- */

	else
	{
		DTA	dta;

		Fsetdta(&dta);
		doserr = Fsfirst(src_path, 0x37);
		if (doserr != E_OK)
			return (int) err_alert(doserr);
		*(++lastb) = EOS;
		doserr = before_cpmv(src_path, &dta);
	}

	*zendp = EOS;
	return (int) doserr;
}
