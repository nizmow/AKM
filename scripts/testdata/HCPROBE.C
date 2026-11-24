/* Install HARDCOPY from a non-START guest path and record the screen hook. */
#include <mint/mintbind.h>
#include <mint/osbind.h>
#include <stdio.h>

#define SCR_DUMP 0x502
#define LOADER "C:\\GEMSYS\\MAGIC\\HCP\\HARDCOPY.PRG"

static long screen_hook(void)
{
    return *(volatile long *)SCR_DUMP;
}

int main(void)
{
    long before = Supexec(screen_hook);
    long status = Pexec(0, LOADER, "", NULL);
    long after = Supexec(screen_hook);
    FILE *out = fopen("C:\\HCPROBE.BIN", "wb");
    if (!out)
        return 1;
    long record[] = {status, before, after};
    int ok = fwrite(record, sizeof(record), 1, out) == 1;
    return fclose(out) != 0 || !ok;
}
