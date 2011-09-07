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
#include "pkg.h"

#define PKG_DB_PATH_D "var/db/pkg"

PkgDb pkg_db_open(const char *base){
	char *path = NULL;
	DIR *dir_h;
	PkgDb pkgdb = NULL;
	
	if(base[strlen(base) - 1] != '/'){
		asprintf(&path, "%s/%s", base, PKG_DB_PATH_D);
	} else
		asprintf(&path, "%s%s", base, PKG_DB_PATH_D);
	assert(path != NULL);
	
	dir_h = opendir(path);
	
	if(dir_h == NULL){
		free(path);		
		
		RETURN_P_ERRNO(NULL);
	}
	
	pkgdb = (PkgDb)malloc(sizeof(struct PkgDbStr));
	assert(pkgdb != NULL);
	
	pkgdb->db_d = path;
	pkgdb->d_h = dir_h;
	
	pkgdb->base = strdup(base);
	assert(pkgdb->base != NULL);
	
	return pkgdb;
}

void pkg_db_close(PkgDb pkgdb){
	if(pkgdb == NULL)
		return;
	if(pkgdb->db_d != NULL)
		free((void *)pkgdb->db_d);
	if(pkgdb->d_h != NULL)
		closedir(pkgdb->d_h);
	if(pkgdb->base != NULL)
		free(pkgdb->base);
	free(pkgdb);
}

Pkg *pkg_db_get_installed_matched(PkgDb db, const char *pattern){
	Pkg *pkgs;
	struct dirent *dirn;
	size_t size;
	int count = 0;
	
	if(db == NULL || db->db_d == NULL || db->d_h == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);

	size = sizeof(Pkg);
	pkgs = malloc(size);
	assert(pkgs != NULL);
	pkgs[0] = NULL;

	while((dirn = readdir(db->d_h)) != NULL){
		if(pattern != NULL){
			if(!strncmp(pattern, dirn->d_name, strlen(pattern)) 
			&& dirn->d_type == DT_DIR
			&& dirn->d_name[0] != '.'){
				size += sizeof(Pkg);
				pkgs = reallocf(pkgs, size);
				assert(pkgs != NULL);
				pkgs[count++] = pkg_new(db->db_d, dirn->d_name);
				pkgs[count] = NULL;
			}
		}
		else if(dirn->d_type == DT_DIR && dirn->d_name[0] != '.'){
			size += sizeof(Pkg);
			pkgs = reallocf(pkgs, size);
			assert(pkgs != NULL);
			pkgs[count++] = pkg_new(db->db_d, dirn->d_name);
			pkgs[count] = NULL;
		}
	}

	return pkgs;
}

Pkg *pkg_db_get_installed(PkgDb db){
	return pkg_db_get_installed_matched(db, NULL);
}
