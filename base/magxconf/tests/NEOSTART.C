/* Scenario-local MagiC configuration probe and delayed program launcher.
 * Accessory tests sample CURRENT.RES on the disposable E: drive. */
#include <gem.h>
#include <mint/osbind.h>
#include <stdio.h>
#include <string.h>

static unsigned long config(void)
{
    return (unsigned long) trap_1_ww(0x33, 0x414b); /* Sconfig 'AK': query */
}

static void record(const char *name, long status)
{
    char path[64];
    FILE *result;
    snprintf(path, sizeof(path), "E:\\%s.RES", name);
    result = fopen(path, "w");
    if (result) {
        fprintf(result, "%08lx %ld\n", config(), status);
        fclose(result);
    }
}

int main(void)
{
    char mode = 'r';
    FILE *input;
    static const char tail[128] = {0};
    static const char *steps[] = {"CANCEL", "OK", "SAVE"};

    appl_init();
    evnt_timer(8000);
    Dsetdrv(2);
    Dsetpath("C:\\");
    input = fopen("C:\\CNFMODE.TXT", "r");
    if (input) {
        mode = (char) fgetc(input);
        fclose(input);
    }
    record("BEFORE", 0);
    if (mode == 'p') {
        for (int i = 0; i < 3; i++) {
            long status = Pexec(0, "C:\\MAGXCONF.PRG", tail, NULL);
            record(steps[i], status);
            evnt_timer(1000);
        }
    } else if (mode == 'a') {
        for (;;) {
            record("CURRENT", 0);
            evnt_timer(200);
        }
    }
    appl_exit();
    return 0;
}
