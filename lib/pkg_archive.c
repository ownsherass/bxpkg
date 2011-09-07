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

short mkdir_tree(char *path, mode_t omode){
	int i,
	    len;
	short rval = 0;
	char *p;
	
	if(path == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, -1);
	
	len = strlen(path);
	
	p = (char *) malloc(len + 1);
	assert(p != NULL);
	memset(p, 0, len + 1);
	strncpy(p, path, len);
	
	if(p[0] == '/')
		i = 1;
	else
		i = 0;
	
	for(;i != len && i < len;){
		for(;i != len && i < len; i++){
			if(p[i] == '/')				
				break;
		}
		if(p[i] == '/')
			p[i] = (char)0;

		rval = mkdir(p, omode);
		
		if(p[i] == (char)0){
			p[i] = '/';
			i++;
		}
		
		if(rval == -1){
			if(errno == EEXIST || errno == EISDIR){
				rval = 0;
			} else
				break;
		}
	}
		
	free(p);
	
	if(rval == -1)
		RETURN_P_ERRNO(-1);
	
	return rval;
}

char *pkg_archive_parse_name(char *ptr){
	char *name, *buff[3];
	int len;
	
	if(ptr == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
	
	buff[0] = ptr;
	
	for(;;){
		ptr = buff[0] = strchr(buff[0], '@');
		buff[0]++;
	
		if(ptr == NULL)
			RETURN_P_ERR(P_ERR_INVALID_CONTROL_FILE, NULL);
		
		buff[1] = strchr(buff[0], ' ');
	
		if(buff[1] == NULL || buff[1] == buff[0])
			RETURN_P_ERR(P_ERR_INVALID_CONTROL_FILE, NULL);
		
		len = buff[1] - buff[0];

		if(!strncmp(buff[0], "name", len)){
			buff[1]++;
			buff[2] = strchr(buff[1], '\n');
			
			if(buff[2] == NULL)
				RETURN_P_ERR(P_ERR_INVALID_CONTROL_FILE, NULL);
			
			len = buff[2] - buff[1];
			break;
		}
	}
	
	name = (char *)malloc(len + 1);
	assert(name != NULL);
	memset((void *)name, 0, len + 1);
	
	strncpy((void *)name, (void *)buff[1], len);
	
	return name;
}

char *pkg_archive_get_pkg_name(char *a_path){
	char ptr[1025],
	     *file;
	char *pkg_n;
	struct archive_entry *entry;
	struct archive *a;
	int r;
	
	if(a_path == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
	
	a = pkg_archive_open(a_path);
	
	if(a == NULL)
		return NULL;
		
	for(;;){
		r = archive_read_next_header(a, &entry);
		if(r == ARCHIVE_EOF)
			break;
		if(r != ARCHIVE_OK)
			RETURN_P_LARCHIVE(NULL, a);
			
		file = (char *)archive_entry_pathname(entry);
		
		if(!strcmp(C_F_CONTENT, file) && 
		strlen(file) == strlen(C_F_CONTENT))
			break;
	}
	
	if(r == ARCHIVE_EOF){
		pkg_archive_close(a);
		RETURN_P_ERR(P_ERR_CONTROL_FILE_MISSING, NULL);
	}
		
	memset(ptr, 0, sizeof(ptr));
	
	/* expecting to find name in first kb, 
	 * this should be valid in packages generated with pkg_install lib
	 */
	r = archive_read_data(a, ptr, 1024);
	
	if(r < 1)
		RETURN_P_LARCHIVE(NULL, a);
	
	pkg_n = pkg_archive_parse_name(ptr);
	
	pkg_archive_close(a);
	
	return pkg_n;
}

Pkg pkg_archive_get_pkg(char *a_path, PkgDb db){
	Pkg pkg;
	char *path;
	char *file,
	           *pkg_n;
	struct archive_entry *entry;
	struct archive *a;
	int r,
	    fd;
	
	if(a_path == NULL || db == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
	
	pkg_n = pkg_archive_get_pkg_name(a_path);
	
	if(pkg_n == NULL)
		return NULL;
		
	asprintf(&path, "%s/%s", db->db_d, pkg_n);
	assert(path != NULL);
	
	r = mkdir(path, DEF_O_MODE);
	
	if(r == -1)
		if(errno != EEXIST && errno != EISDIR){
			free((void *)pkg_n);
			free(path);

			RETURN_P_ERRNO(NULL);
		}
	
	a = pkg_archive_open(a_path);
	
	if(a == NULL)
		return NULL;
	
	for(;;){
		r = archive_read_next_header(a, &entry);
		if(r == ARCHIVE_EOF)
			break;
		if(r != ARCHIVE_OK){
			free((void *)pkg_n);
			free(path);

			RETURN_P_LARCHIVE(NULL, a);
		}
			
		file = (char *)archive_entry_pathname(entry);
		
		if(file[0] == '+'){
			asprintf((void *)&file, "%s/%s", path, file);
			assert(file != NULL);
			
			fd = open(file, O_CREAT | O_WRONLY, DEF_O_MODE);
			
			if(fd == -1){
				pkg_archive_close(a);
				free((void *)pkg_n);
				free(path);
				free((void *)file);

				RETURN_P_ERRNO(NULL);
			}
			
			r = archive_read_data_into_fd(a, fd);
			
			close(fd);
			free((void *)file);
			
			if(r == -1){
				free((void *)pkg_n);
				free(path);

				RETURN_P_LARCHIVE(NULL, a);
			}
		}
	}
	
	pkg = pkg_new(db->db_d, pkg_n);
	
	pkg_archive_close(a);
	free((void *)pkg_n);
	free(path);
	
	return pkg;	
}

static inline const char *_find_link(const char *link, PkgContentFile *cons){
	size_t s, ss;
	u_int i;
	
	if(link[0] == '.')
		return link;
	
	s = strlen(link);
	
	for(i = 0; cons[i] != NULL; i++){
		ss = strlen(cons[i]->file);
		
		if(ss == s){
			if(!strcmp(link, cons[i]->file))
				return cons[i]->file;
		}
		else if(ss > s){
			if(!strcmp(link, &(cons[i]->file[ss - s])))
				return cons[i]->file;
		}
	}
	
	return NULL;
}

int pkg_archive_extract
(Pkg pkg, PkgContentFile *con_files, struct archive *a, 
char **file_path, PkgContentFile *con_file, const char *prefix, INSTALL_STEP_CALLBACK(s_callback)){
	struct archive_entry *entry;
	char *file,
	     *dir;
	const char *link_s;
	int r, rval = 1,
	    fd = 0,
	    len;
	u_int i = 0, ii;
	u_short ok = 0;
	pboolean is_slink,
	         is_hlink;
	
	if(a == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, -1);
		
	for(;;){
		r = archive_read_next_header(a, &entry);

		if(r == ARCHIVE_EOF){
			rval = 0;
			
			break;
		} else if(r != ARCHIVE_OK){
			rval = -1;
			
			break;
		}
		
		is_slink = is_hlink = 0;
		
		if((link_s = archive_entry_symlink(entry)) != NULL){
			is_slink = 1;
		} else if((link_s = archive_entry_hardlink(entry)) != NULL){
			is_hlink = 1;
		}
		
		file = (char *)archive_entry_pathname(entry);
		
		if(file == NULL){
			rval = -1;
			
			break;
		}
		
		char *fp;
		if(file[0] != '+')
			for(i = 0; con_files[i] != NULL; i++){
				len = strlen(con_files[i]->cwd);
				fp = con_files[i]->file;
				fp = fp + len;
			
				if(fp[0] == '/')
					fp++;

				if(strlen(fp) == strlen(file))
					if(!strcmp(fp, file)){
						ok = 1;
				
						break;
					}
			}
			
		if(ok){
			if(s_callback != NULL)
				s_callback(0, 0, con_files[i]->file, PSTEP_T_FCPY, 0, STAGE_PRE_T);
				
			*con_file = con_files[i];			
			*file_path = strdup(con_files[i]->file);
			
			len = strlen(*file_path);
			dir = (char *)malloc(len);
			assert(dir != NULL);
			memset(dir, 0, len);
			strncpy(dir, *file_path, len);
			
			for(ii = (len - 1);ii != 0;ii--)
				if(dir[ii] == '/'){
					dir[ii] = (char)0;
					
					break;
				}
			
			if(access(dir, F_OK) == -1)
				r = mkdir_tree(dir, DEF_O_MODE);
		}
			
		if(ok && (is_slink || is_hlink)){
			const char *dir;
			char *dir_s = NULL;
			
			*file_path = strdup(con_files[i]->file);
			dir = _find_link(link_s, con_files);
			
			if(!dir){
				asprintf(&dir_s, "%s/%s", prefix, link_s);
				assert(dir_s != NULL);
				
				dir = dir_s;
			}
			
			if(is_slink){
				r = symlink(dir, con_files[i]->file);
			} else
				r = link(dir, con_files[i]->file);
				
			free(dir_s);		
			
			if(r == -1)				
				RETURN_P_ERRNO(-1);
				
			break;
				
		} else if(ok){
			if(!r){
				fd = open(*file_path, O_CREAT | O_WRONLY, DEF_O_MODE);
			
				if(fd == -1){
					rval = -1;
				} else {
					rval = archive_read_data_into_fd(a, fd);
					if(rval < 0){
						rval = -1;
					} else
						rval = 1;
					close(fd);
					
					char *script = NULL;
					
					if(con_files[i]->mode != NULL){						
						asprintf(&script, "chmod %s %s 2>/dev/null",
						con_files[i]->mode, *file_path);
						assert(script != NULL);
						
						pkg_exec_script(script);
						free(script);
					}
					if(con_files[i]->group != NULL || con_files[i]->user != NULL){
						char *group;
						char *user;
						
						if(con_files[i]->group != NULL){
							asprintf(&group, ":%s", con_files[i]->group);
							assert(group != NULL);
						} else
							group = "";
						if(con_files[i]->user != NULL){
							user = con_files[i]->user;
						} else
							user = getenv("USER");						
							
						asprintf(&script, "chown %s%s %s 2>/dev/null",
						user, group, *file_path);
						assert(script != NULL);
						
						pkg_exec_script(script);
						free(script);
						
						if(group[0] != '\0')
							free(group);
					}
				}
			}
			else
				rval = -1;
				
			free(dir);			
			break;
		}
	}
	if(rval == -1){
		
		if(fd == -1){
			RETURN_P_ERRNO(-1);
		} else
			RETURN_P_LARCHIVE(-1, a);
	}
	
	return rval;
}

char **pkg_archive_get_values(const char *path, short type){
	struct archive *a;
	struct archive_entry *entry;
	int r;
	const char *file;
	char **vals, *val = NULL;
	void *data;
	struct stat *f_stat;
	PkgFile pfile;
	
	if(type >= T_LAST)
		RETURN_P_ERR(P_ERR_UNKNOWN_TYPE, NULL);
	
	a = pkg_archive_open(path);
	
	if(a == NULL)
		return NULL;
		
	for(;;){
		r = archive_read_next_header(a, &entry);
		if(r == ARCHIVE_EOF)
			break;
		if(r != ARCHIVE_OK)
			RETURN_P_LARCHIVE(NULL, a);
		
		file = (char *)archive_entry_pathname(entry);
		
		if(!strcmp(C_F_CONTENT, file) && 
		strlen(file) == strlen(C_F_CONTENT))
			break;
	}
	
	if(r == ARCHIVE_EOF){
		pkg_archive_close(a);
		RETURN_P_ERR(P_ERR_CONTROL_FILE_MISSING, NULL);
	}
	
	f_stat = (struct stat *)archive_entry_stat(entry);
	
	data = malloc(f_stat->st_size);
	assert(data != NULL);
	memset(data, 0, f_stat->st_size);
	
	r = archive_read_data(a, data, f_stat->st_size);
	
	if(r < 1){
		free(data);
		RETURN_P_LARCHIVE(NULL, a);
	}
	
	pfile = pkg_file_open_virtual(data, f_stat->st_size);	
	free(data);
	
	if(pfile == NULL){
		pkg_archive_close(a);
		return NULL;
	}
	
	vals = (char **)malloc(sizeof(char *));
	vals[0] = NULL;
	
	for(int i = 0;;i++){
		if(type == T_DEPS)
			val = pkg_file_get_dep(pfile);
		else if(type == T_CONFLICTS)
			val = pkg_file_get_conflict(pfile);
		if(val == NULL)
			break;
			
		vals = (char **)realloc(vals, 
		(sizeof(char *) + sizeof(char *)*(i+1)));
		vals[i]   = val;
		vals[i+1] = NULL;
	}
	
	pkg_file_close(pfile);	
	pkg_archive_close(a);
	
	return vals;
}

void pkg_archive_free_vals(char **vals){
	if(vals == NULL)
		RETURN_P_ERR_VOID(P_ERR_INVALID_DESCRIPTOR);
		
	for(int i = 0;;i++){
		if(vals[i] == NULL)
			break;
		
		free(vals[i]);
	}
	
	free(vals);
}

size_t pkg_archive_get_size(const char *path){
	struct archive *a;
	struct archive_entry *entry;
	size_t size = 0;
	struct stat *f_stat;
	int r;
	
	a = pkg_archive_open(path);
	
	if(a == NULL)
		return -1;
	
	for(;;){
		r = archive_read_next_header(a, &entry);
		if(r == ARCHIVE_EOF)
			break;
		if(r != ARCHIVE_OK){
			pkg_archive_close(a);
			return -1;
		}
		
		f_stat = (struct stat *)archive_entry_stat(entry);
		
		size += f_stat->st_size;
	}
	
	return size;
}

int pkg_archive_file_count(const char *path){
	struct archive *a;
	struct archive_entry *entry;
	int files,
	    r;
	
	a = pkg_archive_open(path);
	
	if(a == NULL)
		return -1;
	
	for(files = 0;;files++){
		r = archive_read_next_header(a, &entry);
		if(r == ARCHIVE_EOF)
			break;
		if(r != ARCHIVE_OK){
			RETURN_P_LARCHIVE(-1, a);
		}
	}
	
	pkg_archive_close(a);
	
	return files;
}

struct archive * pkg_archive_open(const char *path){
	struct archive *a;
	
	if(path == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
	
	a = archive_read_new();
	
	if(a == NULL)
		return NULL;
		
	archive_read_support_compression_all(a);
	archive_read_support_format_tar(a);
		
	if(archive_read_open_file(a, path, 10240) != 0){
		pkg_error(0, (char *)archive_error_string(a));
		return NULL;
	}
	
	return a;
}

void pkg_archive_close(struct archive *a){
	archive_read_close(a);
	archive_read_finish(a);
}
