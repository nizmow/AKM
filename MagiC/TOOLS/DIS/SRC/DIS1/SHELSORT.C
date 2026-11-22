/*******************************************************************
*
*                          SHELSORT.O		8.1.89
*                          ==========
*
*         letzte Modifikation:
*
* MODUL für Shellsort- Algorithmus.
* Syntax wie bei QSORT.
* Der Algorithmus ist NICHT rekursiv, funktioniert daher
*  auch bei kleinem Stack.
* Der Algorithmus ist bestmöglich optimiert, indem:
*
* 1) AUSSCHLIEßLICH register- Variablen benutzt werden
* 2) nicht mit Indizes, sondern mit Zeigern gerechnet wird,
*	damit alle Multiplikationen außerhalb der inneren Schleifen
*	ausgeführt werden
* 3) Für long oder void * wird eine besonders schnelle Vertauschung
*    von Elementen durchgeführt
*
* Verbesserungsmöglichkeit: assembleroptimierte memxchg- Funktion
*
*******************************************************************/

#include <string.h>

static void longxchg(char *s1, char *s2, size_t count)
{
	long tmp;

	(void) count;

	tmp = *( (long *) s1);
	*( (long *) s1) = *( (long *) s2);
	*( (long *) s2) = tmp;
}


static void memxchg(char *s1, char *s2, size_t count)
{
	register char c;


	while(count)
		{
		c = *s1;
		*s1++ = *s2;
		*s2++ = c;
		count--;
		}
}


void shelsort(char *base, size_t count, size_t size,
		    int (*compar)(void *s1, void *s2))
{
	register int (*vgl)(void *s1, void *s2) = compar;
	register char *j;
	register long k2,k,i;
	register void (*xchg)(char *s1, char *s2, size_t count) = memxchg;



	k2 = count / 2;				/* k2 = Elementzähler */
	count *= size;					/* count: max-Byte */
	if	(0 == (((long) base) & 1) && size == 4L)
		xchg = longxchg;
	while(k2 > 0)
		{
		k = k2 * size;				/* k = Bytezähler */
		for	(i = k; i < (long) count; i += size)
			{
			j = base + i - k;
			while((j >= base) && ((*vgl)(j,j+k)) > 0)
				{
				(*xchg)(j, j + k, size);
				j -= k;
				}
			}
		k2 >>= 1;
		}
}
