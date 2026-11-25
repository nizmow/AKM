/*      VDI.H

        GEM VDI Definitions

        Copyright (c) Borland International 1990
        All Rights Reserved.
*/


#if  !defined(__VDI__)
#define __VDI__

#include <stdint.h>

typedef struct
{
	int16_t    contrl[15];
	int16_t    int16_tin[1024];
	int16_t    ptsin[128];
	int16_t    int16_tout[45];
	int16_t    ptsout[128];
} VDIPARBLK;

/* PC-Definition
typedef struct
{
	int16_t    contrl[15];
	int16_t    int16_tin[132];
	int16_t    int16_tout[140];
	int16_t    ptsin[145];
	int16_t    ptsout[145];
} VDIPARBLK;
*/

typedef struct
{
	int16_t    *contrl;
	int16_t    *int16_tin;
	int16_t    *ptsin;
	int16_t    *int16_tout;
	int16_t    *ptsout;
} VDIPB;

extern  VDIPARBLK _VDIParBlk;

void vdi(VDIPB *vdipb);


/****** Control definitions *********************************************/

void    v_opnwk(int16_t *work_in,  int16_t *handle, int16_t *work_out);
void    v_clswk(int16_t handle);
void    v_opnvwk(int16_t *work_in, int16_t *handle, int16_t *work_out);
void    v_clsvwk(int16_t handle);
void    v_clrwk(int16_t handle);
void    v_updwk(int16_t handle);
int16_t vst_load_fonts(int16_t handle, int16_t select);
void    vst_unload_fonts(int16_t handle, int16_t select);
void    vs_clip(int16_t handle, int16_t clip_flag, int16_t *pxyarray);


/****** Output definitions **********************************************/

void    v_pline(int16_t handle, int16_t count, int16_t *pxyarray);
void    v_pmarker(int16_t handle, int16_t count, int16_t *pxyarray);
void    v_gtext(int16_t handle, int16_t x, int16_t y, char *string);
void    v_fillarea(int16_t handle, int16_t count, int16_t *pxyarray);
void    v_cellarray(int16_t handle, int16_t *pxyarray, int16_t row_length,
                     int16_t el_used, int16_t num_rows, int16_t wrt_mode,
                     int16_t *colarray);
void    v_contourfill(int16_t handle, int16_t x, int16_t y, int16_t index);
void    vr_recfl(int16_t handle, int16_t *pxyarray);
void    v_bar(int16_t handle, int16_t *pxyarray);
void    v_arc(int16_t handle, int16_t x, int16_t y, int16_t radius,
               int16_t begang, int16_t endang);
void    v_pieslice(int16_t handle, int16_t x, int16_t y, int16_t radius,
                    int16_t begang, int16_t endang);
void    v_circle(int16_t handle, int16_t x, int16_t y, int16_t radius);
void    v_ellarc(int16_t handle, int16_t x, int16_t y, int16_t xradius,
                  int16_t yradius, int16_t begang, int16_t endang);
void    v_ellpie(int16_t handle, int16_t x, int16_t y, int16_t xradius,
                  int16_t yradius, int16_t begang, int16_t endang);
void    v_ellipse(int16_t handle, int16_t x, int16_t y, int16_t xradius,
                   int16_t yradius );
void    v_rbox  (int16_t handle, int16_t *pxyarray);
void    v_rfbox (int16_t handle, int16_t *pxyarray);
void    v_justified(int16_t handle,int16_t x, int16_t y, char *string,
                     int16_t length, int16_t word_space,
                     int16_t char_space);


/****** Attribute definitions *****************************************/

#define IP_HOLLOW       0
#define IP_1PATT        1
#define IP_2PATT        2
#define IP_3PATT        3
#define IP_4PATT        4
#define IP_5PATT        5
#define IP_6PATT        6
#define IP_SOLID        7


/* gsx modes */

#define MD_REPLACE      1
#define MD_TRANS        2
#define MD_XOR          3
#define MD_ERASE        4


/* gsx styles */

#define FIS_HOLLOW      0
#define FIS_SOLID       1
#define FIS_PATTERN     2
#define FIS_HATCH       3
#define FIS_USER        4


/* bit blt rules */

#define ALL_WHITE        0
#define S_AND_D          1
#define S_AND_NOTD       2
#define S_ONLY           3
#define NOTS_AND_D       4
#define D_ONLY           5
#define S_XOR_D          6
#define S_OR_D           7
#define NOT_SORD         8
#define NOT_SXORD        9
#define D_INVERT        10
#define NOT_D           11
#define S_OR_NOTD       12
#define NOTS_OR_D       13
#define NOT_SANDD       14
#define ALL_BLACK       15


/* linetypes */

#define SOLID           1
#define LONGDASH        2
#define DOT             3
#define DASHDOT         4
#define DASH            5
#define DASH2DOT        6
#define USERLINE        7

#define SQUARE          0
#define ARROWED         1
#define ROUND           2

int16_t vswr_mode(int16_t handle, int16_t mode);
void    vs_color(int16_t handle, int16_t index, int16_t *rgb_in);
int16_t vsl_type(int16_t handle, int16_t style);
void    vsl_udsty(int16_t handle, int16_t pattern);
int16_t vsl_width(int16_t handle, int16_t width);
int16_t vsl_color(int16_t handle, int16_t color_index);
void    vsl_ends(int16_t handle, int16_t beg_style, int16_t end_style);
int16_t vsm_type(int16_t handle, int16_t symbol);
int16_t vsm_height(int16_t handle, int16_t height);
int16_t vsm_color(int16_t handle, int16_t color_index);
void    vst_height(int16_t handle, int16_t height, int16_t *char_width,
                    int16_t *char_height, int16_t *cell_width,
                    int16_t *cell_height);
int16_t vst_point(int16_t handle, int16_t point16_t, int16_t *char_width,
                    int16_t *char_height, int16_t *cell_width,
                    int16_t *cell_height);
int16_t vst_rotation(int16_t handle, int16_t angle);
int16_t vst_font(int16_t handle, int16_t font);
int16_t vst_color(int16_t handle, int16_t color_index);
int16_t vst_effects(int16_t handle, int16_t effect);
void    vst_alignment(int16_t handle, int16_t hor_in, int16_t vert_in,
                       int16_t *hor_out, int16_t *vert_out);
int16_t vsf_interior(int16_t handle, int16_t style);
int16_t vsf_style(int16_t handle, int16_t style_index);
int16_t vsf_color(int16_t handle, int16_t color_index);
int16_t vsf_perimeter(int16_t handle, int16_t per_vis);
void    vsf_udpat(int16_t handle, int16_t *pfill_pat, int16_t planes);


/****** Raster definitions *********************************************/

typedef struct
{
	void *fd_addr;
	int16_t  fd_w;
	int16_t  fd_h;
	int16_t  fd_wdwidth;
	int16_t  fd_stand;
	int16_t  fd_nplanes;
	int16_t  fd_r1;
	int16_t  fd_r2;
	int16_t  fd_r3;
} MFDB;

void    vro_cpyfm(int16_t handle, int16_t vr_mode, int16_t *pxyarray,
                   MFDB *psrcMFDB, MFDB *pdesMFDB);
void    vrt_cpyfm(int16_t handle, int16_t vr_mode, int16_t *pxyarray,
                   MFDB *psrcMFDB, MFDB *pdesMFDB,
                   int16_t *color_index);
void    vr_trnfm(int16_t handle, MFDB *psrcMFDB, MFDB *pdesMFDB);
void    v_get_pixel(int16_t handle, int16_t x, int16_t y, int16_t *pel,
                     int16_t *index);


/****** Input definitions **********************************************/

void    vsin_mode(int16_t handle, int16_t dev_type, int16_t mode);
void    vrq_locator(int16_t handle, int16_t x, int16_t y, int16_t *xout,
                     int16_t *yout, int16_t *term);
int16_t vsm_locator(int16_t handle, int16_t x, int16_t y, int16_t *xout,
                     int16_t *yout, int16_t *term);
void    vrq_valuator(int16_t handle, int16_t valuator_in,
                      int16_t *valuator_out, int16_t *terminator);
void    vsm_valuator(int16_t handle, int16_t val_in, int16_t *val_out,
                      int16_t *term, int16_t *status);
void    vrq_choice(int16_t handle, int16_t ch_in, int16_t *ch_out);
int16_t vsm_choice(int16_t handle, int16_t *choice);
void    vrq_string(int16_t handle, int16_t max_length, int16_t echo_mode,
                    int16_t *echo_xy, char *string);
int16_t vsm_string(int16_t handle, int16_t max_length, int16_t echo_mode,
                    int16_t *echo_xy, char *string);
void    vsc_form(int16_t handle, int16_t *pcur_form);
void    vex_timv(int16_t handle, int16_t (*tim_addr)(), int16_t (**otim_addr)(),
                 int16_t *tim_conv);
void    v_show_c(int16_t handle, int16_t reset);
void    v_hide_c(int16_t handle);
void    vq_mouse(int16_t handle, int16_t *pstatus, int16_t *x, int16_t *y);
void    vex_butv(int16_t handle, int16_t (*pusrcode)(), int16_t (**psavcode)());
void    vex_motv(int16_t handle, int16_t (*pusrcode)(), int16_t (**psavcode)());
void    vex_curv(int16_t handle, int16_t (*pusrcode)(), int16_t (**psavcode)());
void    vq_key_s(int16_t handle, int16_t *pstatus);


/****** Inquire definitions *******************************************/

void    vq_extnd(int16_t handle, int16_t owflag, int16_t *work_out);
int16_t vq_color(int16_t handle, int16_t color_index,
                  int16_t set_flag, int16_t *rgb);
void    vql_attributes(int16_t handle, int16_t *attrib);
void    vqm_attributes(int16_t handle, int16_t *attrib);
void    vqf_attributes(int16_t handle, int16_t *attrib);
void    vqt_attributes(int16_t handle, int16_t *attrib);
void    vqt_extent(int16_t handle, char *string, int16_t *extent);
int16_t vqt_width(int16_t handle, int16_t character,
                   int16_t *cell_width, int16_t *left_delta,
                   int16_t *right_delta);
int16_t vqt_name(int16_t handle, int16_t element_num, char *name);
void    vq_cellarray(int16_t handle, int16_t *pxyarray,
                      int16_t row_length, int16_t num_rows,
                      int16_t *el_used, int16_t *rows_used,
                      int16_t *status, int16_t *colarray);
void    vqin_mode(int16_t handle, int16_t dew_type, int16_t *input_mode);
void    vqt_fontinfo(int16_t handle, int16_t *minADE, int16_t *maxADE,
                      int16_t *distances, int16_t *maxwidth,
                      int16_t *effects);


/****** Escape definitions *********************************************/

void    vq_chcells(int16_t handle, int16_t *rows, int16_t *columns);
void    v_exit_cur(int16_t handle);
void    v_enter_cur(int16_t handle);
void    v_curup(int16_t handle);
void    v_curdown(int16_t handle);
void    v_curright(int16_t handle);
void    v_curleft(int16_t handle);
void    v_curhome(int16_t handle);
void    v_eeos(int16_t handle);
void    v_eeol(int16_t handle);
void    vs_curaddress(int16_t handle, int16_t row, int16_t column);
void    v_curaddress(int16_t handle, int16_t row, int16_t column);
void    v_curtext(int16_t handle, char *string);
void    v_rvon(int16_t handle);
void    v_rvoff(int16_t handle);
void    vq_curaddress(int16_t handle, int16_t *row, int16_t *column);
int16_t vq_tabstatus(int16_t handle);
void    v_hardcopy(int16_t handle);
void    v_dspcur(int16_t handle, int16_t x, int16_t y);
void    v_rmcur(int16_t handle);
void    v_form_adv(int16_t handle);
void    v_output_window(int16_t handle, int16_t *xyarray);
void    v_clear_disp_list(int16_t handle);
void    v_bit_image(int16_t handle, const char *filename,
                     int16_t aspect, int16_t x_scale, int16_t y_scale,
                     int16_t h_align, int16_t v_align, int16_t *xyarray);
void    vq_scan(int16_t handle, int16_t *g_slice, int16_t *g_page,
                 int16_t *a_slice, int16_t *a_page, int16_t *div_fac);
void    v_alpha_text(int16_t handle, char *string);
void    vs_palette(int16_t handle, int16_t palette);
void	v_sound(int16_t handle, int16_t frequency, int16_t duration);
int16_t vs_mute(int16_t handle, int16_t action);
void    vqp_films(int16_t handle, char *film_names);
void    vqp_state(int16_t handle, int16_t *port, char *film_name,
                   int16_t *lightness, int16_t *int16_terlace,
                   int16_t *planes, int16_t *indexes);
void    vsp_state(int16_t handle, int16_t port, int16_t film_num,
                   int16_t lightness, int16_t int16_terlace, int16_t planes,
                   int16_t *indexes);
void    vsp_save(int16_t handle);
void    vsp_message(int16_t handle);
int16_t vqp_error(int16_t handle);
void    v_meta_extents(int16_t handle, int16_t min_x, int16_t min_y,
                        int16_t max_x, int16_t max_y);
void    v_write_meta(int16_t handle,
                      int16_t num_intin, int16_t *intin,
                      int16_t num_ptsin, int16_t *ptsin);
void    vm_coords(int16_t handle, int16_t llx, int16_t lly, int16_t urx, int16_t ury);
void    vm_filename(int16_t handle, const char *filename);
void    vm_pagesize(int16_t handle, int16_t pgwidth, int16_t pdheight);
void    v_offset(int16_t handle, int16_t offset);
void    v_fontinit(int16_t handle, int16_t fh_high, int16_t fh_low);
void    v_escape2000(int16_t handle, int16_t times);

void    vt_resolution(int16_t handle, int16_t xres, int16_t yres,
                       int16_t *xset, int16_t *yset);
void    vt_axis(int16_t handle, int16_t xres, int16_t yres,
                 int16_t *xset, int16_t *yset);
void    vt_origin(int16_t handle, int16_t xorigin, int16_t yorigin);
void    vq_tdimensions(int16_t handle, int16_t *xdimension, int16_t *ydimension);
void    vt_alignment(int16_t handle, int16_t dx, int16_t dy);
void    vsp_film(int16_t handle, int16_t index, int16_t lightness);
void    vsc_expose(int16_t handle, int16_t state);


#define GDOS_NONE      -2L            /* no GDOS installed           */
#define GDOS_FSM       0x5F46534DL    /* '_FSM' - FSMGDOS installed  */
#define GDOS_FNT       0x5F464E54L    /* '_FNT' - FONTGDOS installed */

int16_t vq_gdos(void);
int32_t vq_vgdos(void);

int16_t v_bez_on(int16_t handle);
void    v_bez_off(int16_t handle);
void    v_set_app_buff(int16_t handle, void *address, int16_t nparagraphs);
void    v_bez(int16_t handle, int16_t count, int16_t *xyarr,
                char *bezarr, int16_t *extent, int16_t *totpts, int16_t *totmoves);
void    v_bez_fill(int16_t handle, int16_t count, int16_t *xyarr,
                     char *bezarr, int16_t *extent, int16_t *totpts,
                     int16_t *totmoves);
int16_t v_bez_qual(int16_t handle, int16_t prcnt, int16_t *actual);


/****** SpeedoGDOS definitions ********************************************/

typedef int32_t  fix31;

void    vqt_f_extent(int16_t handle, char *string, int16_t *extent);
void    v_ftext(int16_t handle, int16_t x, int16_t y, char *string);
void    v_ftext_offset(int16_t handle, int16_t x, int16_t y, char *string, int16_t *offset);
void    v_killoutline(int16_t handle, void *component);
void    v_getoutline(int16_t handle, int16_t ch, int16_t *xyarray,
                      char *bezarray, int16_t maxverts, int16_t *numverts);
void    vst_scratch(int16_t handle, int16_t mode);
void    vst_error(int16_t handle, int16_t mode, int16_t *errorvar);
void    vqt_advance(int16_t handle, int16_t ch, int16_t *advx, int16_t *advy,
                     int16_t *remx, int16_t *remy);
void    vqt_advance32(int16_t handle, int16_t ch, fix31 *advx, fix31 *advy);
int16_t vst_arbpt(int16_t handle, int16_t point16_t, int16_t *chwd, int16_t *chht,
                   int16_t *cellwd, int16_t *cellht);
fix31   vst_arbpt32(int16_t handle, fix31 point16_t, int16_t *chwd, int16_t *chht,
                     int16_t *cellwd, int16_t *cellht);
void    vst_charmap(int16_t handle, int16_t mode);
void    v_getbitmap_info(int16_t handle, int16_t ch, fix31 *advx, fix31 *advy,
                          fix31 *xoff, fix31 *yoff, fix31 *width,
                          fix31 *height);
void    vqt_pairkern(int16_t handle, int16_t ch1, int16_t ch2, fix31 *x, fix31 *y);
void    vqt_trackkern(int16_t handle, fix31 *x, fix31 *y);
void    vqt_fontheader(int16_t handle, char *buffer, char *pathname);
void    vst_kern(int16_t handle, int16_t tmode, int16_t pmode, int16_t *tracks,
                  int16_t *pairs);
fix31   vst_setsize32(int16_t handle, fix31 point16_t, int16_t *chwd, int16_t *chht,
                       int16_t *cellwd, int16_t *cellht);
void    vqt_devinfo(int16_t handle, int16_t devnum, int16_t *devexits,
                     char *devstr);
int16_t v_flushcache(int16_t handle);
void    vqt_cachesize(int16_t handle, int16_t which_cache, int32_t *size);
void    vqt_get_table(int16_t handle, int16_t **map);
int16_t v_loadcache(int16_t handle, char *filename, int16_t mode);
int16_t v_savecache(int16_t handle, char *filename);
int16_t vst_setsize(int16_t handle, int16_t point16_t, int16_t *chwd, int16_t *chht,
                     int16_t *cellwd, int16_t *cellht);
int16_t vst_skew(int16_t handle, int16_t skew);


#endif

/***********************************************************************/
