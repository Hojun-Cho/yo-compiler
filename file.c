#include "yo.h"
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

char* 
getpath(void)
{
	char buf[8192] = {0};

	char *dir = getcwd(buf, sizeof(buf));	
	int l = strlen(dir);
	char *d = new(l+1);
	memcpy(d, dir, l);
	d[l] = 0;
	return d;
}

int
issufix(char *a, char *sufix)
{
	int l = strlen(sufix);
	char *p = strrchr(a, '.');
	return p && strlen(p)==l && memcmp(p,sufix,l)==0;
}

char**
getfiles(char *path, char *sufix)
{
	DIR *d;
	struct dirent *p;
	char **s;

	int j = 1;
	s = new(sizeof(char*));
	d = opendir(path);
	while((p=readdir(d)) != NULL){
		if(issufix(p->d_name, sufix)){
			s = realloc(s, (j+1)*sizeof(void*));
			int l = strlen(p->d_name);
			s[j-1] = memcpy(new(l+1), p->d_name, l+1);
			s[j++] = nil;
		}
	}
	nsrc = j - 1;
	closedir(d);
	return s;
}