/*********************************************************************
*
* Permanently print MagiC's free internal memory to the console
* until user presses ^C
*
*********************************************************************/

#include <stdio.h>
#include <mgx_dos.h>


int main()
{
	long ret;
	long n[2];
	long n2[2] = {0,0};

	printf("MagiC Internal Memory Monitor\n");
	printf("-----------------------------\n");
	for	(;;)
	{
		/* do garbage collection until no futher memory block was freed */
		while((ret = Sconfig(SC_INTGARBC, 0)) == 1)
			;

		if (ret != 0)
		{
			/* Maybe not running in MagiC */
			printf("Error %d from Sconfig()\n", ret);
			return ret;
		}

		Sconfig(SC_INTMAVAIL, (long) n);

		if	(n[0] != n2[0] || n[1] != n2[1])
		{
			printf("%3ld / %3ld blocks used\n", n[1], n[0]);
			n2[0] = n[0];
			n2[1] = n[1];
		}

		/* allow ^C to end the program */
		if (Cconis())
		{
			Cconin();
		}
	}
}
