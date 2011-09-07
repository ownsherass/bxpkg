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

#include <fnmatch.h>

/*libpkg error message string*/
char LibPkgErr[P_ERR_LEN];
unsigned short int LibPkgHasErr = 0, LibPkgErrCode;

/*Error codes to strings table*/
static struct PkgErrorCodes perror_codes[] = {
	{P_ERR_UNKNOWN             , "Unknown error"                            },
	{P_ERR_INVALID_DESCRIPTOR  , "Descriptor is invalid"                    },
	{P_ERR_NO_DATA             , "No data was found"                        },
	{P_ERR_DATA_CRRUPTED       , "Data got corrupted"                       },
	{P_ERR_CONTROL_FILE_MISSING, "Control file missing for the package"     },
	{P_ERR_MMAP                , "Memory mapping failed"                    },
	{P_ERR_INVALID_POS         , "Invalid position"                         },
	{P_ERR_INVALID_CONTROL_FILE, "Corrupted or invalid control file format" },
	{P_ERR_UNKNOWN_TYPE        , "Unknown type"                             },
	{P_ERR_SYNTAX              , "Syntax error"                             },
	{P_ERR_REQ_FAIL            , "Requirements were not met"                },
	{P_ERR_CONFLICT            , "Conflict exists for the package"          },
	{0                         , NULL                                       }
};

/**
 * @brief Error description fetching routine
 * 
 * Use this function to fetch error description after a
 * LibPkgHasErr == 1 check.
 * @return Error description string or NULL
 */
char *pkg_get_error(void) {
	if(!strlen(LibPkgErr)){
		return NULL;
	} else if(LibPkgHasErr){
		LibPkgHasErr = 0;
		return LibPkgErr;
	}
	
	return NULL;
}

/**
 * @brief Error reporting routine
 * @param err error code (used only if c_str is NULL)
 * @param c_str error string
 *
 * Registers an error or fault
 */
void pkg_error(PkgError err, char *c_str){
	unsigned short int i = 0;
	size_t s;
	
	memset(LibPkgErr, 0, sizeof(LibPkgErr));
	
	if(c_str == NULL){
		for(; perror_codes[i].str != NULL; i++)
			if(perror_codes[i].code == err){
				s = strlen(perror_codes[i].str);

				memmove(LibPkgErr, perror_codes[i].str,
				(s > P_ERR_LEN) ? (P_ERR_LEN - 1) : s);
				LibPkgHasErr = 1;
				
				LibPkgErrCode = err;
			
				return;
			}

		s = strlen(perror_codes[P_ERR_UNKNOWN].str);
		memmove(LibPkgErr, perror_codes[i].str,
		(s > P_ERR_LEN) ? (P_ERR_LEN - 1) : s);
		LibPkgHasErr = 1;
		
		LibPkgErrCode = err;
	}
	else {
		s = strlen(c_str);
		memmove(LibPkgErr, c_str,
		(s > P_ERR_LEN) ? (P_ERR_LEN - 1) : s);
		LibPkgHasErr = 1;
		
		LibPkgErrCode = P_ERR_ERRNO;
	}
}

/**
 * @brief Creates new package descriptor.
 * @param db_d database path
 * @param name package full name
 *
 * This creates a valid package descriptor that can be handled by rest of
 * the library functions.
 * @return A new package object or NULL
 */
Pkg pkg_new(char *db_d, const char *name){
	Pkg pkg;
	
	pkg = malloc(sizeof(struct PkgStr));
	assert(pkg != NULL);
	
	pkg->name = strdup(name);
	assert(pkg->name != NULL);
	
	asprintf(&pkg->path_d, "%s/%s", db_d, name);
	assert(pkg->path_d != NULL);
	
	return pkg;
}

/**
 * @brief Destroys package descriptor.
 * @param pkg valid package descriptor
 *
 * This will free descriptor.
 */
void pkg_free(Pkg pkg){
	if(pkg == NULL)
		RETURN_P_ERR_VOID(P_ERR_INVALID_DESCRIPTOR);
	if(pkg->name != NULL)
		free(pkg->name);
	if(pkg->path_d != NULL)
		free(pkg->path_d);
	free(pkg);
}

/**
 * @brief Destroys package descriptor.
 * @param pkgs package descriptor array
 *
 * This will free descriptor array.
 */
void pkg_free_list(Pkg *pkgs){
	int i;
	
	if(pkgs == NULL)
		RETURN_P_ERR_VOID(P_ERR_INVALID_DESCRIPTOR);
	for(i = 0; pkgs[i] != NULL; i++)
		pkg_free(pkgs[i]);
	free(pkgs);
}

char *pkg_get_conflict(Pkg pkg){
	PkgFile pfile;
	char **c_files;
	char *path,
	     *var = NULL;
	int i;
	
	if(pkg == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
		
	c_files = pkg_get_control_files(pkg);
	if(c_files == NULL)
		return NULL;
		
	for(i = 0; c_files[i] != NULL; i++)
		if(!strcmp(c_files[i], C_F_CONTENT)){
			asprintf(&path, "%s/%s", pkg->path_d, c_files[i]);
			assert(path != NULL);
			
			pfile = pkg_file_open(path);
			
			if(pfile == NULL){
				free(path);
				pkg_free_control_files(c_files);
				
				return NULL;
			}
			
			var = pkg_file_get_conflict(pfile);
			
			pkg_file_close(pfile);
			
			free(path);
			break;
		}
	
	pkg_free_control_files(c_files);
	
	return var;
}

static int csh_match(const char *pattern, const char *string, int flags){
	int ret = FNM_NOMATCH;

	const char *nextchoice = pattern;
	const char *current = NULL;

	int prefixlen = -1;
	int currentlen = 0;

	int level = 0;

	do {
		const char *pos = nextchoice;
		const char *postfix = NULL;

		u_short quoted = 0;

		nextchoice = NULL;
		do {
			const char *eb;
			if (!*pos) {
				postfix = pos;
			} else if (quoted) {
				quoted = 0;
			} else {
				switch (*pos) {
				case '{':
					++level;
					if (level == 1) {
						current = pos+1;
						prefixlen = pos-pattern;
					}
				break;
				case ',':
					if (level == 1 && !nextchoice) {
						nextchoice = pos+1;
						currentlen = pos-current;
					}
				break;
				case '}':
					if (level == 1) {
						postfix = pos+1;
						if (!nextchoice)
							currentlen = pos-current;
					}
					level--;
				break;
				case '[':
					eb = pos+1;
					if (*eb == '!' || *eb == '^')
						eb++;
					if (*eb == ']')
						eb++;
					while(*eb && *eb != ']')
						eb++;
					if (*eb)
						pos=eb;
				break;
				case '\\':
					quoted = 1;
				break;
				default:;
			}
		}
		pos++;
	} while (!postfix);

	if (current) {
		char buf[FILENAME_MAX];
		snprintf(buf, sizeof(buf), "%.*s%.*s%s", prefixlen, 
		pattern, currentlen, current, postfix);
		ret = csh_match(buf, string, flags);
		if (ret) {
			current = nextchoice;
			level = 1;
		} else
			current = NULL;
	} else
		ret = fnmatch(pattern, string, flags);
	} while (current);

	return ret;
}

int pkg_conflict_exists(Pkg *pkgs, const char *conflict){
	u_int i;
	
	if(pkgs == NULL || conflict == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, -1);
		
	for(i = 0; pkgs[i] != NULL; i++)
		if(!csh_match(conflict, pkgs[i]->name, 0))			
			return 0;

	return 1;
}

/**
 * @brief Extracts full package name from descriptor
 * @param pkg a valid package descriptor
 *
 * You should not access package descriptor directly yourself but rather
 * use this function for possible changes in future to remain compatible.
 * @return Full package name
 */
const char *pkg_get_name(Pkg pkg){
	if(pkg == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
	return pkg->name;
}

/**
 * @brief Extracts database base from package descriptor
 * @param pkg a valid package descriptor
 *
 * This is an internal function. You should not call this directly.
 * @return Database base
 */
char *pkg_get_db_base(Pkg pkg){
	char *base;
	int i;
		
	if(pkg == NULL || pkg->path_d == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
	
	i = strlen(pkg->path_d);
	
	for(; pkg->path_d[i] != '/' && i != 0; i--);
	
	if(!i)
		RETURN_P_ERR(P_ERR_NO_DATA, NULL);
	
	base = malloc(i+1);
	assert(base != NULL);
	memset(base, 0, i+1);
	strncpy(base, pkg->path_d, i);
	
	return base;
}

/**
 * @brief Destoroys content file descriptor array
 * @param con_files content file descriptor array
 *
 * This will free content file array.
 */
void pkg_free_content_file_list(PkgContentFile *con_files){
	int i;
	
	if(con_files == NULL)
		RETURN_P_ERR_VOID(P_ERR_INVALID_DESCRIPTOR);
	
	for(i = 0; con_files[i] != NULL; i++)
		free(con_files[i]);
	
	free(con_files);
}

/**
 * @brief Extracts content file information from package
 * @param pkg a valid package descriptor
 *
 * Use this to get list of files that belong to the package.
 * @return Content file descriptor array
 */
PkgContentFile *pkg_get_content_files(Pkg pkg){
	PkgContentFile *con_files;
	PkgFile pfile;
	char **c_files;
	char *path,
	     *prefix = NULL,
	     *mode = NULL,
	     *group = NULL,
	     *user = NULL;
	int i,
		c = 0;
	size_t s;
	
	if(pkg == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
	
	c_files = pkg_get_control_files(pkg);
	if(c_files == NULL)
		return NULL;
	
	s = sizeof(PkgContentFile);
	con_files = malloc(s);
	con_files[c] = NULL;
		
	for(i = 0; c_files[i] != NULL; i++)
		if(!strcmp(c_files[i], C_F_CONTENT)){
			asprintf(&path, "%s/%s", pkg->path_d, c_files[i]);
			assert(path != NULL);
			
			pfile = pkg_file_open(path);
			
			if(pfile == NULL){
				free(path);
				return NULL;
			}
			
			for(;;c++){
				if((con_files[c] = pkg_file_get_con_file(pfile, &prefix, 
				&mode, &group, &user)) == NULL)
					break;
				s += sizeof(struct PkgContentFileStr);
				con_files = reallocf(con_files, s);
				assert(con_files != NULL);
				con_files[c + 1] = NULL;
			}
			
			pkg_file_close(pfile);
			
			free(path);free(prefix);
			break;
		}
	
	pkg_free_control_files(c_files);
	
	return con_files;
}

/**
 * @brief Extracts reverse dependencies of the package
 * @param pkg a valid package descriptor
 *
 * Use this to get packages that depend on this package.
 * @return Package descriptor array of reverse dependencies
 */
Pkg *pkg_get_rdependencies(Pkg pkg){
	Pkg *rdeps;
	PkgFile pfile;
	char **c_files;
	char *path,
	     *rdep,
		 *db_d;
	int i,
		c = 0;
	size_t s;
	
	if(pkg == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
	
	c_files = pkg_get_control_files(pkg);
	if(c_files == NULL)
		return NULL;
	
	s = sizeof(struct PkgStr);
	rdeps = malloc(s);
	assert(rdeps != NULL);
	rdeps[0] = NULL;
		
	for(i = 0; c_files[i] != NULL; i++)
		if(!strcmp(c_files[i], C_F_REQBY)){
			asprintf(&path, "%s/%s", pkg->path_d, c_files[i]);
			assert(path != NULL);
			
			pfile = pkg_file_open(path);
			
			if(pfile == NULL){
				free(path);
				pkg_free_control_files(c_files);
				pkg_free_list(rdeps);
				return NULL;
			}
			
			while((rdep = pkg_file_get_rdep(pfile)) != NULL){
				db_d = pkg_get_db_base(pkg);
				
				s += sizeof(struct PkgStr);
				rdeps = reallocf(rdeps, s);
				assert(rdeps != NULL);
				assert((rdeps[c++] = pkg_new(db_d, rdep)) != NULL);
				rdeps[c] = NULL;
				
				free(rdep);
				free(db_d);
			}
			
			pkg_file_close(pfile);
			
			free(path);
			break;
		}
	
	pkg_free_control_files(c_files);
	
	return rdeps;
}

/**
 * @brief Extracts dependencies of the package
 * @param pkg a valid package descriptor
 *
 * Use this to get packages that the package depends on.
 * @return Package descriptor array of dependencies
 */
Pkg *pkg_get_dependencies(Pkg pkg){
	Pkg *deps;
	PkgFile pfile;
	char **c_files;
	char *path,
	     *dep,
		 *db_d;
	int i,
		c = 0;
	size_t s;
	
	if(pkg == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
	
	c_files = pkg_get_control_files(pkg);
	if(c_files == NULL)
		return NULL;
	
	s = sizeof(struct PkgStr);
	deps = malloc(s);
	assert(deps != NULL);
	deps[0] = NULL;
		
	for(i = 0; c_files[i] != NULL; i++)
		if(!strcmp(c_files[i], C_F_CONTENT)){
			asprintf(&path, "%s/%s", pkg->path_d, c_files[i]);
			assert(path != NULL);
			
			pfile = pkg_file_open(path);
			
			while((dep = pkg_file_get_dep(pfile)) != NULL){
				db_d = pkg_get_db_base(pkg);
				
				s += sizeof(struct PkgStr);
				deps = reallocf(deps, s);
				assert(deps != NULL);
				assert((deps[c++] = pkg_new(db_d, dep)) != NULL);
				deps[c] = NULL;
				
				free(db_d);
			}
			
			pkg_file_close(pfile);
			
			free(path);
			break;
		}
	
	pkg_free_control_files(c_files);
	
	return deps;
}

/**
 * @brief Extracts path prefix of the package
 * @param pkg a valid package descriptor
 *
 * Use this to get prefix for the content files of the package.
 * 
 * Use free() manually after being done using the value.
 * @return Package prefix
 */
char *pkg_get_prefix(Pkg pkg){
	PkgFile pfile;
	char **c_files;
	char *path,
	     *prefix = NULL;
	int i;
	
	if(pkg == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
	
	c_files = pkg_get_control_files(pkg);
	if(c_files == NULL)
		return NULL;
		
	for(i = 0; c_files[i] != NULL; i++)
		if(!strcmp(c_files[i], C_F_CONTENT)){
			asprintf(&path, "%s/%s", pkg->path_d, c_files[i]);
			assert(path != NULL);
			
			pfile = pkg_file_open(path);
			if(pfile == NULL){
				free(path);
				pkg_free_control_files(c_files);
				
				return NULL;
			}
			
			prefix = pkg_file_get_prefix(pfile);
			if(prefix == NULL){
				free(path);
				pkg_file_close(pfile);
				pkg_free_control_files(c_files);
				
				return NULL;
			}
			
			pkg_file_close(pfile);
			
			free(path);
			break;
		}
	
	pkg_free_control_files(c_files);
	
	return prefix;
}

/**
 * @brief Executes shell script
 * @param script a string containting sh script
 *
 * This is libpkg shell script execution wrapper.
 * @todo replace system() with native fork, execl handling.
 * @return /bin/sh exit status
 */
int pkg_exec_script(char *script){
	int rv;
	    
	rv = system(script);
	
	return rv;
}

/**
 * @brief Replace parts of string
 * @param str source string
 * @param c_str replace part of the string description
 * @param b_str replace by description
 *
 * This is an internal function used in few places. It is not defined in
 * main header file.
 * @return processed string
 */
char *replace_in_string(char *str, char *c_str, char *b_str){
	char *n_str;
	int len, c_len, b_len = 0,
	    i, a;
	size_t n_s;
	
	len = strlen(str);
	c_len = strlen(c_str);
	if(b_str != NULL)
		b_len = strlen(b_str);
	
	n_s = len + 1;
	n_str = (char *) malloc(n_s);
	assert(n_str != NULL);
	memset(n_str, 0, n_s);
	
	for(i = a = 0; i != len; i++){
		if(!strncmp(&str[i], c_str, c_len)){
			n_s += (b_len - c_len);
			n_str = reallocf(n_str, n_s);
			assert(n_str != NULL);
			if(b_str != NULL)
				memcpy(&n_str[a], b_str, b_len);
			a += b_len;
			i += (c_len - 1);
		}
		else{
			n_str[a] = str[i];
			a++;
		}
	}
	
	n_str[n_s - 1] = (char)0;
	
	return n_str;
}

void pkg_reqby(Pkg pkg){
	Pkg *deps;
	char **c_files,
	     *path;
	int fd,
	    i, a;
	
	if(pkg == NULL)
		RETURN_P_ERR_VOID(P_ERR_INVALID_DESCRIPTOR);
		
	deps = pkg_get_dependencies(pkg);
	if(deps == NULL)
		return;
	
	for(i = 0; deps[i] != NULL; i++){
		c_files = pkg_get_control_files(deps[i]);
		
		if(c_files == NULL){
			pkg_free_list(deps);
			return;
		}
		
		for(a = 0; c_files[a] != NULL; a++){
			if(!strcmp(c_files[a], C_F_REQBY))
				break;
		}
			
		asprintf(&path, "%s/%s", deps[i]->path_d, C_F_REQBY);
		assert(path != NULL);
		
		if(c_files[a] == NULL){
			fd = open(path, O_WRONLY | O_CREAT, 0);
		} else
			fd = open(path, O_WRONLY | O_APPEND, 0);
			
			
		if(fd == -1){
			free(path);
			pkg_free_list(deps);
			pkg_free_control_files(c_files);
			
			return;
		}
		
		write(fd, pkg->name, strlen(pkg->name));
		write(fd, "\n", 1);
				
		close(fd);
		free(path);
		
		pkg_free_control_files(c_files);
	}
	
	pkg_free_list(deps);
}

/**
 * @brief Removes package from dependency reqby lists
 * @param pkg a valid package descriptor
 *
 * Part of deinstallation procedure. You do not want to call this directly.
 * 
 */
void pkg_dereqby(Pkg pkg){
	Pkg *deps;
	char **c_files,
	     *path,
	     *s_ptr, *d_ptr,
	     *r_pkg;
	PkgFile pfile;
	int i, a,
	    len, d_len;
	    
	if(pkg == NULL)
		RETURN_P_ERR_VOID(P_ERR_INVALID_DESCRIPTOR);
	
	deps = pkg_get_dependencies(pkg);
	
	if(deps == NULL)
		return;
	
	for(i = 0; deps[i] != NULL; i++){
		c_files = pkg_get_control_files(deps[i]);
		
		if(c_files != NULL)
			for(a = 0; c_files[a] != NULL; a++){
				if(!strcmp(c_files[a], C_F_REQBY))
					break;
			}
		
		if(c_files != NULL && c_files[a] != NULL){
			
			asprintf(&path, "%s/%s", deps[i]->path_d, c_files[a]);
			assert(path != NULL);
			
			pfile = pkg_file_open(path);
			
			if(pfile != NULL){
				len = (pfile->f_stat.st_size + 1);
			
				s_ptr = (char *) malloc(len);
				assert(s_ptr != NULL);
				memcpy(s_ptr, pfile->data, pfile->f_stat.st_size);
				s_ptr[pfile->f_stat.st_size] = (char)0;
			
				pkg_file_close(pfile);
			
				asprintf(&r_pkg, "%s\n", pkg->name);
				assert(r_pkg != NULL);
			
				d_ptr = replace_in_string(s_ptr, r_pkg, NULL);			
				free(s_ptr);free(r_pkg);
				
				d_len = strlen(d_ptr);
				
				if(!d_len){
					remove(path);
				}
				else {
					remove(path);
					pkg_file_write(path, d_ptr, d_len);
				}
				free(d_ptr);free(path);
			}
		}
		
		pkg_free_control_files(c_files);
	}
	
	pkg_free_list(deps);
} 

/**
 * @brief processes raw instructions into executable script
 * @param step raw instruction
 * @param base pkg db base
 * @param prefix package main prefix
 * @param step_t instruction type
 *
 * Part of de/installation procedure. You do not want to call this directly.
 * 
 * Use free() manually after being done using the value.
 * @return executable script
 */
char *pkg_process_step(PkgStep step, char *base, char *prefix, 
PkgContentFile *con_fs, int step_t){
	char *p_step = NULL,
	     *f_pref,
	     *con_file_full = NULL,
	     *con_file = NULL,
	     *base_f,
	     *ptr;
	u_int i, s, ss;
	
	if(step == NULL || base == NULL || prefix == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
	
	if(base[0] == '/' && prefix[0] == '/')
		asprintf(&f_pref, "%s%s", base, &prefix[1]);
	else
		asprintf(&f_pref, "%s%s", base, prefix);
		
	switch(step_t){
		case STEP_T_EXEC:
			
			if(con_fs[0]){
				for(i = 0; con_fs[i] != NULL; i++);
				for(i--;(con_fs[i]->l_pos > step->l_pos) && i; i--);
			} else
				i = 0;
			
			if(i){
				/*%F*/
				s = strlen(con_fs[i]->file) - strlen(f_pref);
				con_file_full = malloc(s);
				assert(con_file_full != NULL);
				memset(con_file_full, 0, s);
				memmove(con_file_full, con_fs[i]->file + strlen(f_pref) + 1, s - 1);
				/*%f*/
				con_file = backtrace_ch(con_file_full, '/', s);
				assert(con_file != NULL);
				/*%B*/
				ss = strlen(f_pref);
				s = s - (strlen(con_file) + 1) + ss;
				base_f = malloc(s);
				assert(base_f != NULL);
				memset(base_f, 0, s);			
				snprintf(base_f, ss + 2, "%s/", f_pref);			
				s = strlen(con_file_full) - (strlen(con_file) + 1);
				memmove(base_f + ss + 1, con_file_full, s);
		
				p_step = ptr = replace_in_string(step -> str, "%D", f_pref);
				p_step = replace_in_string(p_step, "%F", con_file_full);
				free(ptr);ptr = p_step;
				p_step = replace_in_string(p_step, "%f", con_file);
				free(ptr);ptr = p_step;
				p_step = replace_in_string(p_step, "%B", base_f);
				free(ptr);
				free(con_file_full);
				free(base_f);
			} else
				p_step = replace_in_string(step -> str, "%D", f_pref);
		break;
		case STEP_T_DIRRM:
			asprintf(&p_step, "rmdir %s/%s 2>/dev/null || true", f_pref, step -> str);
		break;
	}
	
	free(f_pref);
	
	return p_step;
}

/**
 * @brief package removal function
 * @param pkg a valid package descriptor
 * @param pkgdb a valid package db descriptor
 * @param s_callback deinstallation step callback function
 *
 * This is main package deinstallation procedure function with callback
 * mechanism as a method to inform user about process.
 * 
 * callback function example:
 * void my_callback(
 * u_int step,    #on what step are we now
 * u_int steps,   #total amount of steps
 * char *dstr,    #step description string
 * u_short pstep_t, #step type (PSTEP_T_FRM, PSTEP_T_SCRIPT, PSTEP_T_PKGDB)
 *              #PSTEP_T_FRM - file removal, dstr set to filename
 *              #PSTEP_T_SCRIPT - script execution, dstr set to script
 *              #PSTEP_T_PKGDB - db entry manipulation, dstr set to description
 * int err_f    #step error status, 0 - no error, 1 - error, 2 - critical error
 * u_short stage_t #stage type, if stage type is STAGE_PRE_T sptep&steps values will be zeroes
 *                 #STAGE_PRE_T - action is going to be computed
 *                 #STAGE_POST_T - action has been computed
 * 
 * @return 0 - failed, 1 - success
 */
unsigned short int pkg_remove(Pkg pkg, PkgDb pkgdb, REMOVE_STEP_CALLBACK(s_callback)){
	PkgFile pfile;
	PkgContentFile *con_files;
	char **c_files,
	      *path = NULL,
	      *p_step,
	      *prefix;
	PkgStep step;
	int i, a = 0,
	    steps = 3,
	    step_t,
	    rv = 0,
	    deinstall_c_f = -1;
	
	c_files = pkg_get_control_files(pkg);
	if(c_files == NULL)
		return 0;
	
	/*counting removal steps*/
	for(i = 0; c_files[i] != NULL; i++)
		if(!strcmp(c_files[i], C_F_CONTENT)){
			asprintf(&path, "%s/%s", pkg->path_d, c_files[i]);
			assert(path != NULL);
			break;
		}
	
	if(path == NULL){
		pkg_free_control_files(c_files);
		RETURN_P_ERR(P_ERR_CONTROL_FILE_MISSING, 0);
	}
	
	pfile = pkg_file_open(path);
	
	if(pfile == NULL){
		free(path);
		pkg_free_control_files(c_files);
		return 0;
	}
	
	if(s_callback != NULL)
		steps += pkg_file_get_remove_steps(pfile);
	
	free(path);
	
	if(s_callback != NULL)
		for(i = 0; c_files[i] != NULL; i++){
			if(!strcmp(c_files[i], C_F_DEINSTALL)){
				steps += 2;
			}
		}
		
	con_files = pkg_get_content_files(pkg);
	
	for(i = 0; con_files[i] != NULL; i++);
	
	steps += i;
	
	/*removing*/
	prefix = pkg_get_prefix(pkg);
	setenv("PKG_PREFIX", (const char *)prefix, 1);
	free(prefix);
	
	/*executing predeinstallation script*/
	for(i = 0; c_files[i] != NULL; i++)
		if(!strcmp(c_files[i], C_F_DEINSTALL)){
			deinstall_c_f = 1;
			asprintf(&path, "%s/%s _%s_ DEINSTALL", pkg->path_d, 
			C_F_DEINSTALL, pkg->name);
			assert(path != NULL);
			
			if(s_callback != NULL)
			s_callback(0, 0, path, PSTEP_T_SCRIPT, 0, STAGE_PRE_T);
			
			rv = pkg_exec_script(path);
			
			if(s_callback != NULL)
			s_callback(++a, steps, path, PSTEP_T_SCRIPT, (!rv) ? 0 : 1, STAGE_POST_T);
			free(path);
			break;
		}
	
	/*deleting files*/
	for(i = 0; con_files[i] != NULL; i++){
		if(s_callback != NULL)
		s_callback(0, 0, con_files[i]->file, PSTEP_T_FRM, 0, STAGE_PRE_T);
		
		rv = remove(con_files[i]->file);
		
		if(s_callback != NULL)
		s_callback(++a, steps, con_files[i]->file, PSTEP_T_FRM, (!rv) ? 0 : 1, STAGE_POST_T);
	}
	
	/*executing scripts*/
	pfile->pos = 0;
	pfile->l_pos = 0;
	
	while((step = pkg_file_get_remove_step(pfile, &step_t)) != NULL){
		p_step = pkg_process_step(
		step, pkgdb->base, pkg_get_prefix(pkg), con_files, step_t);
		free(step -> str);
		free(step);
		
		if(s_callback != NULL)
		s_callback(0, 0, p_step, PSTEP_T_SCRIPT, 0, STAGE_PRE_T);

		rv = pkg_exec_script(p_step);
		
		if(s_callback != NULL)
		s_callback(++a, steps, p_step, PSTEP_T_SCRIPT, (!rv) ? 0 : 1, STAGE_POST_T);
		free(p_step);
	}
	pkg_file_close(pfile);
	pkg_free_content_file_list(con_files);
	
	/*post deinstall*/
	for(i = 0; c_files[i] != NULL; i++)
		if(!strcmp(c_files[i], C_F_POST_DEINSTALL)){
			asprintf(&path, "%s/%s _%s_ POST-DEINSTALL", pkg->path_d, 
			C_F_POST_DEINSTALL, pkg->name);
			assert(path != NULL);
			
			if(s_callback != NULL)
			s_callback(0, 0, path, PSTEP_T_SCRIPT, 0, STAGE_PRE_T);
			
			rv = pkg_exec_script(path);
			
			if(s_callback != NULL)
			s_callback(++a, steps, path, PSTEP_T_SCRIPT, (!rv) ? 0 : 1, STAGE_POST_T);
			free(path);
			break;
		}		
	if(!(c_files[i]) && deinstall_c_f != -1){
		asprintf(&path, "%s/%s _%s_ POST-DEINSTALL", pkg->path_d, 
		C_F_DEINSTALL, pkg->name);
		assert(path != NULL);
		
		if(s_callback != NULL)
		s_callback(0, 0, path, PSTEP_T_SCRIPT, 0, STAGE_PRE_T);
			
		rv = pkg_exec_script(path);
		
		if(s_callback != NULL)
		s_callback(++a, steps, path, PSTEP_T_SCRIPT, (!rv) ? 0 : 1, STAGE_POST_T);
		free(path);
	}
	
	/*removal from database*/
	if(s_callback != NULL)
	s_callback(0, 0, "Removing from dependency required by lists", 
	PSTEP_T_PKGDB, 0, STAGE_PRE_T);
	
	pkg_dereqby(pkg);
	
	if(s_callback != NULL)
	s_callback(++a, steps, "Removed from dependency required by lists", 
	PSTEP_T_PKGDB, 0, STAGE_POST_T);
	
	if(s_callback != NULL)
	s_callback(0, 0, "Removing control files", PSTEP_T_PKGDB, 0, STAGE_PRE_T);
	
	for(i = 0; c_files[i] != NULL; i++){
		asprintf(&path, "%s/%s", pkg->path_d, c_files[i]);
		assert(path != NULL);
		
		rv = remove(path);
		free(path);
		
		if(rv == -1)
			break;
	}
	
	if(s_callback != NULL)
	s_callback(++a, steps, "Removed control files", 
	PSTEP_T_PKGDB, (!rv) ? 0 : 2, STAGE_POST_T);
	
	if(s_callback != NULL)
	s_callback(0, 0, "Removing database entry", PSTEP_T_PKGDB, 0, STAGE_PRE_T);
	
	rv = remove(pkg->path_d);
	
	if(s_callback != NULL)
	s_callback(++a, steps, "Removed database entry", 
	PSTEP_T_PKGDB, (!rv) ? 0 : 2, STAGE_POST_T);
	
	pkg_free_control_files(c_files);
	
	return 1;
}

/**
 * @brief Extracts package origin
 * @param pkg a valid package descriptor
 *
 * Use this to get package origin in port tree.
 * 
 * @return origin string
 */
char *pkg_get_origin(Pkg pkg){
	PkgFile pfile;
	char **c_files;
	char *path,
	     *var = NULL;
	int i;
	
	if(pkg == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
	
	c_files = pkg_get_control_files(pkg);
	if(c_files == NULL)
		return NULL;
		
	for(i = 0; c_files[i] != NULL; i++)
		if(!strcmp(c_files[i], C_F_CONTENT)){
			asprintf(&path, "%s/%s", pkg->path_d, c_files[i]);
			assert(path != NULL);
			
			pfile = pkg_file_open(path);
			
			if(pfile == NULL){
				free(path);
				pkg_free_control_files(c_files);
				
				return NULL;
			}
			
			var = pkg_file_get_comment(pfile, "ORIGIN");
			
			pkg_file_close(pfile);
			
			free(path);
			break;
		}
	
	pkg_free_control_files(c_files);
	
	return var;
}

unsigned int pkg_get_install_steps
(Pkg pkg, const char *a_path, char **c_files, 
int *install_s_f, int *post_install_s_f, int *req_s_f, int *mtree_s_f){
	unsigned int steps;
	int i, a,
	    found;
	PkgFile pfile;
	char *path;
	
	if(pkg == NULL || c_files == NULL || a_path == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, 0);
	
	for(i = 0; c_files[i] != NULL; i++);
	
	a = pkg_archive_file_count(a_path);
	
	if(a == -1)
		return 0;
	
	steps = !a ? 0 : a - i;
	
	for(i = 0, found = -1; c_files[i] != NULL; i++){
		if(!strcmp(c_files[i], C_F_INSTALL)){
			*install_s_f = i;
			steps++;
		}
		else if(!strcmp(c_files[i], C_F_POST_INSTALL)){
			*post_install_s_f = i;
			steps++;
		}
		else if(!strcmp(c_files[i], C_F_MTREE_DIRS)){
			*mtree_s_f = i;
			steps++;
		}
		else if(!strcmp(c_files[i], C_F_REQUIRE)){
			if(strlen(c_files[i]) == strlen(C_F_REQUIRE)){
				*req_s_f = i;
				steps++;
			}
		}
		else if(!strcmp(c_files[i], C_F_CONTENT))
			found = i;
	}
	
	if(*install_s_f == 1 && *post_install_s_f == -1)
		steps++;
	
	if(found != -1){
		asprintf(&path, "%s/%s", pkg->path_d, c_files[found]);
		assert(path != NULL);
		pfile = pkg_file_open(path);
		free(path);
		
		if(pfile != NULL){
			steps += pkg_file_get_install_steps(pfile);
			pkg_file_close(pfile);
		} else
			return 0;
	}
	
	return steps;
}

pboolean pkg_check_files_md5(const char *path, const char *md5_digest){
	char buff[MD5HASHBUFLEN + 1], *md5_d = NULL;
	
	memset(buff, 0, sizeof(md5_d));

	md5_d = MD5File(path, buff);
	
	if(!md5_d)
		return 0;

	if(!strcmp(md5_digest, md5_d))
		return 1;
	
	return 0;
}

unsigned short int pkg_install_from_archive
(const char *a_path, PkgDb db, Pkg *pkgs, PkgFlag flags, INSTALL_STEP_CALLBACK(s_callback)){
	Pkg pkg;
	PkgFile pfile = NULL;
	PkgContentFile *con_files;
	char **c_files,
	     *buff = NULL,
	     *pstep,
	     *prefix;
	PkgStep rstep;
	int steps = 2,
	    step = 1,
	    i, r = 1,
	    install_s_f = -1,
	    post_install_s_f = -1,
	    req_s_f = -1,
	    mtree_s_f = -1;
	struct archive *a;
	
	if(a_path == NULL || db == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, 0);		
	
	/*creating database entry and counting steps*/
	pkg = pkg_archive_get_pkg((char *)a_path, db);
	
	if(pkg == NULL)
		return 0;
	
	c_files = pkg_get_control_files(pkg);
	
	if(c_files == NULL){
		pkg_free(pkg);
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, 0);
	}
	
	if(s_callback != NULL){
		steps += pkg_get_install_steps(pkg, a_path, c_files, 
		&install_s_f, &post_install_s_f, &req_s_f, &mtree_s_f);	
		s_callback(step, steps, "Created database entry", 
		PSTEP_T_PKGDB, 0, STAGE_POST_T);
	}
	
	prefix = pkg_get_prefix(pkg);
	setenv("PKG_PREFIX", (const char *)prefix, 1);
	
	#define PKG_ENTRY_CLEANUP \
		for(i = 0; c_files[i] != NULL; i++){\
			asprintf(&buff, "%s/%s", pkg->path_d, c_files[i]);\
			assert(buff != NULL);\
			remove(buff);\
			free(buff);\
		}\
		remove(pkg->path_d)
	
	/*conflict checking*/
	buff = pkg_get_conflict(pkg);
	
	if(buff != NULL){
		if(!pkg_conflict_exists(pkgs, buff)){
			PKG_ENTRY_CLEANUP;
			
			free(prefix);
			free(buff);
			pkg_free(pkg);
			pkg_free_control_files(c_files);
			
			RETURN_P_ERR(P_ERR_CONFLICT, 0);
		}
		
		free(buff); buff = NULL;
	}
	
	/*requirements script*/
	if(req_s_f != -1){
		asprintf(&buff, "%s/%s %s INSTALL", pkg->path_d, 
		c_files[req_s_f], pkg->name);
		assert(buff != NULL);
		
		if(s_callback != NULL)
			s_callback(0, 0, buff, PSTEP_T_SCRIPT, 0, STAGE_PRE_T);
		r = pkg_exec_script(buff);
		if(s_callback != NULL)
			s_callback(++step, steps, buff, PSTEP_T_SCRIPT, !r?r:1, STAGE_POST_T);
		
		free(buff);
		buff = NULL;
		
		/*clean up and quit on failure*/
		if(r != 0){
			PKG_ENTRY_CLEANUP;
			
			free(prefix);
			pkg_free(pkg);
			pkg_free_control_files(c_files);
			
			RETURN_P_ERR(P_ERR_REQ_FAIL, 0);
		}
	}
	
	/*preinstall script*/
	if(install_s_f != -1){
		asprintf(&buff, "%s/%s %s PRE-INSTALL", pkg->path_d, 
		c_files[install_s_f], pkg->name);
		assert(buff != NULL);
		
		if(s_callback != NULL)
			s_callback(0, 0, buff, PSTEP_T_SCRIPT, 0, STAGE_PRE_T);
		r = pkg_exec_script(buff);
		if(s_callback != NULL)
			s_callback(++step, steps, buff, PSTEP_T_SCRIPT, !r?r:1, STAGE_POST_T);
		
		free(buff);
		buff = NULL;
	}
	
	/*extracting files*/	
	a = pkg_archive_open(a_path);
	
	if(a == NULL){
		pkg_free(pkg);

		RETURN_P_LARCHIVE(0, a);
	}
	
	con_files = pkg_get_content_files(pkg);
	
	if(con_files == NULL)
		return 0;
	
	r = 1;
	
	PkgContentFile con_file = NULL;
	int rv = 1;
	char *ptr;
	
	while(r){
		r = pkg_archive_extract(pkg, con_files, a, &buff, &con_file, 
		(const char *)prefix, s_callback);
		
		if(!r)
			break;
			
		if(r == -1){
			ptr = buff;
			asprintf(&buff, "%s (%s)", buff, pkg_get_error());
			assert(buff != NULL);
			
			free(ptr);
		}

		if((flags & PKG_DIGEST_CH_F) && rv != -1){
			rv = pkg_check_files_md5(con_file -> file, con_file -> o_md5_hash);
			
			if(!rv){
				ptr = buff;
				asprintf(&buff, "%s (MD5 digest comparison failed)", buff);
				assert(buff != NULL);
			
				free(ptr);
			}
		}
		
		if(s_callback != NULL)
			s_callback(++step, steps, buff, PSTEP_T_FCPY, 
			(r == -1 || !rv)?1:0, STAGE_POST_T);
		
		if(buff != NULL){
			free(buff);
			buff = NULL;
		}
	}
	pkg_archive_close(a);
	
	/*running mtree*/
	if(mtree_s_f != -1){
		asprintf(&buff, "mtree -U -f %s/%s -d -e -p %s", pkg->path_d, 
		c_files[mtree_s_f], prefix);
		assert(buff != NULL);
		
		if(s_callback != NULL)
			s_callback(0, 0, buff, PSTEP_T_SCRIPT, 0, STAGE_PRE_T);
		r = pkg_exec_script(buff);
		if(s_callback != NULL)
			s_callback(++step, steps, buff, PSTEP_T_SCRIPT, !r?r:1, STAGE_POST_T);
		
		free(buff);
		buff = NULL;
	}
	
	/*running scripts*/
	asprintf(&buff, "%s/%s", pkg->path_d, C_F_CONTENT);
	assert(buff != NULL);

	pfile = pkg_file_open(buff);
	assert(pfile != NULL);
	
	free(buff);buff = NULL;
	
	while((rstep = pkg_file_get_install_step(pfile)) != NULL){
		pstep = pkg_process_step(rstep, db->base, prefix, con_files, STEP_T_EXEC);
		
		if(s_callback != NULL)
			s_callback(0, 0, pstep, PSTEP_T_SCRIPT, 0, STAGE_PRE_T);
		r = pkg_exec_script(pstep);		
		if(s_callback != NULL)
			s_callback(++step, steps, pstep, PSTEP_T_SCRIPT, !r?r:1, STAGE_POST_T);
		
		free(rstep -> str);
		free(rstep);
		free(pstep);
	}
	
	pkg_file_close(pfile);
	pkg_free_content_file_list(con_files);
	
	/*add to reqby lists*/
	if(s_callback != NULL)
		s_callback(0, 0, "Registering with dependencies", 
		PSTEP_T_PKGDB, 0, STAGE_PRE_T);
	
	pkg_reqby(pkg);
	
	if(s_callback != NULL)
		s_callback(++step, steps, "Registered with dependencies", 
		PSTEP_T_PKGDB, 0, STAGE_POST_T);
		
	/*postinstall*/
	if(post_install_s_f != -1 || install_s_f != -1){
		if(post_install_s_f != -1){
			asprintf(&buff, "%s/%s %s POST-INSTALL", pkg->path_d, 
			C_F_POST_INSTALL, pkg->name);
		} else if(install_s_f != -1){
			asprintf(&buff, "%s/%s %s POST-INSTALL", pkg->path_d, 
			C_F_INSTALL, pkg->name);
		}
		assert(buff != NULL);
		
		if(s_callback != NULL)
			s_callback(0, 0, buff, PSTEP_T_SCRIPT, 0, STAGE_PRE_T);
		r = pkg_exec_script(buff);
		if(s_callback != NULL)
			s_callback(++step, steps, buff, PSTEP_T_SCRIPT, !r?r:1, STAGE_POST_T);
		
		free(buff);
		buff = NULL;
	}
	
	free(prefix);
	pkg_free_control_files(c_files);
	pkg_free(pkg);
	
	return 1;
}

char *pkg_get_comment(Pkg pkg){
	if(pkg == NULL)
		RETURN_P_ERR(P_ERR_INVALID_DESCRIPTOR, NULL);
	
	return pkg_read_control_file(pkg, C_F_COMMENT);
}
