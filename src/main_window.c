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

void xpkg_show_jobs_dialog(xpkgJob *);
void xpkg_apply_package_list_filter(void);

void xpkg_print_to_status_bar(gchar *str){
	guint context_id;
	
	context_id = gtk_statusbar_get_context_id(
	GTK_STATUSBAR(XPKG.gui.main_window.statusbar), "loading");
	
	gtk_statusbar_push(GTK_STATUSBAR(XPKG.gui.main_window.statusbar),
	context_id, str);	
}

void *xpkg_main_window_refresh_t(void *data){
	gdk_threads_enter();
	gtk_widget_set_sensitive(XPKG.gui.main_window.pkglist, FALSE);
	gdk_threads_leave();
	
	xpkg_installed_pkg_str_free(XPKG.installed_pkgs);	
	xpkg_init_pkg_db();
	
	gdk_threads_enter();
	xpkg_update_pkglist();
	gdk_threads_leave();
	
	gdk_threads_enter();
	gtk_widget_set_sensitive(XPKG.gui.main_window.pkglist, TRUE);
	gdk_threads_leave();
	
	return NULL;
}

static void xpkg_main_window_refresh_callback(
GtkWidget *widget, gpointer data){
	GThread *g_t;
	
	if(XPKG.cont_t_state == T_INSTALLED_STATE){
		g_t = g_thread_create(xpkg_main_window_refresh_t, NULL, FALSE, NULL);
		assert(g_t != NULL);
	}
	else {
		gtk_widget_set_sensitive(XPKG.gui.main_window.notebook_s, FALSE);			
		xpkg_fetch_index_md5_init(NULL);
	}
}

static void xpkg_main_window_about_callback(
GtkWidget *widget, gpointer data){
	xpkg_show_about_dialog();
}

static void xpkg_main_window_search_callback(
GtkWidget *widget, gpointer data){
	if(!XPKG.pkg_search_l){
		XPKG.pkg_search_l = TRUE;
		xpkg_show_search_dialog();
	}
}

void *xpkg_main_window_open_package_callback_t(void *data){
	gchar *filename = (gchar *)data;
	gchar *str;
	xpkg_archive_info a_info;
	
	gdk_threads_enter();
	guint context_id = gtk_statusbar_get_context_id(
	GTK_STATUSBAR(XPKG.gui.main_window.statusbar), "loading");
	gdk_threads_leave();
	str = g_strdup_printf("Examining package file %s...", filename);
	gdk_threads_enter();
	gtk_statusbar_push(GTK_STATUSBAR(XPKG.gui.main_window.statusbar),
	context_id, str);
	gdk_threads_leave();
	g_free(str);
	
	a_info = xpkg_get_archive_info(filename);
	
	if(a_info != NULL){
		gdk_threads_enter();
		gtk_statusbar_push(GTK_STATUSBAR(XPKG.gui.main_window.statusbar),
		context_id, "Done!");
		gdk_threads_leave();		
		gdk_threads_enter();
		xpkg_show_open_pkg_dialog(a_info);
		gdk_threads_leave();
	} else {
		gdk_threads_enter();
		XPKG.pkg_install_l = FALSE;
		gtk_statusbar_push(GTK_STATUSBAR(XPKG.gui.main_window.statusbar),
		context_id, "Error!");
		gdk_threads_leave();
	}
	g_free(filename);
	
	return NULL;
}

static void xpkg_main_window_open_package_callback(
GtkWidget *widget, gpointer data){
	GtkWidget *dialog;
	gchar *filename;
	GThread *g_t;
	
	if(!XPKG.config->root_f){
		xpkg_error("Permission denied!");
	}
	
	else if(!XPKG.pkg_install_l){
		XPKG.pkg_install_l = TRUE;
		dialog = gtk_file_chooser_dialog_new ("Open File",
		GTK_WINDOW(XPKG.gui.main_window.body),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	
		if(gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT){
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));
		
			g_t = g_thread_create(xpkg_main_window_open_package_callback_t,
			(void *)filename, FALSE, NULL);
			assert(g_t != NULL);		
		}
		else
			XPKG.pkg_install_l = FALSE;
	
		gtk_widget_destroy(dialog);
	}
	
}

void xpkg_main_window_umanager_t(void){
	GPtrArray *u_array;
	
	u_array = xpkg_get_updates();
	
	if(u_array == NULL || !(u_array -> len)){
		gdk_threads_enter();
		xpkg_error("System is up to date. Nothing to do!");
		gtk_widget_set_sensitive(XPKG.gui.main_window.body, TRUE);
		gdk_threads_leave();
		
		xpkg_free_array(u_array);
		return;
	}
	
	gdk_threads_enter();
	xpkg_show_selection_dialog(S_UPDATE_T);
	xpkg_fill_update_list(u_array);
	gdk_threads_leave();
}

static void xpkg_main_window_umanager_callback(
GtkWidget *widget, gpointer data){
	XPKG.gui.install_fetch_dlg.p_t = NULL;
	
	if(XPKG.config->root_f){
		gtk_widget_set_sensitive(XPKG.gui.main_window.body, FALSE);	
		
		if(!XPKG.rm_cached_f){
			/*starting remote index caching routine*/		
			xpkg_fetch_index_md5_init(xpkg_main_window_umanager_t);
		} else {
			xpkg_main_window_umanager_t();
		}
	}
	else
		xpkg_error("Permission denied!");
}

GtkWidget * xpkg_create_main_window_toolbar(void){
	GtkWidget *toolbar,
	          *icon;
	
	toolbar = gtk_toolbar_new();
	
	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), 
	GTK_ORIENTATION_HORIZONTAL);
	
	gtk_toolbar_set_style (GTK_TOOLBAR(toolbar),
	GTK_TOOLBAR_BOTH_HORIZ);
	
	/*Refresh*/
	icon = gtk_image_new_from_stock(
	GTK_STOCK_REFRESH, GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
	"Refresh list",
	"Refresh package list",
	"private", icon,
	G_CALLBACK(xpkg_main_window_refresh_callback), NULL);
	
	/*Search*/
	icon = gtk_image_new_from_stock(
	GTK_STOCK_FIND, GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
	"Search",
	"Search menu",
	"private", icon,
	G_CALLBACK(xpkg_main_window_search_callback), NULL);
	
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	
	/*Install package from file*/
	icon = gtk_image_new_from_stock(
	GTK_STOCK_OPEN, GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
	"Open package",
	"Install package from file",
	"private", icon,
	G_CALLBACK(xpkg_main_window_open_package_callback), NULL);
	
	icon = gtk_image_new_from_stock(
	GTK_STOCK_REFRESH, GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
	"Updating manager",
	"Update packages",
	"private", icon,
	G_CALLBACK(xpkg_main_window_umanager_callback), NULL);
	
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	
	/*About*/
	icon = gtk_image_new_from_stock(
	GTK_STOCK_INFO, GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
	"About",
	"About the project",
	"private", icon,
	G_CALLBACK(xpkg_main_window_about_callback), NULL);
		
	return toolbar;
}
	
gchar *portb2origin(gchar *portb){
	gchar *origin;
	size_t s, ss;
	u_int i = 0;

	s = strlen(portb);
	
	for(ss = s; ss != 0; ss--){
		if(portb[ss] == '/'){
			if(!i)
				s = ss;
			i++;
		}
		if(i == 2)
			break;
	}
	
	s = s - ss;
	
	origin = malloc(s + 1);
	assert(origin != NULL);
	
	memset(origin, 0, s + 1);
	memmove(origin, (portb[ss] == '/')?(portb + ss + 1):(portb + ss),
	       (portb[ss] == '/')?(s - 1):s);
	
	return origin;
}

struct xpkg_origin_table_s *append_origin(struct xpkg_origin_table_s *list, 
struct xpkg_origin_table_s o, u_int n){
	struct xpkg_origin_table_s *l;
	size_t s = sizeof(struct xpkg_origin_table_s), ss;
	
	if(list == NULL)
		return NULL;
	
	ss = s + (s * n);
	l = realloc(list, ss);
	assert(l != NULL);
	memset(&l[n], 0, s);

	memmove( &l[n - 1], &o, s);
	
	return l;
}
	
void xpkg_update_available_pkglist(void){
	PkgIdxEntry pie;
	u_short first = 1;
	short int rv = 1;
	struct xpkg_origin_table_s *ao, o;
	size_t s;
	gchar *origin, *ptr;
	u_int a, b, i, n = 0;
	gboolean found_f = FALSE;
	
	GtkTreeStore *store;
	GdkPixbuf *pbuf, *pbuf_d, *pbuf_p;
	GtkTreeIter iter;
	
	store = gtk_tree_store_new(2, 
	GDK_TYPE_PIXBUF, G_TYPE_STRING);
	
	s = sizeof(struct xpkg_origin_table_s);
	
	ao = malloc(s);
	assert(ao != NULL);	
	memset(ao, 0, s);
	
	for(i = 0; icons_s[i].xpm_d != NULL; i++);
	pbuf = gdk_pixbuf_new_from_xpm_data((void *)&m_icon_xpm);
	pbuf_d = gdk_pixbuf_scale_simple(pbuf, 20, 20, GDK_INTERP_BILINEAR);
	UNREF_OBJ(pbuf);
	
	pbuf_p = gdk_pixbuf_new_from_xpm_data((void *)&packages_xpm);
	
	/*form origin list*/
	DB_OPEN;
	
	while(rv != 0 && rv != -1){
		memset(&pie, 0, sizeof(pie));
		rv = pkg_get_cached_entry(XPKG.available_pkgs.dbp, &pie, &first);
		if(rv != 2 && strlen(pie.name) != 0){
			origin = portb2origin(pie.port_base);
			
			for(a = 0; ao[a].name != NULL; a++){
				if(!strcmp(ao[a].name, origin))
					break;
			}
			if(ao[a].name == NULL){
				o.name = strdup(origin);
				for(b = 0; b != i; b++){
					if(!strcmp(
					origin, icons_s[b].category)){
						found_f = TRUE;
						break;
					} else {
						found_f = FALSE;
					}
				}
				if(found_f == TRUE){
					pbuf = gdk_pixbuf_new_from_xpm_data((void *)icons_s[b].xpm_d);
				} else
					pbuf = pbuf_d;
					
				o.pbuf = pbuf;
				
				gtk_tree_store_append(store, &o.iter, NULL);
				gtk_tree_store_set(store, &o.iter, 0, o.pbuf, 1, o.name, -1);
				
				ao = append_origin(ao, o, ++n);
			}
			
			if(ao[a].name == NULL){
				for(a = 0; ao[a].name != NULL; a++){
					if(!strcmp(ao[a].name, origin))
						break;
				}
			}
			
			assert(ao[a].name != NULL);
			
			ptr = g_strdup_printf("%s-%s", pie.name, pie.version);
			
			gtk_tree_store_append(store, &iter, &ao[a].iter);
			gtk_tree_store_set(store, &iter, 0, pbuf_p, 1, ptr, -1);
			
			g_free(ptr);
			free(origin);
		}
	}
	if(rv == -1){
		xpkg_error("Cache corrupted!");
	} else		
		DB_CLOSE;
	
	xpkg_otable_free(ao);
	
	gtk_tree_sortable_set_sort_column_id(
	GTK_TREE_SORTABLE(store), 1, GTK_SORT_ASCENDING);
	
	gtk_tree_view_set_model(
	GTK_TREE_VIEW(XPKG.gui.main_window.a_pkglist), (GtkTreeModel *)store);
	
	DB_OPEN;
	i = pkg_get_cached_entries(DB_H);
	asprintf(&ptr,"%d packages loaded from cached index.", i);
	xpkg_print_to_status_bar(ptr);
	free(ptr);
	DB_CLOSE;
	
	if(XPKG.available_pkgs.model)
		UNREF_OBJ(XPKG.available_pkgs.model);
	
	XPKG.available_pkgs.model = GTK_TREE_MODEL(store);

	UNREF_OBJ(pbuf_p);
}

void xpkg_update_pkglist(void){
	xpkg_cache_pkg_ht pkg_ht;
	struct xpkg_origin_table_s *ao, o;
	size_t s, len;
	Pkg *pkgs;
	gchar *origin;
	u_int a, b, i, n, c = 0;
	gboolean found_f = FALSE;
	
	GtkTreeStore *store;
	GdkPixbuf *pbuf, *pbuf_d, *pbuf_p;
	GtkTreePath *path;
	GtkTreeIter iter;
	
	store = gtk_tree_store_new(2, 
	GDK_TYPE_PIXBUF, G_TYPE_STRING);
	
	pkgs = XPKG.installed_pkgs.pkgs;
	
	s = sizeof(struct xpkg_origin_table_s);
	
	ao = malloc(s);
	assert(ao != NULL);	
	memset(ao, 0, s);
	
	for(i = 0; icons_s[i].xpm_d != NULL; i++);
	pbuf = gdk_pixbuf_new_from_xpm_data((void *)&m_icon_xpm);
	pbuf_d = gdk_pixbuf_scale_simple(pbuf, 20, 20, GDK_INTERP_BILINEAR);
	UNREF_OBJ(pbuf);
	
	pbuf_p = gdk_pixbuf_new_from_xpm_data((void *)&packages_xpm);

	for(n = 0;pkgs[n] != NULL; n++){
		pkg_ht = g_hash_table_lookup(XPKG.installed_pkgs.cache_hts.pkg_ht,
		(gconstpointer)pkg_get_name(pkgs[n]));
		
		if(pkg_ht == NULL || pkg_ht -> pkg_o == NULL){
			xpkg_error("Failed to get origin for %s!", pkg_get_name(pkgs[n]));
			break;
		}
		
		origin = portb2origin(pkg_ht -> pkg_o);
		
		len = strlen(origin);
			
		for(a = 0; ao[a].name != NULL; a++){
			if(!strcmp(ao[a].name, origin) && strlen(ao[a].name) == len)
				break;
		}
		if(ao[a].name == NULL){
			o.name = strdup(origin);
			for(b = 0; b != i; b++){
				if(!strcmp(
				origin, icons_s[b].category)){
					found_f = TRUE;
					break;
				} else {
					found_f = FALSE;
				}
			}
			if(found_f == TRUE){
				pbuf = gdk_pixbuf_new_from_xpm_data((void *)icons_s[b].xpm_d);
			} else
				pbuf = pbuf_d;
					
			o.pbuf = pbuf;
				
			gtk_tree_store_append(store, &o.iter, NULL);
			gtk_tree_store_set(store, &o.iter, 0, o.pbuf, 1, o.name, -1);
				
			ao = append_origin(ao, o, ++c);
		}
			
		assert(ao[a].name != NULL);
			
		gtk_tree_store_append(store, &iter, &ao[a].iter);
		gtk_tree_store_set(store, &iter, 0, pbuf_p, 1, pkg_get_name(pkgs[n]), -1);

		free(origin);
	}

	xpkg_otable_free(ao);
	
	gtk_tree_sortable_set_sort_column_id(
	GTK_TREE_SORTABLE(store), 1, GTK_SORT_ASCENDING);
	
	gtk_tree_view_set_model(
		GTK_TREE_VIEW(XPKG.gui.main_window.pkglist), (GtkTreeModel *)store);
	
	if(XPKG.installed_pkgs.model)
		UNREF_OBJ(XPKG.installed_pkgs.model);
		
	XPKG.installed_pkgs.model = GTK_TREE_MODEL(store);
	
	if(XPKG.filter_leafs_f)
		xpkg_apply_package_list_filter();		
	
	path = gtk_tree_path_new_from_string("0");
	gtk_tree_view_expand_row(GTK_TREE_VIEW(XPKG.gui.main_window.pkglist), 
	path, TRUE);
	gtk_tree_path_free(path);
	
	path = gtk_tree_path_new_from_string("0:0");
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(XPKG.gui.main_window.pkglist),
	path, NULL, FALSE);
	gtk_tree_path_free(path);
	
	UNREF_OBJ(pbuf_p);
}

void  xpkg_pkg_list_selected_callback(GtkWidget *widget, gpointer data){
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *pkg_n;
	
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	
	if(gtk_tree_selection_get_selected(selection, &model, &iter)){
		if(!gtk_tree_model_iter_has_child(model, &iter)){
			gtk_tree_model_get(model, &iter, 1, &pkg_n, -1);
			if(xpkg_pkg_exists(pkg_n))
				xpkg_display_pkg_info_wrapper(pkg_n);
		}
	}
}

void  xpkg_a_pkg_list_selected_callback(GtkWidget *widget, gpointer data){
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *pkg_n;
	
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	
	if(gtk_tree_selection_get_selected(selection, &model, &iter)){
		if(!gtk_tree_model_iter_has_child(model, &iter)){
			gtk_tree_model_get(model, &iter, 1, &pkg_n, -1);
			xpkg_display_a_pkg_info_wrapper(pkg_n);
		}
	}	
}

GtkWidget *xpkg_create_pkg_list(void){
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
	
	gtk_tree_view_column_set_title(column, "Packages");

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	
	g_object_set(view, "enable-tree-lines", TRUE, NULL);

	return view;
}

void xpkg_reqby_list_activated_callback(
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
	}
}

GtkWidget *xpkg_create_pkg_reqby_list(void){
	GtkCellRenderer *renderer1, *renderer2;
	GtkWidget *view;
	GtkTreeViewColumn *column;
	
	view = gtk_tree_view_new();
	
	renderer1 = gtk_cell_renderer_pixbuf_new();
	renderer2 = gtk_cell_renderer_text_new();
	g_object_set(renderer2, "font", "8", NULL);
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);

	gtk_tree_view_column_pack_start(column, renderer1, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer1, "pixbuf", 0, NULL);

	gtk_tree_view_column_pack_start(column, renderer2, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer2, "text", 1, NULL);
	
	gtk_tree_view_column_set_title(column, "Package");

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	
	g_signal_connect(G_OBJECT(view), "row-activated", 
	G_CALLBACK(xpkg_reqby_list_activated_callback), NULL);
	
	return view;
}

void xpkg_dep_list_activated_callback(
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
	}
}

GtkWidget *xpkg_create_pkg_dep_list(void){
	GtkCellRenderer *renderer1, *renderer2;
	GtkWidget *view;
	GtkTreeViewColumn *column;

	view = gtk_tree_view_new();
	
	renderer1 = gtk_cell_renderer_pixbuf_new();
	renderer2 = gtk_cell_renderer_text_new();
	g_object_set(renderer2, "font", "8", NULL);
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);

	gtk_tree_view_column_pack_start(column, renderer1, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer1, "pixbuf", 0, NULL);

	gtk_tree_view_column_pack_start(column, renderer2, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer2, "text", 1, NULL);
	
	gtk_tree_view_column_set_title(column, "Package");

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);

	gtk_tree_view_column_pack_start(column, renderer1, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer1, "pixbuf", 2, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer2, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer2, "text", 3, NULL);
	
	gtk_tree_view_column_set_title(column, "Installed");

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	
	g_signal_connect(G_OBJECT(view), "row-activated", 
	G_CALLBACK(xpkg_dep_list_activated_callback), NULL);
	
	return view;
}

GtkWidget *xpkg_create_pkg_file_list(void){
	GtkCellRenderer *renderer1, *renderer2;
	GtkWidget *view;
	GtkTreeViewColumn *column;

	view = gtk_tree_view_new();
	
	renderer1 = gtk_cell_renderer_pixbuf_new();
	renderer2 = gtk_cell_renderer_text_new();
	g_object_set(renderer2, "font", "8", NULL);
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);

	gtk_tree_view_column_pack_start(column, renderer1, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer1, "pixbuf", 0, NULL);

	gtk_tree_view_column_pack_start(column, renderer2, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer2, "text", 1, NULL);
	
	gtk_tree_view_column_set_title(column, "File");

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);

	gtk_tree_view_column_pack_start(column, renderer1, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer1, "pixbuf", 2, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer2, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer2, "text", 3, NULL);
	
	gtk_tree_view_column_set_title(column, "Status");

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	
	column = gtk_tree_view_column_new();
	g_object_set(column, "resizable", TRUE, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer2, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer2, "text", 4, NULL);
	
	gtk_tree_view_column_set_title(column, "Size");

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	
	return view;
}

void xpkg_main_window_abar_update_callback_indexed(void){
	xpkg_cache_pkg_ht pkg_h;
	gchar *n_pkg, *ptr, *data;
	xpkgJob *jobs = NULL;
	int size;
	const char *pkg_n;
	
	pkg_n = pkg_get_name(XPKG.selected_pkg);
	
	if((pkg_h = g_hash_table_lookup(XPKG.installed_pkgs.cache_hts.pkg_ht,
	(gconstpointer)pkg_n)) != NULL){
		ptr = g_strdup_printf("+%s", pkg_h->pkg_o);
		assert(ptr != NULL);
		
		DB_OPEN;
		size = pkg_get_cached_unit(ptr, &data, DB_H);
		
		if(size > 0){
			n_pkg = strdup(data);
			DB_CLOSE;
			
			jobs = xpkg_get_jobs(n_pkg, NULL, 0);
			
			if(jobs != NULL){
				gdk_threads_enter();
				xpkg_show_jobs_dialog(jobs);
				gtk_window_set_title(GTK_WINDOW(XPKG.gui.install_fetch_dlg.body),
				"Updating");
				gdk_threads_leave();
			} else {
				gdk_threads_enter();
				xpkg_error_dialog("Package is up to date, nothing to do!");
				gtk_widget_set_sensitive(XPKG.gui.main_window.body, TRUE);
				gdk_threads_leave();
			}
			
			free(n_pkg);
			g_free(ptr);
			
			return;
		}
		
		DB_CLOSE;		
		g_free(ptr);
	}
	
	gdk_threads_enter();
	gtk_widget_set_sensitive(XPKG.gui.main_window.body, TRUE);
	xpkg_error_dialog("Failed to calculate update values!");
	gdk_threads_leave();
}

static void xpkg_main_window_abar_update_callback(
GtkWidget *widget, gpointer data){
	if(XPKG.config->root_f){
		if(!XPKG.rm_cached_f){
			/*starting remote index caching routine*/
			gtk_widget_set_sensitive(XPKG.gui.main_window.body, FALSE);			
			xpkg_fetch_index_md5_init(xpkg_main_window_abar_update_callback_indexed);
		} else {
			xpkg_main_window_abar_update_callback_indexed();
		}
	}
	else
		xpkg_error("Permission denied!");
}

static void xpkg_main_window_abar_delete_callback(
GtkWidget *widget, gpointer data){
	if(XPKG.config->root_f){
		if(!XPKG.pkg_delete_l){
			xpkg_show_delete_confirm_dialog();
			XPKG.pkg_delete_l = TRUE;
		}
	}
	else
		xpkg_error("Permission denied!");
}

static void xpkg_main_window_abar_install_callback(
GtkWidget *widget, gpointer data){
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *pname;
	xpkgJob *jobs;
	
	XPKG.gui.install_fetch_dlg.p_t = NULL;
	
	if(XPKG.config->root_f){
		selection = gtk_tree_view_get_selection(
		GTK_TREE_VIEW(XPKG.gui.main_window.a_pkglist));
	
		if(gtk_tree_selection_get_selected(selection, &model, &iter)){
			if(!gtk_tree_model_iter_has_child(model, &iter)){
				gtk_widget_set_sensitive(XPKG.gui.main_window.body, FALSE);
				
				gtk_tree_model_get(model, &iter, 1, &pname, -1);
				XPKG.gui.install_fetch_dlg.p_t = NULL;
				
				jobs = xpkg_get_jobs(pname, NULL, 0);
				
				if(jobs == NULL){
					xpkg_error("Package '%s' is already installed!\nNothing to do.",
					pname);
					
					gtk_widget_set_sensitive(XPKG.gui.main_window.body, TRUE);
					
					return;
				}
				
				jobs = xpkg_short_jobs_by_priority(jobs);
				
				xpkg_show_jobs_dialog(jobs);
			}
		}
	}
	else
		xpkg_error("Permission denied!");
}

GtkWidget *xpkg_create_main_window_actions_bar(void){
	GtkWidget *toolbar,
	          *icon;
	
	toolbar = gtk_toolbar_new();
	
	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), 
	GTK_ORIENTATION_HORIZONTAL);
	
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),
	GTK_TOOLBAR_BOTH_HORIZ);
	
	/*Remove*/
	icon = gtk_image_new_from_stock(
	GTK_STOCK_DELETE, GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
	"Remove Package",
	"Remove package from the system",
	"private", icon,
	G_CALLBACK(xpkg_main_window_abar_delete_callback), NULL);
	
	/*Update*/
	icon = gtk_image_new_from_stock(
	GTK_STOCK_REFRESH, GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
	"Update Package",
	"Update Package using remote repository",
	"private", icon,
	G_CALLBACK(xpkg_main_window_abar_update_callback), NULL);
		
	return toolbar;	
}

GtkWidget *xpkg_create_main_window_actions_bar2(void){
	GtkWidget *toolbar,
	          *icon;
	GdkPixbuf *pbuf;
	
	toolbar = gtk_toolbar_new();
	
	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), 
	GTK_ORIENTATION_HORIZONTAL);
	
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),
	GTK_TOOLBAR_BOTH_HORIZ);
	
	/*Fetch & Install*/
	pbuf = gdk_pixbuf_new_from_xpm_data((void *)&install_xpm);
	icon = gtk_image_new_from_pixbuf(pbuf);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
	"Fetch and install",
	"Fetch and install the package",
	"private", icon,
	G_CALLBACK(xpkg_main_window_abar_install_callback), NULL);
		
	return toolbar;	
}

static gboolean xpkg_filter_ilist
(GtkTreeModel *model, GtkTreeIter *iter, gpointer data){
	xpkg_cache_pkg_ht c_pkg_ht;
	const gchar *pkgn;
	
	if(gtk_tree_model_iter_has_child(model, iter))
		return TRUE;
	
	gtk_tree_model_get(model, iter, 1, &pkgn, -1);

	if((c_pkg_ht = 
	g_hash_table_lookup(XPKG.installed_pkgs.cache_hts.pkg_ht, (void *)pkgn)))
		if((c_pkg_ht -> flags) & F_IS_LEAF)
			return TRUE;
	
	return FALSE;
}

static gboolean xpkg_filter_alist
(GtkTreeModel *model, GtkTreeIter *iter, gpointer data){
	gchar *rdeps;
	const gchar *pkgn;
	
	if(gtk_tree_model_iter_has_child(model, iter))
		return TRUE;
	
	gtk_tree_model_get(model, iter, 1, &pkgn, -1);
	
	DB_OPEN;
	
	if(!(rdeps = pkg_get_cached_rdeps(DB_H, (char *)pkgn))){
		DB_CLOSE;
		
		return TRUE;
	}
	free(rdeps);
	
	DB_CLOSE;
	
	return FALSE;
}

void xpkg_apply_package_list_filter(void){
	GtkTreeModel *filter, *model;
	
	model = XPKG.installed_pkgs.model;
	
	if(XPKG.filter_leafs_f){
		if(!model)
			return;
	
		filter = gtk_tree_model_filter_new(model, NULL);
	
		if(!filter)
			return;
			
		gtk_tree_model_filter_set_visible_func
		(GTK_TREE_MODEL_FILTER(filter), xpkg_filter_ilist, NULL, NULL);
		
		gtk_tree_view_set_model
		(GTK_TREE_VIEW(XPKG.gui.main_window.pkglist), filter);
		
		UNREF_OBJ(filter);
	} else {
		gtk_tree_view_set_model
		(GTK_TREE_VIEW(XPKG.gui.main_window.pkglist), model);
	}
	
	model = XPKG.available_pkgs.model;
	
	if(XPKG.rm_cached_f && XPKG.filter_leafs_f){		
		if(!model)
			return;
	
		filter = gtk_tree_model_filter_new(model, NULL);
	
		if(!filter)
			return;
			
		gtk_tree_model_filter_set_visible_func
		(GTK_TREE_MODEL_FILTER(filter), xpkg_filter_alist, NULL, NULL);
		
		gtk_tree_view_set_model
		(GTK_TREE_VIEW(XPKG.gui.main_window.a_pkglist), filter);
		
		UNREF_OBJ(filter);
	} else if(XPKG.rm_cached_f){
		gtk_tree_view_set_model
		(GTK_TREE_VIEW(XPKG.gui.main_window.a_pkglist), model);
	}
}

static gboolean xpkg_window_key_press_callback
(GtkWidget *widget, GdkEvent  *event, gpointer data){	
	if(((((GdkEventKey*)event) -> state) & GDK_CONTROL_MASK)){
		if((((GdkEventKey*)event) -> keyval) == GDK_KEY_q){
			gtk_widget_destroy(widget);
			
			return TRUE;
		}
		if((((GdkEventKey*)event) -> keyval) == GDK_KEY_f){
			xpkg_main_window_search_callback(NULL, NULL);
			
			return TRUE;
		}
	}
	
	return FALSE;
}

static void xpkg_main_window_show_callback
(GtkWidget *widget, gpointer data){
	GdkPixbuf *pbuf;	
	
	pbuf = gdk_pixbuf_new_from_xpm_data((void *)&m_icon_xpm);	
	gtk_window_set_icon(GTK_WINDOW(widget), pbuf);
	
	UNREF_OBJ(pbuf);
}

static void xpkg_main_window_leaf_f_callback
(GtkToggleButton *togglebutton, gpointer data){
	XPKG.filter_leafs_f = !XPKG.filter_leafs_f;
	
	xpkg_apply_package_list_filter();
}

static void xpkg_main_window_notebook_cfocus_callback(
GtkNotebook *notebook, GtkNotebookPage *page, guint state, gpointer data){
	
	XPKG.cont_t_state = state;
	
	switch(XPKG.cont_t_state){
		case T_INSTALLED_STATE:
			gtk_window_set_title(GTK_WINDOW(XPKG.gui.main_window.body), 
			"Package Manager (Installed Packages)");
		break;
		case T_AVAILABLE_STATE:
			gtk_window_set_title(GTK_WINDOW(XPKG.gui.main_window.body), 
			"Package Manager (Available Packages)");

			if(!XPKG.rm_cached_f){
				/*starting remote index caching routine*/
				gtk_widget_set_sensitive(XPKG.gui.main_window.notebook_s, FALSE);			
				xpkg_fetch_index_md5_init(NULL);
			}
		break;
	}
}

void xpkg_main_window_set_search_pos
(GtkTreeView *tree_view, GtkWidget *search_d, gpointer data){
	gint s_x, s_y,
	     p_x, p_y,
	     d_x, d_y;

	gtk_window_get_size(GTK_WINDOW(XPKG.gui.main_window.body), &s_x, &s_y);
	gtk_window_get_position(GTK_WINDOW(XPKG.gui.main_window.body), &p_x, &p_y);
	
	gtk_window_get_size(GTK_WINDOW(search_d), &d_x, &d_y);
	gtk_window_move(GTK_WINDOW(search_d), p_x, p_y + s_y - d_y);
}

int xpkg_create_main_window(void){
	          /*main widgets*/
	GtkWidget *window,     *pkglist,
	          *a_pkglist,  *indi_lbl,
	          *toolbar,    *notebook,
	          *name_lbl,   *version_lbl,
	          *size_lbl,   *category_lbl,
	          *pkgdeplist, *pkgreqbylist,
	          *pkgfilelist,*statusbar,
	          *atoolbar,   *atoolbar2,
	          *description_view,
	          *p_bar,      *notebook_s,
	          *leaf_f,
	          /*internal widgets*/
	          *infoframe, *swindow,
	          *hpannel,   *main_vbox,
	          *hbox,      *table,
	          *label,     *vvbox,
	          *expander,  *hhbox,
	          *vbox;
			  
	XPKG.gtk_loop_f = TRUE;
	XPKG.gtk_init_f = TRUE;

	/*window*/
	XPKG.gui.main_window.body = window = 
	gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	gtk_window_set_title(GTK_WINDOW(window), 
	"Package Manager");
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 400);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	
	/*toolbar*/
	toolbar = xpkg_create_main_window_toolbar();
	
	/*pkg list*/
	XPKG.gui.main_window.pkglist = pkglist = xpkg_create_pkg_list();
	XPKG.gui.main_window.a_pkglist = a_pkglist = xpkg_create_pkg_list();
	
	/*search box positioning*/
	gtk_tree_view_set_search_position_func(
	GTK_TREE_VIEW(pkglist), xpkg_main_window_set_search_pos, NULL, NULL);
	gtk_tree_view_set_search_position_func(
	GTK_TREE_VIEW(a_pkglist), xpkg_main_window_set_search_pos, NULL, NULL);
	
	/*action toolbar*/
	XPKG.gui.main_window.atoolbar =
	atoolbar = xpkg_create_main_window_actions_bar();
	XPKG.gui.main_window.atoolbar2 =
	atoolbar2 = xpkg_create_main_window_actions_bar2();
	
	/*pkg dep list*/
	XPKG.gui.main_window.pkgdeplist = pkgdeplist =
	xpkg_create_pkg_dep_list();
	
	/*pkg reqby list*/
	XPKG.gui.main_window.pkgreqbylist = pkgreqbylist =
	xpkg_create_pkg_reqby_list();
	
	/*pkg file list*/
	XPKG.gui.main_window.pkgfilelist = pkgfilelist =
	xpkg_create_pkg_file_list();

	/*info frame*/
	infoframe = gtk_frame_new ("General Information");
	
	XPKG.gui.main_window.name_lbl = name_lbl = 
	gtk_label_new("NULL");
	XPKG.gui.main_window.version_lbl = version_lbl = 
	gtk_label_new("NULL");
	XPKG.gui.main_window.size_lbl = size_lbl = 
	gtk_label_new("NULL");
	XPKG.gui.main_window.category_lbl = category_lbl = 
	gtk_label_new("NULL");
	
	XPKG.gui.main_window.description_view = description_view = 
	gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(description_view),
	GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(description_view), FALSE);
		
	/*window layout*/
	main_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), main_vbox);
	
	gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 1);
	
	hpannel = gtk_hpaned_new();	
	gtk_box_pack_start(GTK_BOX(main_vbox), hpannel, TRUE, TRUE, 0);
	
	/*window layout - left side*/
	XPKG.gui.main_window.notebook_s = notebook_s = gtk_notebook_new();
	
	swindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(GTK_WIDGET(swindow), 250, 0);
	
	vvbox = gtk_vbox_new(FALSE, 0);
	expander = gtk_expander_new("Filters");
	
	hhbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(expander), hhbox);
	
	leaf_f = gtk_check_button_new_with_label("Show leafs only");
	gtk_box_pack_start(GTK_BOX(hhbox), leaf_f, FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(vvbox), expander, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vvbox), notebook_s, TRUE, TRUE, 0);
	
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook_s), 
	swindow, gtk_label_new("Installed Packages"));
	
	gtk_scrolled_window_add_with_viewport(
	GTK_SCROLLED_WINDOW(swindow), pkglist);
	
	swindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(GTK_WIDGET(swindow), 250, 0);
	
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook_s), 
	swindow, gtk_label_new("Available Packages"));
	
	gtk_scrolled_window_add_with_viewport(
	GTK_SCROLLED_WINDOW(swindow), a_pkglist);	
	
	XPKG.gui.main_window.p_bar = p_bar = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(vvbox), p_bar, FALSE, FALSE, 0);
	
	XPKG.gui.main_window.vbox = vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), vvbox, TRUE, TRUE, 0);
	
	gtk_paned_add1(GTK_PANED(hpannel), vbox);
	
	/*window layout - right side*/
	vbox = gtk_vbox_new(FALSE, 0);
	
	expander = gtk_expander_new("Actions");
	gtk_box_pack_start(GTK_BOX(vbox), expander, FALSE, FALSE, 0);
	hhbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hhbox), atoolbar, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hhbox), atoolbar2, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(expander), hhbox);
	gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
	
	hbox = gtk_hbox_new(FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), infoframe, TRUE, TRUE, 0);
	
	gtk_paned_add2(GTK_PANED(hpannel), vbox);
	
	vvbox = gtk_vbox_new(FALSE, 0);
	hbox = gtk_hbox_new(FALSE, 0);
	
	table = gtk_table_new(3, 5, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vvbox), hbox, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (infoframe), vvbox);
	
	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Package name:</b> ");
	gtk_box_pack_start(GTK_BOX(hbox), 
	gtk_hbox_new(FALSE, 0), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 0, 1, 0, 1);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), name_lbl, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), 
	gtk_hbox_new(FALSE, 0), TRUE, TRUE, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, 0, 1);
	hbox = gtk_hbox_new(FALSE, 0);
	XPKG.gui.main_window.indi_lbl = indi_lbl = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(indi_lbl), " <b>Size on disk:</b> ");
	gtk_box_pack_start(GTK_BOX(hbox), 
	gtk_hbox_new(FALSE, 0), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), indi_lbl, FALSE, FALSE, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 2, 3, 0, 1);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), size_lbl, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), 
	gtk_hbox_new(FALSE, 0), TRUE, TRUE, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 3, 4, 0, 1);
	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Package version:</b> ");
	gtk_box_pack_start(GTK_BOX(hbox), 
	gtk_hbox_new(FALSE, 0), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_table_attach_defaults(GTK_TABLE(table),	hbox, 0, 1, 1, 2);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), version_lbl, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), 
	gtk_hbox_new(FALSE, 0), TRUE, TRUE, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, 1, 2);
	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), " <b>Category:</b> ");
	gtk_box_pack_start(GTK_BOX(hbox), 
	gtk_hbox_new(FALSE, 0), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 2, 3, 1, 2);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), category_lbl, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), 
	gtk_hbox_new(FALSE, 0), TRUE, TRUE, 0);
	gtk_table_attach_defaults(GTK_TABLE(table),	hbox, 3, 4, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table),	
	gtk_hbox_new(TRUE, 0), 4, 5, 0, 2);
	hbox = gtk_hbox_new(FALSE, 0);
	XPKG.gui.main_window.install_lbl = label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Installed:</b> ");
	gtk_box_pack_start(GTK_BOX(hbox), 
	gtk_hbox_new(FALSE, 0), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_table_attach_defaults(GTK_TABLE(table),	hbox, 0, 1, 2, 3);
	hbox = gtk_hbox_new(FALSE, 0);
	XPKG.gui.main_window.install_lbl_s = label = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), 
	gtk_hbox_new(FALSE, 0), TRUE, TRUE, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, 2, 3);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vvbox), hbox, TRUE, TRUE, 5);
	
	expander = gtk_expander_new("NULL");
	label = gtk_expander_get_label_widget(GTK_EXPANDER(expander));
	gtk_label_set_markup(GTK_LABEL(label), "<b>Description:</b>");
	
	swindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(GTK_WIDGET(swindow), 0, 150);
	
	gtk_container_add(GTK_CONTAINER(swindow), description_view);
	gtk_container_add(GTK_CONTAINER(expander), swindow);
	gtk_box_pack_start(GTK_BOX(hbox), expander, TRUE, TRUE, 0);
	
	XPKG.gui.main_window.notebook = notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
	
	swindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), 
	swindow, gtk_label_new("Dependencies"));
	gtk_container_add(GTK_CONTAINER(swindow), pkgdeplist);
	
	swindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), 
	swindow, gtk_label_new("Required by"));
	gtk_container_add(GTK_CONTAINER(swindow), pkgreqbylist);
	
	swindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow),
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), 
	swindow, gtk_label_new("Files"));
	gtk_container_add(GTK_CONTAINER(swindow), pkgfilelist);
	
	XPKG.gui.main_window.statusbar = statusbar = gtk_statusbar_new();
	gtk_box_pack_start(GTK_BOX(main_vbox), statusbar, FALSE, FALSE, 0);
	
	/*events*/
	gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);

	/*signals*/
	g_signal_connect(G_OBJECT(window), "destroy",
	G_CALLBACK(gtk_main_quit), NULL);
	
	g_signal_connect(G_OBJECT(window), "key-press-event",
	G_CALLBACK(xpkg_window_key_press_callback), NULL);
	
	g_signal_connect(G_OBJECT(window), "show",
	G_CALLBACK(xpkg_main_window_show_callback), NULL);
	
	g_signal_connect(G_OBJECT(notebook_s), "switch-page",
	G_CALLBACK(xpkg_main_window_notebook_cfocus_callback), NULL);
	
	g_signal_connect(G_OBJECT(pkglist), "cursor-changed", 
	G_CALLBACK(xpkg_pkg_list_selected_callback), NULL);
	
	g_signal_connect(G_OBJECT(a_pkglist), "cursor-changed", 
	G_CALLBACK(xpkg_a_pkg_list_selected_callback), NULL);
	
	g_signal_connect(G_OBJECT(leaf_f), "toggled", 
	G_CALLBACK(xpkg_main_window_leaf_f_callback), NULL);
	
	return 0;
}
