/*
 * Exercise MagiC/NVDI's 1-plane offscreen path through OFF002.OSD.
 *
 * This is deliberately a small application-level probe, not a driver unit
 * test. It asks VDI for an offscreen workstation backed by a caller-owned
 * 64x64, one-plane MFDB, fills a rectangle, reads a pixel back and records
 * the resulting bitmap hash. The scenario runs it once with OFF002.OSD
 * installed and once with that filename unavailable.
 */
#include <gem.h>
#include <gemx.h>
#include <mint/mintbind.h>
#include <stdio.h>
#include <string.h>

static unsigned char bitmap[64 * 8]; /* 64 rows x 8 bytes for one plane */
static short control[16];
static short intin[20];
static short ptsin[128];
static short intout[45];
static short ptsout[128];

static void write_result(const char *text)
{
    long handle;
    Fdelete("C:\\OFF002.TXT");
    handle = Fcreate("C:\\OFF002.TXT", 0);
    if (handle >= 0) {
        Fwrite(handle, (long)strlen(text), text);
        Fclose(handle);
    }
}

static unsigned long hash_bitmap(void)
{
    unsigned long h = 2166136261UL;
    int i;
    for (i = 0; i < (int)sizeof(bitmap); i++) {
        h ^= bitmap[i];
        h *= 16777619UL;
    }
    return h;
}

int main(void)
{
    short screen = 0, bitmap_handle = 0;
    short win[11] = { 1 };
    short screen_out[57];
    short pel = 0, index = -1;
    short bar[4] = { 8, 8, 23, 23 };
    MFDB bm;
    VDIPB pb = { control, intin, ptsin, intout, ptsout };
    char text[160];

    memset(&bm, 0, sizeof(bm));
    memset(bitmap, 0, sizeof(bitmap));
    bm.fd_addr = bitmap;
    bm.fd_w = 64;
    bm.fd_h = 64;
    bm.fd_wdwidth = 4;      /* 64 pixels / 16 bits per word */
    bm.fd_stand = 0;        /* device-specific, interleaved plane */
    bm.fd_nplanes = 1;

    write_result("start\n");
    appl_init();
    v_opnvwk(win, &screen, screen_out);
    if (!screen) {
        write_result("fail screen\n");
        return 1;
    }

    memset(control, 0, sizeof(control));
    memset(intin, 0, sizeof(intin));
    control[0] = 100;       /* v_opnvwk family */
    control[3] = 20;        /* intin count required by v_opnbm() */
    control[4] = 45;        /* intout count */
    control[5] = 1;         /* opcode2: bitmap form */
    control[6] = screen;    /* screen workstation as the device */
    *(void **)(control + 7) = &bm; /* s_addr: MFDB */
    intin[0] = 1;
    intin[11] = 63;         /* bitmap width - 1; VDI dimensions are max coords */
    intin[12] = 63;         /* bitmap height - 1; keeps driver bitmap_len at 512 */
    intin[13] = 1;          /* pixel width */
    intin[14] = 1;          /* pixel height */
    vdi(&pb);
    bitmap_handle = control[6];

    if (!bitmap_handle) {
        write_result("fail open\n");
        v_clsvwk(screen);
        return 2;
    }

    vsf_interior(bitmap_handle, FIS_SOLID);
    vsf_color(bitmap_handle, 1);
    v_bar(bitmap_handle, bar);
    v_get_pixel(bitmap_handle, 12, 12, &pel, &index);

    sprintf(text, "ok handle=%d pixel=%d index=%d hash=%08lx\n",
            bitmap_handle, pel, index, hash_bitmap());
    write_result(text);
    v_clsvwk(bitmap_handle);
    v_clsvwk(screen);
    return 0;
}
