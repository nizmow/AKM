#include <gem.h>
#include <mint/osbind.h>
#include <stdlib.h>

#include "VFATCONF_GEM.H"

void ob_sel_dsel(OBJECT *tree, int which, int is_selected)
{
    if (is_selected)
        tree[which].ob_state |= SELECTED;
    else
        tree[which].ob_state &= (uint16_t)~SELECTED;
}

int selected(OBJECT *tree, int which)
{
    return (tree[which].ob_state & SELECTED) != 0;
}

void subobj_draw(OBJECT *tree, int obj, int start, int depth)
{
    short x, y;

    objc_offset(tree, (short)obj, &x, &y);
    objc_draw(tree, (short)start, (short)depth, x, y,
              tree[obj].ob_width, tree[obj].ob_height);
}

void Mgraf_mouse(int type)
{
    graf_mouse((short)type, NULL);
}

void Mrsrc_load(char *filename)
{
    if (!rsrc_load(filename)) {
        form_alert(1, "[1][Unable to load VFATCONF.RSC.][Cancel]");
        appl_exit();
        Pterm((int)EFILNF);
    }
}

WORD Rform_alert(WORD default_button, WORD alert_id)
{
    char *alert;

    if (!rsrc_gaddr(R_STRING, alert_id, &alert))
        return form_alert(default_button, "[1][Resource alert is missing.][Cancel]");
    return form_alert(default_button, alert);
}

int do_exdialog(OBJECT *dialog,
                int (*check)(OBJECT *dialog, int exit_button),
                int *was_redraw)
{
    short x, y, width, height;
    int exit_button;

    form_center(dialog, &x, &y, &width, &height);
    form_dial(FMD_START, 0, 0, 0, 0, x, y, width, height);
    objc_draw(dialog, ROOT, MAX_DEPTH, x, y, width, height);

    do {
        exit_button = form_do(dialog, 0) & 0x7fff;
    } while (!check(dialog, exit_button));

    form_dial(FMD_FINISH, 0, 0, 0, 0, x, y, width, height);
    if (was_redraw != NULL)
        *was_redraw = 0;
    return exit_button;
}
