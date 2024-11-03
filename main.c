#include "yo.h"

int nsrc;
int ninst;
int nimport;
int nfn;
int nexport;
int ntype;
int nvar;
Import *imports;
Decl **fns;
Decl **exports;
Decl **types;
Decl **vars;
Sym *pkgname;
char *path;

int
streq(char *a, char *b)
{
	if(a==b)
		return 1;	
	for(;*a && *b;){
		if(*a != *b)
			return 0;
		a += 1;
		b += 1;
	}
	return *a == *b;
}

int
isimported(Sym *p)
{
	for(int i = 0; i < nimport; ++i){
		if(streq(imports[i].path->name, p->name))
			return 1;
	}
	return 0;
}

int
isexported(Decl *d)
{
	for(int i = 0; i < nexport; ++i){
		if(exports[i] == d)
			return 1;
	}
	return 0;
}

void
buildpkg(char *path, char **srcs, int n)
{
	typestart();
	declstart();

	Node **trees = parsefiles(srcs, n);

	for(int i = 0; i < n; ++i)
		gdecl(trees[i]);	
	for(int i = 0; i < n; ++i)
		gbind(trees[i]);
	for(int i = 0; i < n; ++i)
		gassert(trees[i]);
	maxloopdep = 0;
	fns = new(sizeof(Decl*)*(nfn+1));
	vars = new(sizeof(Decl*)*(nvar+1));
	types = new(sizeof(Decl*)*(ntype+1));
	exports = new(sizeof(Decl*)*(nexport+1));
	for(int i = 0; i < n; ++i)
		gentry(trees[i]);
	for(int i = 0; i < nfn; ++i)
		assertfn(fns[i]);

	genstart();
	for(int i = 0; i < nfn; ++i){
		fncom(fns[i]);
	}
	firstinst = firstinst->next;
}

int
main(int argc, char **argv)
{	
	if(argc == 2)
		path = argv[1];
	else
		path = getpath();
	char **srcs = getfiles(path, ".yo");
	if(nsrc == 0)
		return 0;

	lexinit();
	typeinit();

	buildpkg(path, srcs, nsrc);

	ninst = resolvepcs(firstinst);

	genexports("exported");
	genbin("obj");

	asminst(stdout, firstinst);
}
