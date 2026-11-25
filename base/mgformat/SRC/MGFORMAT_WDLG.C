/* Native MagiC wdlg bindings (AES 160..167). */
#include "MGFORMAT_GEM.H"

static WORD *call_aes(WORD fn, WORD intin_count, WORD intout_count,
                      WORD addrin_count, WORD *intin, WORD *intout,
                      long *addrin, long *addrout, WORD *global)
{
    WORD control[5] = { fn, intin_count, intout_count, addrin_count, 0 };
    AESPB pb = { control, global, intin, intout, addrin, addrout };
    aes(&pb);
    return intout;
}

DIALOG *mg_wdlg_create(HNDL_OBJ handle_exit, OBJECT *tree, void *user_data,
                       WORD code, void *data, WORD flags, WORD *global)
{
    WORD in[2] = { code, flags };
    WORD out[2] = { 0, 0 };
    long ain[4] = { (long) handle_exit, (long) tree, (long) user_data, (long) data };
    long aout[1] = { 0 };
    call_aes(160, 2, 0, 4, in, out, ain, aout, global);
    return (DIALOG *) aout[0];
}

WORD mg_wdlg_open(DIALOG *dialog, const char *title, WORD kind,
                  WORD x, WORD y, WORD code, void *data, WORD *global)
{
    WORD in[4] = { kind, x, y, code };
    WORD out[1] = { 0 };
    long ain[3] = { (long) dialog, (long) title, (long) data };
    long aout[1] = { 0 };
    call_aes(161, 4, 1, 3, in, out, ain, aout, global);
    return out[0];
}

WORD mg_wdlg_close(DIALOG *dialog, WORD *x, WORD *y, WORD *global)
{
    WORD out[3] = { 0, -1, -1 };
    long ain[1] = { (long) dialog };
    long aout[1] = { 0 };
    call_aes(162, 0, 3, 1, NULL, out, ain, aout, global);
    if (x) *x = out[1];
    if (y) *y = out[2];
    return out[0];
}

WORD mg_wdlg_delete(DIALOG *dialog, WORD *global)
{
    WORD out[1] = { 0 };
    long ain[1] = { (long) dialog };
    long aout[1] = { 0 };
    call_aes(163, 0, 1, 1, NULL, out, ain, aout, global);
    return out[0];
}

WORD mg_wdlg_get_tree(DIALOG *dialog, OBJECT **tree, GRECT *r, WORD *global)
{
    WORD in[1] = { 0 };
    WORD out[1] = { 0 };
    long ain[3] = { (long) dialog, (long) tree, (long) r };
    long aout[1] = { 0 };
    call_aes(164, 1, 1, 3, in, out, ain, aout, global);
    return out[0];
}

WORD mg_wdlg_get_handle(DIALOG *dialog, WORD *global)
{
    WORD in[1] = { 3 };
    WORD out[1] = { 0 };
    long ain[1] = { (long) dialog };
    long aout[1] = { 0 };
    call_aes(164, 1, 1, 1, in, out, ain, aout, global);
    return out[0];
}

WORD mg_wdlg_set_iconify(DIALOG *dialog, GRECT *g, const char *title,
                         OBJECT *tree, WORD obj, WORD *global)
{
    WORD in[2] = { 3, obj };
    WORD out[1] = { 0 };
    long ain[4] = { (long) dialog, (long) g, (long) title, (long) tree };
    long aout[1] = { 0 };
    call_aes(165, 2, 1, 4, in, out, ain, aout, global);
    return out[0];
}

WORD mg_wdlg_set_uniconify(DIALOG *dialog, GRECT *g, const char *title,
                           OBJECT *tree, WORD *global)
{
    WORD in[1] = { 4 };
    WORD out[1] = { 0 };
    long ain[4] = { (long) dialog, (long) g, (long) title, (long) tree };
    long aout[1] = { 0 };
    call_aes(165, 1, 1, 4, in, out, ain, aout, global);
    return out[0];
}

WORD mg_wdlg_evnt(DIALOG *dialog, EVNT *events, WORD *global)
{
    WORD out[1] = { 0 };
    long ain[2] = { (long) dialog, (long) events };
    long aout[1] = { 0 };
    call_aes(166, 0, 1, 2, NULL, out, ain, aout, global);
    return out[0];
}

void mg_wdlg_redraw(DIALOG *dialog, GRECT *rect, WORD obj, WORD depth,
                    WORD *global)
{
    WORD in[2] = { obj, depth };
    WORD out[1] = { 0 };
    long ain[2] = { (long) dialog, (long) rect };
    long aout[1] = { 0 };
    call_aes(167, 2, 0, 2, in, out, ain, aout, global);
}
