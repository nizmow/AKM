/* Run with WBDAEMON on a disposable MagiC disk, with no other WB daemon. */
#include <gem.h>
#include <mint/sysbind.h>
#include <stdio.h>

int main(void)
{
    FILE *report;
    short self, daemon_id, remaining, sent = 0;
    short message[8] = { AP_TERM, 0, 0, 0, 0, 0, 0, 0 };
    long registered, released;
    int i, failed;

    self = appl_init();
    if (self < 0)
        return 1;
    evnt_timer(1500);
    daemon_id = appl_find("WBDAEMON");
    /* GET is a zero word, also supplied by a zero generic long argument. */
    registered = Sconfig(4, 0);
    remaining = daemon_id;
    if (daemon_id >= 0) {
        message[1] = self;
        sent = appl_write(daemon_id, sizeof(message), message);
        for (i = 0; i < 40; ++i) {
            evnt_timer(50);
            remaining = appl_find("WBDAEMON");
            if (remaining < 0)
                break;
        }
    }
    released = Sconfig(4, 0);
    failed = daemon_id < 0 || registered <= 0 || sent == 0 ||
             remaining >= 0 || released != 0;
    report = fopen("C:\\WBTEST.TXT", "w");
    if (report == NULL) {
        appl_exit();
        return 2;
    }
    fprintf(report, "%s\nWBDAEMON AES ID: %d\nRegistered daemon PD: %08lx\n"
            "AP_TERM sent: %d\nRemaining AES ID: %d\nReleased daemon PD: %08lx\n",
            failed ? "FAIL" : "PASS", daemon_id, registered, sent, remaining, released);
    fclose(report);
    appl_exit();
    return failed;
}
