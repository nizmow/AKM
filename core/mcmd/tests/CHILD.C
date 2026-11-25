/* Verify shell argument/environment inheritance, stdin/stdout and child status. */
#include <mint/osbind.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "TOS_ARGS.H"

int main(int argc, char **argv)
{
    struct tos_args arguments;
    argc = tos_arguments(argc, &argv, &arguments);
    char input[32];
    long count = Fread(0, sizeof(input), input);
    const char *value = getenv("VALUE");
    FILE *result = fopen("C:\\CHILD.RES", "w");
    /* MCMD removes grouping quotes before building the child's raw TOS tail. */
    int valid = argc == 4 && !strcmp(argv[1], "two") &&
        !strcmp(argv[2], "words") && !strcmp(argv[3], "plain") &&
        value && !strcmp(value, "expanded") &&
        count == 13 && !memcmp(input, "input bytes\r\n", 13);

    if (result) {
        fprintf(result, "argc=%d arg1=%s arg2=%s arg3=%s env=%s input=%ld valid=%d\n",
                argc, argc > 1 ? argv[1] : "", argc > 2 ? argv[2] : "",
                argc > 3 ? argv[3] : "", value ? value : "", count, valid);
        fclose(result);
    }
    Cconws("child output\r\n");
    return valid && result ? 7 : 99;
}
