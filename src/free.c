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

void xpkg_free_jobs(xpkgJob *jobs, gboolean strfree){
	u_int a;
	
	if(jobs == NULL)
		return;
	
	if(strfree)
		for(a = 0; jobs[a].name != NULL; a++){
			free(jobs[a].name);
			free(jobs[a].data);
		}
	
	free(jobs);
}

void xpkg_otable_free(struct xpkg_origin_table_s *o){
	u_int a;
	
	for(a = 0; o[a].name != NULL; a++){
		UNREF_OBJ(o[a].pbuf);
		free(o[a].name);
	}
	
	free(o);
}

void xpkg_free_config(struct xpkg_config_str *config){
	free((void *)config -> pkg_base);
	free((void *)config -> ports_base);
	free((void *)config -> repo_url);
	free((void *)config -> cache_db);
	free((void *)config -> cache_d);
	free(config);
}

void xpkg_cleanup(xpkg XPKG){
	xpkg_free_config(XPKG.config);
	xpkg_installed_pkg_str_free(XPKG.installed_pkgs);
}

void xpkg_index_d_free(struct dirent **index_d, int i){
	u_int a;
	
	if(index_d == NULL || i < 1)
		return;
	
	for(a = 0; a != i; a++)
		free(index_d[a]);
	
	free(index_d);
}

void xpkg_installed_pkg_str_free(struct xpkg_installed_pkg_str ipkgs){

	/*cache*/
	xpkg_free_xpkg_cache_ht(&(ipkgs.cache_hts));
	
	/*libpkg*/
	if(ipkgs.pkgs != NULL)
		pkg_free_list(ipkgs.pkgs);
	if(ipkgs.db != NULL)
		pkg_db_close(ipkgs.db);
}

void xpkg_pkg_s_free(char **pkg_s){
	free(pkg_s[I_PKG_NAME]);
	free(pkg_s[I_PKG_VERSION]);
	
	free(pkg_s);
}

void xpkg_archive_info_free(xpkg_archive_info a_info){
	if(a_info == NULL)
		return;
	
	free(a_info->name);
	free(a_info->full_name);
	free(a_info->version);
	free(a_info->path);
	
	if(a_info->deps != NULL){
		for(int i = 0; a_info->deps[i] != NULL; i++)
			free(a_info->deps[i]);
			
		free(a_info->deps);
	}
	
	if(a_info->conflicts != NULL){
		for(int i = 0; a_info->conflicts[i] != NULL; i++)
			free(a_info->conflicts[i]);
			
		free(a_info->conflicts);
	}
	
	free(a_info);

	a_info = NULL;
}

void xpkg_free_fi_heap(void *data){
	FInstallHeapBlock *fi_h;
	
	if(DB_H != NULL)
		DB_CLOSE;
	
	fi_h = data;
	
	pthread_mutex_destroy(fi_h -> p_m);

	UNREF_OBJ(fi_h -> pbuf_done);
	UNREF_OBJ(fi_h -> pbuf_ndone);
	UNREF_OBJ(fi_h -> pbuf_cur);
	UNREF_OBJ(fi_h -> pbuf_bxpkg);
	UNREF_OBJ(fi_h -> pbuf_net);
	UNREF_OBJ(fi_h -> pbuf_file);
	UNREF_OBJ(fi_h -> pbuf_text);
	UNREF_OBJ(fi_h -> pbuf_script);
	UNREF_OBJ(fi_h -> pbuf_warning);
	
	free(fi_h -> origin);
	free(fi_h -> url);	
	free(fi_h -> path);
	
	if(fi_h -> jobs != NULL)
		xpkg_free_jobs(fi_h -> jobs, TRUE);
	
	memset(fi_h, 0, sizeof(FInstallHeapBlock));
	
	return;
}

void xpkg_free_search_heap(void *data){
	SearchHeapBlock *s_h;
	
	if(DB_H != NULL)
		DB_CLOSE;
	
	s_h = data;
	
	UNREF_OBJ(s_h -> pbuf);
	
	if(s_h -> pfile != NULL)
		pkg_file_close(s_h -> pfile);
		
	if(s_h -> c_files != NULL)
		pkg_free_control_files(s_h -> c_files);
		
	free(s_h -> path);
	free(s_h -> ptr);
	
	memset(s_h, 0, sizeof(SearchHeapBlock));
	return;
}

void xpkg_free_update(gpointer data){
	struct xpkg_update *u = data;
	
	if(u != NULL){
		free(u -> pkg_name);
		free(u -> new_ver);
		
		free(u);
	}	
}

void xpkg_free_pkg_ht(gpointer data){
	xpkg_cache_pkg_ht c_pkg_ht = data;	
	
	if(c_pkg_ht != NULL)
		free(c_pkg_ht -> pkg_o);
	
	free(c_pkg_ht);
}

void xpkg_free_array(gpointer data){
	if(data)
		g_ptr_array_free(data, TRUE);
}

void xpkg_free_xpkg_cache_ht(xpkg_cache_ht *cache_hts){
	g_hash_table_destroy(cache_hts -> pkg_ht);
	g_hash_table_destroy(cache_hts -> ver_ht);
	g_hash_table_destroy(cache_hts -> org_ht);
	
	memset(cache_hts, 0, sizeof(xpkg_cache_ht));
}
