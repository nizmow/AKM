/* Run alongside MGXCLOCK on a disposable MagiC disk with no other WB daemon. */
#include <gem.h>
#include <mint/sysbind.h>
#include <stdio.h>

int main(void)
{
    FILE *report;
    short clock;
    long daemon;
    int failed;

    if (appl_init() < 0)
        return 1;
    evnt_timer(1500);
    clock = appl_find("MGXCLOCK");
    /* A zero long argument also supplies the zero word subcommand (GET). */
    daemon = Sconfig(4, 0);
    failed = clock < 0 || daemon <= 0;
    report = fopen("C:\\MGXTEST.TXT", "w");
    if (report == NULL) {
        appl_exit();
        return 2;
    }
    fprintf(report, "%s\nMGXCLOCK AES ID: %d\nWriteback daemon PD: %08lx\n",
            failed ? "FAIL" : "PASS", clock, daemon);
    fclose(report);
    appl_exit();
    return failed;
}
