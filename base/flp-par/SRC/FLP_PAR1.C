/*********************************************************************
*
*  FLP_PAR1              Andreas Kromke, 27.8.94
*  ========
*
* schaltet die parallele Floppyübertragung ein.
*
*********************************************************************/

#include <mint/sysbind.h>
#define SCB_FLPAR 0x800L

int main( void )
{
	long mode;

	mode = Sconfig(0, 0L);
	Sconfig(1, mode | SCB_FLPAR);		/* Parallelbetrieb ein */
	return(0);
}
