/*********************************************************************
*
*                  CMDLINE.PRG                 Andreas Kromke
*                  ===========                 19.03.90
*
*                              letzte Änderung 19.03.90
*
* Zeigt Kommandozeile an
*
*********************************************************************/

#include <mint/basepage.h>
#include <mint/osbind.h>
#include <gem.h>

#define EOS '\0'
#define TRUE  1
#define FALSE 0
#define tohex(c)	(((c) < '\12')  ? ((c) + '0') : ((c) + ('a'-'\12')))
#define write16x(i)	Cconout(tohex((i) >> 4)),Cconout(tohex((i) & 0xf))

void write32x	(unsigned long zahl);
int	is_atari_printable	(unsigned char c);

int main( void )
{
	unsigned char *ttail;
	unsigned char gcmd[128],gtail[128];
	unsigned char *env,*t;
	unsigned long envsize;
	void print(unsigned char *s, unsigned long size);

	ttail = (unsigned char *) _base->p_cmdlin;
	Cconws("DOS- KOMMANDOZEILE:\r\n");
	print(ttail, 128L);
	env = (unsigned char *) _base->p_env;
	t = env;
	do	{
		while(*t)
			t++;
		t++;
		}
	while(*t);
	envsize = t - env + 1;
	if	(0 <= appl_init())
		{
		shel_read((char *) gcmd, (char *) gtail);
		appl_exit();
		Cconws("GEM- KOMMANDOZEILE:\r\n");
		print(gtail, 128L);
		Cconws("GEM- PROGRAMMNAME:\r\n");
		print(gcmd, 128L);
		}
	Cconws("ENVIRONMENT:\r\n");
	print(env, envsize);
	return(0);
}


void print(unsigned char *s, unsigned long end)
{
	register int i;
	register unsigned long pos = 0;
	int amnt;


	while(pos < end)
		{
		amnt = 16;
		if	((unsigned long) amnt > end - pos)
			amnt = (int) (end - pos);
		if	(amnt == 0)
			break;
		write32x(pos);
		Cconout(' ');
		for	(i = 0; i < 16; i++)
			{
			if	(0 == (i & 1))
				Cconout(' ');
			if	(i < amnt)
				write16x(s[pos+i]);
			else Cconws("  ");
			}
		Cconws("  ");
		for	(i = 0; i < amnt; i++)
			Cconout((is_atari_printable(s[pos+i])) ? s[pos+i] : '.');
		pos += amnt;
		Cconws("\r\n");
		}
}


/*********************************************************************
*
*  Schreibt <zahl> als Hexzahl (6 Stellen) nach stdout
*
*********************************************************************/

void write32x(unsigned long zahl)
{
	register int i;

	for	(i = 20; i >= 0; i-=4)
		Cconout((char) tohex((zahl >> i) & 0xf));
}


/*********************************************************************
*
*  Gibt an, ob ein Zeichen ausgegeben wird
*
*********************************************************************/

int is_atari_printable(unsigned char c)
{
	if	(c  < 0x1f ||				/* Steuerzeichen  */
		 c == 0x7f ||				/* DELete		   */
		 c  > 0xc1 ||				/* hebr.+griech.  */
		 (c  > 0xa5 && c < 0xb0) ||	/* versch. */
		 (c  > 0xb8 && c < 0xc0)		/* versch. */
		)
		return(FALSE);
	else return(TRUE);
}
