#include <pkg.h>

#define PKG_BASE "/"

int main(int argc, char **argv){
	PkgDb db;
	Pkg *pkgs;
	char *comment;
	
	db = pkg_db_open(PKG_BASE);
	
	if(db == NULL){
		fprintf(stderr, "Failed accessing %s as package base.\n", PKG_BASE);
		perror(NULL);
		return 1;
	}
	pkgs = pkg_db_get_installed(db);
		
	for(int i = 0; pkgs[i] != NULL; i++){			
		comment = pkg_get_comment(pkgs[i]);
			
		if(comment == NULL){
			fprintf(stderr, "Failed reading comment!\n");
			perror(NULL);
				
			pkg_free_list(pkgs);
			pkg_db_close(db);
				
			return 1;
		}
			
		printf("%s - %s", pkg_get_name(pkgs[i]), comment);
			
		free(comment);
	}
				
	pkg_free_list(pkgs);	
	pkg_db_close(db);
	
	return 0;
}
