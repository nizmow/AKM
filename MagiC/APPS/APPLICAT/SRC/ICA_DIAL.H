extern void ica_dial_init_rsc(void);
extern void ica_dial_set_icon(int iconnr);
extern int ica_get_target_obj(int x, int y, int whdl, OBJECT **tree,
			int *objnr, void (**set_icon)(int iconnr, int objnr),
			void (**drawfn)(int objnr));
extern DIALOG *d_ica;
extern WORD cdecl ica_wdlg_cb(DIALOG *d, EVNT *ev, WORD exitbutton,
		WORD nclicks, void *data);
extern OBJECT *adr_ica_dialog;
extern int insert_pgm(struct pgm_file *pgm);
