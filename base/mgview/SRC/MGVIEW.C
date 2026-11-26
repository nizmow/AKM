/*******************************************************************
*
*             MGVIEW.C                             7.7.96
*             ========
*                                 letzte Änderung:
*
* geschrieben mit Pure C V1.1
* Projektdatei: MGVIEW.PRJ
*
* Modul zum Anzeigen von ASCII-Dateien.
*
* Parameterübergabe über:
*	Kommandozeile
*	ARGV
*	VA_START
*
****************************************************************/

#include <gem.h>
#include <gemx.h>
#include <mint/osbind.h>
#include <string.h>
#include <stdlib.h>
#include "MGVIEW_GEM.H"
#include "MGVIEW.H"
#include "GLOBALS.H"
#include "WINDOWS.H"
#include "MGWIND.H"
#include "TOS_ARGS.H"

#define AV_STARTED 0x4738

int ap_id;
short text_attrib[10];
short work_in[12];
short work_out[57];
short vdi_handle;
GRECT scrg;
int nwindows = 0;

static void open_work(void)
{
	int i;
	for (i = 0; i < 10; i++)
		work_in[i] = 1;
	work_in[10] = 2;
	v_opnvwk(work_in, &vdi_handle, work_out);
}

int main(int argc, char *argv[])
{
	int i;
	short vdi_dummy;
	WINDOW *w;
	EVMULT_IN event_in;
	EVMULT_OUT event_out;
	short message[8];
	short gl_hhbox, gl_hwbox, gl_hhchar, gl_hwchar;
	char path[256];
	struct tos_args raw_args;

	argc = tos_arguments(argc, &argv, &raw_args);
	if ((ap_id = appl_init()) < 0)
		Pterm(-1);
	graf_mouse(ARROW, NULL);
	/* handle 0 is the AES desktop, whose work area is the full screen */
	wind_get_grect(0, WF_WORKXYWH, &scrg);
	vdi_handle = (short)graf_handle(&gl_hwchar, &gl_hhchar, &gl_hwbox, &gl_hhbox);
	open_work();
	vsf_color(vdi_handle, G_WHITE);
	vst_alignment(vdi_handle, 0, 5, &vdi_dummy, &vdi_dummy);
	vqt_attributes(vdi_handle, text_attrib);
	for (i = 1, argv++; i < argc; i++, argv++)
		open_textwind(*argv);

	event_in.emi_flags = MU_KEYBD | MU_MESAG;
	event_in.emi_bclicks = 2;
	event_in.emi_bmask = 1;
	event_in.emi_bstate = 1;
	while (nwindows) {
		evnt_multi_fast(&event_in, message, &event_out);
		if (event_out.emo_events & MU_KEYBD) {
			w = whdl2window(top_whdl());
			if (event_out.emo_kreturn == 0x1011)
				break;
			if (event_out.emo_kreturn == 0x1117 && w) {
				int wnr = (int)(find_slot_window(w) - windows);
				i = wnr;
				do {
					i = (i + 1) % NWINDOWS;
					if (windows[i]) {
						wind_set(windows[i]->handle, WF_TOP, 0, 0, 0, 0);
						break;
					}
				} while (i != wnr);
			} else if (w)
				w->key(w, event_out.emo_kmeta, event_out.emo_kreturn);
		}
		if (event_out.emo_events & MU_MESAG) {
			if (((message[0] >= 20 && message[0] < 40) || message[0] >= 1040) &&
			    (w = whdl2window(message[3])) != NULL)
				w->message(w, event_out.emo_kmeta, message);
			switch (message[0]) {
			case AP_TERM:
			case PA_EXIT:
				goto ende;
			case AP_DRAGDROP: {
				char pipe[] = "U:\\PIPE\\DRAGDROP.AA";
				char nak = DD_NAK;
				memcpy(pipe + 17, &message[7], 2);
				i = (int)Fopen(pipe, 1); /* GEMDOS mode 1: write-only */
				if (i >= 0) {
					Fwrite(i, 1L, &nak);
					Fclose(i);
				}
				break;
			}
			case VA_START: {
				char *s;
				memcpy(&s, &message[3], sizeof(s));
				while (s && *s) {
					char *p = path;
					char *p2 = p + 255;
					while (*s == ' ')
						s++;
					if (*s == '\'') {
						s++;
						while (*s && p < p2) {
							if (*s == '\'') {
								if (s[1] != '\'')
									break;
								s++;
							}
							*p++ = *s++;
						}
						if (*s)
							s++;
					} else {
						while (*s && *s != ' ' && p < p2)
							*p++ = *s++;
					}
					*p = '\0';
					if (*s)
						s++;
					if (path[0])
						open_textwind(path);
				}
				i = message[1];
				message[0] = AV_STARTED;
				message[1] = (short)ap_id;
				message[2] = 0;
				message[5] = message[6] = message[7] = 0;
				appl_write(i, 16, message);
				break;
			}
			}
		}
	}
ende:
	v_clsvwk(vdi_handle);
	appl_exit();
	return 0;
}


