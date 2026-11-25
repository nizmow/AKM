#ifndef VT52_DRAGDROP_H
#define VT52_DRAGDROP_H

#ifndef AP_DRAGDROP
#define AP_DRAGDROP 63
#endif

#define DD_OK       0
#define DD_NAK      1
#define DD_EXT      2
#define DD_LEN      3
#define DD_TIMEOUT  4000
#define DD_NUMEXTS  8
#define DD_EXTSIZE  32
#define DD_NAMEMAX  128

WORD ddcreate(WORD app_id, WORD rcvr_id, WORD window, WORD mx, WORD my,
              WORD kbstate, ULONG format[8], void **oldpipesig);
WORD ddstry(WORD handle, ULONG format, BYTE *name, LONG size);
WORD ddopen(BYTE *pipe, ULONG format[8], void **oldpipesig);
void ddclose(WORD handle, void *oldpipesig);
WORD ddrtry(WORD handle, BYTE *name, ULONG *format, LONG *size);
WORD ddreply(WORD handle, WORD msg);

#endif
