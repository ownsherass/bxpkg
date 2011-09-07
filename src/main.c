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

#include <getopt.h>

#include "xpkg.h"

xpkg XPKG;

/*The releng to folder table*/
struct rel_table r_t[] = {
		{"packages-7-stable"   , "7.0-STABLE"     },
		{"packages-7.2-release", "7.2-RELEASE"    },
		{"packages-7.3-release", "7.3-RELEASE"    },
		{"packages-7.4-release", "7.4-RELEASE"    },
		{"packages-8-stable"   , "8.1-STABLE"     },
		{"packages-8-stable"   , "8.2-STABLE"     },
		{"packages-8.1-release", "8.1-RELEASE"    },
		{"packages-8.2-release", "8.2-PRERELEASE" },
		{"packages-8.2-release", "8.2-RELEASE"    },
		{"packages-9-current"  , "9.0-CURRENT"    },
		{NULL                  , NULL             }
};

void print_usage(void){
	printf("bxpkg [options]\n");
	printf(" [options]:\n");
	printf("  -h --help             : Show these instructions\n");
	printf("  -r --no-root          : Do not try to open as root\n");
	printf("  -b --pkg-base [path]  : Set the root path\n");
	printf("  -p --port-base [path] : Set the port base path\n");
	printf("  -s --repo-site [URL]  : Set repository URL\n");
}

struct xpkg_config_str * get_config(int argc, char **argv){
	struct xpkg_config_str *config;
	struct utsname un;
	u_int a;
	
	int copt,
	    oindex;
	    
	char *home = getenv("HOME");
	
	config = (void *)malloc(sizeof(struct xpkg_config_str));
	assert(config != NULL);
	
	memset(config, 0, sizeof(struct xpkg_config_str));
	
	/*default values*/	
	char *packageroot = NULL,
	     *packagesite = NULL;

	if((packageroot = getenv("PACKAGEROOT")) != NULL){
		uname(&un);

		for(a = 0; r_t[a].r != NULL; a++)
			if(!strncmp(un.release, r_t[a].l, strlen(r_t[a].l)))
				break;

		asprintf(&(config -> repo_url), "%s%s/%s/%s", 
		packageroot, DEF_REMOTE_REPO_PATH, un.machine, r_t[a].r);
	}
	else if((packagesite = getenv("PACKAGESITE")) != NULL){
		asprintf(&(config -> repo_url), "%s..", packagesite);
	}
	
	config -> root_f = TRUE;
	config -> pkg_base = strdup("/");
	config -> ports_base = strdup("/usr/ports");
	config -> cache_db = g_strdup_printf("%s/%s", CACHE_D, "index.db");
	config -> cache_d = strdup(CACHE_D);
	
	assert(config -> pkg_base != NULL);
	assert(config -> ports_base != NULL);
	assert(config -> cache_db != NULL);
	assert(config -> cache_d != NULL);
	
	static struct option loptions[] = {
		{"help",      no_argument,        0, 'h'},
		{"no-root",   no_argument,        0, 'r'},
		{"pkg-base",  required_argument,  0, 'b'},
		{"port-base", required_argument,  0, 'p'},
		{"repo-site", required_argument,  0, 's'},
		{0,           0,                  0,  0 }
	};

	for(;;){
		oindex = 0;
		
		copt = getopt_long(argc, argv, "hrb:p:s:", loptions, &oindex);
		
		if(copt == -1)
			break;
			
		switch(copt){
			case 'h':
				print_usage();
				exit(0);
			break;
			case 'r':
				config -> root_f = FALSE;
				if(home == NULL){
					xpkg_error("Home variable is not set in shell!");

					exit(1);
				}
				
				g_free((void *)config -> cache_db);				
				config -> cache_db = 
				g_strdup_printf("%s/.config/bxpkg/index.db", home);
				free((void *)config -> cache_d);
				config -> cache_d = 
				g_strdup_printf("%s/.config/bxpkg", home);
			break;
			case 'b':
				free((void *)config -> pkg_base);
				config -> pkg_base = strdup(optarg);
			break;
			case 'p':
				free((void *)config -> ports_base);
				config -> ports_base = strdup(optarg);
			break;
			case 's':
				config -> repo_url = strdup(optarg);
			break;
			case '?':
				print_usage();
				exit(1);
			break;
		}
	}
	
	if(config -> repo_url == NULL){
		uname(&un);
		
		for(a = 0; r_t[a].r != NULL; a++)
			if(!strncmp(un.release, r_t[a].l, strlen(r_t[a].l)))
				break;
				
		if(r_t[a].r != NULL){
			asprintf(&(config -> repo_url), "%s%s/%s/%s",
			DEF_REMOTE_REPO_SITE, DEF_REMOTE_REPO_PATH, un.machine, r_t[a].r);
			assert(config -> repo_url != NULL);
		}
	}

	return config;
}

int main(int argc, char **argv){
	pam_handle_t *pamh = NULL;
	int uid,  gid,
		euid, egid,
		suid, sgid,
		rval;
	pid_t pid;
	char hostname[MAXHOSTNAMELEN],
		 *shmem,
		 *ptr;
	
	memset(&XPKG, 0, sizeof(xpkg));
	
	getresuid(&uid, &euid, &suid);
	getresgid(&gid, &egid, &sgid);
	
	/*You are not in wheel, get out!*/
	/*if(gid != 0){
		xpkg_error("Permission denied!");
		exit(1);
	}*/
	
	XPKG.config = get_config(argc, argv);
	XPKG.argc = argc;
	XPKG.argv = argv;
	
	if(!XPKG.config -> root_f){
		/*break SUID0 on --no-root*/
		if((!euid || !suid) && (uid > 0)){
			setresuid(uid, uid, uid);
			XPKG.config -> root_f = FALSE;
		}
		else if(!uid)
			xpkg_error("Ignoring --no-root while running from root.");
		else
			XPKG.config -> root_f = FALSE;
	}
	/*SUID0 mode implementation, secure GUI used */
	else if(!euid && euid != uid){
		XPKG.shmem = shmem = mmap(NULL, (MAX_PASS_LEN + 1), 
		PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
		assert(shmem != (void *)-1);
		memset(shmem, 0, (MAX_PASS_LEN + 1));
		
		pid = fork();
		assert(pid != -1);
		
		if(!pid){
			setresuid(uid, uid, uid);
			
			XPKG.gtk_init_f = TRUE;
			gtk_init(&argc, &argv);

			ptr = (char *)xpkg_pass_promt_dialog();
			if(ptr == NULL)
				return 1;
			
			strncpy(shmem, ptr, MAX_PASS_LEN);
			free(ptr);
			
			return 0;
		}
		wait(&rval);
		
		if(rval != 0)
			return 1;			
		
		/*PAM here*/
		static struct pam_conv conv = {
			&pamconv,
			NULL
		};
		
		setuid(0);

		rval = pam_start("xpkg", "root", &conv, &pamh);
		assert(rval == PAM_SUCCESS);
			
		gethostname(hostname, sizeof(hostname));
		rval = pam_set_item(pamh, PAM_RHOST, hostname);
		assert(rval == PAM_SUCCESS);
			
		rval = pam_set_item(pamh, PAM_RUSER, "root");
		assert(rval == PAM_SUCCESS);
			
		rval = pam_set_item(pamh, PAM_TTY, ttyname(STDERR_FILENO));
		assert(rval == PAM_SUCCESS);

		rval = pam_authenticate(pamh, 0);
		if(rval != PAM_SUCCESS){
			xpkg_error_secure_gui("Authentication failed!");
			return 1;
		}
		
		rval = pam_end(pamh, rval);
		assert(rval == PAM_SUCCESS);
		
		memset(shmem, 0, MAX_PASS_LEN);
		munmap(shmem, (MAX_PASS_LEN + 1));		
	}
	else if(euid != 0 && suid != 0){
		XPKG.gtk_init_f = 0;
		xpkg_error("Permission denied! Try with --no-root");
		exit(1);
	}
	
	if(!XPKG.gtk_init_f){
		XPKG.gtk_init_f = TRUE;
		g_thread_init(NULL);
		gdk_threads_init();
		gtk_init(&argc, &argv);
	}
	
	/*lock settings*/
	XPKG.pkg_info_load_l = FALSE;
	XPKG.pkg_search_l = FALSE;
	XPKG.pkg_delete_l = FALSE;
	XPKG.pkg_install_l = FALSE;
	
	xpkg_show_splashscreen();
	
	/*xpkg_create_main_window();*/
			
	xpkg_cleanup(XPKG);
	return 0;
}
