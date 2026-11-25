#define G_EDIT 37

typedef void XEDITINFO;
extern XEDITINFO *mt_edit_create(int16_t *global);
extern int16_t mt_edit_open(OBJECT *tree, int16_t obj, int16_t *global);
extern void mt_edit_close(OBJECT *tree, int16_t obj, int16_t *global);
extern void mt_edit_delete(XEDITINFO *xi, int16_t *global);
extern int16_t mt_edit_cursor(OBJECT *tree, int16_t obj, int16_t whdl, int16_t show, int16_t *global);
extern int16_t mt_edit_evnt(OBJECT *tree, int16_t obj, int16_t whdl, EVNT *ev, int32_t *errc, int16_t *global);
extern int16_t mt_edit_get_buf(OBJECT *tree, int16_t obj, char **buf, int32_t *buflen, int32_t *txtlen, int16_t *global);
extern int16_t mt_edit_get_format(OBJECT *tree, int16_t obj, int16_t *tabwidth, int16_t *autowrap, int16_t *global);
extern int16_t mt_edit_get_colour(OBJECT *tree, int16_t obj, int16_t *tcolour, int16_t *bcolour, int16_t *global);
extern int16_t mt_edit_get_cursor(OBJECT *tree, int16_t obj, char **cursorpos, int16_t *global);
extern int16_t mt_edit_get_font(OBJECT *tree, int16_t obj, int16_t *fontID, int16_t *fontH, int16_t *fontPix, int16_t *mono, int16_t *global);
extern void mt_edit_set_buf(OBJECT *tree, int16_t obj, char *buf, int32_t buflen, int16_t *global);
extern void mt_edit_set_format(OBJECT *tree, int16_t obj, int16_t tabwidth, int16_t autowrap, int16_t *global);
extern void mt_edit_set_font(OBJECT *tree, int16_t obj, int16_t fontID, int16_t fontH, int16_t fontPix, int16_t mono, int16_t *global);
extern void mt_edit_set_colour(OBJECT *tree, int16_t obj, int16_t tcolour, int16_t bcolour, int16_t *global);
extern void mt_edit_set_cursor(OBJECT *tree, int16_t obj, char *cursorpos, int16_t *global);
extern int16_t mt_edit_resized(OBJECT *tree, int16_t obj, int16_t *oldrh, int16_t *newrh, int16_t *global);
extern int16_t mt_edit_get_dirty(OBJECT *tree, int16_t obj, int16_t *global);
extern void mt_edit_set_dirty(OBJECT *tree, int16_t obj, int16_t dirty, int16_t *global);
extern void mt_edit_get_sel(OBJECT *tree, int16_t obj, char **bsel, char **esel, int16_t *global);
extern void mt_edit_get_pos(OBJECT *tree, int16_t obj, int16_t *xscroll, int32_t *yscroll, char **cyscroll, char **cursorpos, int16_t *cx, int16_t *cy, int16_t *global);
extern void mt_edit_set_pos(OBJECT *tree, int16_t obj, int16_t xscroll, int32_t yscroll, char *cyscroll, char *cursorpos, int16_t cx, int16_t cy, int16_t *global);
extern void mt_edit_get_scrollinfo(OBJECT *tree, int16_t obj, int32_t *nlines, int32_t *yscroll, int16_t *yvis, int16_t *yval, int16_t *ncols, int16_t *xscroll, int16_t *xvis, int16_t *global);
extern int16_t mt_edit_scroll(OBJECT *tree, int16_t obj, int16_t whdl, int32_t yscroll, int16_t xscroll, int16_t *global);

#define edit_create() mt_edit_create(NULL)
#define edit_open(a,b) mt_edit_open(a,b,NULL)
#define edit_close(a,b) mt_edit_close(a,b,NULL)
#define edit_delete(a) mt_edit_delete(a,NULL)
#define edit_cursor(a,b,c,d) mt_edit_cursor(a,b,c,d,NULL)
#define edit_evnt(a,b,c,d,e) mt_edit_evnt(a,b,c,d,e,NULL)
#define edit_get_buf(a,b,c,d,e) mt_edit_get_buf(a,b,c,d,e,NULL)
#define edit_set_buf(a,b,c,d) mt_edit_set_buf(a,b,c,d,NULL)
#define edit_set_format(a,b,c,d) mt_edit_set_format(a,b,c,d,NULL)
#define edit_get_format(a,b,c,d) mt_edit_get_format(a,b,c,d,NULL)
#define edit_set_font(a,b,c,d,e,f) mt_edit_set_font(a,b,c,d,e,f,NULL)
#define edit_get_font(a,b,c,d,e,f) mt_edit_get_font(a,b,c,d,e,f,NULL)
#define edit_set_colour(a,b,c,d) mt_edit_set_colour(a,b,c,d,NULL)
#define edit_resized(a,b,c,d) mt_edit_resized(a,b,c,d,NULL)
#define edit_get_dirty(a,b) mt_edit_get_dirty(a,b,NULL)
#define edit_set_dirty(a,b,c) mt_edit_set_dirty(a,b,c,NULL)
#define edit_get_sel(a,b,c,d) mt_edit_get_sel(a,b,c,d,NULL)
#define edit_get_pos(a,b,c,d,e,f,g,h) mt_edit_get_pos(a,b,c,d,e,f,g,h,NULL)
#define edit_set_pos(a,b,c,d,e,f,g,h) mt_edit_set_pos(a,b,c,d,e,f,g,h,NULL)
#define edit_get_scrollinfo(a,b,c,d,e,f,g,h,i) mt_edit_get_scrollinfo(a,b,c,d,e,f,g,h,i,NULL)
#define edit_scroll(a,b,c,d,e) mt_edit_scroll(a,b,c,d,e,NULL)
