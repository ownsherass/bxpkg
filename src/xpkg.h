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
#ifndef _XPKG_H_
#define	_XPKG_H_

#include <sys/types.h>
#include <sys/param.h>
#include <sys/limits.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <fetch.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <security/pam_appl.h>
#include <security/openpam.h>

#include "pkg.h"
#include "gui.h"

extern pid_t wait(int *);

#define X_PKG_VERSION "0.0.4_0"

#define DEF_REMOTE_REPO_SITE "ftp://ftp.freebsd.org"
#define DEF_REMOTE_REPO_PATH "/pub/FreeBSD/ports"

/*internal params*/
#define ZERO_DEV        "/dev/zero"
#define CACHE_D         "/var/db/bxpkg"
#define TMP_D           "/tmp"
#define DIST_D          "/usr/ports/distfiles/bxpkg"
#define X_PKG_DB_PREFIX "/var/db/pkg"
#define MAX_PASS_LEN  1024
#define MSG_LEN       1024

enum {
	MMAP_DST,
	MMAP_SRC,
	MMAP_FIELDS
};

enum {
	I_PKG_NAME,
	I_PKG_VERSION,
	I_PKG_FIELDS
};

enum {
	T_INSTALLED_STATE,
	T_AVAILABLE_STATE
};

struct rel_table {
	const gchar *r, *l;
};

extern struct rel_table r_t[];

/*packages cache*/
struct xpkg_cache_ht {
	GHashTable *pkg_ht,
	           *ver_ht,
	           *org_ht;
};
typedef struct xpkg_cache_ht xpkg_cache_ht;

#define F_IS_LEAF 0x01

struct xpkg_cache_pkg_ht_str {
	gchar *pkg_o;
	Pkg pkg_s;
	int8_t flags;
};
typedef struct xpkg_cache_pkg_ht_str *xpkg_cache_pkg_ht;

/*configuration*/
struct xpkg_config_str{
	/*flags*/
	gboolean root_f;
	
	/*options*/
	gchar *pkg_base,
		 *ports_base,
		 *repo_url,
		 *cache_d,
		 *cache_db;
};

/*data for installed packages*/
struct xpkg_installed_pkg_str{
	PkgDb db;
	Pkg* pkgs;
	xpkg_cache_ht cache_hts;
	
	GtkTreeModel *model;
};

struct xpkg_origin_table_s{
	gchar *name;
	GtkTreeIter iter;
	GdkPixbuf *pbuf;
};

/*data for available packages*/
struct xpkg_available_pkg_str{
	DB *dbp;
	struct xpkg_origin_table_s *origin_table;
	int origin_table_l;
	
	GtkTreeModel *model;
};

struct xpkg_main_str{
	/*members*/
	struct xpkg_config_str        *config;
	struct xpkg_gui_str           gui;
	struct xpkg_installed_pkg_str installed_pkgs;
	struct xpkg_available_pkg_str available_pkgs;

	/*data*/
	char *shmem;
	Pkg selected_pkg;

	int argc;
	char **argv;
	
	/*content type state*/
	guint cont_t_state;

	/*flags*/
	gboolean gtk_init_f, /*has been gtk_init() called?*/
	         gtk_loop_f, /*are we in a gtk_main() loop?*/
	         ins_deps_f, /*all dependencies are installed?*/
	         ins_cnfl_f, /*conflicts exists?*/
	         rm_cached_f, /*remote index cached?*/
	         filter_leafs_f; /*leaf filter*/
	
	/*locks*/
	gboolean pkg_info_load_l,
	         pkg_a_info_load_l,
	         pkg_search_l,
	         pkg_delete_l,
	         pkg_install_l,
	         pkg_install_fetch_l;
};
typedef struct xpkg_main_str xpkg;

struct xpkg_update{
	gchar *pkg_name,
	      *new_ver;
};

/*job types*/
enum {
	J_GET,
	J_CH_VERSION,
	J_DELETE,
	J_INSTALL
};

struct xpkg_job_str{
	gchar *name, *data;
	u_short type, priority;
};
typedef struct xpkg_job_str xpkgJob;

struct xpkg_fetch_install_heap_block{
	GdkPixbuf *pbuf,      *pbuf_ndone,
	          *pbuf_done, *pbuf_cur,
	          *pbuf_bxpkg,*pbuf_net,
	          *pbuf_file, *pbuf_text,
	          *pbuf_script, *pbuf_warning;
	xpkgJob *jobs;
	gchar *origin, *url, *path;
	pthread_mutex_t *p_m;
};
typedef struct xpkg_fetch_install_heap_block FInstallHeapBlock;

struct xpkg_search_heap_block{
	GdkPixbuf *pbuf;
	PkgFile pfile;
	gchar **c_files, *path, *ptr;
};
typedef struct xpkg_search_heap_block SearchHeapBlock;

#define XPKG_FETCH_CALLBACK(_FNAME) \
void (*_FNAME)( \
size_t done, size_t total, gdouble fraction, gchar *file \
)

#define DB_H XPKG.available_pkgs.dbp

#define DB_OPEN \
	XPKG.available_pkgs.dbp = \
	dbopen(XPKG.config -> cache_db, O_RDONLY, 0, DB_BTREE, NULL); \
	assert(XPKG.available_pkgs.dbp != NULL)

#define DB_CLOSE \
	{ if(XPKG.available_pkgs.dbp != NULL)                        \
	  XPKG.available_pkgs.dbp -> close(XPKG.available_pkgs.dbp); \
	  XPKG.available_pkgs.dbp = NULL; }
	
#define UNREF_OBJ(_OBJ) \
	{ if(_OBJ != NULL && G_IS_OBJECT(_OBJ)){g_object_unref(_OBJ); _OBJ = NULL;} }

extern xpkg XPKG;

/*misc.c*/
GPtrArray *xpkg_get_updates(void);
void xpkg_error(char *, ...);
void xpkg_error_secure_gui(char *, ...);
int pamconv(int , const struct pam_message **, 
struct pam_response **, void *);
void *xpkg_mmap_fd(int, size_t *);
void xpkg_init_pkg_db(void);
int xpkg_ch_path(const char *);
void *xpkg_display_pkg_info(void *);
void xpkg_get_cache_ht(xpkg_cache_ht *);
void xpkg_display_pkg_info_wrapper(char *);
void xpkg_display_a_pkg_info_wrapper(char *);
void xpkg_search_pkg_list(gchar *);
xpkg_archive_info xpkg_get_archive_info(char *);
char *print_str_size(size_t);
gboolean xpkg_pkg_exists(gchar *);
gchar *portb2origin(gchar *);
xpkgJob * xpkg_get_jobs(gchar *, xpkgJob *, u_short);
char *next_word(char *, size_t *);
char ** xpkg_pkg_n_split(char *);
xpkgJob * xpkg_short_jobs_by_priority(xpkgJob *);
xpkgJob * xpkg_select_job_by_name(gchar *, xpkgJob *);
gboolean xpkg_get_pie(gchar *, PkgIdxEntry *);
xpkgJob * xpkg_get_remove_jobs(Pkg, gboolean, xpkgJob *);
xpkgJob * xpkg_get_archive_jobs(xpkg_archive_info, xpkgJob *);
xpkgJob * xpkg_get_update_jobs(GPtrArray *);
xpkgJob * xpkg_append_job(xpkgJob *, xpkgJob);

/*search.c*/
void *xpkg_init_search(void);
void *xpkg_search_t(void *);

/*index.c*/
void xpkg_read_index(void);
void xpkg_init_index(void);

/*free.c*/
void xpkg_otable_free(struct xpkg_origin_table_s *);
void xpkg_free_config(struct xpkg_config_str *);
void xpkg_cleanup(xpkg);
void xpkg_index_d_free(struct dirent **, int);
void xpkg_pkg_s_free(char **);
void xpkg_installed_pkg_str_free(struct xpkg_installed_pkg_str);
void xpkg_archive_info_free(xpkg_archive_info);
void xpkg_free_jobs(xpkgJob *, gboolean);
void xpkg_free_fi_heap(void *);
void xpkg_free_search_heap(void *);
void xpkg_free_pkg_ht(gpointer);
void xpkg_free_array(gpointer);
void xpkg_free_xpkg_cache_ht(xpkg_cache_ht *);
void xpkg_free_update(gpointer);

/*fetch.c*/
void xpkg_fetch_index_md5_init(void (*callback)(void));
off_t xpkg_fetch_package_size(gchar *, gchar *);
int xpkg_do_fetch(
const gchar *, const gchar *, XPKG_FETCH_CALLBACK(fetch_callback));
gchar *xpkg_pkgn_to_url(gchar *, gchar *);
off_t xpkg_fetch_file_size(char *);

#endif
