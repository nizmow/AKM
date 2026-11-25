#ifndef VFATCONF_GEM_H
#define VFATCONF_GEM_H

#include <gem.h>
#include <stdint.h>
typedef int16_t WORD;
#define SELECTED OS_SELECTED
#define DISABLED OS_DISABLED

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef E_OK
#define E_OK 0
#endif
#define RMODE_RW 2
#define EFILNF (-33L)
#define EPTHNF (-34L)
#define EDRIVE (-15L)
#define KER_DRVSTAT 0x0104

void ob_sel_dsel(OBJECT *tree, int which, int selected);
int selected(OBJECT *tree, int which);
void subobj_draw(OBJECT *tree, int obj, int start, int depth);
void Mgraf_mouse(int type);
void Mrsrc_load(char *filename);
WORD Rform_alert(WORD default_button, WORD alert_id);
int do_exdialog(OBJECT *dialog,
                int (*check)(OBJECT *dialog, int exit_button),
                int *was_redraw);

#endif
