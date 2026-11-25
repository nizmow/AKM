#include <stdint.h>
#include <gem.h>
#include <gemx.h>
#include "EDITOB.H"
#include <mint/basepage.h>

short gl_hhbox, gl_hwbox, gl_hhchar, gl_hwchar;
short aes_handle, vdi_handle;
short work_out[57], work_in[12];

static void put_long(short *out, LONG value)
{
    uint32_t bits = (uint32_t)value;
    out[0] = (short)(bits >> 16);
    out[1] = (short)bits;
}

static LONG get_long(const short *in)
{
    return (LONG)(((uint32_t)(uint16_t)in[0] << 16) | (uint16_t)in[1]);
}

static void *from_aes(long value) { return (void *)(uintptr_t)(uint32_t)value; }
static long to_aes(const void *value) { return (long)(uint32_t)(uintptr_t)value; }

static void *sys_set_getfn(short fn)
{
    short control[5] = {0, 2, 0, 0, 1};
    short intin[2] = {1, fn};
    short intout[1] = {0};
    long addrin[1] = {0};
    long addrout[1] = {0};
    AESPB pb = {control, aes_global, intin, intout, addrin, addrout};
    aes(&pb);
    return from_aes(addrout[0]);
}

static short sys_set_setfn(short fn, void *f)
{
    short control[5] = {0, 2, 1, 1, 0};
    short intin[2] = {2, fn};
    short intout[1] = {0};
    long addrin[1] = {0};
    long addrout[1] = {0};
    AESPB pb = {control, aes_global, intin, intout, addrin, addrout};
    addrin[0] = to_aes(f);
    aes(&pb);
    return intout[0];
}

static void *sys_set_editob(WORD (__CDECL *handler)(PARMBLK *))
{
    short control[5] = {0, 1, 0, 1, 1};
    short intin[1] = {4};
    short intout[1] = {0};
    long addrin[1] = {0};
    long addrout[1] = {0};
    AESPB pb = {control, aes_global, intin, intout, addrin, addrout};
    addrin[0] = to_aes((const void *)handler);
    aes(&pb);
    return from_aes(addrout[0]);
}

static OBJECT *object_for(AESPB *pb)
{
    return (OBJECT *)from_aes(pb->addrin[0]) + pb->intin[0];
}

void editor_fn217(AESPB *pb)
{
    OBJECT *ob = object_for(pb), *tree = (OBJECT *)from_aes(pb->addrin[0]);
    EDITINFO *xi = (EDITINFO *)(uintptr_t)(uint32_t)ob->ob_spec.index;
    short ret = 1, fontpix = 0;
    switch (pb->intin[1]) {
    case 0: xedit_set_buf(xi, from_aes(pb->addrin[1]), get_long(pb->intin + 2)); break;
    case 1: xedit_set_format(xi, pb->intin[2], pb->intin[3]); break;
    case 2:
        if (pb->intin[2] >= 0) xi->tcolour = pb->intin[2];
        if (pb->intin[3] >= 0) xi->bcolour = pb->intin[3];
        break;
    case 3:
        if (pb->control[1] > 5) fontpix = pb->intin[5];
        ret = xedit_set_font(xi, pb->intin[2], pb->intin[3], fontpix, pb->intin[4]); break;
    case 4: ret = xedit_set_cursor(tree, pb->intin[0], pb->intin[2], xi, from_aes(pb->addrin[1])); break;
    case 5: xedit_set_scroll_and_cpos(xi, pb->intin[2], get_long(pb->intin + 3), from_aes(pb->addrin[1]), from_aes(pb->addrin[2]), pb->intin[5], pb->intin[6]); break;
    case 6: ret = xedit_resized(ob, xi, &pb->intout[1], &pb->intout[2]); break;
    case 7: xi->dirty = pb->intin[2]; break;
    case 9: ret = xedit_scroll(tree, pb->intin[0], pb->intin[2], xi, get_long(pb->intin + 3), pb->intin[5]); break;
    default: ret = 0; break;
    }
    pb->intout[0] = ret;
}

void editor_fn216(AESPB *pb)
{
    OBJECT *ob = object_for(pb);
    EDITINFO *xi = (EDITINFO *)(uintptr_t)(uint32_t)ob->ob_spec.index;
    short ret = 1;
    switch (pb->intin[1]) {
    case 0: pb->addrout[0] = to_aes(xi->buf); put_long(pb->intout + 1, xi->buflen); put_long(pb->intout + 3, xi->curr_tlen); break;
    case 1: pb->intout[1] = xi->tabwidth; pb->intout[2] = xi->autowrap; break;
    case 2: pb->intout[1] = xi->tcolour; pb->intout[2] = xi->bcolour; break;
    case 3: pb->intout[1] = xi->fontID; pb->intout[2] = xi->fontH; pb->intout[3] = xi->mono; if (pb->control[2] > 4) pb->intout[4] = xi->fontPix; break;
    case 4: pb->addrout[0] = to_aes(xedit_get_cursor(xi)); break;
    case 5: pb->intout[1] = xi->xscroll; put_long(pb->intout + 2, xi->yscroll); pb->intout[4] = xi->ccurs_x; pb->intout[5] = xi->ccurs_y; pb->addrout[0] = to_aes(xi->lines[0].line); pb->addrout[1] = to_aes(xedit_get_cursor(xi)); break;
    case 7: ret = xi->dirty; break;
    case 8: pb->addrout[0] = to_aes(xi->bsel); pb->addrout[1] = to_aes(xi->esel); break;
    case 9: put_long(pb->intout + 1, xi->nlines); put_long(pb->intout + 3, xi->yscroll); pb->intout[5] = xi->lvis; pb->intout[6] = xi->leff; pb->intout[7] = xi->max_linew; pb->intout[8] = xi->xscroll; pb->intout[9] = ob->ob_width; break;
    default: ret = 0; break;
    }
    pb->intout[0] = ret;
}

void editor_fn215(AESPB *pb)
{
    OBJECT *tree = (OBJECT *)from_aes(pb->addrin[0]), *ob = object_for(pb);
    LONG errcode = 0;
    pb->intout[0] = xedit_evnt(tree, pb->intin[0], pb->intin[1], from_aes(pb->addrin[1]), (EDITINFO *)(uintptr_t)(uint32_t)ob->ob_spec.index, &errcode);
    if (pb->control[2] >= 3) put_long(pb->intout + 1, errcode);
}
void editor_fn214(AESPB *pb) { OBJECT *ob = object_for(pb); pb->intout[0] = xedit_cursor(from_aes(pb->addrin[0]), pb->intin[0], pb->intin[1], pb->intin[2], (EDITINFO *)(uintptr_t)(uint32_t)ob->ob_spec.index); }
void editor_fn213(AESPB *pb) { xedit_delete(from_aes(pb->addrin[0])); pb->intout[0] = 1; }
void editor_fn212(AESPB *pb) { OBJECT *ob = object_for(pb); xedit_close((EDITINFO *)(uintptr_t)(uint32_t)ob->ob_spec.index); pb->intout[0] = 1; }
void editor_fn211(AESPB *pb) { OBJECT *ob = object_for(pb); pb->intout[0] = xedit_open(ob, (EDITINFO *)(uintptr_t)(uint32_t)ob->ob_spec.index); }
void editor_fn210(AESPB *pb) { pb->addrout[0] = to_aes(xedit_create()); }

extern void fn210_entry(void);
extern void fn211_entry(void);
extern void fn212_entry(void);
extern void fn213_entry(void);
extern void fn214_entry(void);
extern void fn215_entry(void);
extern void fn216_entry(void);
extern void fn217_entry(void);

/* Bounds of the C constructor/destructor tables emitted by the SLB linker
 * contract (see edit_slb.ld). Both arrays hold plain void(*)(void) entries. */
extern void (*__slb_init_array_start[])(void);
extern void (*__slb_init_array_end[])(void);
extern void (*__slb_fini_array_start[])(void);
extern void (*__slb_fini_array_end[])(void);

static void slb_run_init_array(void)
{
    void (**fn)(void);
    for (fn = __slb_init_array_start; fn < __slb_init_array_end; ++fn)
        (*fn)();
}

static void slb_run_fini_array(void)
{
    void (**fn)(void);
    for (fn = __slb_fini_array_end; fn > __slb_fini_array_start; --fn)
        (*(fn - 1))();
}

extern LONG __CDECL slb_init(void *basepage, void *parameter)
{
    short i, dummy;
    (void)parameter;
    _base = (BASEPAGE *)basepage;
    slb_run_init_array();
    appl_init();
    if (sys_set_getfn(210)) return -1;
    aes_handle = graf_handle(&gl_hwchar, &gl_hhchar, &gl_hwbox, &gl_hhbox);
    vdi_handle = aes_handle;
    for (i = 0; i < 10; ++i) work_in[i] = 1;
    work_in[10] = 2;
    v_opnvwk(work_in, &vdi_handle, work_out);
    vst_load_fonts(vdi_handle, 0);
    vsf_color(vdi_handle, G_WHITE);
    vst_alignment(vdi_handle, 0, 5, &dummy, &dummy);
    sys_set_setfn(217, (void *)fn217_entry); sys_set_setfn(216, (void *)fn216_entry);
    sys_set_setfn(215, (void *)fn215_entry); sys_set_setfn(214, (void *)fn214_entry);
    sys_set_setfn(213, (void *)fn213_entry); sys_set_setfn(212, (void *)fn212_entry);
    sys_set_setfn(211, (void *)fn211_entry); sys_set_setfn(210, (void *)fn210_entry);
    sys_set_editob(xeditob_userdef);
    return 0;
}
extern void __CDECL slb_exit(void)
{
    sys_set_editob(0);
    sys_set_setfn(217, 0); sys_set_setfn(216, 0); sys_set_setfn(215, 0); sys_set_setfn(214, 0);
    sys_set_setfn(213, 0); sys_set_setfn(212, 0); sys_set_setfn(211, 0); sys_set_setfn(210, 0);
    vst_unload_fonts(vdi_handle, 0); v_clsvwk(vdi_handle);
    slb_run_fini_array();
}
extern LONG __CDECL slb_open(void *pd) { (void)pd; return 0; }
extern void __CDECL slb_close(void *pd) { (void)pd; }
