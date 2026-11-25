#ifndef MGEDIT_GEM_H
#define MGEDIT_GEM_H

#include <stdint.h>
typedef int16_t WORD;
typedef long LONG;
_Static_assert(sizeof(LONG) == 4, "GEM long outputs must be 32-bit");
#include <gem.h>
#include <gemx.h>
#include <mint/osbind.h>
#include <mint/mintbind.h>
#include <mint/slb.h>


#define VA_START 0x4711
#define AV_STARTED 0x4738

char *Rgetstring(WORD string_id, WORD *global);
WORD Rform_alert(WORD defbutton, WORD alert_id, WORD *global);
WORD Rxform_alert(WORD defbutton, WORD alert_id, char drive,
                 const char *path, WORD *global);
char *get_name(char *path);
int do_dialog(OBJECT *dialog);

static inline void ob_dsel(OBJECT *tree, int object)
{
    tree[object].ob_state &= ~OS_SELECTED;
}

#endif
