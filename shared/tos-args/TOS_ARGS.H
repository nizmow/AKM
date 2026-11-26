#ifndef MAGIC_TOS_ARGS_H
#define MAGIC_TOS_ARGS_H

#include <mint/basepage.h>
#include <string.h>

/* PCMXSTRT quoting for raw TOS tails; preserve MiNTLib's extended ARGV vector.
 * Caller-owned storage must live as long as the returned argument vector.
 */
struct tos_args {
    char command[128];
    char *vector[65]; /* argv[0], up to 63 one-byte arguments, then NULL. */
};

static inline int tos_arguments(int argc, char ***argv, struct tos_args *storage)
{
    unsigned int length = (unsigned char)_base->p_cmdlin[0];
    char *read, *write;
    int count = 1;

    if (length >= 127 || (*argv)[0][0] != '\0')
        return argc;
    memcpy(storage->command, _base->p_cmdlin + 1, length);
    storage->command[length] = '\0';
    storage->vector[0] = (*argv)[0];
    read = write = storage->command;
    while (*read) {
        char quote;
        while (*read && (unsigned char)*read <= ' ')
            read++;
        if (!*read)
            break;
        storage->vector[count++] = write;
        quote = (*read == '\'' || *read == '"') ? *read++ : '\0';
        while (*read) {
            if (quote) {
                if (*read == quote) {
                    read++;
                    if (quote == '\'' && *read == '\'') {
                        *write++ = *read++;
                        continue;
                    }
                    break;
                }
            } else if ((unsigned char)*read <= ' ')
                break;
            *write++ = *read++;
        }
        while (*read && (unsigned char)*read <= ' ')
            read++;
        *write++ = '\0';
    }
    storage->vector[count] = NULL;
    *argv = storage->vector;
    return count;
}

#endif
