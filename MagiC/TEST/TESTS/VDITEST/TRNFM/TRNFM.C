/*
*
* Beispielprogramm fÅr VDI.
*
*/

#include <mgx_dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <mt_aes.h>
#include <vdi.h>

typedef unsigned long uint32_t;

int vdi_handle;
int screen_nplanes;
int work_out[57], work_in[12];	 /* VDI-Felder fÅr v_opnvwk() */
int extnd_out[128];

int gl_hhbox, gl_hwbox, gl_hhchar, gl_hwchar;
int ap_id;
int aes_handle;		/* Screen-Workstation des AES */

#define MFDB_W 10		/* convert a 10x7 rectangle */
#define MFDB_H 7
#define MFDB_WW (((MFDB_W + 15) / 16) * 16)		/* width must be divideable by 16 */

static uint32_t srcMem[MFDB_WW * MFDB_H];
static uint32_t dstMem[MFDB_WW * MFDB_H];
static MFDB srcMFDB;
static MFDB dstMFDB;


static void clearMFDB(MFDB *mfdb)
{
	uint32_t *p;

	p = (uint32_t *) mfdb->fd_addr;
	if (p != NULL)
	{
		int xy;

		for (xy = 0; xy < MFDB_H * MFDB_WW; xy++)
		{
			*p++ = 0;
		}
	}
}

static void printMFDB(const char *name, const MFDB *mfdb)
{
	const uint32_t *p;

	printf("%sMFDB addr = 0x%08lx\n", name, mfdb->fd_addr);
	printf("     fd_w/h/nplanes = %d / %d / %d\n", mfdb->fd_w, mfdb->fd_h, mfdb->fd_nplanes);
	printf("     fd_wdwidth = %d\n", mfdb->fd_wdwidth);
	printf("     fd_stand = %d\n", mfdb->fd_stand);
	p = (const uint32_t *) mfdb->fd_addr;
	if (p != NULL)
	{
		int x, y;

		for (y = 0; y < MFDB_H; y++)
		{
			printf("     0x");
			for (x = 0; x < MFDB_WW; x++)
			{
				printf("%08lx ", *p++);
			}
			printf("\n");
		}
		printf("\n");
	}
}


/* offscreen, inplace */

static void test1(int src_stand, int nplanes, int inplace)
{
	printf("--- run test ---\n");
	srcMFDB.fd_addr = srcMem;
	srcMFDB.fd_w = MFDB_W;			/* IGNORED BY MVDI */
	srcMFDB.fd_h = MFDB_H;
	srcMFDB.fd_nplanes = nplanes;
	srcMFDB.fd_wdwidth = (MFDB_W + 15) / 16;
	srcMFDB.fd_stand = src_stand;
	if (inplace)
	{
		dstMFDB.fd_addr = srcMem;
	}
	else
	{
		dstMFDB.fd_addr = dstMem;
	}
	dstMFDB.fd_stand = -1;		/* to be set by callee */

#if 0
	/* is this necessary? */
	dstMFDB.fd_w = MFDB_W;
	dstMFDB.fd_h = MFDB_H;
	dstMFDB.fd_nplanes = nplanes;
	dstMFDB.fd_wdwidth = srcMFDB.fd_wdwidth;
#endif

	printMFDB("src", &srcMFDB);
	printMFDB("dstBefore", &dstMFDB);
	vr_trnfm(vdi_handle, &srcMFDB, &dstMFDB);
	printMFDB("dstAfter", &dstMFDB);
	clearMFDB(&dstMFDB);
}

/* screen to offscreen */

static void test2(int nplanes)
{
	srcMFDB.fd_addr = NULL;
	srcMFDB.fd_w = MFDB_W;
	srcMFDB.fd_h = MFDB_H;
	srcMFDB.fd_nplanes = nplanes;
	srcMFDB.fd_wdwidth = MFDB_W * 2;
	srcMFDB.fd_stand = 0;
	dstMFDB.fd_addr = dstMem;
	vr_trnfm(vdi_handle, &srcMFDB, &dstMFDB);
}

/* offscreen to screen */

static void test3(int nplanes)
{
	srcMFDB.fd_addr = srcMem;
	srcMFDB.fd_w = MFDB_W;
	srcMFDB.fd_h = MFDB_H;
	srcMFDB.fd_nplanes = nplanes;
	srcMFDB.fd_wdwidth = MFDB_W * 2;
	srcMFDB.fd_stand = 1;
	dstMFDB.fd_addr = NULL;
	vr_trnfm(vdi_handle, &srcMFDB, &dstMFDB);
}


#if 0
void ExecLine(char *line)
{
	char c;
	int x,y,w,h;
	int wrmode,color;
	int interior,style;
	int n;

	if	(line[0] == ';')
		return;

	n = sscanf(line, "%c %d %d %d %d %d %d %d %d",
					&c,
					&x, &y, &w, &h,
					&wrmode,
					&color,
					&interior, &style);	

	if	(n != 9)
	{
		Cconws("Fehler in Zeile \"");
		Cconws(line);
		Cconws("\"\r\n");
		Cconws("form x y w h wrmode colour interior style\r\n");
		Cconws("form = e[llipse] r[ectangle]\r\n");
		return;
	}


	pxy[0] = x;		/* x1 */
	pxy[1] = y;		/* y1 */
	pxy[2] = x+w-1;	/* x2 */
	pxy[3] = y+h-1;	/* y2 */

	vswr_mode(vdi_handle, wrmode);
	vsf_color(vdi_handle, color);
	vsf_interior(vdi_handle, interior);
	vsf_style(vdi_handle, style);
	vsf_udpat(vdi_handle, planes_32, 32);

	switch(c)
	{
		case 'e':
		case 'E':
			v_ellipse(vdi_handle, x, y, w, h);
			break;

		default:
			vr_recfl(vdi_handle, pxy);
			break;
	}
}
#endif


int main( void )
{
	register int i;
	char c;

	if ((ap_id = appl_init()) < 0)
	{
		Cconws("Fehler bei appl_init()");
		Pterm(-1);
	}

	aes_handle = graf_handle(&gl_hwchar, &gl_hhchar, &gl_hwbox, &gl_hhbox);

	vdi_handle = aes_handle;
	for (i = 0; i < 10; work_in[i++] = 1)
		;
	work_in[10]=2;                     /* Rasterkoordinaten */
	v_opnvwk(work_in, &vdi_handle, work_out);
	vq_extnd(vdi_handle, 1, extnd_out);
	screen_nplanes = extnd_out[4];

	/* Fill memory block with pseudo random values */
	/* Note that for less then 32 bits per pixel, the block still is large enough. */
	srand(4711);	/* make sure the result is always the same between runs */
	for (i = 0; i < MFDB_WW * MFDB_H; i++)
	{
		/* compose 32-bit number from two calls */
		uint32_t v = Random() << 16;
		v |= Random();
		srcMem[i] = v;
	}

	printf("Choose:\n");
	printf(" 1 - dev-specific->standard, monochrome, not in-place\n");
	printf(" 2 - dev-specific->standard, 7 planes, not in-place\n");
	printf(" 3 - dev-specific->standard, 4 planes, not in-place\n");
	printf(" 4 - dev-specific->standard, %d planes, not in-place\n", screen_nplanes);
	printf(" 5 - standard->dev-specific, %d planes, not in-place\n", screen_nplanes);
	c = Cconin();
	switch(c)
	{
		case '1':
			test1(0, 1, 0);
			break;

		case '2':
			test1(0, 7, 0);
			break;

		case '3':
			test1(0, 4, 0);
			break;

		case '4':
			test1(0, screen_nplanes, 0);
			break;

		case '5':
			test1(1, screen_nplanes, 0);
			break;
	}
	printf(" .. press key ");
	Cconin();

#if 1

#if 0
	/* success */
	test1(0, 1, 0);	/* dev-specific -> standard, monochrome, not in-place */
	/* ignored by true-colour driver */
	test1(0, 7, 0);	/* dev-specific -> standard, 16 planes, not in-place */
#endif

#if 0
	/* ? */
	test1(1, screen_nplanes, 0);	/* standard -> dev-specific, native planes, not in-place */
	/* crashes */
	test1(0, screen_nplanes, 0);	/* dev-specific -> standard, native planes, not in-place */
#endif
	return 0;

#else
	long err;
	int fh;

	err = Fopen("pvditst.ini", 0);
	if	(err < 0)
	{
		if	(err == EFILNF)
			err = 0;

		test1(100, 20, 100, 50);

		test2();

		test3();

		return((int) err);
	}

	fh = (int) err;

	i = 0;
	do
	{
		char line[256];
		char c;

		err = Fread(fh, 1, &c);
		if	((err == 0) || (c == '\r') || (c == '\n') || (i > 254))
		{
			line[i] = '\0';
			if	(i)
				ExecLine(line);
			i = 0;
		}
		else
		{
			line[i++] = c;
		}
	}
	while(err == 1);

	return((int) Fclose(fh));
#endif
}
