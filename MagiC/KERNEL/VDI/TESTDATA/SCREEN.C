/* Runs from C:\GEMSYS\MAGIC\START after the screen driver initialized. */
#include <stdio.h>

int main(void)
{
    FILE *marker = fopen("C:\\SCREEN.TXT", "w");

    if (marker == 0)
        return 1;
    fputs("MAGIC\n", marker);
    fclose(marker);
    return 0;
}
