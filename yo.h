#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "isa.h"
#include "dat.h"
#include "fn.h"

#define elem(x) (sizeof(x)/sizeof((x)[0]))
#define nil ((void*)0)

extern Decl *nildecl;
extern Decl *truedecl;
extern Decl *falsedecl;
extern Decl *_decl;
extern Sym *anontupsym;
extern Type *tnone;
extern Type *tint;
extern Type *tbool;
extern Type *tstring;
extern Type *tany;
extern Node	retnode;
extern char* instname[];

extern int nimport;
extern int nfn;
extern int nexport;
extern int ntype;
extern int nvar;
extern Import *imports;
extern Decl **fns;
extern Decl **exports;
extern Decl **types;
extern Decl **vars;
extern Inst *lastinst, *firstinst;

extern Sym *pkgname;
extern char *path;
extern int ninst;
extern int nsrc;
extern Decl *fndecls;
extern int *blockstk;
extern int blockdep;
extern int nblock;
extern int blocks;
extern Decl *fndecls;
extern Node **labstk;
extern int loopdep;
extern int maxloopdep;