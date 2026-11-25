/* Verify OFF16MOV through an explicit 32-bit TrueColor bitmap request. */
#include "OSD_PROBE.H"
#include <stdio.h>

enum { WIDTH = 64, HEIGHT = 64, PLANES = 32 };
static unsigned char bitmap[HEIGHT * WIDTH * 4];

static const short pens[] = { 1, 2, 3, 4, 5 };
static const short rgb[][3] = {
    { 1000, 0, 0 }, { 0, 1000, 0 }, { 0, 0, 1000 },
    { 1000, 500, 0 }, { 500, 0, 1000 },
};

int main(void)
{
    short work_in[11] = { 1 }, work_out[57], screen = 0;
    MFDB bm = { 0 };
    const char *result = "screen failed\n";
    char samples[224];
    char *next = samples;

    osd_save("C:\\OFF16MV.TXT", "start\n", 6);
    appl_init();
    v_opnvwk(work_in, &screen, work_out);
    if (screen > 0) {
        bm.fd_addr = bitmap;
        bm.fd_w = WIDTH;
        bm.fd_h = HEIGHT;
        bm.fd_wdwidth = WIDTH / 16;
        bm.fd_nplanes = PLANES;
        short handle = osd_open(&bm, screen, 16777216L, PLANES, 2, 129);
        result = "bitmap open rejected\n";
        if (handle != 0) {
            vsf_interior(handle, FIS_SOLID);
            vsf_perimeter(handle, 0);
            for (unsigned i = 0; i < sizeof(pens) / sizeof(pens[0]); ++i) {
                short bar[4] = { 14, (short)(2 + 2 * i), 17, (short)(3 + 2 * i) };
                osd_set_rgb(handle, pens[i], rgb[i][0], rgb[i][1], rgb[i][2]);
                vsf_color(handle, pens[i]);
                v_bar(handle, bar);
            }
            next += sprintf(next, "pixels");
            for (unsigned i = 0; i < sizeof(pens) / sizeof(pens[0]); ++i) {
                for (short x = 15; x <= 16; ++x) {
                    long pel = osd_get_pixel_long(handle, x, (short)(2 + 2 * i));
                    next += sprintf(next, " %ld/-1", pel);
                }
            }
            long pel = osd_get_pixel_long(handle, 18, 2);
            sprintf(next, " %ld/-1\n", pel);
            if (handle != screen)
                v_clsvwk(handle);
            result = osd_save("C:\\OFF16MV.BIN", bitmap, sizeof(bitmap)) ? samples :
                     "bitmap write failed\n";
        }
        v_clsvwk(screen);
    }
    appl_exit();
    return !osd_save("C:\\OFF16MV.TXT", result, strlen(result));
}
