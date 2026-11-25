#ifndef OSD_PROBE_H
#define OSD_PROBE_H

#include <gem.h>
#include <gemx.h>
#include <mint/mintbind.h>
#include <string.h>

/* Minimal VDI helpers shared by the packed/direct-colour OSD probes. */

static __attribute__((unused)) int osd_save(const char *path, const void *data, long size)
{
    long fd = Fcreate(path, 0);
    if (fd < 0)
        return 0;
    long written = Fwrite((short)fd, size, data);
    return Fclose((short)fd) == 0 && written == size;
}

static __attribute__((unused)) void osd_set_rgb(short handle, short index, short r, short g, short b)
{
    static short control[16], intin[4], ptsin[4], intout[4], ptsout[4];
    VDIPB pb = { control, intin, ptsin, intout, ptsout };

    memset(control, 0, sizeof(control));
    memset(intin, 0, sizeof(intin));
    control[0] = 14;            /* v_set_color */
    control[3] = 4;
    control[6] = handle;
    intin[0] = index;
    intin[1] = r;
    intin[2] = g;
    intin[3] = b;
    vdi(&pb);
}

static __attribute__((unused)) short osd_open(MFDB *bm, short screen, long colors, short planes,
                      short format, short org)
{
    static short control[16], intin[20], ptsin[128], intout[45], ptsout[128];
    VDIPB pb = { control, intin, ptsin, intout, ptsout };
    MFDB *address = bm;

    memset(control, 0, sizeof(control));
    memset(intin, 0, sizeof(intin));
    control[0] = 100;
    control[3] = 20;
    control[4] = 45;
    control[5] = 1;
    control[6] = screen;
    memcpy(&control[7], &address, sizeof(address));
    intin[0] = 1;
    intin[11] = 63;
    intin[12] = 63;
    intin[13] = intin[14] = 1;
    intin[15] = (short)(colors >> 16);
    intin[16] = (short)colors;
    intin[17] = planes;
    intin[18] = format;
    intin[19] = org;
    vdi(&pb);
    return control[6];
}

/* v_get_pixel for a TrueColor workstation: the driver returns a LONG. */
static __attribute__((unused)) long osd_get_pixel_long(short handle, short x, short y)
{
    static short control[16], intin[2], ptsin[4], intout[4], ptsout[4];
    VDIPB pb = { control, intin, ptsin, intout, ptsout };

    memset(control, 0, sizeof(control));
    memset(ptsin, 0, sizeof(ptsin));
    control[0] = 105;
    control[1] = 2;
    control[4] = 2;
    control[6] = handle;
    ptsin[0] = x;
    ptsin[1] = y;
    vdi(&pb);
    long value;
    memcpy(&value, intout, sizeof(value));
    return value;
}

#endif
