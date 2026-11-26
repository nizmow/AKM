/*******************************************************************
*
*             VFATCONF.PRG                             6.1.96
*             ============
*                                 letzte Änderung:	29.6.96
*
* geschrieben mit Pure C V1.1
* Projektdatei: VFATCONF.PRJ
*
* (De)aktiviert lange Dateinamen auf VFAT-Laufwerken
*
****************************************************************/

#include <gem.h>
#include <mint/dcntl.h>
#include <mint/errno.h>
#include <mint/mintbind.h>
#include <mint/osbind.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "VFATCONF.H"
#include "VFATCONF_GEM.H"

static long conf_temp;
static long conf_temp_valid;
static long old_conf_temp;

/* hier sind Dinge drin, die nicht in magx.h enthalten sind,
   weil sie nicht für die Öffentlichkeit sind */

typedef struct {
	int32_t magic;
	void *membot;
	void *aes_start;
	int32_t magic2;
	uint32_t date;
	void (*chgres)(int res, int txt);
	int32_t (**shel_vector)(void);
	char *aes_bootdrv;
	int16_t *vdi_device;
	void *reservd1;
	void *reservd2;
	void *reservd3;
	int16_t version;
	int16_t release;
	void *_basepage;
	int16_t **moff_cnt;
	uint16_t **shel_buf_len;
	void **shel_buf;
	void *notready_list;
	void **menu_app;
	void **menutree;
	void **desktree;
	int16_t **desktree_1stob;
	void **dos_magic;
	int16_t *nwindows;
	void **p_fsel;
	void *ctrl_timeslice;
	void **topwind_app;
	void **mouse_app;
	void **keyb_app;
	void *suspend_list;
	int16_t install_drv;
	char kernelpath[16];
} AESVARS2;



static long old_conf_perm;
static long conf_perm;
static int is_temp = TRUE;


void ob_disable_enable(OBJECT *tree, int which, int dis)
{
	if	(dis)
		(tree+which)->ob_state |=  DISABLED;
	else (tree+which)->ob_state &= ~DISABLED;
}


/************************************************************
*
* Speichert die Daten in die MAGX.INF. Deren Laufwerk liegt
* in den AES-Variablen.
*
************************************************************/

void	save_conf_to_inf( void )
{
	char infpath[] = "A:\\MAGX.INF";
	char infpath2[] = "A:\\MAGX.&&&";
	char *buf = NULL;
	AESVARS2 *av;
	long errc;
	long inf_size;
	int handle = -1;
	char *s;
	char *sec_vfat,*next_sec,*drives_asng;
	char c_rett;
	char drives_line[] = "drives=abcdefghijklmnopqrstuvwxyz123456\r\n";
	char *end_sec1;
	char *sec2;
	char *insert = NULL;
	char *insert2 = NULL;



	/* "drives="-Zeile zusammenstellen */
	/* ------------------------------- */

	s = drives_line+7;
	errc = conf_perm;
	for	(c_rett = 'a'; errc; c_rett++,errc >>= 1L)
		{
		if	(errc & 1L)
			*s++ = c_rett;
		}
	*s++ = '\r';
	*s++ = '\n';
	*s = '\0';

	/* INF-Datei öffnen */
	/* ---------------- */

	av = *((AESVARS2 **)(aes_global + 11));
	infpath[0] += *(av->aes_bootdrv);
	infpath2[0] += *(av->aes_bootdrv);
	errc = Fopen(infpath, RMODE_RW);
	if	((errc == EFILNF) || (errc == EPTHNF) || (errc == EDRIVE))
		{
		Rform_alert(1, ALRT_NO_INF);
		return;
		}

	if	(errc < E_OK)
		goto err;

	handle = (int) errc;

	/* INF-Datei einlesen */
	/* ------------------ */

	errc = Fseek(handle, 0L, 2);
	if	(errc < E_OK)
		goto err;
	inf_size = errc;
	errc = Fseek(handle, 0L, 0);
	if	(errc < E_OK)
		goto err;
	buf = (char *)Malloc(inf_size+1);		/* Platz für EOS! */
	if	(!buf)
		{
		Rform_alert(1, ALRT_MEMERR);
		goto err2;
		}
	buf[inf_size] = '\0';

	errc = Fread(handle, inf_size, buf);
	if	(errc < E_OK)
		goto err;
	Fclose(handle);

	/* Zieldatei öffnen */
	/* ---------------- */

	errc = Fcreate(infpath2, 0);
	if	(errc < E_OK)
		goto err;
	handle = (int) errc;

	sec_vfat = strstr(buf, "\r\n#[vfat]\r\n");
	if	(sec_vfat)
		sec_vfat += 2;

	/* 1. Fall: Section [vfat] existiert */
	/* --------------------------------- */

	if	(sec_vfat)
		{
		/* Suche das Ende der Section */
		/* Falls es keine nächste gibt, Dateiende */

		next_sec = strstr(sec_vfat, "\r\n#[");
		if	(next_sec)
			next_sec += 2;
		else	next_sec = sec_vfat+strlen(sec_vfat);

		/* Suche existierenden "drives=" */

		c_rett = *next_sec;
		*next_sec = '\0';
		drives_asng = strstr(sec_vfat, "\r\ndrives=");
		*next_sec = c_rett;
		if	(drives_asng)
			{
			end_sec1 = drives_asng + 2;
			sec2 = strstr(end_sec1, "\r\n");
			if	(sec2)
				sec2 += 2;
			else	sec2 = next_sec;
			}
		else	{
			end_sec1 = strstr(sec_vfat, "\r\n");
			if	(end_sec1)
				end_sec1 += 2;
			else	end_sec1 = next_sec;
			sec2 = end_sec1;
			}
		}
	else

	/* 2. Fall: Es gibt eine andere Section */
	/* ------------------------------------ */

	if	(NULL != (next_sec = strstr(buf, "\r\n#[")))
		{
		end_sec1 = next_sec+2;
		sec2 = end_sec1;
		insert = "#[vfat]\r\n";
		}

	/* 3. Fall: Es gibt überhaupt keine Section */
	/* ---------------------------------------- */

	else	{
		next_sec = strstr(buf, "#_MAG MAG!X V");
		insert = "#[vfat]\r\n";
		insert2 = "#[aes]\r\n";
		if	(next_sec)
			{
			end_sec1 = strstr(next_sec,"\r\n");
			if	(end_sec1)
				end_sec1 += 2;
			else	end_sec1 = next_sec+strlen(next_sec);
			}
		else	{
			end_sec1 = buf;	/* einfach nach vorn */
			}
		sec2 = end_sec1;
		}

	/* Dateianfang */
	errc = Fwrite(handle, end_sec1-buf, buf);
	if	(errc != end_sec1-buf)
		goto err;

	/* Einfügung */
	if	(insert)
		{
		errc = Fwrite(handle, strlen(insert), insert);
		if	(errc != (long)strlen(insert))
			goto err;
		}
	/* neue Zeile */
	errc = Fwrite(handle, strlen(drives_line), drives_line);
	if	(errc != (long)strlen(drives_line))
		goto err;

	if	(insert2)
		{
		errc = Fwrite(handle, strlen(insert2), insert2);
		if	(errc != (long)strlen(insert2))
			goto err;
		}

	/* Dateiende */
	errc = Fwrite(handle, strlen(sec2), sec2);
	if	(errc != (long)strlen(sec2))
		goto err;

	errc = Fclose(handle);
	handle = -1;
	if	(errc)
		goto err;

	errc = Fdelete(infpath);
	if	(errc)
		goto err;
	errc = Frename(0, infpath2, infpath);

	if	(errc < E_OK)
		{
		err:
		Rform_alert(1, ALRT_WRTERR);
		}
	err2:
	if	(handle >= 0)
		Fclose(handle);
	if	(buf)
		Mfree(buf);
}


/************************************************************
*
* Ermittelt conf_temp und conf_perm.
*
************************************************************/

static void init_conf( void )
{
	register int i;
	long ret;
	long bit;
	char path[] = "U:\\";
	int16_t stat[2] = {0,0};


	ret = Dcntl(VFAT_CNFDFLN, path, -1L);
	if	(ret >= E_OK)
		conf_perm = old_conf_perm = ret;
	for	(i = 0, bit = 1L; i <= 'Z'-'A'; i++,bit <<= 1)
		{
		stat[1] = i;
		ret = Dcntl(KER_DRVSTAT, "U:\\", (long) stat);
		if	(ret > 0L)		/* gemountet */
			{
			path[0] = i+'A';
			ret = Dcntl(VFAT_CNFLN, path, -1L);
			if	(ret >= E_OK)	/* VFAT */
				{
				conf_temp_valid |= bit;
				if	(ret)
					conf_temp |= bit;
				}
			}
		}
	old_conf_temp = conf_temp;
}


/************************************************************
*
* Der Dialog.
*
************************************************************/

static void set_conf(OBJECT *tree, long conf, long valid)
{
	register int i;
	long bit;

	for	(i = 0,bit = 1L; i <= 'Z'-'A'; i++,bit <<= 1L)
		{
		ob_sel_dsel(tree, i+DISKA, (bit&conf) != 0);
		ob_disable_enable(tree, i+DISKA, (bit&valid) == 0);
		}
}


static long get_conf(OBJECT *tree)
{
	register int i;
	long conf;

	conf = 0L;
	for	(i = 0; i <= 'Z'-'A'; i++)
		conf |= ((long) selected(tree, i+DISKA) << i);
	return(conf);
}


static void exit_conf( OBJECT *tree )
{
	register int i;
	long ret,old;
	long bit;
	char path[] = "U:\\";
	int16_t msg[8];


	if	(is_temp)
		conf_temp = get_conf(tree);
	else	conf_perm = get_conf(tree);

	Dcntl(VFAT_CNFDFLN, path, conf_perm);
	for	(i = 0, bit = 1L; i <= 'Z'-'A'; i++,bit <<= 1)
		{
		path[0] = i+'A';
		if	(conf_temp_valid & bit)
			{
			ret = (conf_temp & bit) ? 1L : 0L;
			old = (old_conf_temp & bit) ? 1L : 0L;
			if	(ret != old)
				{
				Dcntl(VFAT_CNFLN, path, ret);
				msg[0] = SH_WDRAW;
				msg[2] = 0;
				msg[3] = i;
				msg[4] = msg[5] = msg[6] = msg[7] = 0;
				appl_write(0, 16, msg);
				}
			}
		}
}


static int chk_conf(OBJECT *tree, int exitbutton)
{
	if	(exitbutton == CANCEL)
		return(TRUE);
	if	(exitbutton == OK)
		return(TRUE);
	if	((exitbutton == PERMA) && (is_temp))
		{
		conf_temp = get_conf(tree);
		set_conf(tree, conf_perm, -1L);
		subobj_draw(tree, LAUFWERKE, LAUFWERKE, 1);
		is_temp = FALSE;
		}
	if	((exitbutton == TEMPOR) && (!is_temp))
		{
		conf_perm = get_conf(tree);
		set_conf(tree, conf_temp, conf_temp_valid);
		subobj_draw(tree, LAUFWERKE, LAUFWERKE, 1);
		is_temp = TRUE;
		}
	return(FALSE);
}


/**************** HAUPTPROGRAMM ******************/

int main( void )
{
	OBJECT *tree;
	int button;
	char cmd[128],tail[128];


	if   ((appl_init()) < 0)
		Pterm(-1);

	/* Testen, ob als AUTO gestartet. Wenn ja, nur	*/
	/* Konfiguration setzen und beenden.			*/
	/* ---------------------------------------------- */

	shel_read(cmd, tail);
	if	(!memcmp(tail, "\0\0AUTO", 7))
		{
/*
	wir machen nix mehr, das macht jetzt der Kernel

		Dcntl(VFAT_CNFDFLN, "u:\\", saved_status.conf_saved);
*/
		}

	else	{

		Mrsrc_load("vfatconf.rsc");
	
		Mgraf_mouse(ARROW);
		rsrc_gaddr(R_TREE, T_SELECT, &tree);
	
		wind_update(BEG_UPDATE);
		init_conf();
		set_conf(tree, conf_temp, conf_temp_valid);
		button = do_exdialog(tree, chk_conf, NULL);
		if	(button == OK)
			{
			exit_conf( tree );
			if	(conf_perm != old_conf_perm)
				save_conf_to_inf();
			}
		wind_update(END_UPDATE);
	
		rsrc_free();
		}

	appl_exit();
	return(0);
}
