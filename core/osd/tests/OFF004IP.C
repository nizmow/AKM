/* Exercise two-plane interleaved drawing through NVDI's v_opnbm. */
#include <gem.h>
#include <gemx.h>
#include <mint/mintbind.h>
#include <stdio.h>
#include <string.h>

enum { WIDTH = 64, HEIGHT = 64, PLANES = 2 };
static unsigned short bitmap[HEIGHT][WIDTH / 16 * PLANES];

static int save(const char *path, const void *data, long size)
{
    long fd = Fcreate(path, 0);
    long written;
    if (fd < 0)
        return 0;
    written = Fwrite((short)fd, size, data);
    return Fclose((short)fd) == 0 && written == size;
}

int main(void)
{
    static short control[16], intin[20], ptsin[128], intout[45], ptsout[128];
    static short work_in[11] = { 1 }, work_out[57];
    static short points[][2] = { {14, 2}, {16, 2}, {15, 4}, {18, 5}, {18, 2} };
    short pen2[4] = {14, 2, 17, 3}, pen3[4] = {15, 4, 18, 5};
    short pel[5], index[5], screen = 0, handle;
    MFDB bm = {0}, *address = &bm;
    VDIPB pb = { control, intin, ptsin, intout, ptsout };
    const char *result = "screen failed\n";
    char samples[80];
    unsigned int i;

    save("C:\\OFF4.TXT", "start\n", 6);
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
        vdi(&pb);
        handle = control[6];
        result = "bitmap open failed\n";
        if (handle > 0) {
            vsf_interior(handle, FIS_SOLID);
            vsf_perimeter(handle, 0);
            vsf_color(handle, 2);
            v_bar(handle, pen2);
            vsf_color(handle, 3);
            v_bar(handle, pen3);
            for (i = 0; i < sizeof(points) / sizeof(points[0]); ++i)
                v_get_pixel(handle, points[i][0], points[i][1], &pel[i], &index[i]);
            result = "bitmap write failed\n";
            if (save("C:\\OFF4.BIN", bitmap, sizeof(bitmap))) {
                sprintf(samples, "pixels %d/%d %d/%d %d/%d %d/%d %d/%d\n",
                        pel[0], index[0], pel[1], index[1], pel[2], index[2],
                        pel[3], index[3], pel[4], index[4]);
                result = samples;
            }
            v_clsvwk(handle);
        }
        v_clsvwk(screen);
    }
    appl_exit();
    return !save("C:\\OFF4.TXT", result, strlen(result));
}
