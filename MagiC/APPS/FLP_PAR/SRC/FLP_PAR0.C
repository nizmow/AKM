/*********************************************************************
*
*  FLP_PAR0              Andreas Kromke, 27.8.94
*  ========
*
* schaltet die parallele Floppyübertragung ab.
*
*********************************************************************/

#include <mint/sysbind.h>
#define SCB_FLPAR 0x800L

int main( void )
{
	long mode;

	mode = Sconfig(0, 0L);
	Sconfig(1, mode & ~SCB_FLPAR);		/* Parallelbetrieb aus */
	return(0);
}
