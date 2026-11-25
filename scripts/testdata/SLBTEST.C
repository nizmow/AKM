#include <gem.h>
#include <mint/mintbind.h>
#include <mint/osbind.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int16_t WORD;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef unsigned char UBYTE;

typedef void *SLB_HANDLE;
typedef long (*SLB_EXEC)(SLB_HANDLE, long, ...);

/*
 * Slbopen consults already-loaded libraries by name before any path and always
 * also searches SLBPATH/the default XTENSION directory.  Both opens therefore
 * name this dedicated directory explicitly; the scenario installs only the
 * library under test there and removes inherited copies from the search paths.
 */
#define SLB_AUDIT_DIR "C:\\SLBAUDIT"

typedef struct {
    VDIPB *vdipb;
    WORD nplanes;
} XP_INIT;

typedef struct {
    WORD (*xp_raster)(LONG, LONG, WORD, void *, void *);
    LONG xp_ret;
    WORD nplanes;
} LOAD_INIT;

typedef struct {
    UBYTE *buf;
    WORD w;
    WORD h;
    WORD line_width;
    WORD nplanes;
    WORD *palette;
    WORD pal_entries;
} IMG;

extern long legacy_xp0(void *, void *, void **, void **);
extern long legacy_load0(void *, void *, char *, void **);
extern long legacy_load1(void *, void *, void *);
extern WORD legacy_raster(void *, LONG, LONG, WORD, void *, void *);
extern long legacy_load2(void *, void *, void *, MFDB *, WORD, WORD, WORD);
extern long legacy_load3(void *, void *, WORD, MFDB *, WORD, WORD);

static void out(const char *text)
{
    int handle;
    Fdelete("C:\\SLBTEST.TXT");
    handle = Fcreate("C:\\SLBTEST.TXT", 0);
    if (handle >= 0) {
        Fwrite(handle, strlen(text), text);
        Fclose(handle);
    }
}

static void outf(const char *fmt, long a, long b)
{
    char text[120];
    sprintf(text, fmt, a, b);
    out(text);
}

static ULONG hash(const void *data, LONG len)
{
    const UBYTE *p = data;
    ULONG h = 2166136261UL;
    while (len-- > 0) {
        h ^= *p++;
        h *= 16777619UL;
    }
    return h;
}

static ULONG screen_hash(WORD vh, int width, int height)
{
    ULONG pixels = 2166136261UL;
    short rgb[16][3] = {{ 0 }};
    for (short pen = 0; pen < 16; pen++)
        vq_color(vh, pen, COLOR_ACTUAL, rgb[pen]);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            short pel, index = -1;
            v_get_pixel(vh, x, y, &pel, &index);
            if ((unsigned short)index >= 16)
                return 0;
            for (int component = 0; component < 3; component++) {
                pixels ^= (unsigned short)rgb[index][component];
                pixels *= 16777619UL;
            }
        }
    }
    return pixels;
}

int main(void)
{
    short control[15] = { 0 };
    short intin[128] = { 0 }, ptsin[128] = { 0 };
    short intout[45] = { 0 }, ptsout[128] = { 0 };
    VDIPB pb = { control, intin, ptsin, intout, ptsout };
    WORD win[11] = { 1 };
    WORD wout[57], vh = 0;
    XP_INIT xip = { &pb, 4 };
    LOAD_INIT lip;
    SLB_HANDLE xsl = 0, lsl = 0;
    SLB_EXEC xexec = 0, lexec = 0;
    void *raster_ass = 0, *raster_c = 0;
    UBYTE src[16] = {
        0x11,0x11,0x22,0x22, 0x33,0x33,0x44,0x44,
        0x55,0x55,0x66,0x66, 0x77,0x77,0x88,0x88
    };
    UBYTE dst[16] = { 0 };
    static const UBYTE expected[16] = {
        0x11,0x11,0x33,0x33,0x55,0x55,0x77,0x77,
        0x22,0x22,0x44,0x44,0x66,0x66,0x88,0x88
    };
    IMG *img = 0;
    MFDB fill;
    ULONG pixels = 2166136261UL;
    LONG ret, lr;
    int width, height;

    appl_init();
    v_opnvwk(win, &vh, wout);
    if (!vh) { out("vdi open\n"); return 1; }
    control[6] = vh;
    ret = trap_1_wllllll(0x16, (long)"XP_RASTR.SLB", (long)SLB_AUDIT_DIR, 1L,
                         (long)&xsl, (long)&xexec, (long)&xip);
    if (ret < 0) { outf("xp open %ld %ld\n", ret, 0); return 2; }
    if (legacy_xp0(xexec, xsl, &raster_ass, &raster_c) < 0 ||
        !raster_ass || !raster_c) { out("xp fn\n"); return 3; }
    {
        WORD r = legacy_raster(raster_c, 2, 4, 4, src, dst);
        if (!r || memcmp(dst, expected, sizeof(expected)) != 0) {
            char text[160];
            int i, n = sprintf(text, "raster %d ", (int)r);
            for (i = 0; i < 16; i++) n += sprintf(text + n, "%02x", dst[i]);
            strcat(text, "\n"); out(text); return 4;
        }
    }

    memset(&lip, 0, sizeof(lip));
    lip.xp_raster = (void *)raster_c;
    lip.nplanes = 4;
    ret = trap_1_wllllll(0x16, (long)"LOAD_IMG.SLB", (long)SLB_AUDIT_DIR, 1L,
                         (long)&lsl, (long)&lexec, (long)&lip);
    if (ret < 0) { outf("load open %ld %ld\n", ret, 0); return 5; }
    lr = legacy_load0(lexec, lsl,
                      "C:\\GEMSYS\\GEMDESK\\PAT\\16\\PAT6.IMG", (void **)&img);
    if (lr < 0 || !img || img->w != 64 || img->h != 64 || img->nplanes != 4) {
        char text[160];
        sprintf(text, "image %ld %d %d %d\n", lr, img ? img->w : -1,
                img ? img->h : -1, img ? img->nplanes : -1);
        out(text); return 6;
    }
    width = img->w;
    height = img->h;

    memset(&fill, 0, sizeof(fill));
    lr = legacy_load2(lexec, lsl, img, &fill, 0, 15, vh);
    if (lr < 0 || !fill.fd_addr || fill.fd_w != width || fill.fd_h != height) {
        outf("convert %ld %ld\n", lr, fill.fd_w); return 7;
    }
    ULONG before = screen_hash(vh, width, height);
    lr = legacy_load3(lexec, lsl, vh, &fill, 0, 0);
    if (lr < 0) { outf("draw %ld %ld\n", lr, 0); return 8; }
    pixels = screen_hash(vh, width, height);
    if (!before || !pixels || pixels == before) {
        out("draw did not produce observable colour pixels\n"); return 8;
    }

    if (fill.fd_addr && fill.fd_addr != img->buf)
        Mfree(fill.fd_addr);
    if (legacy_load1(lexec, lsl, img) < 0 ||
        trap_1_wl(0x17, (long)lsl) < 0 ||
        trap_1_wl(0x17, (long)xsl) < 0) {
        out("close\n"); return 9;
    }
    {
        char text[96];
        sprintf(text, "ok %d %d %lx %lx\n", width, height,
                (unsigned long)pixels, (unsigned long)hash(dst, sizeof(dst)));
        out(text);
    }
    return 0;
}
