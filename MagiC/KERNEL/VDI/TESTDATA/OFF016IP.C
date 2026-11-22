/* Verify OFF016IP through an explicit four-plane interleaved VDI request. */
#include <gem.h>
#include <gemx.h>
#include <mint/mintbind.h>
#include <stdio.h>
#include <string.h>

enum { WIDTH = 64, HEIGHT = 64, PLANES = 4 };
static unsigned short bitmap[HEIGHT][WIDTH / 16 * PLANES];

static int save(const char *path, const void *data, long size)
{
    long fd = Fcreate(path, 0);
    if (fd < 0)
        return 0;
    long written = Fwrite((short)fd, size, data);
    return Fclose((short)fd) == 0 && written == size;
}

int main(void)
{
    static short control[16], intin[20], ptsin[128], intout[45], ptsout[128];
    short work_in[11] = {1}, work_out[57];
    static const short pens[] = {2, 3, 4, 9, 1};
    short screen = 0;
    MFDB bm = {0}, *address = &bm;
    VDIPB pb = {control, intin, ptsin, intout, ptsout};
    const char *result = "screen failed\n";
    char samples[128];
    char *next = samples;

    save("C:\\OFF16.TXT", "start\n", 6);
    appl_init();
    v_opnvwk(work_in, &screen, work_out);
    if (screen > 0) {
        bm.fd_addr = bitmap;
        bm.fd_w = WIDTH;
        bm.fd_h = HEIGHT;
        bm.fd_wdwidth = WIDTH / 16;
        bm.fd_nplanes = PLANES;
        control[0] = 100;
        control[3] = 20;
        control[4] = 45;
        control[5] = 1;
        control[6] = screen;
        memcpy(&control[7], &address, sizeof(address));
        intin[0] = 1;
        intin[11] = WIDTH - 1;
        intin[12] = HEIGHT - 1;
        intin[13] = intin[14] = 1;
        /* Long color count (words 15/16), planes, format, organization. */
        intin[16] = 16;
        intin[17] = PLANES;
        intin[19] = 1;
        vdi(&pb);
        short handle = control[6];
        result = "bitmap open rejected\n";
        if (handle != 0) {
            vsf_interior(handle, FIS_SOLID);
            vsf_perimeter(handle, 0);
            for (unsigned i = 0; i < sizeof(pens) / sizeof(pens[0]); ++i) {
                short y = 2 + 2 * i;
                short bar[4] = {14, y, 17, y + 1};
                vsf_color(handle, pens[i]);
                v_bar(handle, bar);
            }
            next += sprintf(next, "pixels");
            for (unsigned i = 0; i < sizeof(pens) / sizeof(pens[0]); ++i) {
                for (short x = 15; x <= 16; ++x) {
                    short pel = -1, index = -1;
                    v_get_pixel(handle, x, 2 + 2 * i, &pel, &index);
                    next += sprintf(next, " %d/%d", pel, index);
                }
            }
            short pel = -1, index = -1;
            v_get_pixel(handle, 18, 2, &pel, &index);
            sprintf(next, " %d/%d\n", pel, index);
            if (handle != screen)
                v_clsvwk(handle);
            result = save("C:\\OFF16.BIN", bitmap, sizeof(bitmap)) ? samples :
                     "bitmap write failed\n";
        }
        v_clsvwk(screen);
    }
    appl_exit();
    return !save("C:\\OFF16.TXT", result, strlen(result));
}
