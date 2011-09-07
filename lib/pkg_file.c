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

char *pkg_read_control_file(Pkg pkg, const char *cfile){
	char *cont = NULL,
	     *path,
	     **cfiles;
	int fd;
	struct stat f_stat;
	
	cfiles = pkg_get_control_files(pkg);
	
	if(cfiles == NULL)
		return NULL;
	
	for(int a = 0; cfiles[a] != NULL; a++){
		if(!strcmp(cfiles[a], cfile)){
			asprintf(&path, "%s/%s", pkg->path_d, cfiles[a]);
			assert(path != NULL);
			
			if((fd = open(path, O_RDONLY)) == -1){
				free(path);
				RETURN_P_ERRNO(NULL);
			}
			free(path);
			
			if(fstat(fd, &f_stat) == -1){
				free(path);
				close(fd);
				
				RETURN_P_ERRNO(NULL);
			}
			
			cont = (void *)malloc(f_stat.st_size + 1);
			assert(cont != NULL);
			cont[f_stat.st_size] = '\0';
			
			if(read(fd, cont, f_stat.st_size) == -1){
				free(cont);
				close(fd);
				RETURN_P_ERRNO(NULL);
			}
				
			close(fd);			
			break;		
		}
	}
	
	return cont;
}

char **pkg_get_control_files(Pkg pkg){
	DIR *dir_h;
	char **c_files;
	struct dirent *dirn;
	size_t size;
	int count = 0;
	
	if(pkg == NULL || pkg->path_d == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
	
	dir_h = opendir(pkg->path_d);
	
	if(dir_h == NULL)
		RETURN_P_ERRNO(NULL);
	
	size = sizeof(char *);
	c_files = malloc(size);
	assert(c_files != NULL);
	c_files[count] = NULL;
	
	while((dirn = readdir(dir_h)) != NULL){
		if(dirn->d_name[0] == '+'){
			size += sizeof(char *);
			c_files = reallocf(c_files, size);
			assert(c_files != NULL);
			c_files[count] = strdup(dirn->d_name);
			count++;
			c_files[count] = NULL;
		}
	}
	
	closedir(dir_h);
	
	return c_files;
}

void pkg_free_control_files(char **c_files){
	int i;
	
	if(c_files == NULL)
		return;

	for(i = 0; c_files[i] != NULL; i++){
		free(c_files[i]);
	}
	
	free(c_files);
}

int pkg_file_write(char *path, char *data, size_t size){
	int fd, rv;
	
	fd = open(path, O_CREAT | O_WRONLY, DEF_O_MODE);
	
	if(fd == -1)
		RETURN_P_ERRNO(-1);
		
	rv = write(fd, data, size);
	
	if(rv == -1){
		close(fd);
		RETURN_P_ERRNO(-1);
	}
	
	close(fd);
	
	return 0;
}

PkgFile pkg_file_open(const char *path){
	PkgFile pfile;
	int fd;
	
	if(path == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
	
	if(access(path, R_OK) != 0)
		RETURN_P_ERRNO(NULL);
		
	if((fd = open(path, O_RDONLY)) == -1)
		RETURN_P_ERRNO(NULL);
	
	pfile = malloc(sizeof(struct PkgFileStr));
	assert(pfile != NULL);
	memset(pfile, 0, sizeof(struct PkgFileStr));
	
	pfile->fd = fd;
	strncpy(pfile->path, path, MAXNAMLEN);
	fstat(fd, &pfile->f_stat);
	
	pfile->data = mmap(NULL, pfile->f_stat.st_size, PROT_READ,
	MAP_SHARED, fd, 0);
	
	if(pfile->data == MAP_FAILED){
		free(pfile);
		close(fd);
		
		RETURN_P_ERR(P_ERR_MMAP, NULL);
	}
	
	return pfile;
}

PkgFile pkg_file_open_virtual(void *data, size_t size){
	PkgFile pfile;
	
	pfile = malloc(sizeof(struct PkgFileStr));
	assert(pfile != NULL);
	memset(pfile, 0, sizeof(struct PkgFileStr));
	
	pfile->fd = 0;	
	pfile->f_stat.st_size = size;
	
	pfile->data = mmap(NULL, size, 
	PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
	
	if(pfile->data == MAP_FAILED){
		free(pfile);
		close(pfile->fd);
		
		RETURN_P_ERR(P_ERR_MMAP, NULL);
	}
	
	memcpy(pfile->data, data, size);
	
	return pfile;	
}

void pkg_file_close(PkgFile pfile){
	if(pfile == NULL)
		return;
	if(pfile->data != NULL)
		munmap(pfile->data, pfile->f_stat.st_size);
	
	if(pfile->fd > 0)
		close(pfile->fd);
	
	free(pfile);		
}

char *pkg_file_get_line(PkgFile pfile, long *pos){
	long ps, i;
	char *ptr;
	
	if(pfile == NULL || pos == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
		
	if(pfile->pos >= pfile->f_stat.st_size)
		RETURN_P_ERR(P_ERR_INVALID_POS, NULL);
		
	ptr = pfile->data;
	ps = pfile->pos;
	
	ptr += ps;
	
	for(i = 0; 
	ps != pfile->f_stat.st_size && ps < pfile->f_stat.st_size; 
	ps++, i++)
		if(ptr[i] == '\n'){
			break;
		}
		
	if(ptr[i] != '\n')
		return NULL;
	
	*pos = ps + 1;
	pfile->l_pos++;
	
	return ptr;
}

PkgContentFile pkg_file_get_con_file(PkgFile pfile, char **prefix,
char **mode, char **group, char **user){
	PkgContentFile con_file = NULL;
	char *ptr,
	     *file,
	     *md5,
	     *c;;
	long pos,
	     len,
	     mlen;
	int ignore = 0,
	    ilen;
	
	if(pfile == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
		
#define D_STR  "@ignore"
#define D_STR2 "@cwd"
#define D_STR3 "@cd"
#define D_STR4 "@mode"
#define D_STR5 "@group"
#define D_STR6 "@user"

	ilen = strlen(D_STR);

	while((ptr = pkg_file_get_line(pfile, &pos)) != NULL){
		len = pos - pfile->pos;
		pfile->pos = pos;
		
		if((len > 1 && ptr[0] != '@') && !ignore){
			con_file = malloc(sizeof(struct PkgContentFileStr));
			assert(con_file != NULL);
			memset(con_file, 0, sizeof(struct PkgContentFileStr));
			
			mlen = (len > MAXNAMLEN) ? (MAXNAMLEN - 1) : (len - 1);
			memset(con_file->file, 0, MAXNAMLEN);
			strncpy(con_file->file, ptr, mlen);
			
			asprintf(&file, "%s/%s", *prefix, con_file->file);
			assert(file != NULL);
			memset(con_file->file, 0, MAXNAMLEN);
			mlen = ((strlen(file) - 1)  > MAXNAMLEN) 
			? (MAXNAMLEN - 1) : strlen(file);
			strncpy(con_file->file, file, mlen);
			
			free(file);
			
			#define DO_WRITE(_DST, _SRC, _SIZE)	       \
			if(_DST != NULL){	                       \
				memset(_SRC, 0, _SIZE);                \
				mlen = strlen(_DST);                   \
				memmove(_SRC, _DST,                    \
				(mlen < _SIZE - 1) ? mlen : _SIZE - 1);\
			}
			
			DO_WRITE(*prefix, con_file->cwd, PATH_MAX);
			DO_WRITE(*mode, con_file->mode, MAX_INPUT);
			DO_WRITE(*group, con_file->group, MAX_INPUT);
			DO_WRITE(*user, con_file->user, MAX_INPUT);

			#undef DO_WRITE
			
			con_file->l_pos = pfile->l_pos;
			
			md5 = pkg_file_get_comment(pfile, "MD5");
			if(md5 != NULL){
				strncpy(con_file->o_md5_hash, md5, (MD5HASHBUFLEN - 1));
				
				free(md5);
			}
			
			return con_file;
		}
		else {
			#define DO_READ(_DST)	                  \
			c = strchr(ptr, ' ');	                  \
			if(c != NULL && c[1] != '\0'){	          \
				c++;                                  \
				free(_DST);                           \
				_DST = malloc(len - (c - ptr));       \
				assert(_DST != NULL);                 \
				memset(_DST, 0, len - (c - ptr));     \
				strncpy(_DST, c, len - (c - ptr) - 1);\
			}
			
			if((len == ilen || len > ilen) && (!strncmp(ptr, D_STR, ilen))){
				ignore = 1;
			} else if((len > strlen(D_STR2) && !strncmp(D_STR2, ptr, strlen(D_STR2)))
			           ||
					  (len > strlen(D_STR3) && !strncmp(D_STR3, ptr, strlen(D_STR3)))
			){
				DO_READ(*prefix);
			} else if((len == (ilen = strlen(D_STR4)) || len > ilen) && 
			(!strncmp(ptr, D_STR4, ilen))){
				DO_READ(*mode)
				else {
					free(*mode); *mode = NULL;
				}
			} else if((len == (ilen = strlen(D_STR5)) || len > ilen) && 
			(!strncmp(ptr, D_STR5, ilen))){
				DO_READ(*group)
				else {
					free(*group); *group = NULL;
				}
			} else if((len == (ilen = strlen(D_STR6)) || len > ilen) && 
			(!strncmp(ptr, D_STR6, ilen))){
				DO_READ(*user)
				else {
					free(*user); *user = NULL;
				}
			} else if(ignore)
				ignore = 0;
		}
	}
#undef DO_READ
	
#undef D_STR
#undef D_STR2
#undef D_STR3
#undef D_STR4
#undef D_STR5
#undef D_STR6
		
	return con_file;
}

char *pkg_file_get_prefix(PkgFile pfile){
	char *prefix = NULL,
	     *ptr,
		 *c;
	long pos,
	     len;
	
	if(pfile == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);

#define D_STR "@cwd"

	while((ptr = pkg_file_get_line(pfile, &pos)) != NULL){
		len = pos - pfile->pos;
		pfile->pos = pos;
		
		if(len > strlen(D_STR) && !strncmp(D_STR, ptr, strlen(D_STR))){
			c = strchr(ptr, ' ');
			if(c != NULL){
				c++;
				prefix = malloc(len - (c - ptr));
				assert(prefix != NULL);
				memset(prefix, 0, len - (c - ptr));
				strncpy(prefix, c, len - (c - ptr) - 1);
				
				return prefix;
			}
		}
	}

#undef D_STR
		
	return prefix;
}

char *pkg_file_get_rdep(PkgFile pfile){
	char *rdep = NULL,
	     *ptr;
	long pos;
	size_t len;
	
	if(pfile == NULL)
		return NULL;

	if((ptr = pkg_file_get_line(pfile, &pos)) != NULL){
		len = pos - pfile->pos;
		pfile->pos = pos;
		
		if(len > 1){
			rdep = malloc(len);
			assert(rdep != NULL);
			memset(rdep, 0, len);
			strncpy(rdep, ptr, len - 1);
		}
	}
		
	return rdep;
}

int pkg_file_get_install_steps(PkgFile pfile){
	char *ptr;
	long pos,
	     len;
	int steps = 0;
	
	if(pfile == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, 0);
		
#define D_STR  "@exec"
	
	while((ptr = pkg_file_get_line(pfile, &pos)) != NULL){
		len = pos - pfile->pos;
		pfile->pos = pos;
		
		if(len > strlen(D_STR) && !strncmp(D_STR, ptr, strlen(D_STR)))
			steps++;
	}
	
#undef D_STR

	return steps;
}

PkgStep pkg_file_get_install_step(PkgFile pfile){
	char *ptr,
	     *c, *step;
	PkgStep step_s;
	long pos,
	     len;
	int len2;
	
	if(pfile == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
		
#define D_STR  "@exec"

	len2 = strlen(D_STR);
	
	while((ptr = pkg_file_get_line(pfile, &pos)) != NULL){
		len = pos - pfile->pos;
		pfile->pos = pos;
		
		if(len > len2 && !strncmp(D_STR, ptr, len2)){
			c = strchr(ptr, ' ');
			if(c != NULL){
				c++;
				step = malloc(len - (c - ptr));
				assert(step != NULL);
				memset(step, 0, len - (c - ptr));
				strncpy(step, c, len - (c - ptr) - 1);
				
				step_s = malloc(sizeof(struct PkgStepStr));
				assert(step_s != NULL);
				memset(step_s, 0, sizeof(struct PkgStepStr));
				
				step_s -> str = step;
				step_s -> l_pos = pfile -> l_pos;
				
				return step_s;
			}
		}
	}
	
#undef D_STR

	return NULL;
}

int pkg_file_get_remove_steps(PkgFile pfile){
	char *ptr;
	long pos,
	     len;
	int steps = 0;
	
	if(pfile == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, 0);
		
#define D_STR  "@unexec"
#define D_STR2 "@dirrm"
	
	while((ptr = pkg_file_get_line(pfile, &pos)) != NULL){
		len = pos - pfile->pos;
		pfile->pos = pos;
		
		if(
		(len > strlen(D_STR) && !strncmp(D_STR, ptr, strlen(D_STR)))
		||
		(len > strlen(D_STR2) && !strncmp(D_STR2, ptr, strlen(D_STR2)))
		){
			steps++;
		}
	}
	
#undef D_STR
#undef D_STR2

	return steps;
}

PkgStep pkg_file_get_remove_step(PkgFile pfile, int *step_t){
	char *ptr,
	     *c, *step;
	long pos,
	     len;
	int len2, len3;
	PkgStep step_s;
	
	if(pfile == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
		
#define D_STR  "@unexec"
#define D_STR2 "@dirrm"

	len2 = strlen(D_STR);
	len3 = strlen(D_STR2);
	
	while((ptr = pkg_file_get_line(pfile, &pos)) != NULL){
		len = pos - pfile->pos;
		pfile->pos = pos;
		
		if(
		(len > len2 && !strncmp(D_STR, ptr, len2))
		||
		(len > len3 && !strncmp(D_STR2, ptr, len3))
		){
			if(len > len2 && !strncmp(D_STR, ptr, len2))
				*step_t = STEP_T_EXEC;
			else
				*step_t = STEP_T_DIRRM;
			c = strchr(ptr, ' ');
			if(c != NULL){
				c++;
				step = malloc(len - (c - ptr));
				assert(step != NULL);
				memset(step, 0, len - (c - ptr));
				strncpy(step, c, len - (c - ptr) - 1);
				
				step_s = malloc(sizeof(struct PkgStepStr));
				assert(step_s != NULL);
				memset(step_s, 0, sizeof(struct PkgStepStr));
				
				step_s -> str = step;
				step_s -> l_pos = pfile -> l_pos;
				
				return step_s;
			}
		}
	}
	
#undef D_STR
#undef D_STR2

	return NULL;
}

char *pkg_file_get_dep(PkgFile pfile){
	char *dep = NULL,
	     *ptr,
		 *c;
	long pos,
	     len;
	
	if(pfile == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);

#define D_STR "@pkgdep"

	while((ptr = pkg_file_get_line(pfile, &pos)) != NULL){
		len = pos - pfile->pos;
		pfile->pos = pos;
		
		if(len > strlen(D_STR) && !strncmp(D_STR, ptr, strlen(D_STR))){
			c = strchr(ptr, ' ');
			if(c != NULL){
				c++;
				dep = malloc(len - (c - ptr));
				assert(dep != NULL);
				memset(dep, 0, len - (c - ptr));
				strncpy(dep, c, len - (c - ptr) - 1);
				
				return dep;
			}
		}
	}

#undef D_STR
		
	return dep;
}

char *pkg_file_get_conflict(PkgFile pfile){
	char *confl = NULL,
	     *ptr,
		 *c;
	long pos,
	     len;
	
	if(pfile == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);

#define D_STR "@conflicts"

	while((ptr = pkg_file_get_line(pfile, &pos)) != NULL){
		len = pos - pfile->pos;
		pfile->pos = pos;
		
		if(len > strlen(D_STR) && !strncmp(D_STR, ptr, strlen(D_STR))){
			c = strchr(ptr, ' ');
			if(c != NULL){
				c++;
				confl = malloc(len - (c - ptr));
				assert(confl != NULL);
				memset(confl, 0, len - (c - ptr));
				strncpy(confl, c, len - (c - ptr) - 1);
				
				return confl;
			}
		}
	}

#undef D_STR
		
	return confl;
}

char *pkg_file_get_comment(PkgFile pfile, char *c_tag){
	char *ptr, 
	     *c[2],
		 *tag,
		 *var;
	long pos,
		 len;
	
	if(pfile == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);

	
#define C_STR "@comment"
	
	while((ptr = pkg_file_get_line(pfile, &pos)) != NULL){
		len = pos - pfile->pos;
		pfile->pos = pos;
		
		if(len > strlen(C_STR) && !strncmp(C_STR, ptr, strlen(C_STR))){
			c[0] = strchr(ptr, ' ');
			c[1] = strchr(ptr, ':');
			if(c[0] != NULL && c[1] != NULL){
				c[0]++;
				tag = malloc((c[1] - c[0])+1);
				assert(tag != NULL);
				memset(tag, 0, (c[1] - c[0])+1);
				strncpy(tag, c[0], c[1] - c[0]);
				
				c[1]++;
				var = malloc((ptr + len) - c[1]);
				assert(var != NULL);
				memset(var, 0, (ptr + len) - c[1]);
				strncpy(var, c[1], ((ptr + len) - c[1])-1);
				
				if(strlen(tag) == strlen(c_tag))
					if(!strcmp(tag, c_tag)){
						free(tag);
						return var;
					}
			
				free(tag);free(var);		
			}
		}
	}
	
#undef C_STR

	return NULL;
}
