#include "yo.h"

static FILE *f;

static int
retlen(Type *t)
{
	if(t == tnone)
		return 0;
	if(t->kind != Ttup)
		return 1;
	return decllen(t->ids);
}

static void
tyexported(Decl **arr, int n)
{
	for(int i = 0; i < n; ++i){
		Decl *d = arr[i];
		if(isexported(d)==0)
			continue;
		Type *t = d->ty;
		int l = decllen(t->ids);
		fprintf(f, "type .%s struct %d %d\n",
			d->sym->name, t->size,l);
		for(Decl *dd=t->ids; dd;dd=dd->next){
			Type *tt = dd->ty;
			fprintf(f,"\t%s %s %d %d %d\n",
				dd->sym->name,
				tt->decl->sym->name,
				tt->kind,
				tt->size,
				dd->offset);
		}
	}
}

static void
fnexported(Decl **arr, int n)
{
	for(int i = 0; i < n; ++i){
		Decl *d = arr[i];
		if(isexported(d)==0)
			continue;
		Type *t = d->ty;
		fprintf(f, "fn .%s %d %d %d %d\n",
			d->sym->name, d->offset, d->pc->pc,
			decllen(t->ids),
			retlen(t->tof));
		fprintf(f, "\t%d\n", t->size);
		for(Decl *dd=t->ids; dd;dd=dd->next){
			Type *tt = dd->ty;
			fprintf(f,"\t%s %d %d %d\n",
				tt->decl->sym->name,
				tt->kind,
				tt->size,
				dd->offset);
		}
		t = t->tof;
		bindsize(t);
		fprintf(f, "\t%d\n", t->size);
		if(t->kind==Ttup){
			for(Decl *dd=t->ids; dd;dd=dd->next){
				Type *tt = dd->ty;
				fprintf(f,"\t%s %d %d %d\n",
					tt->decl->sym->name,
					tt->kind,
					tt->size,
					dd->offset);
			}
		}else
			fprintf(f,"\t%s %d %d %d\n",
				t->decl->sym->name,
				t->kind,
				t->size,
				t->decl->offset);
	}
}
void
genexports(char *name)
{
	f = fopen(name, "w+");
	assert(f != nil);

	fprintf(f, "%s\n", pkgname->name);
	tyexported(types, ntype);
	fnexported(fns, nfn);
	fprintf(f, "\n");
	fclose(f);
}

void
genbin(char *name)
{
	f = fopen(name, "w+");
	// fprintf(f, "%s\n", pkgname->name);
	// fprintf(f, "%d\n", nimport);
	// for(int i = 0; i < nimport; ++i){
	// 	fprintf(f, "%d %s %s\n", i, imports[i].path->name, imports[i].sym->name);
	// }

	wr4(f, ninst);	
	disinst(f, firstinst);
	fclose(f);
}