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

#include <stdarg.h>

#include "xpkg.h"

GPtrArray *xpkg_get_updates(void){
	GPtrArray *u_array;
	struct xpkg_update *update;
	Pkg *pkgs = XPKG.installed_pkgs.pkgs;
	xpkg_cache_pkg_ht c_pkg_ht;
	PkgIdxEntry pie;
	u_int i, s;
	gchar *org = NULL,
	      *data = NULL;
	
	u_array = g_ptr_array_new_with_free_func(xpkg_free_update);
	
	for(i = 0; pkgs[i] != NULL; i++){
		c_pkg_ht = g_hash_table_lookup(XPKG.installed_pkgs.cache_hts.pkg_ht,
		(gconstpointer)pkg_get_name(pkgs[i]));
		
		if(c_pkg_ht == NULL)
			return u_array;
			
		asprintf(&org, "+%s", c_pkg_ht -> pkg_o);
		assert(org != NULL);
		
		DB_OPEN;
		s = pkg_get_cached_unit(org, &data, DB_H);
		
		if(s > 0){
			if(strcmp(pkg_get_name(pkgs[i]), data) &&
			!xpkg_pkg_exists(data) &&
			xpkg_get_pie(data, &pie) != FALSE){
				update = malloc(sizeof(struct xpkg_update));
				assert(update != NULL);
				memset(update, 0, sizeof(struct xpkg_update));
				
				update -> pkg_name = strdup((char *)pkg_get_name(pkgs[i]));
				update -> new_ver = strdup(pie.version);
				
				g_ptr_array_add(u_array, (gpointer)update);
			}
		}
		
		DB_CLOSE;
		free(org);
	}
	
	return u_array;
}

char ** xpkg_pkg_n_split(char *pkg_n){
	char **pkg_s;
	int len = strlen(pkg_n),
	    i, ii = 0;
	    
	 if(pkg_n == NULL || !len)
		return NULL;
	
	pkg_s = (char **) malloc(sizeof(char *) * I_PKG_FIELDS);
	    
	for(i = 0; i != len; i++)
		if(pkg_n[i] == '-')
			ii = i;
	
	pkg_s[I_PKG_NAME] = (char *) malloc(ii + 1);
	assert(pkg_s[I_PKG_NAME] != NULL);
	memset(pkg_s[I_PKG_NAME], 0, ii + 1);
	
	pkg_s[I_PKG_VERSION] = (char *) malloc(len - (ii - 1));
	assert(pkg_s[I_PKG_VERSION] != NULL);
	memset(pkg_s[I_PKG_VERSION], 0, len - (ii - 1));
	
	strncpy(pkg_s[I_PKG_NAME], pkg_n, ii);
	strncpy(pkg_s[I_PKG_VERSION], &pkg_n[ii + 1], len - ii - 1);
	
	return pkg_s;
}

void *xpkg_mmap_fd(int fd, size_t *size){
	void *ptr[MMAP_FIELDS];	
	int z_fd;
	size_t st_size;
	struct stat f_stat;
	
	if(fd < 1)
		return NULL;
	
	assert(fstat(fd, &f_stat) != -1);
	
	st_size = f_stat.st_size;
	
	z_fd = open(ZERO_DEV, O_RDWR);
	assert(z_fd > 0);
	
	ptr[MMAP_DST] = mmap(0, st_size, PROT_READ | PROT_WRITE,
	MAP_PRIVATE, z_fd, 0);
	if(ptr[MMAP_DST] == (void *)-1)
		perror(NULL);
	assert(ptr[MMAP_DST] != (void *)-1);
	
	ptr[MMAP_SRC] = mmap(0, st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	
	memcpy(ptr[MMAP_DST], ptr[MMAP_SRC], st_size);
	
	munmap(ptr[MMAP_SRC], st_size);
	
	size[0] = st_size;
		
	return ptr[MMAP_DST];
}

void xpkg_error(char *format, ...){
	va_list ptr;
	const char msg[MSG_LEN];
	
	memset((void *)msg, 0, MSG_LEN);
	
	va_start(ptr, format);
	vsnprintf((void *)msg, (MSG_LEN - 1), format, ptr);
	va_end(ptr);
	
	if(!XPKG.gtk_init_f)
		fprintf(stderr, "%s\n", msg);
	else
		xpkg_error_dialog(msg);
}

void xpkg_error_secure_gui(char *format, ...){
	va_list ptr;
	const char msg[MSG_LEN];
	int rval;
	pid_t pid,
		uid,
		euid,
		suid;
		
	getresuid(&uid, &euid, &suid);
	
	if(uid != euid || uid != suid){
		pid = fork();
		assert(pid != -1);
	
		if(!pid){
			setresuid(uid, uid, uid);
			
			memset((void *)msg, 0, MSG_LEN);
	
			va_start(ptr, format);
			vsnprintf((void *)msg, (MSG_LEN - 1), format, ptr);
			va_end(ptr);
			
			XPKG.gtk_init_f = TRUE;
			gtk_init(&XPKG.argc, &XPKG.argv);
			
			xpkg_error_dialog(msg);
		}
		else
			wait(&rval);
	}
	else{
		memset((void *)msg, 0, MSG_LEN);
	
		va_start(ptr, format);
		vsnprintf((void *)msg, (MSG_LEN - 1), format, ptr);
		va_end(ptr);
			
		XPKG.gtk_init_f = TRUE;
		g_thread_init(NULL);
		gdk_threads_init();
		gtk_init(&XPKG.argc, &XPKG.argv);
			
		xpkg_error_dialog(msg);
	}
}

int pamconv(
	int num_msg, 
	const struct pam_message **msg, 
	struct pam_response **resp, 
	void *appdata_ptr
	){

	struct pam_response *aresp;
	int i;
	
	if (num_msg <= 0 || num_msg > PAM_MAX_NUM_MSG)
		return PAM_CONV_ERR;
	if ((aresp = calloc(num_msg, sizeof *aresp)) == NULL)
		return PAM_BUF_ERR;
	
	for(i = 0; i < num_msg; ++i){
		aresp[i].resp_retcode = 0;
		aresp[i].resp = NULL;
		
		switch(msg[i] -> msg_style){
			case PAM_PROMPT_ECHO_OFF:
				aresp[i].resp = strdup((void *)XPKG.shmem);
			break;
			case PAM_PROMPT_ECHO_ON: /*ignore*/
			break;
			case PAM_TEXT_INFO:      /*ignore*/
			break;
			case PAM_ERROR_MSG:      /*ignore*/
			break;
			default:
				for(i = 0; i < num_msg; ++i){
					if(aresp[i].resp != NULL){
						memset(aresp[i].resp, 0, strlen(aresp[i].resp));
						free(aresp[i].resp);
					}
				}
				
				memset(aresp, 0, num_msg * sizeof *aresp);
				free(aresp);
				*resp = NULL;
				
				return PAM_CONV_ERR;
			break;
		}
	}
	*resp = aresp;
	
	return PAM_SUCCESS;
}

void xpkg_init_pkg_db(void){
	XPKG.installed_pkgs.db = 
	pkg_db_open(XPKG.config -> pkg_base);
	
	if(XPKG.installed_pkgs.db == NULL){
		xpkg_error("Failed opening package database!");
		xpkg_cleanup(XPKG);
		exit(1);
	}
	
	XPKG.installed_pkgs.pkgs = 
	pkg_db_get_installed(XPKG.installed_pkgs.db);
	
	if(XPKG.installed_pkgs.pkgs == NULL){
		xpkg_error("Failed reading package database!");
		xpkg_cleanup(XPKG);
		exit(1);
	}
	
	xpkg_get_cache_ht(&(XPKG.installed_pkgs.cache_hts));
}

int xpkg_ch_path(const char *path){
	int rval,
	    len,
	    i,
	    *pos,
	    pos_c,
	    stat = 0;
	char *buff;
	    
	len = strlen(path);
	
	buff = malloc(len);
	assert(buff != NULL);
	
	for(i = pos_c = 0; i != len; i++)
		if(path[i] == '/')
			pos_c++;
	
	pos = malloc(sizeof(int) * pos_c);
	assert(pos != NULL);
	
	for(i = pos_c = 0; i != len; i++){
		if(path[i] == '/'){
			pos[pos_c] = i;
			pos_c++;
		}
	}
	
	if(pos_c > 1){
		for(i = 0; i != (pos_c - 1); i++){
			memset(buff, 0, len);
			memcpy(buff, path, pos[i + 1]);
			
			rval = access(buff, R_OK);
			
			if((rval == -1) && (errno == ENOENT)){
				stat = 1;
				
				rval = mkdir(buff, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP);
				if(rval == -1){
					stat = -1;
					goto end;
				}
			}
			else if(rval == -1){
				stat = -1;
				goto end;
			}
		}
	}
	
	rval = access(path, R_OK | W_OK);
	
	if((rval == -1) && (errno == ENOENT)){
		stat = 1;
		
		rval = mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP);
		if(rval == -1){
			stat = -1;
			goto end;
		}
	}
	else if(rval == -1){
		stat = -1;
		goto end;
	}
	
	end:
	free(buff);
	free(pos);
	
	return stat;
}

char *print_str_size(size_t size){
	char *size_str;
	
	unsigned short b, kb, mb, left;
	     
#define KB 1024
	
	if(size < KB){
		asprintf(&size_str, "%db", (int)size);
		return size_str;
	}
	else if(size <= (KB * KB)){
		kb = size / KB;
		b = size % KB;
		
		if(b != 0)
			asprintf(&size_str, "%dKb %db", kb, b);
		else
			asprintf(&size_str, "%dKb", kb);
		return size_str;
	}
	else {
		mb = size / (KB * KB);
		left = size % (KB * KB);
		if(!left){
			kb = 0;
			b = 0;
		}
		else{
			kb = left / KB;
			b = left % KB;
		}
		
		if(kb != 0 && b != 0)
			asprintf(&size_str, "%dMb %dKb %db", mb, kb, b);
		else if(kb != 0 && !b)
			asprintf(&size_str, "%dMb %dKb", mb, kb);
		else if(!kb && b != 0)
			asprintf(&size_str, "%dMb %db", mb, b);
		else 
			asprintf(&size_str, "%dMb", mb);
		return size_str;
	}
	
#undef KB
	return NULL;
}

char *next_word(char *src, size_t *pos){
	char *word, *ptr;
	u_int i;
	
	if(!src || !pos)
		return NULL;
	
	ptr = &src[*pos];
	
	if(*pos != 0 && src[*pos - 1] == 0)
		return NULL;
	
	if(!ptr[0]){
		return NULL;
	} else if(*pos != 0 && ptr[0] == ' '){
		ptr += 1;
	}
		
	for(i = 0; ptr[i] != ' ' && ptr[i] != '\0'; i++);
	
	word = malloc(i + 1);
	assert(word != NULL);
	memset(word, 0, i + 1);

	memmove(word, ptr, i);	
	*pos += i + 1;
	
	return word;
}

void *xpkg_display_a_pkg_info(void *data){
	PkgIdxEntry pie;
	gint rv;
	gchar *pkg_n = data, *ptr, *str, *deps, *rdeps;
	gchar *origin;
	GtkTextBuffer *tbuffer;
	GtkWidget *icon;
	GdkPixbuf *pkg_pbuf, *ok_pbuf, *missing_pbuf, *wrong_pbuf, *pbuf;
	GtkListStore *store;
	GtkTreeIter iter;
	
	memset(&pie, 0, sizeof(pie));
	
	gdk_threads_enter();
	asprintf(&str, "Loading package '%s' information...", pkg_n);
	xpkg_print_to_status_bar(str);
	free(str);
	gtk_label_set_text(GTK_LABEL(XPKG.gui.main_window.size_lbl), "");
	
	gtk_widget_hide(XPKG.gui.main_window.atoolbar);
	gtk_widget_show(XPKG.gui.main_window.atoolbar2);
	
	gtk_widget_show(XPKG.gui.main_window.install_lbl);
	gtk_widget_show(XPKG.gui.main_window.install_lbl_s);

	store = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gtk_tree_view_set_model(
	GTK_TREE_VIEW(XPKG.gui.main_window.pkgreqbylist), GTK_TREE_MODEL(store));
	g_object_unref(store);
	
	store = gtk_list_store_new(5, GDK_TYPE_PIXBUF, G_TYPE_STRING,
	GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(
	GTK_TREE_VIEW(XPKG.gui.main_window.pkgfilelist), GTK_TREE_MODEL(store));
	g_object_unref(store);
	gdk_threads_leave();
	
	DB_OPEN;
	rv = pkg_get_cached_unit(pkg_n, &ptr, XPKG.available_pkgs.dbp);
	
	if(rv != sizeof(PkgIdxEntry)){
		gdk_threads_enter();
		xpkg_error("Failed loading package information from index cache!");
		gdk_threads_leave();
		
		DB_CLOSE;
		
		return NULL;
	} else
		memmove(&pie, ptr, sizeof(pie));
		
	DB_CLOSE;
	
	gdk_threads_enter();
	gtk_label_set_markup(GTK_LABEL(XPKG.gui.main_window.indi_lbl),
	" <b>Size of package:</b> ");
	
	gtk_label_set_text(GTK_LABEL(XPKG.gui.main_window.name_lbl),
	pie.name);
	
	gtk_label_set_text(GTK_LABEL(XPKG.gui.main_window.version_lbl),
	pie.version);
	
	origin = portb2origin(pie.port_base);
	
	gtk_label_set_text(GTK_LABEL(XPKG.gui.main_window.category_lbl),
	origin);
	
	tbuffer = gtk_text_view_get_buffer(
	GTK_TEXT_VIEW(XPKG.gui.main_window.description_view));
	
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(tbuffer), pie.descr, strlen(pie.descr));
	gdk_threads_leave();
	
	off_t s = xpkg_fetch_package_size(pkg_n, origin);
	free(origin);
	
	if(s > 0){
		ptr = print_str_size(s);
	
		gdk_threads_enter();
		gtk_label_set_text(GTK_LABEL(XPKG.gui.main_window.size_lbl), ptr);
		gdk_threads_leave();
	
		free(ptr);
	} else {
		gdk_threads_enter();
		gtk_label_set_markup(GTK_LABEL(XPKG.gui.main_window.size_lbl),
		"<span foreground=\"red\">Package file not found</span>");
		gdk_threads_leave();
	}
	
	xpkg_cache_pkg_ht pkg_ht;
	gchar **pkg_s;
	
	pkg_ht = g_hash_table_lookup(XPKG.installed_pkgs.cache_hts.pkg_ht,
	(gconstpointer)pkg_n);

	if(pkg_ht != NULL){
		gdk_threads_enter();
		gtk_label_set_markup(GTK_LABEL(XPKG.gui.main_window.install_lbl_s),
		"<b><span background=\"black\" foreground=\"green\">Yes</span></b>");
		gdk_threads_leave();
	} else {
		pkg_s = xpkg_pkg_n_split(pkg_n);
		
		if(g_hash_table_lookup(XPKG.installed_pkgs.cache_hts.ver_ht,
		   (gconstpointer)pkg_s[I_PKG_NAME]) == NULL){
			gdk_threads_enter();
			gtk_label_set_markup(GTK_LABEL(XPKG.gui.main_window.install_lbl_s),
			"<b><span background=\"black\" foreground=\"red\">No</span></b>");
			gdk_threads_leave();
		} else {
			gdk_threads_enter();
			gtk_label_set_markup(GTK_LABEL(XPKG.gui.main_window.install_lbl_s),
			"<b><span background=\"black\" foreground=\"orange\">Different version</span></b>");
			gdk_threads_leave();
		}
		
		xpkg_pkg_s_free(pkg_s);
	}
	
	DB_OPEN;
	deps = pkg_get_cached_deps(DB_H, pkg_n);
	rdeps = pkg_get_cached_rdeps(DB_H, pkg_n);
	DB_CLOSE;
	
	gdk_threads_enter();
	pkg_pbuf = gdk_pixbuf_new_from_xpm_data((void *)&packages_xpm);
	
	icon = gtk_image_new();
	pbuf = gtk_widget_render_icon(icon, GTK_STOCK_CANCEL, 
	       GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	missing_pbuf = gdk_pixbuf_scale_simple(pbuf, 
		           20, 20, GDK_INTERP_BILINEAR);
	g_object_unref(pbuf);
	pbuf = gtk_widget_render_icon(icon, GTK_STOCK_APPLY, 
	       GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	ok_pbuf = gdk_pixbuf_scale_simple(pbuf, 
		      20, 20, GDK_INTERP_BILINEAR);
	g_object_unref(pbuf);
	pbuf = gtk_widget_render_icon(icon, GTK_STOCK_DIALOG_WARNING, 
	       GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	wrong_pbuf = gdk_pixbuf_scale_simple(pbuf, 
		         20, 20, GDK_INTERP_BILINEAR);
	g_object_unref(pbuf); pbuf = NULL;
	gtk_widget_destroy(icon);
	gdk_threads_leave();
	
	size_t pos = 0;
	gchar *msg, *dep;
	
	gdk_threads_enter();
	store = gtk_list_store_new(
	4, GDK_TYPE_PIXBUF, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gdk_threads_leave();
	
	if(deps != NULL)
	while((dep = next_word(deps, &pos)) != NULL){
		pkg_s = xpkg_pkg_n_split(dep);
		pbuf = missing_pbuf;
		msg = "Missing";		
		
		if(xpkg_pkg_exists(dep)){
			pbuf = ok_pbuf;
			msg = "OK";
		} else {
			if(g_hash_table_lookup(XPKG.installed_pkgs.cache_hts.ver_ht,
			   (gconstpointer)pkg_s[I_PKG_NAME]) != NULL){
				pbuf = wrong_pbuf;
				msg = "Different version";
			}
		}
		
		gdk_threads_enter();
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, pkg_pbuf, 1, dep, 
		2, pbuf, 3, msg, -1);
		gdk_threads_leave();
		
		xpkg_pkg_s_free(pkg_s);
		free(dep);
	}
	free(deps);
	
	gdk_threads_enter();
	gtk_tree_view_set_model(
	GTK_TREE_VIEW(XPKG.gui.main_window.pkgdeplist), GTK_TREE_MODEL(store));
	UNREF_OBJ(store);
	
	store = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gdk_threads_leave();
	
	pos = 0;
	
	if(rdeps != NULL)
	while((dep = next_word(rdeps, &pos)) != NULL){
		gdk_threads_enter();
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, pkg_pbuf, 1, dep, -1);
		gdk_threads_leave();
		
		free(dep);
	}
	free(rdeps);
	
	gdk_threads_enter();
	gtk_tree_view_set_model(
	GTK_TREE_VIEW(XPKG.gui.main_window.pkgreqbylist), GTK_TREE_MODEL(store));
	
	UNREF_OBJ(store);
	UNREF_OBJ(pkg_pbuf);
	UNREF_OBJ(missing_pbuf);
	UNREF_OBJ(wrong_pbuf);
	UNREF_OBJ(ok_pbuf);
	
	asprintf(&str, "Information for '%s' loaded.", pkg_n);
	xpkg_print_to_status_bar(str);
	free(str);
	gdk_threads_leave();	
	
	return NULL;
}

void xpkg_display_a_pkg_info_wrapper(char *pkg_n){
	GThread *g_t;
	
	if(!XPKG.pkg_a_info_load_l){
		g_t = g_thread_create(xpkg_display_a_pkg_info, 
		(void *)pkg_n, FALSE, (void *)pkg_n);
		assert(g_t != NULL);
		XPKG.pkg_a_info_load_l = FALSE;
	}
}

void *xpkg_display_pkg_info(void *data){
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTextBuffer *tbuffer;
	GtkWidget *icon;
	GdkPixbuf *pkg_pbuf = NULL,
	          *ok_pbuf = NULL,
	          *missing_pbuf = NULL,
	          *wrong_pbuf = NULL,
	          *file_pbuf = NULL,
	          *pbuf = NULL;
	gchar **pkg_s,
	      *ptr,
	      *pkg_n = data,
	      *origin,
	      *descr_f_path,
		  *msg;
	int fd, i;
	size_t size;
	Pkg *pkgdeps;
	gboolean installed,
	         v_mmatch = FALSE;
	xpkg_cache_pkg_ht pkg_ht;
	
	gdk_threads_enter();
	gtk_widget_show(XPKG.gui.main_window.atoolbar);
	gtk_widget_hide(XPKG.gui.main_window.atoolbar2);
	
	gtk_widget_hide(XPKG.gui.main_window.install_lbl);
	gtk_widget_hide(XPKG.gui.main_window.install_lbl_s);
	gdk_threads_leave();
	
	pkg_s = xpkg_pkg_n_split(pkg_n);
	
	asprintf(&ptr, "Loading package '%s' information...", pkg_n);
	gdk_threads_enter();
	xpkg_print_to_status_bar(ptr);
	gdk_threads_leave();
	free(ptr);
	
	gdk_threads_enter();
	gtk_label_set_markup(GTK_LABEL(XPKG.gui.main_window.indi_lbl),
	" <b>Size on disk:</b> ");
	
	gtk_label_set_text(GTK_LABEL(XPKG.gui.main_window.name_lbl),
	pkg_s[I_PKG_NAME]);
	
	gtk_label_set_text(GTK_LABEL(XPKG.gui.main_window.version_lbl),
	pkg_s[I_PKG_VERSION]);
	gdk_threads_leave();
	
	xpkg_pkg_s_free(pkg_s);
	
	pkg_ht = g_hash_table_lookup(XPKG.installed_pkgs.cache_hts.pkg_ht,
	(gconstpointer)pkg_n);
	
	if(pkg_ht == NULL){
		xpkg_error("Failed to access cached data fo '%s'!", pkg_n);
		pthread_exit(NULL);
	}
	
	XPKG.selected_pkg = pkg_ht -> pkg_s;
	
	origin = portb2origin(pkg_ht -> pkg_o);
	
	gdk_threads_enter();
	gtk_label_set_text(GTK_LABEL(XPKG.gui.main_window.category_lbl), origin);
	gdk_threads_leave();
	
	free(origin);
	
	descr_f_path = g_strdup_printf("%s%s/%s/+DESC", 
	XPKG.config -> pkg_base, 
	X_PKG_DB_PREFIX, pkg_n);
	assert(descr_f_path != NULL);
	
	fd = open(descr_f_path, O_RDONLY);
	g_free(descr_f_path);
	
	if(fd == -1){
		gdk_threads_enter();
		xpkg_error("Failed opening package description: %s", 
		strerror(errno));
		gdk_threads_leave();
		return NULL;
	}
	
	ptr = xpkg_mmap_fd(fd, &size);
	close(fd);
	
	gdk_threads_enter();
	tbuffer = gtk_text_view_get_buffer(
	GTK_TEXT_VIEW(XPKG.gui.main_window.description_view));
	
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(tbuffer), ptr, size);
	gdk_threads_leave();
	
	munmap(ptr, size);
	
	/*pbufs init*/
	pkg_pbuf = gdk_pixbuf_new_from_xpm_data((void *)&packages_xpm);
	
	icon = gtk_image_new();
	pbuf = gtk_widget_render_icon(icon, GTK_STOCK_CANCEL, 
           GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	missing_pbuf = gdk_pixbuf_scale_simple(pbuf, 
		           20, 20, GDK_INTERP_BILINEAR);
	g_object_unref(pbuf);
	pbuf = gtk_widget_render_icon(icon, GTK_STOCK_APPLY, 
	       GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	ok_pbuf = gdk_pixbuf_scale_simple(pbuf, 
		      20, 20, GDK_INTERP_BILINEAR);
	g_object_unref(pbuf);
	pbuf = gtk_widget_render_icon(icon, GTK_STOCK_DIALOG_WARNING, 
	       GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	wrong_pbuf = gdk_pixbuf_scale_simple(pbuf, 
		         20, 20, GDK_INTERP_BILINEAR);
	g_object_unref(pbuf); pbuf = NULL;
	gtk_widget_destroy(icon);
	
	/*pkg deps list*/
	gdk_threads_enter();
	store = gtk_list_store_new(
	4, GDK_TYPE_PIXBUF, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gdk_threads_leave();
	
	pkgdeps = pkg_get_dependencies(XPKG.selected_pkg);
	for(i = 0; pkgdeps[i] != NULL; i++){
		ptr = (char *)pkg_get_name(pkgdeps[i]);
		
		pkg_ht = g_hash_table_lookup(XPKG.installed_pkgs.cache_hts.pkg_ht,
		(gconstpointer)ptr);
		
		if(pkg_ht != NULL){
			installed = TRUE;
		} else {
			installed = FALSE;
			
			pkg_s = xpkg_pkg_n_split(ptr);
			
			if(g_hash_table_lookup(XPKG.installed_pkgs.cache_hts.ver_ht,
			(gconstpointer)pkg_s[I_PKG_NAME]) != NULL){
				v_mmatch = TRUE;
			} else
				v_mmatch = FALSE;
		
			xpkg_pkg_s_free(pkg_s);
		}

		if(!installed){
			if(v_mmatch){
				msg = "Different version";
				pbuf = wrong_pbuf;
			}
			else{
				msg = "Missing";
				pbuf = missing_pbuf;
			}
		}
		else{
			msg = "OK";
			pbuf = ok_pbuf;
		}
		gdk_threads_enter();
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, pkg_pbuf, 1, ptr, 
		2, pbuf, 3, msg, -1);
		gdk_threads_leave();
	}
	
	pkg_free_list(pkgdeps);
	
	gdk_threads_enter();
	gtk_tree_view_set_model(
	GTK_TREE_VIEW(XPKG.gui.main_window.pkgdeplist), GTK_TREE_MODEL(store));

	g_object_unref(store);
	gdk_threads_leave();
	
	/*pkg reqby list*/
	gdk_threads_enter();
	store = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gdk_threads_leave();
	
	pkgdeps = pkg_get_rdependencies(XPKG.selected_pkg);
	
	if(pkgdeps != NULL)
	for(i = 0; pkgdeps[i] != NULL; i++){
		ptr = (char *)pkg_get_name(pkgdeps[i]);
		gdk_threads_enter();
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, pkg_pbuf, 1, ptr, -1);
		gdk_threads_leave();
	}
	
	if(pkgdeps != NULL)
	pkg_free_list(pkgdeps);
	
	gdk_threads_enter();
	gtk_tree_view_set_model(
	GTK_TREE_VIEW(XPKG.gui.main_window.pkgreqbylist), GTK_TREE_MODEL(store));
	
	g_object_unref(store);
	gdk_threads_leave();
	
	/*pkg file list*/
	PkgContentFile *con_files;
	char *stat_s = NULL;
	size_t fsize = 0,
	       tsize = 0;
	struct stat f_stat;
	
	gdk_threads_enter();
	store = gtk_list_store_new(5, GDK_TYPE_PIXBUF, G_TYPE_STRING,
	GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
	file_pbuf = gdk_pixbuf_new_from_xpm_data((void *)&file_xpm);
	gdk_threads_leave();
	
	con_files = pkg_get_content_files(XPKG.selected_pkg);
	assert(con_files != NULL);
	
	for(i = 0; con_files[i] != NULL; i++){
		if(stat(con_files[i]->file, &f_stat) == -1){
			stat_s = "Missing";
			pbuf = missing_pbuf;
			
			fsize = 0;
		}
		else{
			stat_s = "OK";
			pbuf = ok_pbuf;
			
			fsize = f_stat.st_size;
			tsize += fsize;
		}
		
		gdk_threads_enter();
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
		0, file_pbuf, 
		1, con_files[i]->file,
		2, pbuf,
		3, stat_s,
		4, ptr = print_str_size(fsize), -1);
		gdk_threads_leave();
		
		if(ptr != NULL)
			free(ptr);
	}
	
	ptr = print_str_size(tsize);
	
	gdk_threads_enter();
	gtk_label_set_text(GTK_LABEL(XPKG.gui.main_window.size_lbl), ptr);
	gdk_threads_leave();
	
	if(ptr != NULL)
		free(ptr);
	
	pkg_free_content_file_list(con_files);
	
	gdk_threads_enter();
	gtk_tree_view_set_model(
	GTK_TREE_VIEW(XPKG.gui.main_window.pkgfilelist), GTK_TREE_MODEL(store));
	gdk_threads_leave();
	
	asprintf(&ptr, "Information for '%s' loaded.", pkg_n);
	gdk_threads_enter();
	xpkg_print_to_status_bar(ptr);
	gdk_threads_leave();
	free(ptr);
	
	gdk_threads_enter();
	UNREF_OBJ(store);
	UNREF_OBJ(pkg_pbuf);
	UNREF_OBJ(file_pbuf);
	UNREF_OBJ(missing_pbuf);
	UNREF_OBJ(wrong_pbuf);
	UNREF_OBJ(ok_pbuf);
	gdk_threads_leave();
	
	return NULL;
}

void xpkg_display_pkg_info_wrapper(char *pkg_n){
	GThread *g_t;
	
	if(!XPKG.pkg_info_load_l){
		XPKG.pkg_info_load_l = FALSE;
		g_t = g_thread_create(xpkg_display_pkg_info, 
		(void *)pkg_n, FALSE, NULL);
		assert(g_t != NULL);
	}
}

void xpkg_search_pkg_list(gchar *s_str){
	GtkTreeModel *model;
	GtkTreeIter  iter,
	             iter_c;
	GtkTreePath *path;
	GtkWidget *list = NULL;
	gchar *pkg_n;
	gint c, i,
	     len1, len2;
	gboolean rval;
	
	switch(XPKG.cont_t_state){
		case T_INSTALLED_STATE:
			list = XPKG.gui.main_window.pkglist;
		break;
		case T_AVAILABLE_STATE:
			list = XPKG.gui.main_window.a_pkglist;
		break;
	}	

	model = gtk_tree_view_get_model(
	GTK_TREE_VIEW(list));
	
	if(model == NULL)
		return;
		
	rval = gtk_tree_model_get_iter_first(model, &iter);
	
	while(rval){
		c = gtk_tree_model_iter_n_children(model, &iter);
		
		for(i = 0; i != c; i++){
			if(gtk_tree_model_iter_nth_child(model, &iter_c, &iter, i)){
				gtk_tree_model_get(model, &iter_c, 1, &pkg_n, -1);
				len1 = strlen(s_str);
				len2 = strlen(pkg_n);
				if((len1 == len2) && !strcmp(s_str, pkg_n)){
					path = gtk_tree_model_get_path(model, &iter);
					gtk_tree_view_expand_row(GTK_TREE_VIEW(list), path, TRUE);
					gtk_tree_path_free(path);
					
					path = gtk_tree_model_get_path(model, &iter_c);
					gtk_tree_view_set_cursor(
					GTK_TREE_VIEW(list),
					path, NULL, FALSE);
					gtk_tree_path_free(path);
				}
				g_free(pkg_n);
			}
		}
				
		rval = gtk_tree_model_iter_next(model, &iter);
	}
}

void xpkg_get_cache_ht(xpkg_cache_ht *cache){
	Pkg *pkgs = XPKG.installed_pkgs.pkgs, *rdeps;
	xpkg_cache_pkg_ht c_pkg_ht;
	gchar *pkg_key, **ps;
	GPtrArray *vers;
	u_int i;
	
	cache -> pkg_ht = g_hash_table_new_full(g_str_hash, g_str_equal,
	g_free, xpkg_free_pkg_ht);
	cache -> ver_ht = g_hash_table_new_full(g_str_hash, g_str_equal,
	g_free, xpkg_free_array);
	cache -> org_ht = g_hash_table_new(g_str_hash, g_str_equal);
	
	for(i = 0; pkgs[i] != NULL; i++){
		pkg_key = g_strdup(pkg_get_name(pkgs[i]));
		
		c_pkg_ht = malloc(sizeof(struct xpkg_cache_pkg_ht_str));
		assert(c_pkg_ht != NULL);
		
		memset(c_pkg_ht, 0, sizeof(struct xpkg_cache_pkg_ht_str));
		
		c_pkg_ht -> pkg_o = pkg_get_origin(pkgs[i]);
		c_pkg_ht -> pkg_s = pkgs[i];

		rdeps = pkg_get_rdependencies(pkgs[i]);
		
		if(!rdeps || !rdeps[0])
			c_pkg_ht -> flags = c_pkg_ht -> flags | F_IS_LEAF;
		
		pkg_free_list(rdeps);

		if(c_pkg_ht -> pkg_o == NULL)
			xpkg_error("Failed to get origin for %s!", pkg_get_name(pkgs[i]));
		
		g_hash_table_insert(cache -> pkg_ht, pkg_key, c_pkg_ht);
		g_hash_table_insert(cache -> org_ht, c_pkg_ht -> pkg_o, c_pkg_ht);
		
		ps = xpkg_pkg_n_split(pkg_key);
		vers = g_hash_table_lookup(cache -> ver_ht, 
		(gconstpointer)ps[I_PKG_NAME]);
		
		if(vers == NULL){
			vers = g_ptr_array_new_with_free_func(g_free);
			g_ptr_array_add(vers, g_strdup(ps[I_PKG_VERSION]));
			
			g_hash_table_insert(cache -> ver_ht, g_strdup(ps[I_PKG_NAME]), vers);
		} else
			g_ptr_array_add(vers, g_strdup(ps[I_PKG_VERSION]));
		
		xpkg_pkg_s_free(ps);
	}
/*	TESTING CODE
	u_int ii;
	for(i = 0; pkgs[i] != NULL; i++){
		c_pkg_ht = g_hash_table_lookup(cache -> pkg_ht,
		(gconstpointer)pkg_get_name(pkgs[i]));
		
		if(c_pkg_ht == NULL){
			printf("FAIL\n");
		} else
			printf("'%s':'%s'\n", pkg_get_name(pkgs[i]), c_pkg_ht -> pkg_o);
			
		ps = xpkg_pkg_n_split((char *)pkg_get_name(pkgs[i]));
		
		vers = g_hash_table_lookup(cache -> ver_ht, 
		(gconstpointer)ps[I_PKG_NAME]);
		
		if(vers == NULL){
			printf("FAIL\n");
		} else {
			printf("'%s':", ps[I_PKG_NAME]);
			for(ii = 0; ii != vers -> len; ii++)
				printf("'%s'", g_ptr_array_index(vers, ii));
			printf("\n");			
		}
		
		xpkg_pkg_s_free(ps);
	}
*/
}

#define R_ERROR_CHECK(_VAL_) if(_VAL_ == NULL){\
gdk_threads_enter();\
xpkg_error("Failed opening archive!\n%s", pkg_get_error());\
gdk_threads_leave();\
xpkg_archive_info_free(a_info);\
return NULL;}

xpkg_archive_info xpkg_get_archive_info(char *a_path){
	xpkg_archive_info a_info;
	char *val, **aval;
	
	if(a_path == NULL)
		return NULL;
		
	a_info = (void *) malloc(sizeof(struct xpkg_archive_info_str));
	assert(a_info != NULL);
	memset(a_info, 0, sizeof(struct xpkg_archive_info_str));
	
	a_info->path = strdup(a_path);
	assert(a_info->path != NULL);
	
	val = (char *) pkg_archive_get_pkg_name(a_path);
	R_ERROR_CHECK(val);
	
	a_info->full_name = val;
	
	aval = xpkg_pkg_n_split(val);
	
	a_info->name = aval[I_PKG_NAME];
	a_info->version = aval[I_PKG_VERSION];
	
	free(aval);
	
	a_info->size = pkg_archive_get_size(a_path);
	
	aval = pkg_archive_get_values(a_path, T_DEPS);
	
	R_ERROR_CHECK(aval);	
	a_info->deps = (gchar **)aval;
	
	aval = pkg_archive_get_values(a_path, T_CONFLICTS);
	
	R_ERROR_CHECK(val);	
	a_info->conflicts = (gchar **)aval;	
	a_info->f_install = FALSE;
	
	return a_info;
}

#undef R_ERROR_CHECK

gboolean xpkg_pkg_exists(gchar *pkg_n){	
	if(!g_hash_table_lookup(XPKG.installed_pkgs.cache_hts.pkg_ht,
	(gconstpointer)pkg_n))
		return FALSE;

	return TRUE;
}
