/*
 * Copyright (C) 2010, 2011 Kostas Petrikas, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name(s) of the author(s) may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "img/images.h"

enum {
	S_UPDATE_T,
	S_REMOVE_T
};

struct xpkg_fi_log_str {
	GtkTreeIter n_iter,
	            f_iter,
	            s_iter,
	            b_iter;
};

struct xpkg_archive_info_str {
	GtkWidget *parient;
	
	gchar *full_name,
	      *name,
	      *version,
	      *path;
	      
	gchar **deps,
	      **conflicts;
	      
	gboolean f_install;
	      
	size_t size;
};
typedef struct xpkg_archive_info_str *xpkg_archive_info;

struct xpkg_search_flag_str{
	gboolean name_t_f,
	         descr_t_f;
};
typedef struct xpkg_search_flag_str xpkg_search_flags;

/*Authentication dialog*/
struct xpkg_gui_pass_dlg_str {
	GtkWidget *body,
	          *input;
};

/*Confirm removal dialog*/
struct xpkg_gui_confirm_remove_dlg_str{
	GtkWidget *body,
	          *ch_box_rleafs;
};

/*Update manager dialog*/
struct xpkg_gui_selection_dlg_str{
	GtkWidget *body,
	          *ch_box,
	          *list;
};

/*Jobs dialog*/
struct xpkg_gui_install_fetch_dlg_str{
	GtkWidget *body,
	          *p_label,
	          *p_bar,
	          *log_list,
	          *problem_list,
	          *job_list,
	          *image,
	          *start_button,
	          *pause_button;
	pthread_t p_t;
	pthread_mutex_t p_m;
	struct xpkg_fi_log_str cur_fi_log_e;
};

/*Search dialog*/
struct xpkg_gui_search_dlg_str {
	GtkWidget *body,  *p_bar,
	          *input, *list,
	          *name_toggle,
	          *descr_toggle,
	          *clear_button;
	pthread_t p_t;
};

/*Splash screen*/
struct xpkg_gui_splash_str {
	GtkWidget *body,
		      *stats_lbl;
};

/*Main window*/
struct xpkg_gui_main_win_str {
	GtkWidget *body,        *pkglist,
	          *a_pkglist,   *indi_lbl,
	          *notebook,    *name_lbl,
			  *size_lbl,    *category_lbl,
			  *install_lbl, *install_lbl_s,
			  *version_lbl, *description_view,
			  *pkgdeplist,  *pkgreqbylist,
			  *pkgfilelist, *statusbar,
			  *notebook_s,  *p_bar,
			  *atoolbar,    *atoolbar2,
			  *vbox;
};

/*Main GUI structure*/
struct xpkg_gui_str {
	struct xpkg_gui_pass_dlg_str           pass_dlg;
	struct xpkg_gui_main_win_str           main_window;
	struct xpkg_gui_splash_str             splash_window;
	struct xpkg_gui_search_dlg_str         search_dlg;
	struct xpkg_gui_confirm_remove_dlg_str confirm_remove_dlg;
	struct xpkg_gui_install_fetch_dlg_str  install_fetch_dlg;
	struct xpkg_gui_selection_dlg_str     selection_dlg;
};

/*main_window.c*/
int xpkg_create_main_window(void);
void xpkg_update_pkglist(void);
void xpkg_update_available_pkglist(void);
gchar *portb2origin(gchar *);
void xpkg_print_to_status_bar(gchar *);

/*dialogs.c*/
void *xpkg_init_jobs(void *);
const char * xpkg_pass_promt_dialog(void);
void xpkg_error_dialog(const char *);
void xpkg_show_splashscreen(void);
void xpkg_show_about_dialog(void);
void xpkg_show_search_dialog(void);
void xpkg_clear_search_dialog(gboolean);
void xpkg_set_search_flags(xpkg_search_flags *);
const gchar *xpkg_get_search_dialog_input(void);
void xpkg_set_search_progress_fraction(gdouble);
void xpkg_append_search_result(Pkg, gchar *);
void xpkg_show_delete_confirm_dialog(void);
void xpkg_show_open_pkg_dialog(xpkg_archive_info);
void xpkg_show_install_pkg_dialog(xpkg_archive_info);
void xpkg_show_selection_dialog(u_short);
void xpkg_fill_update_list(GPtrArray *);
