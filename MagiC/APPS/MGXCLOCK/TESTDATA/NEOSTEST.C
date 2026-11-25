/* Run alongside MGXCLOCK on a disposable MagiC disk with no other WB daemon. */
#include <gem.h>
#include <mint/sysbind.h>
#include <stdio.h>

int main(void)
{
    FILE *report, *persist;
    short self, clock, remaining, sent = 0;
    short message[8] = { AP_TERM, 0, 0, 0, 0, 0, 0, 0 };
    long registered, released;
    int i, failed;

    self = appl_init();
    if (self < 0)
        return 1;
    evnt_timer(1500);
    clock = appl_find("MGXCLOCK");
    /* A zero long argument also supplies the zero word subcommand (GET). */
    registered = Sconfig(4, 0);
    /* Written while the daemon is active: close must not be the only flush. */
    persist = fopen("C:\\WBFLUSH.TXT", "w");
    if (persist != NULL) {
        fputs("writeback persistence\n", persist);
        fclose(persist);
    }
    remaining = clock;
    if (clock >= 0) {
        message[1] = self;
        sent = appl_write(clock, sizeof(message), message);
        for (i = 0; i < 40; ++i) {
            evnt_timer(50);
            remaining = appl_find("MGXCLOCK");
            if (remaining < 0)
                break;
        }
    }
    released = Sconfig(4, 0);
    failed = clock < 0 || registered <= 0 || sent == 0 ||
             remaining >= 0 || released != 0;
    report = fopen("C:\\MGXTEST.TXT", "w");
    if (report == NULL) {
        appl_exit();
        return 2;
    }
    fprintf(report, "%s\nMGXCLOCK AES ID: %d\nRegistered daemon PD: %08lx\n"
            "AP_TERM sent: %d\nRemaining AES ID: %d\nReleased daemon PD: %08lx\n",
            failed ? "FAIL" : "PASS", clock, registered, sent, remaining, released);
    fclose(report);
    appl_exit();
    return failed;
}
