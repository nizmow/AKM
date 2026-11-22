/* Run the tool named in C:\DIAG.TXT and capture stdout/status. */
#include <mint/osbind.h>
#include <stdio.h>

int main(void)
{
    char config[128];
    char tail[128] = {0};
    char line[24];
    char *end;
    short handle;
    short saved;
    long count;
    long result;
    int length;

    handle = Fopen("C:\\DIAG.TXT", 0);
    if (handle < 0)
        return 1;
    count = Fread(handle, sizeof(config) - 1, config);
    Fclose(handle);
    if (count <= 0)
        return 2;
    config[count] = '\0';
    end = config;
    while (*end && *end != '\r' && *end != '\n')
        end++;
    *end = '\0';
    if (*config == '\0')
        return 2;

    handle = Fcreate("C:\\DIAG.OUT", 0);
    saved = Fdup(1);
    if (handle < 0 || saved < 0 || Fforce(1, handle) < 0)
        return 3;
    result = Pexec(0, config, tail, 0);
    Fforce(1, saved);
    Fclose(saved);
    Fclose(handle);

    handle = Fcreate("C:\\DIAG.STA", 0);
    if (handle < 0)
        return 4;
    length = snprintf(line, sizeof(line), "%ld\n", result);
    if (length < 0 || Fwrite(handle, length, line) != length)
        return 5;
    if (Fclose(handle) < 0)
        return 5;
    return 0;
}
