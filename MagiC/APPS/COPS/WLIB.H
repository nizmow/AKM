/*
 * COPS (c) 1995 - 2003 Sven & Wilfried Behne
 *                 2004 F.Naumann & O.Skancke
 *
 * A XCONTROL compatible CPX manager.
 *
 * This file is part of COPS.
 *
 * COPS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * COPS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _wlib_h
#define _wlib_h

#include "GLOBAL.H"
#include "WSTRUCT.H"

int16_t	init_wlib(int16_t id);
int16_t	reset_wlib(void);
WINDOW *get_window_list(void);
WINDOW *search_struct(int16_t handle);
WINDOW *create_window(int16_t kind, GRECT *border, int16_t *handle, char *name, char *iconified_name);
void	set_slpos(WINDOW *window);
void	set_slsize(WINDOW *window);
void	redraw_window(int16_t handle, GRECT *area);
void	delete_window(int16_t handle, int16_t delete);
void	move_window(int16_t handle, GRECT *area);
void	arr_window(int16_t handle, int16_t command);
void	up_window(WINDOW *window, int32_t dy);
void	dn_window(WINDOW *window, int32_t dy);
void	lf_window(WINDOW *window, int32_t dx);
void	rt_window(WINDOW *window, int32_t dx);
void	hlsid_window(int16_t handle, int16_t hslid);
void	vslid_window(int16_t handle, int16_t vslid);
void	size_window(int16_t handle, GRECT *size);
void	full_window(int16_t handle, int16_t max_width, int16_t max_height);
void	iconify_window(int16_t handle, GRECT *size);
void	uniconify_window(int16_t handle, GRECT *size);
void	switch_window(void);

#define	uppage_window(w) up_window(w, w->workarea.g_h)
#define	dnpage_window(w) dn_window(w, w->workarea.g_h)
#define	upline_window(w) up_window(w, w->dy)
#define	dnline_window(w) dn_window(w, w->dy)
#define	lfpage_window(w) lf_window(w, w->workarea.g_w)
#define	rtpage_window(w) rt_window(w, w->workarea.g_w)
#define	lfline_window(w) lf_window(w, w->dx)
#define	rtline_window(w) rt_window(w, w->dx)

#endif /* _wlib_h */
