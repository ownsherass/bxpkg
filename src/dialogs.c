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
#include "xpkg.h"

void *xpkg_main_window_refresh_t(void *);
GtkWidget *xpkg_create_install_log_widget(void);
GtkWidget *xpkg_create_problem_log_widget(void);

#define HANDLE_LOOP              \
	if(!XPKG.gtk_loop_f){        \
		XPKG.gtk_loop_f = TRUE;  \
		gdk_threads_enter();     \
		gtk_main();              \
		gdk_threads_leave();     \
		XPKG.gtk_loop_f = FALSE; \
	}
	
static gboolean xpkg_window_key_press_callback
(GtkWidget *widget, GdkEvent  *event, gpointer data){	
	if(((((GdkEventKey*)event) -> state) & GDK_CONTROL_MASK)){
		if((((GdkEventKey*)event) -> keyval) == GDK_KEY_q){
			gtk_widget_destroy(widget);
			
			return TRUE;
		}
	}
	
	return FALSE;
}
	
FInstallHeapBlock fi_heap;
gint problems;

struct xpkg_fi_log_str xpkg_append_job_log(gchar *pname){
	struct xpkg_fi_log_str fi_log_e;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean new_model = FALSE;
	
	memset(&fi_log_e, 0, sizeof(fi_log_e));
	
	model = gtk_tree_view_get_model(
	GTK_TREE_VIEW(XPKG.gui.install_fetch_dlg.log_list));
	
	if(model == NULL){
		new_model = TRUE;
		
		model = (GtkTreeModel *)gtk_tree_store_new(3, 
		GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
	}
	
	gtk_tree_store_append(GTK_TREE_STORE(model), &iter, NULL);
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
	0, fi_heap.pbuf_text, 1, pname, -1);
	
	gtk_tree_store_append(GTK_TREE_STORE(model), &fi_log_e.n_iter, &iter);
	gtk_tree_store_set(GTK_TREE_STORE(model), &fi_log_e.n_iter,
	0, fi_heap.pbuf_net, 1, "Network operations", -1);
	
	gtk_tree_store_append(GTK_TREE_STORE(model), &fi_log_e.f_iter, &iter);
	gtk_tree_store_set(GTK_TREE_STORE(model), &fi_log_e.f_iter,
	0, fi_heap.pbuf_file, 1, "File operations", -1);
	
	gtk_tree_store_append(GTK_TREE_STORE(model), &fi_log_e.s_iter, &iter);
	gtk_tree_store_set(GTK_TREE_STORE(model), &fi_log_e.s_iter,
	0, fi_heap.pbuf_script, 1, "Shell operations", -1);
	
	gtk_tree_store_append(GTK_TREE_STORE(model), &fi_log_e.b_iter, &iter);
	gtk_tree_store_set(GTK_TREE_STORE(model), &fi_log_e.b_iter,
	0, fi_heap.pbuf_bxpkg, 1, "Database operations", -1);
	
	if(new_model){
		gtk_tree_view_set_model(
		GTK_TREE_VIEW(XPKG.gui.install_fetch_dlg.log_list), model);
		
		UNREF_OBJ(model);
	}
	
	return fi_log_e;
}

gchar *xpkg_get_log_last_entry_name(void){
	gchar *ename = NULL;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean rval;
	
	model = gtk_tree_view_get_model(
	GTK_TREE_VIEW(XPKG.gui.install_fetch_dlg.log_list));
	
	if(model == NULL)
		return NULL;

	rval = gtk_tree_model_get_iter_first(model, &iter);
	
	while(rval){
		gtk_tree_model_get(model, &iter, 1, &ename, -1);
		rval = gtk_tree_model_iter_next(model, &iter);
	}
	
	return ename;
}

void xpkg_job_append_log_entry(GtkTreeIter l_iter, gchar *str, gchar *stat){
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	model = gtk_tree_view_get_model(
	GTK_TREE_VIEW(XPKG.gui.install_fetch_dlg.log_list));
	
	if(model == NULL)
		return;
		
	gtk_tree_store_append(GTK_TREE_STORE(model), &iter, &l_iter);
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
	0, fi_heap.pbuf_text, 1, str, 2, stat, -1);
}

void xpkg_job_append_problem_entry(gchar *str){
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeStore *store = NULL;
	
	model = gtk_tree_view_get_model(
	GTK_TREE_VIEW(XPKG.gui.install_fetch_dlg.problem_list));
	
	if(model == NULL){
		store = gtk_tree_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
		
		model = (GtkTreeModel *)store;
	}
		
	gtk_tree_store_append(GTK_TREE_STORE(model), &iter, NULL);
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
	0, fi_heap.pbuf_warning, 1, str, -1);
	
	if(store){
		gtk_tree_view_set_model(
		GTK_TREE_VIEW(XPKG.gui.install_fetch_dlg.problem_list), model);
		
		UNREF_OBJ(store);
	}
}

size_t xpkg_get_file_size(gchar *path){
	struct stat sb;
	gint rv;
	
	rv = stat(path, &sb);
	
	if(rv == -1)
		return 0;
	
	return sb.st_size;
}

void xpkg_fi_fetch_cb(size_t done, size_t total, gdouble fraction, gchar *file){
	pthread_mutex_lock(&(XPKG.gui.install_fetch_dlg.p_m));
	pthread_mutex_unlock(&(XPKG.gui.install_fetch_dlg.p_m));
	
	gdk_threads_enter();	
	gtk_progress_bar_set_fraction(
	GTK_PROGRESS_BAR(XPKG.gui.install_fetch_dlg.p_bar), fraction);
	gdk_threads_leave();
}

void xpkg_fi_general_cb(u_int step, u_int steps, char *d_str, u_short pstep_t, int err_f, u_short stage_t){
	gdouble fraction;
	gchar *str, *category;
	
	pthread_mutex_lock(&(XPKG.gui.install_fetch_dlg.p_m));
	pthread_mutex_unlock(&(XPKG.gui.install_fetch_dlg.p_m));
	
	gdk_threads_enter();
	if(stage_t == STAGE_POST_T){
		fraction = (gdouble)((gdouble)step / (gdouble)steps);
	
		if(fraction > 1)
			fraction = 1;
		
		gtk_progress_bar_set_fraction(
		GTK_PROGRESS_BAR(XPKG.gui.install_fetch_dlg.p_bar), fraction);
	}
	switch(pstep_t){
		case PSTEP_T_FRM:
			category = g_strdup("file remove");
			if(stage_t == STAGE_PRE_T){
				asprintf(&str, "Removing: %s", d_str);
				assert(str != NULL);
				gtk_label_set_text(GTK_LABEL(XPKG.gui.install_fetch_dlg.p_label), str); 
			}
			else if(stage_t == STAGE_POST_T){
				asprintf(&str, "Removed: %s", d_str);
				assert(str != NULL);
				xpkg_job_append_log_entry(XPKG.gui.install_fetch_dlg.cur_fi_log_e.f_iter,
				str, (!err_f)?"OK":"Failed");
			}
			free(str);
		break;
		case PSTEP_T_FCPY:
			category = g_strdup("file extract");
			if(stage_t == STAGE_PRE_T){
				asprintf(&str, "Extracting: %s", d_str);
				assert(str != NULL);
				gtk_label_set_text(GTK_LABEL(XPKG.gui.install_fetch_dlg.p_label), str); 
			}
			else if(stage_t == STAGE_POST_T){
				asprintf(&str, "Extracted: %s", d_str);
				assert(str != NULL);
				xpkg_job_append_log_entry(XPKG.gui.install_fetch_dlg.cur_fi_log_e.f_iter,
				str, (!err_f)?"OK":"Failed");
			}
			free(str);
		break;
		case PSTEP_T_SCRIPT:
			category = g_strdup("script execution");
			if(stage_t == STAGE_PRE_T){
				asprintf(&str, "Executing: %s", d_str);
				assert(str != NULL);
				gtk_label_set_text(GTK_LABEL(XPKG.gui.install_fetch_dlg.p_label), str); 
			}
			else if(stage_t == STAGE_POST_T){
				asprintf(&str, "Executed: %s", d_str);
				assert(str != NULL);
				xpkg_job_append_log_entry(XPKG.gui.install_fetch_dlg.cur_fi_log_e.s_iter,
				str, (!err_f)?"OK":"Failed");
			}
			free(str);
		break;
		case PSTEP_T_PKGDB:
			category = g_strdup("database operation");
			if(stage_t == STAGE_PRE_T){
				gtk_label_set_text(GTK_LABEL(XPKG.gui.install_fetch_dlg.p_label), d_str);
			}
			else if(stage_t == STAGE_POST_T)
				xpkg_job_append_log_entry(XPKG.gui.install_fetch_dlg.cur_fi_log_e.b_iter,
				d_str, (!err_f)?"OK":"Failed");
		break;
	}
	
	if(stage_t == STAGE_POST_T && err_f){
		gchar *problem_str;
		
		problem_str = g_strdup_printf("(%s->%s): %s", 
		xpkg_get_log_last_entry_name(), category, d_str);
		xpkg_job_append_problem_entry(problem_str);
		
		g_free(problem_str);
	}
	
	g_free(category);
	gdk_threads_leave();
	
	if(err_f != 0)
		problems++;
}

int xpkg_save_reqby_list(gchar *pname){
	gchar *path,
	      *temp_path,
	      **pkgs,
	      *script;
	int rv;
	
	path = g_strdup_printf("%s%s/%s/%s", 
	(XPKG.config->pkg_base[0] != '/')?XPKG.config->pkg_base:"", 
	X_PKG_DB_PREFIX, pname, C_F_REQBY);
	
	pkgs = xpkg_pkg_n_split(pname);	
	temp_path = g_strdup_printf("%s/%s%s", TMP_D, pkgs[I_PKG_NAME], C_F_REQBY);	
	xpkg_pkg_s_free(pkgs);
	
	rv = access(path, F_OK);
	
	if(rv == 0){
		script = g_strdup_printf("mv -f %s %s 2>/dev/null", path, temp_path);
		rv = pkg_exec_script(script);
	}
	
	g_free(path);g_free(temp_path);
	
	return rv;
}

int xpkg_restore_reqby_list(gchar *pname){
	gchar *path,
	      *temp_path,
	      **pkgs,
	      *script;
	int rv;
	
	path = g_strdup_printf("%s%s/%s/%s", 
	(XPKG.config->pkg_base[0] != '/')?XPKG.config->pkg_base:"", 
	X_PKG_DB_PREFIX, pname, C_F_REQBY);
	
	pkgs = xpkg_pkg_n_split(pname);	
	temp_path = g_strdup_printf("%s/%s%s", TMP_D, pkgs[I_PKG_NAME], C_F_REQBY);	
	xpkg_pkg_s_free(pkgs);
	
	rv = access(temp_path, F_OK);
	
	if(rv == 0){
		script = g_strdup_printf("mv -f %s %s 2>/dev/null", temp_path, path);
		rv = pkg_exec_script(script);
	}
	
	g_free(path);g_free(temp_path);
	
	return rv;
}

void xpkg_handle_job(xpkgJob job){
	gchar *origin, *url, *path, *str;
	PkgIdxEntry pie;
	gint rv, rv2;
	gboolean reset = FALSE;
	
	gdk_threads_enter();
	gtk_progress_bar_set_fraction(
	GTK_PROGRESS_BAR(XPKG.gui.install_fetch_dlg.p_bar), 0);
	gtk_image_set_from_stock(GTK_IMAGE(XPKG.gui.install_fetch_dlg.image),
	GTK_STOCK_EXECUTE, GTK_ICON_SIZE_DIALOG);
	gtk_label_set_text(GTK_LABEL(XPKG.gui.install_fetch_dlg.p_label), 
	"Checking package..."); 
	
	XPKG.gui.install_fetch_dlg.cur_fi_log_e = xpkg_append_job_log(job.name);
	gdk_threads_leave();
	
	if(job.type == J_CH_VERSION || job.type == J_GET){
		if(xpkg_get_pie(job.name, &pie) == FALSE){
			gdk_threads_enter();
			xpkg_error("Failed to get index entry for %s", job.name);
			gdk_threads_leave();
		
			pthread_exit(NULL);
		}
	
		rv = xpkg_ch_path(DIST_D);
	
		if(rv == -1){
			gdk_threads_enter();
			xpkg_error("Failed to access/create %s", DIST_D);
			gdk_threads_leave();
		
			pthread_exit(NULL);
		}
	
		asprintf(&path, "%s/%s.tbz", DIST_D, job.name);
		fi_heap.origin = origin = portb2origin(pie.port_base);
		fi_heap.url = url = xpkg_pkgn_to_url(job.name, origin);
	
		rv2 = access(path, R_OK);
	
		if(rv2 != -1)
			if(xpkg_get_file_size(path) != xpkg_fetch_file_size(url)){
				remove(path);
				reset = TRUE;
			}
	
		if((rv2 == -1 && errno == ENOENT) || reset){		
			asprintf(&str ,"Fetching: %s", url);
	
			gdk_threads_enter();
			gtk_image_set_from_stock(GTK_IMAGE(XPKG.gui.install_fetch_dlg.image),
			GTK_STOCK_NETWORK, GTK_ICON_SIZE_DIALOG);
			gtk_label_set_text(GTK_LABEL(XPKG.gui.install_fetch_dlg.p_label), str); 
			gdk_threads_leave();
	
			rv = xpkg_do_fetch(url, path, xpkg_fi_fetch_cb);
		
			gdk_threads_enter();
			xpkg_job_append_log_entry(XPKG.gui.install_fetch_dlg.cur_fi_log_e.n_iter,
			str, (rv < 1) ? "Failed" : "OK");
			free(str);
			gdk_threads_leave();
		
			if(rv < 1){
				gdk_threads_enter();
				switch(rv){
					case 0:
						xpkg_error("xpkg_handle_install_job->xpkg_do_fetch(): %s", 
						fetchLastErrString);
					break;
					case -1:
						xpkg_error(strerror(errno));
					break;
					case -2:
						xpkg_error("xpkg_handle_install_job->xpkg_do_fetch()"
						           ":NULL attribute");
					break;
					case -3:
						xpkg_error("xpkg_handle_install_job->xpkg_do_fetch():"
						           "Received file smaller than expected");
					break;
				}
				gdk_threads_leave();
		
				pthread_exit(NULL);
			}			
		} else if(rv2 == -1 && errno != ENOENT){
			gdk_threads_enter();
			xpkg_error(strerror(errno));
			gdk_threads_leave();
		
			pthread_exit(NULL);
		}
	
		gdk_threads_enter();
		gtk_progress_bar_set_fraction(
		GTK_PROGRESS_BAR(XPKG.gui.install_fetch_dlg.p_bar), 0);
		gdk_threads_leave();
	
		free(fi_heap.origin); fi_heap.origin = NULL;
		free(fi_heap.url); fi_heap.url = NULL;
	}
	
	if(job.type == J_CH_VERSION || job.type == J_DELETE){
		xpkg_cache_pkg_ht pkg_ht;
		
		if(job.type == J_CH_VERSION)
			if(job.data == NULL)
				return;
			
		gdk_threads_enter();
		gtk_image_set_from_stock(GTK_IMAGE(XPKG.gui.install_fetch_dlg.image),
		GTK_STOCK_DELETE, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(GTK_LABEL(XPKG.gui.install_fetch_dlg.p_label), ""); 
		gdk_threads_leave();
		
		pkg_ht = g_hash_table_lookup(XPKG.installed_pkgs.cache_hts.pkg_ht,
		(gconstpointer)((job.type == J_DELETE)?job.name:job.data));
		
		if(pkg_ht == NULL)
			return;
		
		if(job.type == J_CH_VERSION)
			xpkg_save_reqby_list((gchar *)pkg_get_name(pkg_ht -> pkg_s));
			
		pkg_remove(pkg_ht -> pkg_s,
		XPKG.installed_pkgs.db,	xpkg_fi_general_cb);
		
		gdk_threads_enter();
		gtk_progress_bar_set_fraction(
		GTK_PROGRESS_BAR(XPKG.gui.install_fetch_dlg.p_bar), 0);
		gdk_threads_leave();
	}
	
	if(job.type == J_GET || job.type == J_CH_VERSION || job.type == J_INSTALL){
		gdk_threads_enter();
		gtk_image_set_from_stock(GTK_IMAGE(XPKG.gui.install_fetch_dlg.image),
		GTK_STOCK_COPY, GTK_ICON_SIZE_DIALOG);
		gdk_threads_leave();
		
		if(job.type == J_INSTALL)
			path = job.data;
	
		rv = pkg_install_from_archive(path, XPKG.installed_pkgs.db,
		XPKG.installed_pkgs.pkgs, PKG_DIGEST_CH_F, xpkg_fi_general_cb);
	
		if(!rv){
			gdk_threads_enter();
			xpkg_error("Installing '%s' failed!\n%s", job.name, pkg_get_error());
			gdk_threads_leave();
			pthread_exit(NULL);
		}
	}
	
	if(job.type == J_CH_VERSION)
		xpkg_restore_reqby_list(job.name);
	
	free(fi_heap.path); fi_heap.path = NULL;
}

void *xpkg_init_jobs(void *data){
	GtkTreeModel *model;
	GtkTreeIter  iter;
	GtkWidget *list, *icon;
	gint rval;
	GdkPixbuf *pbuf,      *pbuf_ndone,
	          *pbuf_done, *pbuf_cur,
	          *pbuf_bxpkg,*pbuf_net,
	          *pbuf_file, *pbuf_text,
	          *pbuf_script, *pbuf_warning;
	xpkgJob *js, *jobs = data;
	
	problems = 0;
	
	memset(&fi_heap, 0, sizeof(fi_heap));
	fi_heap.jobs = jobs;
	fi_heap.p_m = &(XPKG.gui.install_fetch_dlg.p_m);
	
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &rval);
	pthread_cleanup_push(xpkg_free_fi_heap, &fi_heap);
	
	list = XPKG.gui.install_fetch_dlg.job_list;
	
	gdk_threads_enter();
	model = gtk_tree_view_get_model(
	GTK_TREE_VIEW(list));
	
	if(model == NULL)
		return NULL;
		
	icon = gtk_image_new();
	
	pbuf = gtk_widget_render_icon(icon, GTK_STOCK_YES,
	       GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
	pbuf_done = gdk_pixbuf_scale_simple(pbuf, 
	            20, 20, GDK_INTERP_BILINEAR);
	UNREF_OBJ(pbuf);
	pbuf = gtk_widget_render_icon(icon, GTK_STOCK_NO,
	       GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
	pbuf_ndone = gdk_pixbuf_scale_simple(pbuf, 
	             20, 20, GDK_INTERP_BILINEAR);
	UNREF_OBJ(pbuf);
	pbuf = gtk_widget_render_icon(icon, GTK_STOCK_GO_FORWARD,
	       GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
	pbuf_cur = gdk_pixbuf_scale_simple(pbuf, 
	             20, 20, GDK_INTERP_BILINEAR);
	UNREF_OBJ(pbuf);
	pbuf = gdk_pixbuf_new_from_xpm_data((void *)&m_icon_xpm);
	pbuf_bxpkg = gdk_pixbuf_scale_simple(pbuf, 
	             20, 20, GDK_INTERP_BILINEAR);
	UNREF_OBJ(pbuf);
	pbuf = gtk_widget_render_icon(icon, GTK_STOCK_NETWORK,
	       GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
	pbuf_net = gdk_pixbuf_scale_simple(pbuf, 
	           20, 20, GDK_INTERP_BILINEAR);
	UNREF_OBJ(pbuf);
	pbuf = gtk_widget_render_icon(icon, GTK_STOCK_FILE,
	       GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
	pbuf_file = gdk_pixbuf_scale_simple(pbuf, 
	           20, 20, GDK_INTERP_BILINEAR);
	UNREF_OBJ(pbuf);
	pbuf = gtk_widget_render_icon(icon, GTK_STOCK_EDIT,
	       GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
	pbuf_text = gdk_pixbuf_scale_simple(pbuf, 
	           20, 20, GDK_INTERP_BILINEAR);
	UNREF_OBJ(pbuf);
	pbuf = gtk_widget_render_icon(icon, GTK_STOCK_DIALOG_WARNING,
	       GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
	pbuf_warning = gdk_pixbuf_scale_simple(pbuf, 
	           20, 20, GDK_INTERP_BILINEAR);
	UNREF_OBJ(pbuf);
	pbuf_script = gdk_pixbuf_new_from_xpm_data((void *)&terminal_xpm);
	
	fi_heap.pbuf_done = pbuf_done;
	fi_heap.pbuf_ndone = pbuf_ndone;
	fi_heap.pbuf_cur = pbuf_cur;
	fi_heap.pbuf_bxpkg = pbuf_bxpkg;
	fi_heap.pbuf_net = pbuf_net;
	fi_heap.pbuf_file = pbuf_file;
	fi_heap.pbuf_text = pbuf_text;
	fi_heap.pbuf_script = pbuf_script;
	fi_heap.pbuf_warning = pbuf_warning;
		
	rval = gtk_tree_model_get_iter_first(model, &iter);
	gdk_threads_leave();
	
	gchar *jn;
	
	while(rval){
		gdk_threads_enter();
		gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, pbuf_cur, -1);
		gtk_tree_model_get(model, &iter, 1, &jn, -1);
		gdk_threads_leave();
		
		js = xpkg_select_job_by_name(jn, jobs);
		xpkg_handle_job(js[0]);
		
		gdk_threads_enter();
		gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, pbuf_done, -1);
		
		rval = gtk_tree_model_iter_next(model, &iter);
		gdk_threads_leave();
	}

	gdk_threads_enter();
	gtk_progress_bar_set_fraction(
	GTK_PROGRESS_BAR(XPKG.gui.install_fetch_dlg.p_bar), 1);
	gtk_image_set_from_stock(GTK_IMAGE(XPKG.gui.install_fetch_dlg.image),
	GTK_STOCK_APPLY, GTK_ICON_SIZE_DIALOG);
	
	gchar *ptr;
	ptr = g_strdup_printf("All jobs completed with %d problems!", problems);
	
	gtk_label_set_text(GTK_LABEL(XPKG.gui.install_fetch_dlg.p_label), ptr);
	g_free(ptr);
	
	gtk_widget_set_sensitive(XPKG.gui.install_fetch_dlg.pause_button, FALSE);
	gdk_threads_leave();
	
	pthread_cleanup_pop(0);
	
	return NULL;
}
	
void xpkg_fill_job_list(xpkgJob *jobs){
	u_int a;
	
	GtkListStore *store;
	GtkTreeIter iter;
	GdkPixbuf *pbuf, *pbuf_ndone;
	GtkWidget *icon;
	gchar *str = NULL;
	
	if(jobs == NULL)
		return;
		
	store = gtk_list_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
	
	icon = gtk_image_new();
	
	pbuf = gtk_widget_render_icon(icon, GTK_STOCK_NO,
	       GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	pbuf_ndone = gdk_pixbuf_scale_simple(pbuf, 
	             20, 20, GDK_INTERP_BILINEAR);
	UNREF_OBJ(pbuf);
	
	gtk_widget_destroy(icon);
	
	for(a = 0; jobs[a].name != NULL; a++){
		switch(jobs[a].type){
			case J_GET:
				str = "Install";
			break;
			case J_CH_VERSION:
				str = "Change version";
			break;
			case J_DELETE:
				str = "Remove";
			break;
			case J_INSTALL:
				str = "Install";
			break;
			default:
				str = "Unknown";
			break;
		}
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, pbuf_ndone,	1, jobs[a].name, 2, str, -1);
	}
	
	gtk_tree_view_set_model(
	GTK_TREE_VIEW(XPKG.gui.install_fetch_dlg.job_list), GTK_TREE_MODEL(store));
	
	UNREF_OBJ(pbuf_ndone);
	UNREF_OBJ(store);
}
	
GtkWidget *xpkg_create_install_job_list(void){
	GtkCellRenderer *renderer, *renderer2;
	GtkWidget *view;
	GtkTreeViewColumn *column;
	
	view = gtk_tree_view_new();
	
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "font", "8", NULL);
	renderer2 = gtk_cell_renderer_pixbuf_new();
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer2, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer2, "pixbuf", 0, NULL);

	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", 1, NULL);
	
	gtk_tree_view_column_set_title(column, "Jobs");

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", 2, NULL);
	
	gtk_tree_view_column_set_title(column, "Job types");

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	
	return view;
}

GtkWidget *xpkg_create_job_start_button(void){
	GtkWidget *button,
	          *image,
	          *label,
	          *hbox;
	          
	label = gtk_label_new("Start/Continue");
	image = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON);
	
	hbox = gtk_hbox_new(FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button), hbox);
	
	return button;
}

GtkWidget *xpkg_create_job_pause_button(void){
	GtkWidget *button,
	          *image,
	          *label,
	          *hbox;
	          
	label = gtk_label_new("Pause/Suspend");
	image = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_BUTTON);
	
	hbox = gtk_hbox_new(FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button), hbox);
	
	return button;
}

static void xpkg_jobs_dialog_close_callback
(GtkWidget *widget, gpointer data){
	gtk_widget_destroy(GTK_WIDGET(data));
}

static void xpkg_jobs_dialog_destroy_callback
(GtkWidget *widget, gpointer data){
	GThread *g_t;
	
	if(XPKG.gui.install_fetch_dlg.p_t != NULL)
		pthread_cancel(XPKG.gui.install_fetch_dlg.p_t);
		
	gtk_widget_set_sensitive(XPKG.gui.main_window.body, TRUE);
	
	g_t = g_thread_create(xpkg_main_window_refresh_t, NULL, FALSE, NULL);
	assert(g_t != NULL);
}

static void xpkg_jobs_start_callback
(GtkWidget *widget, gpointer data){
	pthread_t p_t;
	xpkgJob *jobs = data;
	
	gtk_widget_set_sensitive(XPKG.gui.install_fetch_dlg.start_button, FALSE);
	gtk_widget_set_sensitive(XPKG.gui.install_fetch_dlg.pause_button, TRUE);
	
	if(XPKG.gui.install_fetch_dlg.p_t == NULL){
		assert(!pthread_create
		(&p_t, NULL, xpkg_init_jobs, (void *)jobs));
				
		XPKG.gui.install_fetch_dlg.p_t = p_t;
	} else
		pthread_mutex_unlock(&(XPKG.gui.install_fetch_dlg.p_m));
}

static void xpkg_jobs_pause_callback
(GtkWidget *widget, gpointer data){
	gtk_widget_set_sensitive(XPKG.gui.install_fetch_dlg.pause_button, FALSE);
	gtk_widget_set_sensitive(XPKG.gui.install_fetch_dlg.start_button, TRUE);
	
	pthread_mutex_lock(&(XPKG.gui.install_fetch_dlg.p_m));
}

static void xpkg_jobs_dialog_pexpand_callback
(GtkExpander *expander, gpointer data){
	gboolean exp, fill;
	guint padding;
	GtkPackType pack_type;
	GtkWidget *vbox = (void *)data;
	
	exp = fill = FALSE;
	padding = 0;
	
	gtk_box_query_child_packing(GTK_BOX(vbox), GTK_WIDGET(expander),
	&exp, &fill, &padding, &pack_type);
	
	gtk_box_set_child_packing(GTK_BOX(vbox), GTK_WIDGET(expander),
	!exp, !fill, padding, pack_type);
}

void xpkg_show_jobs_dialog(xpkgJob *jobs){
	GtkWidget *window,   *log_view,
	          *label,    *p_bar,
	          *vbox,     *hbox,
	          *button,   *image,
	          *expander, *swindow,
	          *mhbox,    *mvbox,
	          *job_list, *hpannel,
	          *problem_view;
	
	XPKG.pkg_install_fetch_l = TRUE;
	
	XPKG.gui.install_fetch_dlg.body = 
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request(window, 680, 400);
	gtk_window_set_transient_for(GTK_WINDOW(window), 
	GTK_WINDOW(XPKG.gui.main_window.body));
	gtk_window_set_position(GTK_WINDOW(window), 
	GTK_WIN_POS_CENTER_ON_PARENT);
	
	gtk_window_set_title(GTK_WINDOW(window), "Jobs manager");
	
	g_signal_connect(G_OBJECT(window), "destroy",
	G_CALLBACK(xpkg_jobs_dialog_destroy_callback), NULL);
	
	mvbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), mvbox);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mvbox),hbox, FALSE, FALSE, 2);
	
	XPKG.gui.install_fetch_dlg.image = image = gtk_image_new_from_stock(
	GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 2);
	
	XPKG.gui.install_fetch_dlg.p_label = label = gtk_label_new("Idling... Hit Start button.");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	XPKG.gui.install_fetch_dlg.p_bar = p_bar = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(mvbox), p_bar, FALSE, FALSE, 2);
	
	mhbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mvbox), mhbox, TRUE, TRUE, 0);
	
	XPKG.gui.install_fetch_dlg.job_list =
	job_list = xpkg_create_install_job_list();
	
	swindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	gtk_scrolled_window_add_with_viewport(
	GTK_SCROLLED_WINDOW(swindow), job_list);
	
	hpannel = gtk_hpaned_new();	
	gtk_box_pack_start(GTK_BOX(mhbox), hpannel, TRUE, TRUE, 0);
	
	gtk_paned_add1(GTK_PANED(hpannel), swindow);
	gtk_widget_set_size_request(GTK_WIDGET(swindow), 300, 0);
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_paned_add2(GTK_PANED(hpannel), vbox);
	
	expander = gtk_expander_new("Problem log");
	gtk_expander_set_expanded(GTK_EXPANDER(expander), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), expander, FALSE, FALSE, 0);
	
	g_signal_connect(G_OBJECT(expander), "activate", 
	G_CALLBACK(xpkg_jobs_dialog_pexpand_callback), (gpointer)vbox);
	
	swindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(expander), swindow);
	
	XPKG.gui.install_fetch_dlg.problem_list = problem_view = 
	xpkg_create_problem_log_widget();
	gtk_container_add(GTK_CONTAINER(swindow), problem_view);
	
	expander = gtk_expander_new("Full log");
	gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), expander, TRUE, TRUE, 0);
	
	g_signal_connect(G_OBJECT(expander), "activate", 
	G_CALLBACK(xpkg_jobs_dialog_pexpand_callback), (gpointer)vbox);
	
	swindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(expander), swindow);
	
	XPKG.gui.install_fetch_dlg.log_list = log_view = 
	xpkg_create_install_log_widget();
	gtk_container_add(GTK_CONTAINER(swindow), log_view);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(mvbox),hbox, FALSE, FALSE, 2);
	
	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 2);
	
	g_signal_connect(G_OBJECT(button), "clicked", 
	G_CALLBACK(xpkg_jobs_dialog_close_callback), (gpointer)window);
	
	XPKG.gui.install_fetch_dlg.start_button = button = xpkg_create_job_start_button();
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 2);
	
	g_signal_connect(G_OBJECT(button), "clicked", 
	G_CALLBACK(xpkg_jobs_start_callback), (gpointer)jobs);
	
	XPKG.gui.install_fetch_dlg.pause_button = button = xpkg_create_job_pause_button();
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 2);
	gtk_widget_set_sensitive(button, FALSE);
	
	g_signal_connect(G_OBJECT(button), "clicked", 
	G_CALLBACK(xpkg_jobs_pause_callback), NULL);
	
	gtk_widget_set_sensitive(XPKG.gui.main_window.body, FALSE);
	gtk_widget_show_all(window);
	
	gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
	g_signal_connect(G_OBJECT(window), "key-press-event",
	G_CALLBACK(xpkg_window_key_press_callback), NULL);
	
	assert(!pthread_mutex_init(&(XPKG.gui.install_fetch_dlg.p_m), NULL));
	xpkg_fill_job_list(jobs);	
}

GtkWidget *xpkg_create_selection_start_button(void){
	GtkWidget *button,
	          *image,
	          *label,
	          *hbox;
	          
	label = gtk_label_new("Start");
	image = gtk_image_new_from_stock(GTK_STOCK_APPLY, GTK_ICON_SIZE_BUTTON);
	
	hbox = gtk_hbox_new(FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button), hbox);
	
	return button;
}

void xpkg_fill_update_list(GPtrArray *u_array){
	struct xpkg_update *update;
	GdkPixbuf *pbuf_p;
	GtkTreeStore *store;
	GtkTreeIter iter;
	
	pbuf_p = gdk_pixbuf_new_from_xpm_data((void *)&packages_xpm);
	
	store = gtk_tree_store_new(4, G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF, 
	G_TYPE_STRING, G_TYPE_STRING);
	
	for(u_int i = 0; i != u_array -> len; i++){
		update = g_ptr_array_index(u_array, i);
		if(update != NULL){
			gtk_tree_store_append(store, &iter, NULL);
			gtk_tree_store_set(store, &iter, 0, TRUE, 1, pbuf_p, 2, update -> pkg_name, 3, update -> new_ver, -1);
		}
	}
	
	gtk_tree_sortable_set_sort_column_id(
	GTK_TREE_SORTABLE(store), 2, GTK_SORT_ASCENDING);
	
	gtk_tree_view_set_model(
	GTK_TREE_VIEW(XPKG.gui.selection_dlg.list), (GtkTreeModel *)store);
	
	xpkg_free_array(u_array);
	UNREF_OBJ(store);
	UNREF_OBJ(pbuf_p);
}

void xpkg_fill_remove_list(xpkgJob *jobs){
	GdkPixbuf *pbuf_p;
	GtkTreeStore *store;
	GtkTreeIter iter;
	
	pbuf_p = gdk_pixbuf_new_from_xpm_data((void *)&packages_xpm);
	
	store = gtk_tree_store_new(3, G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	
	for(u_int i = 0; jobs[i].name != NULL; i++){
		gtk_tree_store_append(store, &iter, NULL);
		gtk_tree_store_set(store, &iter, 0, TRUE, 1, pbuf_p, 2, jobs[i].name, -1);
	}
	
	gtk_tree_sortable_set_sort_column_id(
	GTK_TREE_SORTABLE(store), 2, GTK_SORT_ASCENDING);
	
	gtk_tree_view_set_model(
	GTK_TREE_VIEW(XPKG.gui.selection_dlg.list), (GtkTreeModel *)store);
	
	xpkg_free_jobs(jobs, TRUE);
	UNREF_OBJ(store);
	UNREF_OBJ(pbuf_p);
}

static void xpkg_update_list_toggled
(GtkCellRendererToggle *cell_renderer, gchar *pathstr, gpointer data){
	GtkTreeIter iter;
	gboolean enabled;
	GtkTreePath *path;
	GtkTreeModel *model;
	
	model = gtk_tree_view_get_model
	(GTK_TREE_VIEW(XPKG.gui.selection_dlg.list));
	
	path = gtk_tree_path_new_from_string(pathstr);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 0, &enabled, -1);
	
	enabled = !enabled;
	
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 0, enabled, -1);
}

GtkWidget *xpkg_create_selection_list(u_short type){
	GtkCellRenderer *renderer, *renderer2, *renderer3;
	GtkWidget *view;
	GtkTreeViewColumn *column;
	
	view = gtk_tree_view_new();
	
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "font", "8", NULL);
	
	renderer2 = gtk_cell_renderer_pixbuf_new();
	
	renderer3 = gtk_cell_renderer_toggle_new();
	g_signal_connect(G_OBJECT(renderer3), "toggled", 
	G_CALLBACK(xpkg_update_list_toggled), NULL);
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer3, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer3, "active", 0, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer2, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer2, "pixbuf", 1, NULL);

	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", 2, NULL);
	
	gtk_tree_view_column_set_title(column, "Packages");

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	
	if(type == S_UPDATE_T){
		column = gtk_tree_view_column_new();
		g_object_set(column, "resizable", TRUE, NULL);
	
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_set_attributes(column, renderer, "text", 3, NULL);
	
		gtk_tree_view_column_set_title(column, "Update to");

		gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	}
	
	return view;
}

static void xpkg_update_all_toggled
(GtkToggleButton *togglebutton, gpointer data){
	GtkTreeModel *model;
	GtkTreeIter  iter;
	GtkWidget *list = XPKG.gui.selection_dlg.list;
	gboolean rval, state;
	
	state = gtk_toggle_button_get_active(togglebutton);	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	
	if(model == NULL)
		return;
		
	rval = gtk_tree_model_get_iter_first(model, &iter);
	
	while(rval){
		gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 0, state, -1);
		rval = gtk_tree_model_iter_next(model, &iter);
	}
}

static void xpkg_selection_dialog_start_callback(
GtkWidget *widget, gpointer data){
	GtkTreeModel *model;
	GtkTreeIter  iter;
	GtkWidget *list = XPKG.gui.selection_dlg.list;
	gboolean rval, state = FALSE;
	gchar *pkg_n = NULL, *new_ver = NULL, **pkg_s, *pkg_u;
	GPtrArray *u_array;
	xpkgJob *jobs = NULL, job;
	u_short type = *(u_short *)data;
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	
	if(model == NULL)
		return;
		
	rval = gtk_tree_model_get_iter_first(model, &iter);
	u_array = g_ptr_array_new_with_free_func(free);
	
	if(type == S_UPDATE_T){
		while(rval){
			gtk_tree_model_get(model, &iter, 0, &state, 2, &pkg_n, 3, &new_ver, -1);
		
			if(state){
				pkg_s = xpkg_pkg_n_split(pkg_n);
			
				asprintf(&pkg_u, "%s-%s", pkg_s[I_PKG_NAME], new_ver);
				assert(pkg_u != NULL);
			
				g_ptr_array_add(u_array, pkg_u);
			
				xpkg_pkg_s_free(pkg_s);
			}
			rval = gtk_tree_model_iter_next(model, &iter);
		}
		
		if(!(u_array -> len)){
			xpkg_error("No packages were selected for upgrading. Nothing to do!");
			g_ptr_array_free(u_array, FALSE);
		} else {		
			jobs = xpkg_get_update_jobs(u_array);
			jobs = xpkg_short_jobs_by_priority(jobs);
		
			xpkg_show_jobs_dialog(jobs);
			gtk_window_set_title(GTK_WINDOW(XPKG.gui.install_fetch_dlg.body), "Updating");
		}
	} else {		
		while(rval){
			gtk_tree_model_get(model, &iter, 0, &state, 2, &pkg_n, -1);
		
			if(state){
				memset(&job, 0, sizeof(job));
				job.name = strdup(pkg_n);
				job.type = J_DELETE;
			
				jobs = xpkg_append_job(jobs, job);
			}
		
			rval = gtk_tree_model_iter_next(model, &iter);
		}
		
		if(!jobs || !(jobs[0].name)){
			xpkg_error("No packages were selected for removing. Nothing to do!");
			g_ptr_array_free(u_array, FALSE);
		} else {		
			xpkg_show_jobs_dialog(jobs);
			gtk_window_set_title(GTK_WINDOW(XPKG.gui.install_fetch_dlg.body), "Removing");
		}
	}	
	
	gtk_widget_destroy(XPKG.gui.selection_dlg.body);
	gtk_widget_set_sensitive(XPKG.gui.main_window.body, FALSE);
}

static void xpkg_selection_dialog_close_callback(
GtkWidget *widget, gpointer data){
	gtk_widget_destroy(GTK_WIDGET(data));
}

static void xpkg_selection_dialog_destroy_callback(
GtkWidget *widget, gpointer data){
	gtk_widget_set_sensitive(XPKG.gui.main_window.body, TRUE);
}

void xpkg_show_selection_dialog(u_short type){
	GtkWidget *window, *button,
	          *list,   *chbox,
	          *hbox,   *swindow,
	          *vbox;
	 
	XPKG.gui.selection_dlg.body =
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request(window, 450, 600);
	gtk_window_set_transient_for(GTK_WINDOW(window), 
	GTK_WINDOW(XPKG.gui.main_window.body));
	gtk_window_set_position(GTK_WINDOW(window), 
	GTK_WIN_POS_CENTER_ON_PARENT);
	
	g_signal_connect(G_OBJECT(window), "destroy",
	G_CALLBACK(xpkg_selection_dialog_destroy_callback), NULL);
	
	if(type == S_UPDATE_T){
		gtk_window_set_title(GTK_WINDOW(window), "Select packages to update");
	} else 
		gtk_window_set_title(GTK_WINDOW(window), "Select packages to remove");
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	
	XPKG.gui.selection_dlg.ch_box =
	chbox = gtk_check_button_new_with_label("All");
	gtk_box_pack_start(GTK_BOX(hbox), chbox, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chbox), TRUE);
	g_signal_connect(G_OBJECT(chbox), "toggled",
	G_CALLBACK(xpkg_update_all_toggled), NULL);
	
	swindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), swindow, TRUE, TRUE, 0);
	
	XPKG.gui.selection_dlg.list = list = xpkg_create_selection_list(type);
	
	gtk_container_add(GTK_CONTAINER(swindow), list);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox),hbox, FALSE, FALSE, 6);
	
	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 4);
	
	g_signal_connect(G_OBJECT(button), "clicked", 
	G_CALLBACK(xpkg_selection_dialog_close_callback), (gpointer)window);
	
	button = xpkg_create_selection_start_button();
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	
	g_signal_connect(G_OBJECT(button), "clicked", 
	G_CALLBACK(xpkg_selection_dialog_start_callback), &type);
	
	gtk_widget_show_all(window);
	
	gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
	g_signal_connect(G_OBJECT(window), "key-press-event",
	G_CALLBACK(xpkg_window_key_press_callback), NULL);
}

static void xpkg_pass_promt_dialog_ok_callback(
GtkWidget *widget, gpointer data){
	const char **str = (void *) data;
	
	str[0] = (void *)strdup((void *)gtk_entry_get_text
	(GTK_ENTRY(XPKG.gui.pass_dlg.input)));
	gtk_widget_destroy(GTK_WIDGET(XPKG.gui.pass_dlg.body));
};

static void xpkg_pass_promt_dialog_cancel_callback(
GtkWidget *widget, gpointer data){
	gtk_widget_destroy(GTK_WIDGET(XPKG.gui.pass_dlg.body));
	
	exit(2);
};

const char * xpkg_pass_promt_dialog(void){
	GtkWidget *dlg,
	          *vbox,
	          *hbox,
			  *hhbox,
	          *input,
			  *ok_button,
			  *cancel_button,
			  *icon;
	GdkPixbuf *pbuf;
	const char *str = NULL;
	
	XPKG.gui.pass_dlg.body = dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);	
	g_signal_connect(G_OBJECT(dlg), "destroy", 
	G_CALLBACK(gtk_main_quit), NULL);
	
	gtk_window_set_title(GTK_WINDOW(dlg), "Authentication");
	gtk_widget_set_size_request(GTK_WIDGET(dlg), 400, 120);
	gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
	gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);
	
	icon = gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION,
	GTK_ICON_SIZE_DIALOG);	
	
	pbuf = gtk_widget_render_icon(icon, GTK_STOCK_DIALOG_AUTHENTICATION, 
	GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	
	gtk_window_set_icon(GTK_WINDOW(dlg), pbuf);
	
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 1);
	gtk_container_add(GTK_CONTAINER(dlg), vbox);
	
	hhbox = gtk_hbox_new(FALSE, 1);
	gtk_box_pack_start(GTK_BOX(vbox), hhbox, FALSE, FALSE, 10);

	gtk_box_pack_start(GTK_BOX(hhbox), icon, FALSE, FALSE, 5);
	
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_box_pack_start(GTK_BOX(hhbox), hbox, TRUE, TRUE, 10);
	
	XPKG.gui.pass_dlg.input = input = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(input), FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), input, TRUE, TRUE, 5);
	
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 20);
	
	ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_box_pack_end(GTK_BOX(hbox), ok_button, FALSE, FALSE, 5);
	
	cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_box_pack_end(GTK_BOX(hbox), cancel_button, FALSE, FALSE, 0);
	
	g_signal_connect (G_OBJECT(input), "activate",
	G_CALLBACK(xpkg_pass_promt_dialog_ok_callback), (gpointer)&str);
	g_signal_connect (G_OBJECT(ok_button), "clicked",
	G_CALLBACK(xpkg_pass_promt_dialog_ok_callback), (gpointer)&str);
	g_signal_connect (G_OBJECT(cancel_button), "clicked",
	G_CALLBACK(xpkg_pass_promt_dialog_cancel_callback), NULL);
	
	gtk_widget_show_all(dlg);
	
	gtk_widget_add_events(dlg, GDK_KEY_PRESS_MASK);
	g_signal_connect(G_OBJECT(dlg), "key-press-event",
	G_CALLBACK(xpkg_window_key_press_callback), NULL);
	
	HANDLE_LOOP
	
	return str;
}

void xpkg_error_dialog(const char *msg){
	GtkWidget *dlg;
	
	dlg = gtk_message_dialog_new(NULL, 0,
	      GTK_MESSAGE_ERROR,
	      GTK_BUTTONS_OK,
	      (gchar *)msg);
		  
	gtk_window_set_title(GTK_WINDOW(dlg), "Error");
	gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
	
	if(!XPKG.gtk_loop_f)
		g_signal_connect(G_OBJECT(dlg), "destroy", 
		G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(dlg), "response", 
	G_CALLBACK(gtk_widget_destroy), NULL);
	
	gtk_widget_show_all(dlg);
	
	gtk_widget_add_events(dlg, GDK_KEY_PRESS_MASK);
	g_signal_connect(G_OBJECT(dlg), "key-press-event",
	G_CALLBACK(xpkg_window_key_press_callback), NULL);
	
	HANDLE_LOOP
}

void *xpkg_splashscreen_t(void *data){
	GtkWidget *label = GTK_WIDGET(data);
	
	gdk_threads_enter();
	gtk_label_set_markup(GTK_LABEL(label),
	"<small><b>Loading packages...</b></small>");
	gdk_threads_leave();
	xpkg_init_pkg_db();
	gdk_threads_enter();
	gtk_label_set_markup(GTK_LABEL(label),
	"<small><b>Loading Package Manager...</b></small>");
	gdk_threads_leave();
	gdk_threads_enter();
	xpkg_create_main_window();
	gdk_threads_leave();
	gdk_threads_enter();
	xpkg_update_pkglist();
	gdk_threads_leave();
	gdk_threads_enter();
	gtk_widget_show_all(XPKG.gui.main_window.body);
	gtk_widget_hide(XPKG.gui.main_window.p_bar);
	gtk_widget_hide(XPKG.gui.main_window.atoolbar2);
	gtk_widget_hide(XPKG.gui.main_window.install_lbl);
	gtk_widget_hide(XPKG.gui.main_window.install_lbl_s);
	gdk_threads_leave();
	gdk_threads_enter();
	gtk_widget_destroy(XPKG.gui.splash_window.body);
	gdk_threads_leave();
	
	return NULL;
}

static void xpkg_splashscreen_g_tread_init(GtkWidget *widget, gpointer data){
	GThread *g_t;
	
	g_t = g_thread_create(xpkg_splashscreen_t, (void *)widget, FALSE, NULL);
	assert(g_t != NULL);
}

void xpkg_show_splashscreen(void){
	GtkWidget *window,
		      *img,
		      *vbox,
		      *hbox,
		      *label;
	GdkPixmap *spixmap;
	GdkBitmap *mask;
	GdkPixbuf *pbuf;
	
	XPKG.gui.splash_window.body = window = 
	gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);	
	gtk_widget_set_size_request(GTK_WIDGET(window), 350, 165);
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	gtk_widget_show(window);
	
	spixmap = gdk_pixmap_create_from_xpm_d(
	window -> window, &mask, NULL, splash_xpm);
	
	img = gtk_image_new_from_pixmap(spixmap, mask);
	gtk_box_pack_start(GTK_BOX(vbox), img, FALSE, FALSE, 0);
	
	UNREF_OBJ(spixmap);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	
	XPKG.gui.splash_window.stats_lbl = label = gtk_label_new(NULL);
	gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	gtk_widget_show_all(window);
	
	pbuf = gdk_pixbuf_new_from_xpm_data((void *)&m_icon_xpm);	
	gtk_window_set_icon(GTK_WINDOW(window), pbuf);
	
	UNREF_OBJ(pbuf);
	
	xpkg_splashscreen_g_tread_init(label, NULL);
	
	HANDLE_LOOP
}

static void xpkg_about_dialog_close_callback(
GtkWidget *widget, gpointer data){
	GtkWidget *window = (GtkWidget *)data;
	
	gtk_widget_destroy(window);
}

void xpkg_show_about_dialog(void){
	GtkWidget *window,
		      *img,
		      *vbox,
		      *hbox,
		      *label,
		      *button;
	GdkPixbuf *pbuf;
	
	gchar *ptr;
	
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "About");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);	
	gtk_widget_set_size_request(GTK_WIDGET(window), 350, 300);
	
	pbuf = gdk_pixbuf_new_from_xpm_data((void *)&m_icon_xpm);	
	gtk_window_set_icon(GTK_WINDOW(window), pbuf);
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	
	img = gtk_image_new_from_pixbuf(pbuf);
	gtk_box_pack_start(GTK_BOX(hbox), img, TRUE, FALSE, 0);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), 
	"<span font='8'><b>X frontend for BSD Package Management System</b></span>");
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new(NULL);
	ptr = g_strdup_printf("<span font='8'><b>Version:</b> <i>%s</i></span>",
	X_PKG_VERSION);
	gtk_label_set_markup(GTK_LABEL(label), ptr);
	g_free(ptr);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 15);
	
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), 
	"<span font='8'><b>Developers</b></span>");
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), 
	"<span font='8'>Kostas Petrikas <i>(Owner)</i></span>");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 10);
	
	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
	
	g_signal_connect(G_OBJECT(button), "clicked", 
	G_CALLBACK(xpkg_about_dialog_close_callback), (gpointer)window);
	
	gtk_widget_show_all(window);
	
	gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
	g_signal_connect(G_OBJECT(window), "key-press-event",
	G_CALLBACK(xpkg_window_key_press_callback), NULL);
}

void xpkg_search_result_list_activated_callback(
GtkTreeView *view, GtkTreePath *path,
GtkTreeViewColumn *col, gpointer data){
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *pkg_n;
	
	model = gtk_tree_view_get_model(view);
	
	if(gtk_tree_model_get_iter(model, &iter, path)){
		gtk_tree_model_get(model, &iter, 1, &pkg_n, -1);
		xpkg_search_pkg_list(pkg_n);
		
		g_free(pkg_n);
		
		gtk_widget_destroy(XPKG.gui.search_dlg.body);
	}
}

GtkWidget *xpkg_create_sresult_list(void){
	GtkWidget *list;
	GtkCellRenderer *renderer1, 
	                *renderer2;
	GtkTreeViewColumn *column;
	GtkListStore *store;
	
	list = gtk_tree_view_new();
	
	renderer1 = gtk_cell_renderer_text_new();
	g_object_set(renderer1, "font", "8", NULL);
	renderer2 = gtk_cell_renderer_pixbuf_new();
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer2, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer2, "pixbuf", 0, NULL);

	gtk_tree_view_column_pack_start(column, renderer1, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer1, "text", 1, NULL);
	
	gtk_tree_view_column_set_title(column, "Search results");

	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
	
	
	store = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(list), (GtkTreeModel *)store);
	g_object_unref(store);
	
	g_signal_connect(G_OBJECT(list), "row-activated", 
	G_CALLBACK(xpkg_search_result_list_activated_callback), NULL);
	
	return list;
}

void xpkg_clear_search_dialog(gboolean clear_input_f){
	GtkTreeModel *model;
	gchar clear[1];
		
	model = (GtkTreeModel *)
	gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
		
	gtk_tree_view_set_model(
	GTK_TREE_VIEW(XPKG.gui.search_dlg.list), model);
		
	g_object_unref(model);
	
	if(clear_input_f){
		clear[0] = (gchar)0;
		gtk_entry_set_text(GTK_ENTRY(XPKG.gui.search_dlg.input), clear);
	}
}

void xpkg_set_search_flags(xpkg_search_flags *s_flags){
	s_flags->name_t_f = gtk_toggle_button_get_active(
	GTK_TOGGLE_BUTTON(XPKG.gui.search_dlg.name_toggle));
	s_flags->descr_t_f = gtk_toggle_button_get_active(
	GTK_TOGGLE_BUTTON(XPKG.gui.search_dlg.descr_toggle));
}

const gchar *xpkg_get_search_dialog_input(void){
	return gtk_entry_get_text(GTK_ENTRY(XPKG.gui.search_dlg.input));
}

void xpkg_set_search_progress_fraction(gdouble fraction){
	gtk_progress_bar_set_fraction(
	GTK_PROGRESS_BAR(XPKG.gui.search_dlg.p_bar), fraction);
}

static void xpkg_search_dialog_destroy_callback(
GtkWidget *widget, gpointer data){
	XPKG.pkg_search_l = FALSE;
	
	pthread_cancel(XPKG.gui.search_dlg.p_t);
}

static void xpkg_search_dialog_close_callback(
GtkWidget *widget, gpointer data){
	GtkWidget *window = (GtkWidget *)data;
	
	gtk_widget_destroy(window);
}

static void xpkg_search_dialog_clear_callback(
GtkWidget *widget, gpointer data){
	xpkg_clear_search_dialog(TRUE);
}

static void xpkg_search_dialog_search_callback(
GtkWidget *widget, gpointer data){
	
	xpkg_clear_search_dialog(FALSE);
	gtk_widget_set_sensitive(XPKG.gui.search_dlg.clear_button, FALSE);
	
	assert(!pthread_create
	(&XPKG.gui.search_dlg.p_t, NULL, xpkg_search_t, NULL));
}

void xpkg_show_search_dialog(void){
	GtkWidget *window,      *expander,
	          *img,         *infoframe,
	          *input,       *vbox,
	          *button,      *vvbox,
	          *list,        *hbox,
	          *label,       *name_check,
	          *descr_check, *swindow,
	          *p_bar;
	
	XPKG.gui.search_dlg.body = window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	gtk_widget_set_size_request(GTK_WIDGET(window), 350, 300);
	gtk_window_set_transient_for(GTK_WINDOW(window), 
	GTK_WINDOW(XPKG.gui.main_window.body));
	gtk_window_set_position(GTK_WINDOW(window), 
	GTK_WIN_POS_CENTER_ON_PARENT);
	
	switch(XPKG.cont_t_state){
		case T_INSTALLED_STATE:
			gtk_window_set_title(GTK_WINDOW(window), "Search (Installed Packages)");
		break;
		case T_AVAILABLE_STATE:
			gtk_window_set_title(GTK_WINDOW(window), "Search (Available Packages)");
		break;
	}
	
	g_signal_connect(G_OBJECT(window), "destroy",
	G_CALLBACK(xpkg_search_dialog_destroy_callback), NULL);
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new("Search:");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	XPKG.gui.search_dlg.input = input = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), input, TRUE, TRUE, 0);
	
	g_signal_connect (G_OBJECT(input), "activate",
	G_CALLBACK(xpkg_search_dialog_search_callback), (gpointer)window);
	
	XPKG.gui.search_dlg.clear_button = button = gtk_button_new();
	img = gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), img);
	g_signal_connect(G_OBJECT(button), "clicked", 
	G_CALLBACK(xpkg_search_dialog_clear_callback), NULL);
	
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	
	button = gtk_button_new();
	img = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), img);
	g_signal_connect(G_OBJECT(button), "clicked", 
	G_CALLBACK(xpkg_search_dialog_search_callback), (gpointer)window);
	
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	
	expander = gtk_expander_new("Options");
	gtk_box_pack_start(GTK_BOX(vbox), expander, FALSE, FALSE, 0);
	
	infoframe = gtk_frame_new("Search in:");
	gtk_container_add(GTK_CONTAINER(expander), infoframe);
	
	vvbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(infoframe), vvbox);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vvbox), hbox, FALSE, FALSE, 0);
	
	XPKG.gui.search_dlg.name_toggle = name_check = 
	gtk_check_button_new_with_label("Package Name");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(name_check), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), name_check, FALSE, FALSE, 0);
	
	XPKG.gui.search_dlg.descr_toggle = descr_check = 
	gtk_check_button_new_with_label("Description");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(descr_check), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), descr_check, FALSE, FALSE, 0);
	
	swindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), swindow, TRUE, TRUE, 0);
	
	XPKG.gui.search_dlg.list = list = xpkg_create_sresult_list();
	gtk_scrolled_window_add_with_viewport(
	GTK_SCROLLED_WINDOW(swindow), list);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
	
	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 2);
	
	g_signal_connect(G_OBJECT(button), "clicked", 
	G_CALLBACK(xpkg_search_dialog_close_callback), (gpointer)window);
	
	XPKG.gui.search_dlg.p_bar = p_bar = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(hbox), p_bar, TRUE, TRUE, 0);
	
	gtk_widget_show_all(window);
	
	gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
	g_signal_connect(G_OBJECT(window), "key-press-event",
	G_CALLBACK(xpkg_window_key_press_callback), NULL);
}

GtkWidget *xpkg_create_install_button(void){
	GtkWidget *button,
	          *image,
	          *label,
	          *hbox;
	          
	label = gtk_label_new("Install");
	image = gtk_image_new_from_stock(GTK_STOCK_OK, GTK_ICON_SIZE_BUTTON);
	
	hbox = gtk_hbox_new(FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button), hbox);
	
	return button;
}

GtkWidget *xpkg_create_pkgopen_dependencies_list(void){
	GtkWidget *list;
	GtkCellRenderer *renderer1, 
	                *renderer2;
	GtkTreeViewColumn *column;
	GtkListStore *store;
	
	list = gtk_tree_view_new();
	
	renderer1 = gtk_cell_renderer_text_new();
	g_object_set(renderer1, "font", "8", NULL);
	renderer2 = gtk_cell_renderer_pixbuf_new();
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer2, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer2, "pixbuf", 0, NULL);

	gtk_tree_view_column_pack_start(column, renderer1, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer1, "text", 1, NULL);
	
	gtk_tree_view_column_set_title(column, "Package");

	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer2, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer2, "pixbuf", 2, NULL);
	
	gtk_tree_view_column_set_title(column, "State");

	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);	
	
	store = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(list), (GtkTreeModel *)store);
	g_object_unref(store);
	
	return list;
}

void xpkg_fill_pkgopen_dependencies_list(xpkg_archive_info a_info,
GtkWidget *list){
	GtkListStore *store;
	GtkTreeIter iter;
	GtkWidget *icon;
	GdkPixbuf *pbuf, *pbuf2;
	int i;
	
	store = gtk_list_store_new(
	3, GDK_TYPE_PIXBUF, G_TYPE_STRING, GDK_TYPE_PIXBUF);
	
	pbuf = gdk_pixbuf_new_from_xpm_data((void *)&packages_xpm);
	
	for(i=0; a_info->deps[i] != NULL; i++){		
		if(xpkg_pkg_exists(a_info->deps[i])){
			icon = gtk_image_new_from_stock(GTK_STOCK_APPLY,
			GTK_ICON_SIZE_DIALOG);
			pbuf2 = gtk_widget_render_icon(icon, GTK_STOCK_APPLY, 
			GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
			gtk_widget_destroy(icon);
			pbuf2 = gdk_pixbuf_scale_simple(pbuf2, 
			20, 20, GDK_INTERP_BILINEAR);
		}
		else {
			icon = gtk_image_new_from_stock(GTK_STOCK_CANCEL,
			GTK_ICON_SIZE_DIALOG);
			pbuf2 = gtk_widget_render_icon(icon, GTK_STOCK_CANCEL, 
			GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
			gtk_widget_destroy(icon);
			pbuf2 = gdk_pixbuf_scale_simple(pbuf2, 
			20, 20, GDK_INTERP_BILINEAR);
			
			XPKG.ins_deps_f = FALSE;
		}
		
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, pbuf, 1, a_info->deps[i],
		2, pbuf2, -1);
	}
	
	gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));
	
	g_object_unref(store);
}

GtkWidget *xpkg_create_pkgopen_conflicts_list(void){
	GtkWidget *list;
	GtkCellRenderer *renderer1, 
	                *renderer2;
	GtkTreeViewColumn *column;
	GtkListStore *store;
	
	list = gtk_tree_view_new();
	
	renderer1 = gtk_cell_renderer_text_new();
	g_object_set(renderer1, "font", "8", NULL);
	renderer2 = gtk_cell_renderer_pixbuf_new();
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer2, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer2, "pixbuf", 0, NULL);

	gtk_tree_view_column_pack_start(column, renderer1, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer1, "text", 1, NULL);
	
	gtk_tree_view_column_set_title(column, "Conflict");

	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer2, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer2, "pixbuf", 2, NULL);
	
	gtk_tree_view_column_set_title(column, "State");

	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);	
	
	store = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(list), (GtkTreeModel *)store);
	g_object_unref(store);
	
	return list;
}

void xpkg_fill_pkgopen_conflicts_list(xpkg_archive_info a_info,
GtkWidget *list){
	GtkListStore *store;
	GtkTreeIter iter;
	GtkWidget *icon;
	GdkPixbuf *pbuf, *pbuf2;
	int conf_exists, i;
	
	store = gtk_list_store_new(
	3, GDK_TYPE_PIXBUF, G_TYPE_STRING, GDK_TYPE_PIXBUF);
	
	pbuf = gdk_pixbuf_new_from_xpm_data((void *)&m_icon_xpm);
	pbuf = gdk_pixbuf_scale_simple(pbuf, 20, 20, GDK_INTERP_BILINEAR);
	
	for(i=0; a_info->conflicts[i] != NULL; i++){
		conf_exists = pkg_conflict_exists(XPKG.installed_pkgs.pkgs, 
		a_info->conflicts[i]);

		if(conf_exists || conf_exists == -1){
			icon = gtk_image_new_from_stock(GTK_STOCK_APPLY,
			GTK_ICON_SIZE_DIALOG);
			pbuf2 = gtk_widget_render_icon(icon, GTK_STOCK_APPLY, 
			GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
			gtk_widget_destroy(icon);
			pbuf2 = gdk_pixbuf_scale_simple(pbuf2, 
			20, 20, GDK_INTERP_BILINEAR);
		}
		else {
			icon = gtk_image_new_from_stock(GTK_STOCK_CANCEL,
			GTK_ICON_SIZE_DIALOG);
			pbuf2 = gtk_widget_render_icon(icon, GTK_STOCK_CANCEL, 
			GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
			gtk_widget_destroy(icon);
			pbuf2 = gdk_pixbuf_scale_simple(pbuf2, 
			20, 20, GDK_INTERP_BILINEAR);
			
			XPKG.ins_cnfl_f = FALSE;
		}
		
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, pbuf, 1, a_info->conflicts[i],
		2, pbuf2, -1);
	}
	
	gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));
	
	g_object_unref(store);
}

static void xpkg_open_dialog_install_callback
(GtkWidget *widget, gpointer data){
	xpkg_archive_info a_info = (void *) data;
	xpkgJob *jobs;
	
	XPKG.gui.install_fetch_dlg.p_t = 0;
	
	if(!XPKG.ins_deps_f && !a_info->f_install){
		xpkg_error("One or more dependency is missing!");
	}
	else if(!XPKG.ins_cnfl_f && !a_info->f_install){
		xpkg_error("Package conflicts with an existing package!");
	}
	else {
		gtk_widget_set_sensitive(XPKG.gui.main_window.body, FALSE);
		
		jobs = xpkg_get_archive_jobs(a_info, NULL);
		
		if(jobs == NULL){
				xpkg_error("Package %s or its older version is already installed!", a_info->full_name);
				
				gtk_widget_destroy(a_info->parient);
				gtk_widget_set_sensitive(XPKG.gui.main_window.body, TRUE);
				
				return;
		}
		
		xpkg_show_jobs_dialog(jobs);

		gtk_widget_destroy(a_info->parient);
	}
}

static void xpkg_open_dialog_close_callback(
GtkWidget *widget, gpointer data){
	xpkg_archive_info a_info = (void *)data;
	
	gtk_widget_destroy(a_info->parient);
	
	xpkg_archive_info_free(a_info);
}

static void xpkg_install_dialog_destroy_callback(
GtkWidget *widget, gpointer data){
	XPKG.pkg_install_l = FALSE;
}

static void xpkg_open_dialog_finstall_callback(
GtkWidget *widget, gpointer data){
	xpkg_archive_info a_info = (void *) data;
	
	if(a_info->f_install){
		a_info->f_install = FALSE;
	}
	else
		a_info->f_install = TRUE;
}

void xpkg_show_open_pkg_dialog(xpkg_archive_info a_info){
	GtkWidget *window,   *swindow,
	          *label,    *hbox,
	          *button,   *list,
	          *expander, *vbox,
	          *chbutton;
	gchar *str;
	
	XPKG.ins_cnfl_f = XPKG.ins_deps_f = TRUE;
	XPKG.pkg_install_l = TRUE;
	          
	a_info->parient = window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	gtk_widget_set_size_request(GTK_WIDGET(window), 340, 350);
	gtk_window_set_transient_for(GTK_WINDOW(window), 
	GTK_WINDOW(XPKG.gui.main_window.body));
	gtk_window_set_position(GTK_WINDOW(window), 
	GTK_WIN_POS_CENTER_ON_PARENT);
	
	gtk_window_set_title(GTK_WINDOW(window), "Package Info");
	
	g_signal_connect(G_OBJECT(window), "destroy",
	G_CALLBACK(xpkg_install_dialog_destroy_callback), NULL);
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
	
	label = gtk_label_new("Package name: ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	
	label = gtk_label_new(NULL);
	str = g_strdup_printf("<b>%s</b>", a_info->name);
	gtk_label_set_markup(GTK_LABEL(label), str);
	g_free(str);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new("Package version: ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	
	label = gtk_label_new(NULL);
	str = g_strdup_printf("<b>%s</b>", a_info->version);
	gtk_label_set_markup(GTK_LABEL(label), str);
	g_free(str);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new("Size on disk: ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	
	label = gtk_label_new(NULL);
	str = g_strdup_printf("<b>%s</b>", print_str_size(a_info->size));
	gtk_label_set_markup(GTK_LABEL(label), str);
	g_free(str);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	expander = gtk_expander_new("");
	label = gtk_expander_get_label_widget(GTK_EXPANDER(expander));
	gtk_label_set_markup(GTK_LABEL(label), "<b>Dependencies:</b>");
	gtk_box_pack_start(GTK_BOX(vbox), expander, TRUE, TRUE, 2);
	gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
	
	swindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(expander), swindow);
	
	list = xpkg_create_pkgopen_dependencies_list();
	gtk_container_add(GTK_CONTAINER(swindow), list);
	
	xpkg_fill_pkgopen_dependencies_list(a_info, list);
	
	expander = gtk_expander_new("");
	label = gtk_expander_get_label_widget(GTK_EXPANDER(expander));
	gtk_label_set_markup(GTK_LABEL(label), "<b>Conflicts:</b>");
	gtk_box_pack_start(GTK_BOX(vbox), expander, FALSE, FALSE, 2);
	gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
	
	swindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(expander), swindow);
	
	list = xpkg_create_pkgopen_conflicts_list();
	gtk_container_add(GTK_CONTAINER(swindow), list);
	
	xpkg_fill_pkgopen_conflicts_list(a_info, list);
	
	expander = gtk_expander_new("");
	label = gtk_expander_get_label_widget(GTK_EXPANDER(expander));
	gtk_label_set_markup(GTK_LABEL(label), "<b>Options:</b>");
	gtk_box_pack_start(GTK_BOX(vbox), expander, FALSE, FALSE, 2);
	gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(expander), hbox);
	
	chbutton = gtk_check_button_new_with_label("Force install");
	gtk_box_pack_start(GTK_BOX(hbox), chbutton, FALSE, FALSE, 0);
	
	g_signal_connect(G_OBJECT(chbutton), "toggled", 
	G_CALLBACK(xpkg_open_dialog_finstall_callback), (gpointer)a_info);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
	
	button = xpkg_create_install_button();
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 2);
	
	g_signal_connect(G_OBJECT(button), "clicked", 
	G_CALLBACK(xpkg_open_dialog_install_callback), (gpointer)a_info);
	
	button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 2);
	
	g_signal_connect(G_OBJECT(button), "clicked", 
	G_CALLBACK(xpkg_open_dialog_close_callback), (gpointer)a_info);
	
	gtk_widget_show_all(window);
	
	gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
	g_signal_connect(G_OBJECT(window), "key-press-event",
	G_CALLBACK(xpkg_window_key_press_callback), NULL);
}

static void xpkg_delete_confirm_dialog_confirm_callback(
GtkWidget *widget, gpointer data){
	Pkg pkg = (Pkg)data;
	xpkgJob *jobs;
	gboolean r_leafs;
	
	XPKG.gui.install_fetch_dlg.p_t = 0;
	
	r_leafs = gtk_toggle_button_get_active(
	GTK_TOGGLE_BUTTON(XPKG.gui.confirm_remove_dlg.ch_box_rleafs));
	
	gtk_widget_set_sensitive(XPKG.gui.main_window.body, FALSE);
	
	jobs = xpkg_get_remove_jobs(pkg, r_leafs, NULL);
	
	if(jobs == NULL){
		xpkg_error("Package '%s' is not installed!\nNothing to do.",
		pkg_get_name(pkg));
					
		gtk_widget_set_sensitive(XPKG.gui.main_window.body, TRUE);
					
		return;
	}
	
	if(r_leafs){
		xpkg_show_selection_dialog(S_REMOVE_T);
		xpkg_fill_remove_list(jobs);
	} else
		xpkg_show_jobs_dialog(jobs);
	
	gtk_widget_destroy(XPKG.gui.confirm_remove_dlg.body);
}

static void xpkg_delete_confirm_dialog_close_callback(
GtkWidget *widget, gpointer data){
	gtk_widget_destroy(XPKG.gui.confirm_remove_dlg.body);
}

static void xpkg_delete_confirm_dialog_destroy_callback(
GtkWidget *widget, gpointer data){
	XPKG.pkg_delete_l = FALSE;
}

void xpkg_show_delete_confirm_dialog(void){
	GtkWidget *window, *label,
	          *button, *ch_box,
	          *image, *frame,
	          *vbox, *vvbox,
	          *hbox, *hhbox;
	gchar *str;
	Pkg pkg = XPKG.selected_pkg,
	    *rdeps;
	          
	XPKG.gui.confirm_remove_dlg.body = window = 
	gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	gtk_widget_set_size_request(GTK_WIDGET(window), 470, 200);
	gtk_window_set_transient_for(GTK_WINDOW(window), 
	GTK_WINDOW(XPKG.gui.main_window.body));
	gtk_window_set_position(GTK_WINDOW(window), 
	GTK_WIN_POS_CENTER_ON_PARENT);
	
	gtk_window_set_title(GTK_WINDOW(window), "Confirm package removal");
	
	g_signal_connect(G_OBJECT(window), "destroy",
	G_CALLBACK(xpkg_delete_confirm_dialog_destroy_callback), NULL);
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
	
	image = gtk_image_new_from_stock(
	GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 2);
	
	vvbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vvbox, FALSE, FALSE, 0);
	
	hhbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vvbox), hhbox, TRUE, TRUE, 0);
	label = gtk_label_new(NULL);
	str = g_strdup_printf("%s%s%s",
	"<b>WARNING:</b> You are about to remove <b>", pkg->name,
	"</b>!\nDo you wish to continue?");
	gtk_label_set_markup(GTK_LABEL(label), str);
	g_free(str);
	gtk_box_pack_start(GTK_BOX(hhbox), label, FALSE, FALSE, 0);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	
	rdeps = pkg_get_rdependencies(pkg);
	if(rdeps[0] != NULL){
		hbox = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
		label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label),
		"<b>One or more packages depend on package being removed!"
        "\nRemoving it might brake them.</b>");
		gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
		gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	}
	pkg_free_list(rdeps);
	
	frame = gtk_frame_new("Options");
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 2);
	
	XPKG.gui.confirm_remove_dlg.ch_box_rleafs =
	ch_box = gtk_check_button_new_with_label("Remove leaf dependencies as well.\n"
	"(will remove all dependencies that are only required by this package)");

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), ch_box, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), hbox);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
	
	button = gtk_button_new_from_stock(GTK_STOCK_YES);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 2);
	g_signal_connect(G_OBJECT(button), "clicked", 
	G_CALLBACK(xpkg_delete_confirm_dialog_confirm_callback), (gpointer)pkg);
	
	button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", 
	G_CALLBACK(xpkg_delete_confirm_dialog_close_callback), NULL);
	
	gtk_widget_show_all(window);
	
	gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
	g_signal_connect(G_OBJECT(window), "key-press-event",
	G_CALLBACK(xpkg_window_key_press_callback), NULL);
}

GtkWidget *xpkg_create_problem_log_widget(void){
	GtkCellRenderer *renderer, *renderer2;
	GtkWidget *view;
	GtkTreeViewColumn *column;
	
	view = gtk_tree_view_new();
	
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "font", "8", NULL);
	renderer2 = gtk_cell_renderer_pixbuf_new();
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer2, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer2, "pixbuf", 0, NULL);

	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", 1, NULL);
	
	gtk_tree_view_column_set_title(column, "Description");

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	
	return view;
}

GtkWidget *xpkg_create_install_log_widget(void){
	GtkCellRenderer *renderer, *renderer2;
	GtkWidget *view;
	GtkTreeViewColumn *column;
	
	view = gtk_tree_view_new();
	
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "font", "8", NULL);
	renderer2 = gtk_cell_renderer_pixbuf_new();
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer2, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer2, "pixbuf", 0, NULL);

	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", 1, NULL);
	
	gtk_tree_view_column_set_title(column, "Actions");

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", 2, NULL);
	
	gtk_tree_view_column_set_title(column, "Status description");

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	
	return view;
}
