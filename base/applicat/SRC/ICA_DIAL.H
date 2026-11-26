extern void ica_dial_init_rsc(void);
extern void ica_dial_set_icon(int iconnr);
extern int ica_get_target_obj(int x, int y, int whdl, OBJECT **tree,
			WORD *objnr, void (**set_icon)(int iconnr, int objnr),
			void (**drawfn)(int objnr));
extern DIALOG *d_ica;
extern WORD cdecl ica_wdlg_cb(struct HNDL_OBJ_args args);
extern OBJECT *adr_ica_dialog;
extern int insert_pgm(struct pgm_file *pgm);
