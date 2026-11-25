/*
 * Erledigt für Mag!X 4.01 den Writeback-Cache.
 */

#include <stddef.h>
#include <gem.h>
#include <mint/osbind.h>
#include <mint/sysbind.h>

#define SIGFREEZE 100
#define SIGIGN 1L

/* MagiC-specific shel_write and Sconfig extensions. */
#define SC_WBACK 4
#define SCWB_SET 1
#define Ssync() ((void)trap_1_w(0x150))

int main(void)
{
    short ev;
    short buf[16];

    if (appl_init() < 0)
        return -1;

    /* Tell AES that this application understands AP_TERM. */
    shel_write(SHW_INFRECGN, 1, 0, NULL, NULL);

    /* SC_WBACK takes a word subcommand, unlike generic Sconfig's long argument. */
    if (trap_1_www(0x33, SC_WBACK, SCWB_SET) < 0L)
        return -1;

    /* Prevent freezing. */
    if (Psignal(SIGFREEZE, (void *)SIGIGN))
        return -2;

    for (;;) {
        /* MagiC 3.0: appl_read supports a timeout in 50 Hz ticks. */
        ev = appl_read(-2, 25, buf);
        if ((ev & MU_MESAG) && buf[0] == AP_TERM)
            return 0;

        Ssync();
    }
}
