/* Verify OFF32KFL through an explicit 32768-colour flat bitmap request. */
#include "OSD_PROBE.H"
#include <stdio.h>

enum { WIDTH = 64, HEIGHT = 64, PLANES = 16 };
static unsigned char bitmap[HEIGHT * WIDTH * 2];

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
    char samples[160];
    char *next = samples;

    osd_save("C:\\OFF32KF.TXT", "start\n", 6);
    appl_init();
    v_opnvwk(work_in, &screen, work_out);
    if (screen > 0) {
        bm.fd_addr = bitmap;
        bm.fd_w = WIDTH;
        bm.fd_h = HEIGHT;
        bm.fd_wdwidth = WIDTH / 16;
        bm.fd_nplanes = PLANES;
        short handle = osd_open(&bm, screen, 32768, PLANES, 2, 2);
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
                    short pel = -1, index = -1;
                    v_get_pixel(handle, x, (short)(2 + 2 * i), &pel, &index);
                    next += sprintf(next, " %d/%d", pel, index);
                }
            }
            short pel = -1, index = -1;
            v_get_pixel(handle, 18, 2, &pel, &index);
            sprintf(next, " %d/%d\n", pel, index);
            if (handle != screen)
                v_clsvwk(handle);
            result = osd_save("C:\\OFF32KF.BIN", bitmap, sizeof(bitmap)) ? samples :
                     "bitmap write failed\n";
        }
        v_clsvwk(screen);
    }
    appl_exit();
    return !osd_save("C:\\OFF32KF.TXT", result, strlen(result));
}
