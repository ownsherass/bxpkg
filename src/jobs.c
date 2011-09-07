#include "xpkg.h"

xpkgJob * xpkg_select_job_by_name(gchar *jname, xpkgJob *jobs){
	u_int i;
	
	if(jname == NULL || jobs == NULL)
		return NULL;
	
	for(i = 0; jobs[i].name != NULL; i++){
		if(!strcmp(jobs[i].name, jname))
			return &jobs[i];
	}
	
	return NULL;
}

xpkgJob * xpkg_append_job(xpkgJob *jobs, xpkgJob job){
	size_t s, ss;
	xpkgJob *js;
	
	if(jobs == NULL){
		s = 0;
	} else
		for(s = 0; jobs[s].name != NULL; s++);
	
	ss = sizeof(xpkgJob) * (s + 2);
	
	js = realloc(jobs, ss);
	assert(js != NULL);

	memmove(&js[s], &job, sizeof(xpkgJob));
	memset(&js[s + 1], 0, sizeof(xpkgJob));
	
	return js;
}

xpkgJob * xpkg_short_jobs_by_priority(xpkgJob *jobs){
	xpkgJob *js = NULL;
	short hpriority = 0;
	u_int a;
	
	if(jobs == NULL)
		return NULL;
	
	for(a = 0; jobs[a].name != NULL; a++){
		if(hpriority < jobs[a].priority)
			hpriority = jobs[a].priority;
	}
	
	for(; hpriority != -1; hpriority--){
		for(a = 0; jobs[a].name != NULL; a++){
			if(jobs[a].priority == hpriority){
				js = xpkg_append_job(js, jobs[a]);				
			}
		}
	}
	
	xpkg_free_jobs(jobs, FALSE);
		
	return js;
}

gboolean xpkg_get_pie(gchar *pname, PkgIdxEntry *pie){
	char *u_data;
	int rv;
	
	DB_OPEN;
	rv = pkg_get_cached_unit(pname, &u_data, DB_H);
	
	if(rv != sizeof(PkgIdxEntry)){
		DB_CLOSE;
		return FALSE;
	}
	
	memmove(pie, u_data, sizeof(PkgIdxEntry));	
	DB_CLOSE;
	
	return TRUE;
}

gboolean xpkg_job_exists(xpkgJob *jobs, gchar *pname){
	size_t s;
	
	if(jobs == NULL)
		return FALSE;
	
	for(s = 0; jobs[s].name != NULL; s++){
		if(!strcmp(pname, jobs[s].name))
			return TRUE;
	}
	
	return FALSE;
}

xpkgJob * xpkg_get_archive_jobs(xpkg_archive_info a_info, xpkgJob *jobs){
	xpkgJob job, *js = jobs;
	
	memset(&job, 0, sizeof(xpkgJob));
	
	if(!xpkg_job_exists(js, (gchar *)a_info->full_name) &&
	xpkg_pkg_exists(a_info->full_name) == FALSE){
		job.name = strdup(a_info->full_name);
		job.type = J_INSTALL;
		job.data = strdup(a_info->path);
		
		js = xpkg_append_job(js, job);
	}
	
	return js;
}

static inline gboolean _is_leaf(Pkg pkg, xpkgJob *js){
	gboolean i_f = FALSE;
	
	if(!js)
		return TRUE;
	
	for(u_int n = 0; js[n].name; n++){
		if(!strcmp(pkg_get_name(pkg), js[n].name))
			i_f = TRUE;
	}
	
	return i_f;
}

xpkgJob * xpkg_get_remove_jobs(Pkg pkg, gboolean r_leafs, xpkgJob *jobs){
	xpkgJob job, *js = jobs;
	
	memset(&job, 0, sizeof(xpkgJob));
	
	if(!xpkg_job_exists(js, (gchar *)pkg_get_name(pkg))){
		job.name = strdup(pkg_get_name(pkg));
		job.type = J_DELETE;
		
		js = xpkg_append_job(js, job);
	}
	
	if(r_leafs == TRUE){
		Pkg *deps, *rdeps;
		u_int i, ii;
		gboolean is_leaf = FALSE;

		deps = pkg_get_dependencies(pkg);
		
		if(deps != NULL){
			for(i = 0; deps[i] != NULL; i++){				
				rdeps = pkg_get_rdependencies(deps[i]);
				
				if(rdeps != NULL){
					for(ii = 0; rdeps[ii] != NULL; ii++){
						is_leaf = _is_leaf(rdeps[ii], js);
						
						if(!is_leaf)
							break;
					}
					if(is_leaf)
						js = xpkg_get_remove_jobs(deps[i], r_leafs, js);
				}
				pkg_free_list(rdeps);
			}

			pkg_free_list(deps);
		}
	}
	
	return js;
}

xpkgJob * xpkg_get_jobs(gchar *pname, xpkgJob *jobs, u_short priority){
	xpkgJob job, *js = jobs;
	PkgIdxEntry pie;
	xpkg_cache_pkg_ht c_pkg_ht;
	gboolean found = FALSE, dversion = FALSE;
	gchar *deps, *dep, *org;
	
	memset(&job, 0, sizeof(xpkgJob));
	
	if(xpkg_get_pie(pname, &pie) == FALSE){
		gdk_threads_enter();
		xpkg_error("Could not locate entry for '%s'.\n"
		"Cache is corrupted or repositorie's index incomplete!", pname);
		gdk_threads_leave();
		
		return js;
	}
	
	if(xpkg_pkg_exists(pname)){
		found = TRUE;
	} else {
		u_int i, ii;
		
		for(ii = 0, i = strlen(pie.port_base); i != 0 && ii != 2; i--)
			if(pie.port_base[i] == '/')
				ii++;
		org = &pie.port_base[i + 2];
			
		if((c_pkg_ht = g_hash_table_lookup(XPKG.installed_pkgs.cache_hts.org_ht,
		org)) != NULL){
			dversion = TRUE;
		}
	}
	
	if(!found && !xpkg_job_exists(js, pname)){
		job.name = strdup(pname);
		job.priority = priority;
		
		if(!dversion){
			job.type = J_GET;
		} else {
			job.type = J_CH_VERSION;
			asprintf(&job.data, "%s", pkg_get_name(c_pkg_ht->pkg_s));
		}
		
		js = xpkg_append_job(js, job);
	}
	else
		return js;
	
	DB_OPEN;
	deps = pkg_get_cached_deps(DB_H, pname);
	DB_CLOSE;
	
	if(deps != NULL){
		size_t pos = 0;
		for(;;){
			dep = next_word(deps, &pos);
			if(dep == NULL)
				break;
			
			js = xpkg_get_jobs(dep, js, (priority + 1));			
			free(dep);
		}
		free(deps);
	}
	
	return js;
}

xpkgJob * xpkg_get_update_jobs(GPtrArray *u_array){
	xpkgJob *jobs = NULL;
	u_int i;
	gchar *pkg_u;
	
	for(i = 0; i != u_array -> len; i++){
		pkg_u = g_ptr_array_index(u_array, i);		
		jobs = xpkg_get_jobs(pkg_u, jobs, 0);
	}
	
	xpkg_free_array(u_array);
	
	return jobs;
}
