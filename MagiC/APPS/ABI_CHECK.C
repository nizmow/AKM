/* Compile-time ABI checks for the application build. */
#include <stddef.h>

#include <gem.h>
#include <mint/ostruct.h>

#include "WINDOWS.H"

_Static_assert(sizeof(short) == 2, "16-bit short");
_Static_assert(sizeof(int) == 4, "32-bit application int");
_Static_assert(sizeof(long) == 4, "32-bit Atari long");
_Static_assert(sizeof(void *) == 4, "32-bit pointers");

_Static_assert(sizeof(GRECT) == 8, "GRECT is four words");
_Static_assert(offsetof(GRECT, g_x) == 0, "GRECT.g_x");
_Static_assert(offsetof(GRECT, g_y) == 2, "GRECT.g_y");
_Static_assert(offsetof(GRECT, g_w) == 4, "GRECT.g_w");
_Static_assert(offsetof(GRECT, g_h) == 6, "GRECT.g_h");

_Static_assert(sizeof(OBJECT) == 24, "OBJECT layout");
_Static_assert(offsetof(OBJECT, ob_spec) == 12, "OBJECT.ob_spec");
_Static_assert(offsetof(OBJECT, ob_x) == 16, "OBJECT.ob_x");

_Static_assert(sizeof(PARMBLK) == 30, "PARMBLK layout");
_Static_assert(offsetof(PARMBLK, pb_obj) == 4, "PARMBLK.pb_obj");
_Static_assert(offsetof(PARMBLK, pb_parm) == 26, "PARMBLK.pb_parm");

_Static_assert(sizeof(_DTA) == 44, "GEMDOS DTA layout");
_Static_assert(offsetof(_DTA, dta_attribute) == 21, "DTA attribute");
_Static_assert(offsetof(_DTA, dta_size) == 26, "DTA size");
_Static_assert(offsetof(_DTA, dta_name) == 30, "DTA name");

_Static_assert(sizeof(WMESAG) == 16, "AES message is eight words");
_Static_assert(offsetof(WMESAG, msg.code) == 0, "message code");
_Static_assert(offsetof(WMESAG, msg.dest_apid) == 2, "message destination");
_Static_assert(offsetof(WMESAG, msg.whdl) == 6, "message window handle");
_Static_assert(offsetof(WMESAG, msg.g) == 8, "message rectangle");
