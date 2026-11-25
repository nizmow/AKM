/* Run the MCMD batch fixture after the MagiC desktop has initialized. */
#include <gem.h>
#include <mint/osbind.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    static const char text[] = "C:\\CASE.BAT \"batch argument\"";
    char tail[128] = {0};
    long status;
    FILE *result;

    appl_init();
    evnt_timer(8000);
    Dsetdrv(2);
    Dsetpath("C:\\");
    tail[0] = sizeof(text) - 1;
    memcpy(tail + 1, text, sizeof(text));
    status = Pexec(0, "C:\\MCMD.TOS", tail, NULL);
    result = fopen("C:\\MCMD.RES", "w");
    if (result) {
        fprintf(result, "STATUS %ld\n", status);
        fclose(result);
    }
    appl_exit();
    return result ? 0 : 1;
}
