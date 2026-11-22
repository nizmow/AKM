/*
 * Classic XCONTROL CPX ABI used by the MagiC control panels.
 *
 * The original project used the external XCONTROL CPX SDK, which is not part
 * of this repository.  These declarations reproduce the documented callback
 * block, CPX header and entry record used by the archived MAGXCONF.CPX.
 */
#ifndef CPXHEAD_H
#define CPXHEAD_H

#include <gem.h>
#include <stdint.h>

#ifndef cdecl
#define cdecl
#endif

typedef short BOOLEAN;

struct cpxhead {
    uint16_t magic;
    uint16_t flags;
    int32_t cpx_id;
    uint16_t cpx_version;
    char i_text[14];
    uint16_t icon[48];
    uint16_t i_info;
    char title_txt[18];
    uint16_t t_info;
    char buffer[64];
    char reserved[306];
};

typedef struct cpxhead CPXHEAD;

typedef struct {
    short cdecl (*cpx_call)(GRECT *);
    void cdecl (*cpx_draw)(GRECT *);
    void cdecl (*cpx_wmove)(GRECT *);
    void cdecl (*cpx_timer)(short *);
    void cdecl (*cpx_key)(short, short, short *);
    void cdecl (*cpx_button)(void *, short, short *);
    void cdecl (*cpx_m1)(void *, short *);
    void cdecl (*cpx_m2)(void *, short *);
    short cdecl (*cpx_hook)(short, short *, void *, short *, short *);
    void cdecl (*cpx_close)(short);
} CPXINFO;

typedef struct xcpb {
    short handle;
    short booting;
    short reserved;
    short SkipRshFix;
    void *cdecl (*get_cpx_list)(void);
    short cdecl (*save_header)(void *);
    void cdecl (*rsh_fix)(void);
    void cdecl (*rsh_obfix)(OBJECT *, short);
    short cdecl (*Popup)(char **, short, short, short, GRECT *, GRECT *);
    void cdecl (*Sl_size)(void);
    void cdecl (*Sl_x)(void);
    void cdecl (*Sl_y)(void);
    void cdecl (*Sl_arrow)(void);
    void cdecl (*Sl_dragx)(void);
    void cdecl (*Sl_dragy)(void);
    short cdecl (*Xform_do)(OBJECT *, short, short *);
    GRECT *cdecl (*GetFirstRect)(GRECT *);
    GRECT *cdecl (*GetNextRect)(void);
    void cdecl (*Set_Evnt_Mask)(void);
    short cdecl (*XGen_Alert)(short);
    short cdecl (*CPX_Save)(void *, long);
    void *cdecl (*Get_Buffer)(void);
    short cdecl (*getcookie)(long, long *);
    short Country_Code;
    void cdecl (*MFsave)(void);
} XCPB;

#define CPX_SAVE_DEFAULTS 0

#endif
