extern void spc_dial_init_rsc(void);
extern void spc_dial_set_icon(int iconnr);
extern int spc_get_target_obj(int x, int y, int whdl, OBJECT **tree,
			WORD *objnr, void (**set_icon)(int iconnr, int objnr),
			void (**drawfn)(int objnr));
extern OBJECT *adr_spc_dialog;
extern DIALOG *d_spc;
extern WORD cdecl spc_wdlg_cb(struct HNDL_OBJ_args args);

