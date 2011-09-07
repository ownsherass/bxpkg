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

SearchHeapBlock s_heap;

void xpkg_append_search_result(Pkg pkg, gchar *pkg_s){
	GtkListStore *store;
	GtkTreeIter iter;
	
	store = (GtkListStore *)gtk_tree_view_get_model(
	GTK_TREE_VIEW(XPKG.gui.search_dlg.list));
	
	if(store == NULL)
		return;
	
	gtk_list_store_append(store, &iter);
	
	if(pkg != NULL){
		gtk_list_store_set(store, &iter, 0, s_heap.pbuf, 1, pkg->name, -1);
	} else 
		gtk_list_store_set(store, &iter, 0, s_heap.pbuf, 1, pkg_s, -1);
}

gboolean xpkg_search_string(char *name, const char *str){
	int len, len2, i;
	gboolean res = FALSE;
	    
	len2 = strlen(name);
	len = strlen(str);
	
	if(len2 < len)
		return res;
	
	if(len == len2){
		i = -1;
	} else i = 0;
		
	for(; i != (len2 - len); i++){
		if(!strncmp((i == -1) ? name : &name[i], str, len)){
			res = TRUE;
			break;
		}
	}

	return res;
}

gboolean xpkg_search_file(char *path, const char *str){
	const char *s_str;
	int i, len;
	gboolean res = FALSE;
	
	s_heap.pfile = pkg_file_open(path);
	
	if(s_heap.pfile == NULL)
		return FALSE;
		
	len = strlen(str);		
	s_str = s_heap.pfile->data;
	
	for(i = 0; i < (s_heap.pfile->f_stat.st_size - len); i++){
		if(!strncmp(&s_str[i], str, len)){
			res = TRUE;
			break;
		}
	}
	
	pkg_file_close(s_heap.pfile);
	
	s_heap.pfile = NULL;
	
	return res;
}

gboolean xpkg_search_control_file(
Pkg pkg, const char *str, char *cfile_n){
	gboolean rval;
	int i;
	
	s_heap.c_files = pkg_get_control_files(pkg);
		if(s_heap.c_files == NULL)
			return FALSE;
			
	for(i = 0; s_heap.c_files[i] != NULL; i++){
		if(!strcmp(s_heap.c_files[i], cfile_n))
			break;
	}
	
	if(s_heap.c_files[i] == NULL){
		pkg_free_control_files(s_heap.c_files);
		s_heap.c_files = NULL;
		
		return FALSE;
	}
		
	asprintf(&s_heap.path, "%s/%s", pkg->path_d, s_heap.c_files[i]);
	assert(s_heap.path != NULL);

	rval = xpkg_search_file(s_heap.path, str);
	
	free(s_heap.path);
	s_heap.path = NULL;
	
	pkg_free_control_files(s_heap.c_files);
	s_heap.c_files = NULL;
		
	return rval;
}

gboolean xpkg_search_comment(Pkg pkg, const char *str){
	return xpkg_search_control_file(pkg, str, C_F_COMMENT);
}

gboolean xpkg_search_description(Pkg pkg, const char *str){
	return xpkg_search_control_file(pkg, str, C_F_DESCR);
}

void *xpkg_search_t(void *data){
	xpkg_init_search();
	
	if(XPKG.pkg_search_l){
		gdk_threads_enter();
		gtk_widget_set_sensitive(XPKG.gui.search_dlg.clear_button, TRUE);
		gdk_threads_leave();
	}
	
	return NULL;
}

void *xpkg_init_search(void){
	int count, rv,
	    i, len;
	const gchar *str;
	gboolean found_f = FALSE;
	xpkg_search_flags s_flags;
	gdouble fraction;
	unsigned short int first = 1;
	PkgIdxEntry pie;
	u_int entries;
	
	memset(&s_heap, 0, sizeof(s_heap));
	
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &rv);
	pthread_cleanup_push(xpkg_free_search_heap, &s_heap);
	
	gdk_threads_enter();
	xpkg_set_search_flags(&s_flags);
	str = xpkg_get_search_dialog_input();
	
	s_heap.pbuf = gdk_pixbuf_new_from_xpm_data((void *)&packages_xpm);
	gdk_threads_leave();
	
	len = strlen(str);
	
	if(!len)
		pthread_exit(NULL);

	if(XPKG.cont_t_state == T_AVAILABLE_STATE){
		DB_OPEN;
		entries = pkg_get_cached_entries(XPKG.available_pkgs.dbp);
		DB_CLOSE;
		
		i = 0;
		DB_OPEN;
		while((rv = pkg_get_cached_entry(XPKG.available_pkgs.dbp, 
		&pie, &first)) > 0){
			if(rv == 1){
				i++;
				/*name search*/
				if(s_flags.name_t_f){
					if(xpkg_search_string(pie.name, str)){
						gdk_threads_enter();
						asprintf(&s_heap.ptr, "%s-%s", pie.name, pie.version);
						assert(s_heap.ptr != NULL);
						xpkg_append_search_result(NULL, s_heap.ptr);
						
						free(s_heap.ptr);
						s_heap.ptr = NULL;
						
						gdk_threads_leave();
						found_f = TRUE;
					}
				}
				
				/*name description*/
				if(s_flags.descr_t_f){
					if(xpkg_search_string(pie.descr, str)){
						gdk_threads_enter();
						asprintf(&s_heap.ptr, "%s-%s", pie.name, pie.version);
						assert(s_heap.ptr != NULL);
						xpkg_append_search_result(NULL, s_heap.ptr);
						
						free(s_heap.ptr);
						s_heap.ptr = NULL;
						
						gdk_threads_leave();
						found_f = TRUE;
					}
				}
				
				gdk_threads_enter();
				fraction = (float)i / entries;
				xpkg_set_search_progress_fraction(fraction);
				gdk_threads_leave();
			}
		}
		DB_CLOSE;
		
		gdk_threads_enter();
		gtk_widget_set_sensitive(XPKG.gui.search_dlg.clear_button, TRUE);
		xpkg_set_search_progress_fraction(0);
		gdk_threads_leave();
		
		pthread_exit(NULL);
	}
	
	Pkg *pkgs = XPKG.installed_pkgs.pkgs;
	
	for(count = 0; pkgs[count] != NULL; count++);
	
	for(i = 0; i != count; i++){
		
		gdk_threads_enter();
		fraction = (float)(i + 1) / count;
		xpkg_set_search_progress_fraction(fraction);
		gdk_threads_leave();
		
		/*name search*/
		if(s_flags.name_t_f){
			if(xpkg_search_string((char *)pkg_get_name(pkgs[i]), str)){
				gdk_threads_enter();
				xpkg_append_search_result(pkgs[i], NULL);
				gdk_threads_leave();
				found_f = TRUE;
			}
		}
		/*comment search*/
		if(s_flags.descr_t_f && !found_f){
			if(xpkg_search_comment(pkgs[i], str)){
				gdk_threads_enter();
				xpkg_append_search_result(pkgs[i], NULL);
				gdk_threads_leave();
				found_f = TRUE;
			}
		}
		/*description search*/
		if(s_flags.descr_t_f && !found_f){
			if(xpkg_search_description(pkgs[i], str)){
				gdk_threads_enter();
				xpkg_append_search_result(pkgs[i], NULL);
				gdk_threads_leave();
				found_f = TRUE;
			}
		}
		found_f = FALSE;
	}
	
	gdk_threads_enter();
	xpkg_set_search_progress_fraction(0);
	gdk_threads_leave();
	
	pthread_cleanup_pop(0);
	
	return NULL;
}
