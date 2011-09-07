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

#ifndef _PKG_H_
#define	_PKG_H_

#include <sys/types.h>
#include <sys/limits.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <md5.h>
#include <db.h>

#include <archive.h>
#include <archive_entry.h>

#define MD5HASHBUFLEN 33

#define C_F_CONTENT        "+CONTENTS"
#define C_F_COMMENT        "+COMMENT"
#define C_F_DESCR          "+DESC"
#define C_F_DEINSTALL      "+DEINSTALL"
#define C_F_POST_DEINSTALL "+POST-DEINSTALL"
#define C_F_INSTALL        "+INSTALL"
#define C_F_POST_INSTALL   "+POST-INSTALL"
#define C_F_REQBY          "+REQUIRED_BY"
#define C_F_MTREE_DIRS     "+MTREE_DIRS"
#define C_F_REQUIRE        "+REQUIRE"

#define DEF_O_MODE 0755

#define IDX_FIELD_SIZE 50
#define IDX_FIELD_SIZE_S 25
#define IDX_FIELD_SIZE_L 256

#define PKG_INDEX_VERSION "1.2"

#ifdef __cplusplus
extern "C" {
#endif

/*raw step types*/
enum {
	STEP_T_EXEC,
	STEP_T_DIRRM
};

/*processed step types*/
enum {
	PSTEP_T_FRM,
	PSTEP_T_FCPY,
	PSTEP_T_SCRIPT,
	PSTEP_T_PKGDB
};

/*archive value types*/
enum {
	T_DEPS,
	T_CONFLICTS,
	T_LAST
};

/*callback stage types*/
enum {
	STAGE_PRE_T,
	STAGE_POST_T
};

/*flags*/
#define PKG_DIGEST_CH_F 0x01

typedef char PkgFlag;
typedef unsigned short int pboolean;

struct PkgStr {
	char *name,
	     *path_d;
};
typedef struct PkgStr *Pkg;

struct PkgDbStr {
	char *db_d,
	     *base;
	DIR *d_h;
};
typedef struct PkgDbStr *PkgDb;

struct PkgFileStr {
	struct stat f_stat;
	char path[MAXNAMLEN];
	void *data;
	u_int pos, l_pos;
	int fd;	
};
typedef struct PkgFileStr *PkgFile;

struct PkgStepStr {
	char *str;
	u_int l_pos;
};
typedef struct PkgStepStr *PkgStep;

struct PkgContentFileStr {
	char file[MAXNAMLEN],
	     o_md5_hash[MD5HASHBUFLEN],
	     cwd[PATH_MAX],
	     mode[MAX_INPUT],
	     user[MAX_INPUT],
	     group[MAX_INPUT];
	u_int l_pos;
};
typedef struct PkgContentFileStr *PkgContentFile;

struct PkgIndexEntry {
	char name[IDX_FIELD_SIZE],
	     version[IDX_FIELD_SIZE_S],
	     port_base[IDX_FIELD_SIZE],
	     prefix[IDX_FIELD_SIZE],
	     descr[IDX_FIELD_SIZE_L],
	     descr_f[IDX_FIELD_SIZE],
	     maintainer[IDX_FIELD_SIZE],
	     category[IDX_FIELD_SIZE_S],
	     www[IDX_FIELD_SIZE];     
};
typedef struct PkgIndexEntry PkgIdxEntry;

typedef unsigned short int PkgError;

struct PkgErrorCodes {
	PkgError code;
	const char *str;
};

/*Error codes*/
enum {
	P_ERR_UNKNOWN,
	P_ERR_INVALID_DESCRIPTOR,
	P_ERR_NO_DATA,
	P_ERR_DATA_CRRUPTED,
	P_ERR_CONTROL_FILE_MISSING,
	P_ERR_MMAP,
	P_ERR_INVALID_POS,
	P_ERR_INVALID_CONTROL_FILE,
	P_ERR_UNKNOWN_TYPE,
	P_ERR_SYNTAX,
	P_ERR_REQ_FAIL,
	P_ERR_CONFLICT,
	P_ERR_ERRNO
};

/*error return macros*/
#define RETURN_P_ERR(_CODE, _RVAL) \
{ pkg_error(_CODE, NULL); return _RVAL; }

#define RETURN_P_ERR_VOID(_CODE) \
{ pkg_error(_CODE, NULL); return; }

#define RETURN_P_ERRNO(_RVAL) \
{ pkg_error(0, strerror(errno)); return _RVAL; }

#define RETURN_P_LARCHIVE(_RVAL, _A) \
{ pkg_error(0, (char *)archive_error_string(_A)); \
pkg_archive_close(_A); return _RVAL; }

#define P_ERR_LEN 254
extern char LibPkgErr[P_ERR_LEN];
extern unsigned short int LibPkgHasErr, LibPkgErrCode;

#define REMOVE_STEP_CALLBACK(_FNAME) \
void (*_FNAME)(u_int step, u_int steps, char *d_str, u_short pstep_t, int err_f, u_short stage_t)

#define INSTALL_STEP_CALLBACK(_FNAME) \
void (*_FNAME)(u_int step, u_int steps, char *d_str, u_short pstep_t, int err_f, u_short stage_t)

#define CACHE_STEP_CALLBACK(_FNAME) \
void (*_FNAME)(int step, int steps, double fraction, char *d_str)

PkgDb pkg_db_open(const char *base);
void pkg_db_close(PkgDb pkgdb);
Pkg *pkg_db_get_installed_matched(PkgDb db, const char *pattern);
Pkg *pkg_db_get_installed(PkgDb db);

void pkg_error(PkgError err, char *c_str);
char *pkg_get_error(void);
Pkg pkg_new(char *db_d, const char *name);
void pkg_free(Pkg pkg);
void pkg_free_list(Pkg *pkgs);
const char *pkg_get_name(Pkg pkg);
char *pkg_get_origin(Pkg pkg);
char *pkg_get_prefix(Pkg pkg);
Pkg *pkg_get_dependencies(Pkg pkg);
Pkg *pkg_get_rdependencies(Pkg pkg);
PkgContentFile *pkg_get_content_files(Pkg pkg);
void pkg_free_content_file_list(PkgContentFile *con_files);
unsigned short int pkg_remove(Pkg pkg, PkgDb pkgdb, REMOVE_STEP_CALLBACK(s_callback));
void pkg_dereqby(Pkg pkg);
void pkg_reqby(Pkg pkg);
unsigned short int pkg_install_from_archive
(const char *a_path, PkgDb db, Pkg *pkgs, PkgFlag flags, INSTALL_STEP_CALLBACK(s_callback));
char *pkg_get_comment(Pkg pkg);
int pkg_exec_script(char *script);
char *pkg_get_conflict(Pkg pkg);
int pkg_conflict_exists(Pkg *pkgs, const char *conflict);

char *pkg_read_control_file(Pkg pkg, const char *cfile);
char **pkg_get_control_files(Pkg pkg);
void pkg_free_control_files(char **c_files);
PkgFile pkg_file_open(const char *path);
PkgFile pkg_file_open_virtual(void *data, size_t size);
void pkg_file_close(PkgFile pfile);
char *pkg_file_get_line(PkgFile pfile, long *pos);
char *pkg_file_get_comment(PkgFile pfile, char *c_tag);
char *pkg_file_get_dep(PkgFile pfile);
char *pkg_file_get_rdep(PkgFile pfile);
char *pkg_file_get_prefix(PkgFile pfile);
PkgContentFile pkg_file_get_con_file(PkgFile pfile, char **prefix, 
char **mode, char **group, char **user);
int pkg_file_get_remove_steps(PkgFile pfile);
PkgStep pkg_file_get_remove_step(PkgFile pfile, int *step_t);
int pkg_file_write(char *path, char *data, size_t size);
int pkg_file_get_install_steps(PkgFile pfile);
PkgStep pkg_file_get_install_step(PkgFile pfile);
char *pkg_file_get_conflict(PkgFile pfile);

struct archive * pkg_archive_open(const char *path);
void pkg_archive_close(struct archive *a);
Pkg pkg_archive_get_pkg(char *a_path, PkgDb db);
char *pkg_archive_get_pkg_name(char *a_path);
int pkg_archive_file_count(const char *path);
int pkg_archive_extract(Pkg pkg, PkgContentFile *con_files, struct archive *a, 
char **file_path, PkgContentFile *con_file, const char *prefix, INSTALL_STEP_CALLBACK(s_callback));
char **pkg_archive_get_values(const char *path, short type);
void pkg_archive_free_vals(char **vals);
size_t pkg_archive_get_size(const char *path);

unsigned short int pkg_cache_index(
const char *index_path, const char *cache_path, char *md5_str,
CACHE_STEP_CALLBACK(c_callback));
short int pkg_get_cached_entry(DB *dbp, PkgIdxEntry *pie, unsigned short int *first);
int pkg_get_cached_unit(char *u_key, char **u_data, DB *dbp);
u_int pkg_get_cached_entries(DB *dbp);
char *pkg_get_cached_deps(DB *dbp, char *pname);
char *backtrace_ch(char *ptr, char ch, size_t s_limit);
char * pkg_get_cached_rdeps(DB *dbp, char *pname);

#ifdef __cplusplus
}
#endif

#endif
