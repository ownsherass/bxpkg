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

struct IndexEntryPage {
	char *data;
	size_t size;
};

typedef unsigned int **EntryPageMap;

enum {
	M_START,
	M_END,
	M_FIELDS
};

#define READ_BUFF_S 10240

EntryPageMap pkg_get_index_page_map(const char *index_path){
	unsigned int i, e = 0, a;
	char r_buf[READ_BUFF_S];
	int fd;
	size_t s, s_o = 0;
	EntryPageMap epm = NULL;
	
	fd = open(index_path, O_RDONLY);
	
	if(fd == -1)
		RETURN_P_ERRNO((EntryPageMap)-1);
		
	memset(r_buf, 0, READ_BUFF_S);
	
	for(;;){
		memset(r_buf, 0, sizeof(r_buf));
		if((s = read(fd, r_buf, READ_BUFF_S - 1)) <= 0)
			break;
		
		for(a = i = 0; r_buf[i] != '\0'; i++){
			if(r_buf[i] == '\n'){
				epm = (void *)reallocf(epm, sizeof(unsigned int *) * (++e));
				assert(epm != NULL);
				
				epm[(e - 1)] = (int *)malloc(sizeof(unsigned int) * M_FIELDS);
				assert(epm[(e - 1)] != NULL);
				
				epm[(e - 1)][M_START] = (e == 1) ? 0 : (epm[(e - 2)][M_END] + 1);
				epm[(e - 1)][M_END] = (s_o + i);
			}
		}
		
		s_o += s;
	}
	
	epm = (void *)reallocf(epm, sizeof(unsigned int *) * (++e));
	assert(epm != NULL);
	epm[(e - 1)] = NULL;
	
	close(fd);
	
	return epm;
}

void pkg_free_page_map(EntryPageMap epm){
	unsigned short int i = 0;
	
	if(epm == NULL)
		return;
	
	for(; epm[i] != NULL; i++)
		free(epm[i]);
		
	free(epm);
}

char *trace_ch(char *ptr, char ch, size_t s_limit){
	size_t s;
	
	for(s = 0; (s != s_limit) && (ptr[s] != ch); s++);
	
	if(s == s_limit && ptr[s] != ch)
		return NULL;
	
	return &ptr[s];
}

char *backtrace_ch(char *ptr, char ch, size_t s_limit){
	size_t s;
	
	for(s = (s_limit - 1); (s != 0) && (ptr[s] != ch); s--);
	
	if(s == 0 && ptr[s] != ch)
		return NULL;
	
	return &ptr[(s + 1)];
}

char *backtrace_ch_n(char *ptr, char ch, size_t s_limit, u_int n){
	size_t s;
	u_int i;
	
	for(s = (s_limit - 1), i = 0; i != n; i++){
		for(; (s != 0) && (ptr[s] != ch); s--);
		
		if(s == 0 && ptr[s] != ch)
			return NULL;
		s--;
	}
	
	return &ptr[(s + 1)];
}

short int pkg_parse_entry_page(
struct IndexEntryPage ire, PkgIdxEntry *pie, char **deps){
	char *ptr, *ptr_o, *ptr_t;
	size_t s, ss, st = 0;
	
	s = ire.size;
	ptr = ire.data;
	
	#define DO_READ                                 \
	ptr_o = ptr;                                    \
	if(ptr_o[0] == '|') ptr_o++;                    \
	ptr = trace_ch(ptr + 1, '|', s);                \
	                                                \
	if(ptr == NULL)	RETURN_P_ERR(P_ERR_SYNTAX, -1); \
	                                                \
	ss = (ptr - ptr_o);                             \
	s -= ss
	
	/*name & version*/
	DO_READ;
	
	if(ss != 0){
		ptr_t = backtrace_ch(ptr_o, '-', ss);
		if(ptr_t != NULL){
			st = (ptr - ptr_t);
			if(st > 0)
				memcpy(pie -> version, ptr_t, 
				(st < IDX_FIELD_SIZE_S) ? st : (IDX_FIELD_SIZE_S - 1));
		}
		if((ss - 1) > st){
			st = (ss - 1) - st;
			memcpy(pie -> name, ptr_o, 
			(st < IDX_FIELD_SIZE) ? st : (IDX_FIELD_SIZE - 1));
		}
	}
	
	/*port base*/
	DO_READ;
	
	if(ss != 0)
		memcpy(pie -> port_base, ptr_o, 
		(ss < IDX_FIELD_SIZE) ? ss : (IDX_FIELD_SIZE - 1));
	
	/*prefix*/
	DO_READ;
	
	if(ss != 0)
		memcpy(pie -> prefix, ptr_o, 
		(ss < IDX_FIELD_SIZE) ? ss : (IDX_FIELD_SIZE - 1));
	
	/*description*/
	DO_READ;
	
	if(ss != 0)
		memcpy(pie -> descr, ptr_o, 
		(ss < IDX_FIELD_SIZE_L) ? ss : (IDX_FIELD_SIZE_L - 1));
	
	/*long description file*/
	DO_READ;
	
	if(ss != 0)
		memcpy(pie -> descr_f, ptr_o, 
		(ss < IDX_FIELD_SIZE) ? ss : (IDX_FIELD_SIZE - 1));
	
	/*maintainer*/
	DO_READ;
	
	if(ss != 0)
		memcpy(pie -> maintainer, ptr_o, 
		(ss < IDX_FIELD_SIZE) ? ss : (IDX_FIELD_SIZE - 1));
	
	/*category*/
	DO_READ;
	
	if(ss != 0)
		memcpy(pie -> category, ptr_o, 
		(ss < IDX_FIELD_SIZE_S) ? ss : (IDX_FIELD_SIZE_S - 1));
	
	/*dependencies (build)*/
	DO_READ;	
	
	/*dependencies (run)*/
	DO_READ;
	if(ss != 0){
		*deps = (void *)malloc(ss + 1);
		assert(*deps != NULL);
		
		memset(*deps, 0, ss + 1);		
		memcpy(*deps, ptr_o, ss);
	}
	else
		*deps = NULL;
	
	/*website*/
	DO_READ;
	
	if(ss != 0)
		memcpy(pie -> www, ptr_o, 
		(ss < IDX_FIELD_SIZE) ? ss : (IDX_FIELD_SIZE - 1));

	#undef DO_READ
	
	return 1;
}

unsigned short int pkg_cache_entry(PkgIdxEntry *pie, char *deps, DB *dbp){
	DBT key, data;
	char *ptr = NULL;
		
	asprintf(&ptr, "%s-%s", pie -> name, pie -> version);
	assert(ptr != NULL);
	
	key.size = strlen(ptr);
	key.data = (void *)ptr;
	
	data.size = sizeof(PkgIdxEntry);
	data.data = (void *)pie;
	
	if(dbp -> put(dbp, &key, &data, 0) != 0){
		free(ptr);
		dbp -> close(dbp);
		RETURN_P_ERRNO(0);
	}
	
	free(ptr);
	
	return 1;
}

u_short pkg_cache_unit(char *u_key, char *u_data, size_t s_data, DB *dbp){
	DBT key, data;
	
	key.size = strlen(u_key);
	key.data = (void *)u_key;
	
	data.size = s_data;
	data.data = (void *)u_data;
	
	if(dbp -> put(dbp, &key, &data, 0) != 0){
		dbp -> close(dbp);
		RETURN_P_ERRNO(0);
	}
	
	return 1;
}

u_int pkg_get_cached_entries(DB *dbp){
	u_int entries = 0;
	DBT key, data;
	int rv;
	
	key.data = "+ENTRIES";
	key.size = strlen(key.data);
	
	data.size = 0;
	data.data = NULL;
	
	if((rv = dbp -> get(dbp, &key, &data, 0)) == -1){
		RETURN_P_ERRNO(0);
	} else if(rv == 1)
		return 0;
	
	if(data.size != sizeof(u_int))
		return 0;

	memmove(&entries, data.data, sizeof(u_int));
	
	return entries;
}

int pkg_get_cached_unit(char *u_key, char **u_data, DB *dbp){
	DBT key, data;
	int rv;
	
	key.size = strlen(u_key);
	key.data = (void *)u_key;
	
	data.size = 0;
	data.data = NULL;
	
	if((rv = dbp -> get(dbp, &key, &data, 0)) == -1){
		RETURN_P_ERRNO(-1);
	} else if(rv == 1){
		return 0;
	}
	
	*u_data = data.data;
	
	return data.size;
}

short int pkg_get_cached_entry(DB *dbp, PkgIdxEntry *pie, unsigned short int *first){
	DBT key, data;
	unsigned int flag;
	int rval;
	char *ptr;

	if(*first){
		flag = R_FIRST;
		*first = 0;
	}
	else
		flag = R_NEXT;
		
	rval = dbp -> seq(dbp, &key, &data, flag);
	ptr = (char *)key.data;
	
	if(rval == -1)
		RETURN_P_ERRNO(-1);
		
	if(rval == 1)
		return 0;

	if(ptr[0] == '+' || ptr[0] == '-')
		return 2;
		
	if(data.size != sizeof(PkgIdxEntry))
		RETURN_P_ERR(P_ERR_DATA_CRRUPTED, -1);
	
	memset(pie, 0, sizeof(PkgIdxEntry));
	memcpy(pie, data.data, data.size);
	
	return 1;
}

char * pkg_get_cached_deps(DB *dbp, char *pname){
	char *deps, *ptr, *str;
	int rv;
	
	asprintf(&str, "+%s", pname);
	assert(str != NULL);
	
	rv = pkg_get_cached_unit(str, &ptr, dbp);
	
	free(str);
	
	if(rv > 0){
		deps = malloc(rv + 1);
		assert(deps != NULL);
		memmove(deps, ptr, rv);
		deps[rv] = (char)0;
	}
	else
		return NULL;
		
	return deps;
}

char * pkg_get_cached_rdeps(DB *dbp, char *pname){
	char *rdeps, *ptr, *str;
	int rv;
	
	asprintf(&str, "-%s", pname);
	assert(str != NULL);
	
	rv = pkg_get_cached_unit(str, &ptr, dbp);
	
	free(str);
	
	if(rv > 0){
		rdeps = malloc(rv + 1);
		assert(rdeps != NULL);
		memmove(rdeps, ptr, rv);
		rdeps[rv] = (char)0;
	}
	else
		return NULL;
		
	return rdeps;
}

void pkg_cache_append_rdep(DB *dbp, char *pname, char *dname){
	char *rdeps, *ptr, *str;
	int rv;
	size_t s;
	
	asprintf(&str, "-%s", pname);
	assert(str != NULL);
	
	rv = pkg_get_cached_unit(str, &ptr, dbp);
	
	if(rv > 0){
		s = strlen(dname);
		
		rdeps = malloc(rv + s + 3);
		assert(rdeps != NULL);
		memset(rdeps, 0, rv + s + 3);
		
		memmove(rdeps, ptr, rv);
		rdeps[rv] = ' ';
		memmove(&rdeps[rv + 1], dname, s);
		
		if(!pkg_cache_unit(str, rdeps, s + rv + 1, dbp))
			printf("error!\n");

		free(rdeps);
	} else {
		pkg_cache_unit(str, dname, strlen(dname), dbp);
	}
	
	free(str);
}

void pkg_cache_rdeps(DB *dbp, char *pname, char *deps){
	u_int i, ii;
	char *dep;
	size_t s;
	
	for(i = ii = 0; deps[i]; i++){
		if(deps[i] == ' ' && deps[i + 1]){
			s = i - ii;
			dep = malloc(s + 1);
			assert(dep != NULL);
			memset(dep, 0, s + 1);
			
			memmove(dep, &deps[ii], s);
			
			pkg_cache_append_rdep(dbp, dep, pname);
			
			free(dep);
			
			ii = i + 1;
		}
	}
	if(i){
		s = i - ii;
		dep = malloc(s + 1);
		assert(dep != NULL);
		memset(dep, 0, s + 1);
		
		memmove(dep, &deps[ii], s);
		
		pkg_cache_append_rdep(dbp, dep, pname);
			
		free(dep);
	}
}

unsigned short int pkg_cache_index(
const char *index_path, const char *cache_path, char *md5_str,
CACHE_STEP_CALLBACK(c_callback)){
	DB *dbp = NULL;
	u_int entries, entry = 0, i;
	int fd = 0;
	PkgIdxEntry i_entry;
	struct IndexEntryPage ire;
	EntryPageMap epm = NULL;
	char *deps = NULL, *ptr = NULL, *ptr2 = NULL;
	
	/*getting page locations*/
	epm = pkg_get_index_page_map(index_path);
	if(epm == (void *)-1)
		return 0;
		
	for(entries = 0; epm[entries] != NULL; entries++);
	
	if(!entries)
		goto done;
	
	dbp = dbopen(cache_path, O_RDWR | O_CREAT, DEF_O_MODE, DB_BTREE, NULL);
		
	if(dbp == NULL)
		goto done;

	fd = open(index_path, O_RDONLY);

	if(fd == -1){
		pkg_free_page_map(epm);

		RETURN_P_ERRNO(0);
	}
	
	pkg_cache_unit("+VERSION", PKG_INDEX_VERSION, strlen(PKG_INDEX_VERSION) + 1, dbp);
	pkg_cache_unit("+MD5", md5_str, strlen(md5_str), dbp);

	/*mapping the pages into memory*/
	for(i = 0; epm[i] != NULL; i++){
		ire.size = epm[i][M_END] - epm[i][M_START];
		if(ire.size  > 0){
			if(c_callback != NULL)
				c_callback(entry, entries, 
				!entry ? 0 : (double)((double)entry / (double)entries), NULL);
			ire.data = mmap(NULL, ire.size, PROT_READ,
			MAP_SHARED, fd, epm[i][M_START]);

			if(ire.data == MAP_FAILED || ire.data == (void *)-1){
				close(fd);
				pkg_free_page_map(epm);

				return 0;
			}

			/*parsing page*/
			memset(&i_entry, 0, sizeof(i_entry));
			deps = NULL;

			pkg_parse_entry_page(ire, &i_entry, &deps);
			
			/*caching entry*/
			if(!pkg_cache_entry(&i_entry, NULL, dbp))
				return 0;
			
			if(deps != NULL){
				asprintf(&ptr, "+%s-%s", i_entry.name, i_entry.version);
				assert(ptr != NULL);
				pkg_cache_unit(ptr, deps, strlen(deps), dbp);				
				free(ptr);
				
				asprintf(&ptr, "%s-%s", i_entry.name, i_entry.version);
				pkg_cache_rdeps(dbp, ptr, deps);
				free(ptr);				
				free(deps);
			}
			
			/*caching origin*/
			ptr2 = backtrace_ch_n(i_entry.port_base, '/', strlen(i_entry.port_base), 2) + 1;
			
			if(ptr2){
				asprintf(&ptr, "%s-%s", i_entry.name, i_entry.version);
				asprintf(&ptr2, "+%s", ptr2);
				assert(ptr2 != NULL && ptr != NULL);
				pkg_cache_unit(ptr2, ptr, strlen(ptr) + 1, dbp);
				
				free(ptr);
				free(ptr2);
			}
			
			munmap(ire.data, ire.size);
			
			if(c_callback != NULL){
				entry++;
				c_callback(entry, entries, 
				(double)((double)entry / (double)entries), NULL);
			}
		}
	}
	
	pkg_cache_unit("+ENTRIES", (void *)&entries, sizeof(u_int), dbp);
	
	done:
	
	if(fd != 0)
		close(fd);
	
	if(epm != NULL)
		pkg_free_page_map(epm);
	
	if(dbp -> close(dbp) != 0){printf("close() made a bubu\n");
		RETURN_P_ERRNO(0);}
	
	return 1;
}
