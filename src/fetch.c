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

/*libpkg, not in header*/
extern short mkdir_tree(char *, mode_t);

#define BUFSIZE	4096

int xpkg_parse_md5_file(gchar *path, gchar *buf){
	FILE *f;
	size_t s = (MD5HASHBUFLEN - 1);
	
	f = fopen(path, "r");
	if(f == NULL)
		return 0;
		
	if(fseek(f, -MD5HASHBUFLEN, SEEK_END) == -1){
		fclose(f);

		return 0;
	}
	
	if(fread(buf, 1, s, f) != s){
		fclose(f);
		
		return 0;
	}
	
	buf[s] = '\0';
	
	fclose(f);	
	return 1;
}

int xpkg_extract(const gchar *path, const gchar *lpath){
	struct archive *a;
	struct archive_entry *e;
	gint rval, fd = 0;
	
	if((path == NULL || path[0] == '\0') ||
	   (lpath == NULL || lpath[0] == '\0'))
		return 0;
		
	a = archive_read_new();
	archive_read_support_compression_bzip2(a);
	archive_read_support_format_raw(a);
	
	if((rval = archive_read_open_file(a, path, 10240)))
		return 0;
		
	rval = archive_read_next_header(a, &e);
	
	if(rval != ARCHIVE_OK)
		goto fail;
		
	fd = open(lpath, O_CREAT | O_WRONLY, DEF_O_MODE);
	
	if(fd == -1)
		goto fail;
	
	if(archive_read_data_into_fd(a, fd) == -1)
		goto fail;
	close(fd);
	
	archive_read_close(a);
	archive_read_finish(a);
	
	return 1;
	
	fail:
	if(fd != 0)
		close(fd);
	
	archive_read_close(a);
	archive_read_finish(a);
	
	return 0;
}

void xpkg_do_fetch_check(gint rv, gchar *URL){
	switch(rv){
		case 0:
			gdk_threads_enter();
			xpkg_error("Error while fetching!\n"
			"Error code: %d (%s)\nURL: %s", fetchLastErrCode, fetchLastErrString, URL);
			gdk_threads_leave();
			return;
		break;
		case -1:
			gdk_threads_enter();
			xpkg_error("%s\nURL: '%s'", strerror(errno), URL);
			gdk_threads_leave();
			return;
		break;
		case -2:
			gdk_threads_enter();
			xpkg_error("URL missing!");
			gdk_threads_leave();
			return;
		break;
		case -3:
			gdk_threads_enter();
			xpkg_error("File received is smaller than expected!\nURL: '%s'", URL);
			gdk_threads_leave();
			return;
		break;
	}
}

void xpkg_fetch_report(
size_t done, size_t total, gdouble fraction, gchar *file){
	gdk_threads_enter();
	gtk_progress_bar_set_fraction(
	GTK_PROGRESS_BAR(XPKG.gui.main_window.p_bar), fraction);
	gdk_threads_leave();
}

void *xpkg_fetch_init_t(void *data){
	struct utsname un;
	gchar *URL = NULL,
	      *path, 
	      *lpath = NULL;
	char md5_str[MD5HASHBUFLEN], md5_str2[MD5HASHBUFLEN], *ptr;
	const gchar *file;
	guint i, r;
	DB *dbp;
	
	void (*callback)(void) = data;
	
	/*fetching md5*/
	file = "INDEX.bz2.md5";
	
	uname(&un);
			
	if(XPKG.config -> repo_url == NULL){
		gdk_threads_enter();
		xpkg_error("Your system %s is unknown! Try updating " 
		"bxpkg or supply repository URL with a --repo-site option", un.release);
		gdk_threads_leave();
		
		return NULL;
	}
	
	URL = g_strdup_printf("%s/%s", XPKG.config -> repo_url, file);
	assert(URL != NULL);
	
	path = g_strdup_printf("%s/%s", TMP_D, file);
	assert(path != NULL);
	
	gdk_threads_enter();
	gtk_widget_show(XPKG.gui.main_window.p_bar);
	gtk_progress_bar_set_text(
	GTK_PROGRESS_BAR(XPKG.gui.main_window.p_bar),
	"Fetching md5");
	gdk_threads_leave();
	
	i = xpkg_do_fetch(URL, path, xpkg_fetch_report);
	
	if(i != 1){
		xpkg_do_fetch_check(i, URL);
		gdk_threads_enter();
		gtk_widget_set_sensitive(XPKG.gui.main_window.notebook_s, TRUE);
		gtk_widget_set_sensitive(XPKG.gui.main_window.body, TRUE);
		gdk_threads_leave();
		pthread_exit(NULL);
	}
	
	if(!xpkg_parse_md5_file(path, md5_str)){
		gdk_threads_enter();
		xpkg_error("Failed parsing md5 file!");
		gtk_widget_set_sensitive(XPKG.gui.main_window.notebook_s, TRUE);
		gtk_widget_set_sensitive(XPKG.gui.main_window.body, TRUE);
		gdk_threads_leave();
		pthread_exit(NULL);
	}
	
	/*checking if cache already exists and if so is it upto date?*/
	XPKG.available_pkgs.dbp =
	dbp = dbopen(XPKG.config -> cache_db, O_RDONLY, 0, DB_BTREE, NULL);
	
	if(dbp != NULL){
		if(pkg_get_cached_unit("+MD5", &ptr, dbp) == (MD5HASHBUFLEN - 1)){
			memmove(md5_str2, ptr, (MD5HASHBUFLEN - 1));
			md5_str2[MD5HASHBUFLEN - 1] = (char)0;
			
			if(!strcmp(md5_str, md5_str2)){
				if(pkg_get_cached_unit("+VERSION", &ptr, dbp) > 0){
					if(!strcmp(ptr, PKG_INDEX_VERSION)){
						dbp -> close(dbp);
						goto done;
					}
				}
			}
		}
		/*cache md5 differs from index md5 (or the index is of different version),
		 *  get rid of it and create new*/
		dbp -> close(dbp);
		remove(XPKG.config -> cache_db);
	}
	
	remove(path);
	g_free(URL);
	g_free(path);
	
	/*fetching INDEX itself*/
	file = "INDEX.bz2";
	
	gdk_threads_enter();
	gtk_progress_bar_set_fraction(
	GTK_PROGRESS_BAR(XPKG.gui.main_window.p_bar), 0);
	gtk_progress_bar_set_text(
	GTK_PROGRESS_BAR(XPKG.gui.main_window.p_bar),
	"Fetching index");
	gdk_threads_leave();
	
	URL = g_strdup_printf("%s/%s", XPKG.config -> repo_url, file);
	assert(URL != NULL);
	
	path = g_strdup_printf("%s/%s", TMP_D, file);
	assert(path != NULL);
	
	i = xpkg_do_fetch(URL, path, xpkg_fetch_report);
	
	if(i != 1){
		xpkg_do_fetch_check(i, URL);
		remove(path);
		goto done;
	}
	
	gdk_threads_enter();
	gtk_progress_bar_set_fraction(
	GTK_PROGRESS_BAR(XPKG.gui.main_window.p_bar), 0);
	gtk_progress_bar_set_text(
	GTK_PROGRESS_BAR(XPKG.gui.main_window.p_bar),
	"Extracting index");
	gdk_threads_leave();
	
	lpath = g_strdup_printf("%s/INDEX", TMP_D);
	r = xpkg_extract(path, lpath);
	
	if(!r){
		xpkg_error("Failed extracting %s!", path);
		goto done;
	}
	
	gdk_threads_enter();
	gtk_widget_show(XPKG.gui.main_window.p_bar);
	gtk_progress_bar_set_text(
	GTK_PROGRESS_BAR(XPKG.gui.main_window.p_bar),
	"Caching index");
	gdk_threads_leave();
	
	mkdir_tree(XPKG.config -> cache_d, DEF_O_MODE);
	
	pkg_cache_index(lpath, XPKG.config -> cache_db, 
	md5_str, (void *)xpkg_fetch_report);
	
	/*goto done start*/
	done:
	remove(path);
	remove(lpath);

	g_free(URL);
	g_free(path);
	
	gdk_threads_enter();
	gtk_widget_hide(XPKG.gui.main_window.p_bar);
	gtk_widget_set_sensitive(XPKG.gui.main_window.notebook_s, TRUE);
	gdk_threads_leave();
	
	gdk_threads_enter();
	xpkg_update_available_pkglist();
	gdk_threads_leave();
	
	XPKG.rm_cached_f = TRUE;
	
	if(callback != NULL)
		callback();
	
	return NULL;	
}

void xpkg_fetch_index_md5_init(void (*callback)(void)){
	GThread *g_t;
	
	g_t = g_thread_create(xpkg_fetch_init_t, callback, FALSE, NULL);
	assert(g_t != NULL);
}

off_t xpkg_fetch_file_size(char *URL){
	
	struct url *url = NULL;
	struct url_stat us;
	
	if((url = fetchParseURL(URL)) == NULL)
		return 0;
	
	if(!*url->scheme)
		strcpy(url->scheme, SCHEME_FTP);
		
	fetchStatFTP(url, &us, NULL);
	
	if(us.size == -1)
		return 0;
	
	return us.size;
}

gchar *xpkg_pkgn_to_url(gchar *pkgn, gchar *origin){
	gchar *URL;
			
	URL = g_strdup_printf("%s/%s/%s.tbz", 
	XPKG.config -> repo_url, origin, pkgn);
	
	assert(URL != NULL);
	
	return URL;
}

off_t xpkg_fetch_package_size(gchar *package, gchar *origin){
	off_t size;
	gchar *URL;
	
	URL = xpkg_pkgn_to_url(package, origin);
	
	size = xpkg_fetch_file_size(URL);
	
	g_free(URL);
	
	return size;
}

/*
 * Return values:
 *  1 - success
 *  0 - libfetch error
 * -1 - libc error
 * -2 - emty or null URL
 * -3 - received file smaller than expected
 */
int xpkg_do_fetch(
const gchar *URL, const gchar *path, 
XPKG_FETCH_CALLBACK(fetch_callback)){
	struct url *url = NULL;
	struct url_stat us;
	FILE *f, *of;
	gdouble fraction;
	size_t size, count = 0;
	char buf[BUFSIZE];
	
	if(URL == NULL || URL[0] == '\0')
		return -2;
	
	if((url = fetchParseURL(URL)) == NULL)
		return 0;
		
	if(!*url->scheme)
		strcpy(url->scheme, SCHEME_FTP);
		
	f = fetchXGet(url, &us, NULL);
	
	if(f == NULL)
		return 0;
		
	if(us.size == -1)
		return 0;
		
	if((of = fopen(path, "w")) == NULL){
		fclose(f);
		return -1;
	}
	
	while((size = fread(buf, 1, BUFSIZE, f)) > 0){
		fraction = !count ? 0 : (gdouble)((double)count / (double)us.size);

		if(fetch_callback != NULL)
			fetch_callback(count, us.size, fraction, url -> doc);

		count += size;
		
		if(fwrite(buf, 1, size, of) < size){
			 fclose(f);
			 fclose(of);
			 
			 return -3;
		}
	}

	if(fetch_callback != NULL){
		fraction = (gdouble)((double)count / (double)us.size);
		fetch_callback(count, us.size, fraction, url -> doc);
	}
	
	fclose(f);
	fclose(of);
	
	return 1;
}
