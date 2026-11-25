/*****************************************************************
*
* Stellt eine Uhr dar.
* Erledigt außerdem für MagiC den Writeback-Cache.
*
*****************************************************************/

#include <stddef.h>
#include <gem.h>
#include <mint/osbind.h>

#define DATESEP 	'-'
#define TIMESEP	':'
#define RIGHTOFFSET 1

/* MagiC writeback configuration. */
#define SC_WBACK 4
#define SCWB_SET 1
#define Ssync() ((void)trap_1_w(0x150))

char	datetime[19] = "                 ";

void date_to_str(char *s, unsigned int date);
void time_to_str(char *s, unsigned int time);

/* Modi:
	0	hh:mm
	1	hh:mm:ss
	2	tt.mm  hh:mm:ss
	3	tt.mm.jj  hh:mm:ss
*/

int main(void)
{
	short work_out[57],work_in[11];	 /* VDI-Felder für v_opnvwk() */
	int sec,tp,ln;
	unsigned short i;
	int x,y;
	int mode;
	int len    [] = { 5, 8, 15, 18};
	int timepos[] = { 0, 0,  7, 10};
	int secpos [] = {-1, 7, 14, 17};
	short vdi_handle;
	short dummy, scrw;
	short hwchar, hhbox, hhchar;
	short event, message[8];


	if (appl_init() < 0)
		return 1;
	if (!shel_write(SHW_INFRECGN, 1, 0, NULL, NULL))
		{
		appl_exit();
		return 1;
		}

	/* SC_WBACK takes a word subcommand, unlike generic Sconfig's long argument. */
	if (trap_1_www(0x33, SC_WBACK, SCWB_SET) < 0L)
		{
		appl_exit();
		return 1;
		}

	vdi_handle = graf_handle(&hwchar, &hhchar, &dummy, &hhbox);

	for  (i = 0; i < 10; work_in[i++] = 1)
		;
	work_in[10]=2;                     /* Rasterkoordinaten */
	v_opnvwk(work_in, &vdi_handle, work_out);
	/* Ausrichtung Zeichenzellenoberkante */
	vst_alignment(vdi_handle, 0, 5, &dummy, &dummy);
	y = (hhbox-hhchar) >> 1;
	wind_get(0, WF_WORKXYWH, &dummy, &dummy, &scrw, &dummy);
	i = 0;
	mode = 3;
	goto begin;

	while(1)
		{
		i++;
		if	(i == 20)
			{
			mode = 1;
			begin:
			sec = secpos[mode];
			tp = timepos[mode];
			ln = len[mode];
			x  = scrw - (ln+RIGHTOFFSET) * hwchar;
			datetime[sec] = 1;

			}

		if	(i & 1)
			{
			if	(sec >= 0 && datetime[sec] & 1)
				{
				if	(mode > 1)
					date_to_str(datetime, Tgetdate());
				if	(tp > 0)
					datetime[tp - 1] = datetime[tp - 2] = ' ';
				time_to_str(datetime+tp, Tgettime());
				datetime[ln] = '\0';
				}
			else	datetime[sec] += 1;
			v_gtext(vdi_handle, x, y, datetime);
			}

		/* MagiC timed message wait: 25 ticks at 50 Hz. */
		event = appl_read(-2, 25, message);
		if ((event & MU_MESAG) && message[0] == AP_TERM)
			{
			v_clsvwk(vdi_handle);
			appl_exit();
			return 0;
			}

		Ssync();

		}
}


/*********************************************************************
*
* Wandelt DOS- Datum in eine Zeichenkette um.
*
*********************************************************************/

void date_to_str(char *s, unsigned int date)
{
	int t,m;

	t = date & 31;
	date >>= 5;
	m = date & 15;
	date >>= 4;
	date += 80;
	date %=100;
	*s++ = t/10 + '0';
	*s++ = t%10 + '0';
	*s++ = DATESEP;
	*s++ = m/10 + '0';
	*s++ = m%10 + '0';
	*s++ = DATESEP;
	*s++ = date/10 + '0';
	*s++ = date%10 + '0';
/*	*s = '\0'; */
}


/*********************************************************************
*
* Wandelt DOS- Zeit in eine Zeichenkette um.
*
*********************************************************************/

void time_to_str(char *s, unsigned int time)
{
	int min,sec;

	sec = 2 * (time & 31);
	time >>= 5;
	min = time & 63;
	time >>= 6;
	*s++ = time/10 + '0';
	*s++ = time%10 + '0';
	*s++ = TIMESEP;
	*s++ = min/10 + '0';
	*s++ = min%10 + '0';
	*s++ = TIMESEP;
	*s++ = sec/10 + '0';
	*s++ = sec%10 + '0';
	*s = '\0';
}
