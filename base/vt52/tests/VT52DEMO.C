/* Minimal TOS console client for the VT52 smoke scenario. */
#include <mint/ostruct.h>
#include <mint/osbind.h>

int main(void)
{
	static const char text[] = "VT52 DEMO READY\r\n";
	volatile long i;

	for (i = 0; i < 1000000L; i++)
		;

	for (const char *p = text; *p; p++)
		Bconout(2, *p);

	Pterm0();
	return 0;
}
