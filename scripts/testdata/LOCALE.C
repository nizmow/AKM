/* MagiC locale probe: public Keytbl/Bioskeys semantics, guest identity and
 * console key translation.  Writes table dumps and a text report to E:. */
#include <gem.h>
#include <mint/osbind.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

_Static_assert(sizeof(long) == 4 && sizeof(void *) == 4, "Atari trap ABI");
#define TABLES 9
#define TABLE_BYTES 128
#define REPLACED 3

static unsigned long idt, akp;
static int have_idt, have_akp;

static long cookies(void)
{
    volatile unsigned long *cookie = *(volatile unsigned long **)0x5a0;
    if (!cookie)
        return 0;
    for (; cookie[0]; cookie += 2) {
        if (cookie[0] == 0x5f494454UL) { idt = cookie[1]; have_idt = 1; }
        if (cookie[0] == 0x5f414b50UL) { akp = cookie[1]; have_akp = 1; }
    }
    return 0;
}

static int dump(const char *name, unsigned char **tables)
{
    FILE *file = fopen(name, "wb");
    int index, failed;
    if (!file)
        return 1;
    failed = 0;
    for (index = 0; index < TABLES; index++)
        if (fwrite(tables[index], 1, TABLE_BYTES, file) != TABLE_BYTES)
            failed = 1;
    if (fclose(file))
        failed = 1;
    return failed;
}

int main(void)
{
    unsigned char **tables, *saved[TABLES], replacement[REPLACED][TABLE_BYTES];
    unsigned short keys[7];
    short language = 0, one = 0, two = 0, three = 0;
    int index, replaced, reset, i;
    FILE *file;
    if (appl_init() < 0)
        return 1;
    Supexec(cookies);
    if (!appl_getinfo(3, &language, &one, &two, &three))
        return 2;
    tables = (unsigned char **)Keytbl((void *)-1L, (void *)-1L, (void *)-1L);
    if (!tables)
        return 3;
    memcpy(saved, tables, sizeof(saved));
    if (dump("E:\\BOOTKEYS.BIN", tables))
        return 4;

    memset(replacement, 0x41, sizeof(replacement));
    memset(replacement[1], 0x42, TABLE_BYTES);
    memset(replacement[2], 0x43, TABLE_BYTES);
    Keytbl(replacement[0], replacement[1], replacement[2]);
    replaced = 1;
    for (index = 0; index < TABLES; index++)
        if (index < REPLACED ? tables[index] != replacement[index]
                             : tables[index] != saved[index])
            replaced = 0;
    Bioskeys();
    reset = memcmp(saved, tables, sizeof(saved)) == 0;
    if (dump("E:\\RESETKEYS.BIN", tables))
        return 5;

    file = fopen("E:\\LOCALE.TXT", "w");
    if (!file)
        return 6;
    fprintf(file, "language=%d\nidt_present=%d\nidt=%lu\nakp_present=%d\n"
                  "akp=%lu\nkeytbl=%d\nbioskeys=%d\nready=1\n",
            language, have_idt, idt, have_akp, akp, replaced, reset);
    if (fclose(file))
        return 7;

    /* Seven bounded console keys; the host injects them after boot. */
    for (i = 0; i < 7; i++)
        keys[i] = (unsigned short)Bconin(2);
    file = fopen("E:\\KEYS.TXT", "w");
    if (!file)
        return 8;
    for (i = 0; i < 7; i++)
        fprintf(file, "%02x\n", keys[i] & 0xff);
    fprintf(file, "DONE\n");
    if (fclose(file))
        return 9;
    appl_exit();
    return 0;
}
