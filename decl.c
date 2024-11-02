#include "yo.h"

Decl *nildecl;
Decl *truedecl;
Decl *falsedecl;
Decl *_decl;
static Decl *scopes[MaxScope] = {0};
static Decl *tails[MaxScope] = {0};
static Node *scopenode[MaxScope] = {0};
static int scopekind[MaxScope] = {0};
static int scope;

static void freeloc(Decl *d);
static void recdecl(Node *n ,int store, int *id);
static int recmark(Node *n, int id);

void
declstart(void)
{
	scope = ScopeNil;
	scopes[scope] = nil;
	tails[scope] = nil;
	
	nildecl = mkdecl(Dglobal, tany);
	nildecl->sym = enter("nil", 0);
	installids(Dglobal, nildecl);

	_decl = mkdecl(Dglobal, tany);
	_decl->sym = enter("_", Lid);
	installids(Dglobal, _decl);

	truedecl = mkdecl(Dglobal, tbool);
	truedecl->sym = enter("true", Lconst);
	installids(Dglobal, truedecl);

	falsedecl = mkdecl(Dglobal, tbool);
	falsedecl->sym = enter("false", Lconst);
	installids(Dglobal, falsedecl);
	
	scope = ScopeGlobal;
	scopes[scope] = nil;
	tails[scope] = nil;
}

Decl*
popids(Decl *d)
{
	for(; d; d=d->next){
		if(d->sym){
			d->sym->decl = d->old;
			d->old = nil;
		}
	}
	return popscope();
}

void
repushids(Decl *d)
{
	assert(scope < MaxScope);
	scope++;
	scopes[scope] = nil;
	tails[scope] = nil;
	scopenode[scope] = nil;
	scopekind[scope] = Sother;

	for(; d; d=d->next){
		if(d->scope != scope
			&& (d->scope != ScopeGlobal||scope != ScopeGlobal +1))
			assert(0);
		Sym *s = d->sym;
		if(s != nil){
			if(s->decl != nil && s->decl->scope >= scope)
				d->old = s->decl->old;
			else
				d->old = s->decl;
			s->decl = d;
		}
	}
}

void
pushscope(Node *scp, int kind)
{
	assert(scope+1 < MaxScope);
	scope += 1;
	scopes[scope] = nil;
	tails[scope] = nil;
	scopenode[scope] = scp;
	scopekind[scope] = kind;
}

Decl*
popscope(void)
{
	for(Decl *p=scopes[scope]; p; p=p->next){
		if(p->sym){
			p->sym->decl = p->old;
			p->old = nil;
		}
		if(p->store == Dlocal)
			freeloc(p);
	}
	return scopes[scope--];
}

void
installids(int store, Decl *ids)
{
	if(ids == nil)
		return;
	Decl *last = nil;
	for(Decl *d = ids; d; d=d->next){
		d->scope = scope;
		if(d->store == Dundef)
			d->store = store;
		Sym *s = d->sym;
		if(s){
			if(s->decl && s->decl->scope >= scope){
				assert(0);
			}else
				d->old = s->decl;
			s->decl = d;
		}
		last = d;
	}	
	Decl *d = tails[scope];
	if(d == nil)
		scopes[scope] = ids;
	else
		d->next = ids;
	tails[scope] = last;
}

void
fielddecled(Node *n)
{
	for(; n; n=n->r)	
		switch(n->op){
		case Oseq: fielddecled(n->l); break;
		case Ofielddecl: installids(Dfield, n->decl); return;
		case Ostrdecl: structdecled(n); return;
		default: assert(0);
		}
}

void
structdecled(Node *n)
{
	Decl *d = n->ty->decl;
	installids(Dtype, d);
	pushscope(nil, Sother);
	fielddecled(n->l);
	n->ty->ids = popscope();
}

void
fndecled(Node *n)
{
	Node *l = n->l;
	assert(l->op == Oname);
	Decl *d = l->decl->sym->decl;
	assert(d == nil);
	d = mkids(l->decl->sym, n->ty, nil);
	installids(Dfn, d);
	l->decl = d;
	pushscope(nil, Sother);
	installids(Darg, n->ty->ids);
	n->ty->ids = popscope();
}

void
vardecled(Node *n)
{
	int store = Dlocal;
	if(scope == ScopeGlobal)
		store = Dglobal;
	Decl *ids = n->decl;
	installids(store, ids);
	Type *t = n->ty;
	for(; ids; ids=ids->next)
		ids->ty = t;
}

void
dasdecl(Node *n)
{
	int store = 0;
	if(scope == ScopeGlobal)
		store = Dglobal;
	else
		store = Dlocal;
	int id = 0;
	recdecl(n, store, &id);
	if(store == Dlocal && id > 1)
		recmark(n, id);
}

Node*
vardecl(Decl *ids, Type *t)
{
	Node *n = mkn(Ovardecl, mkn(Oseq, nil, nil), nil);
	n->decl = ids;
	n->ty = t;
	return n;
}

Node*
mkdeclname(Decl *d)
{
	Node *n = mkn(Oname, nil, nil);
	n->decl = d;
	n->ty = d->ty;
	return n;	
}

Node*
varinit(Decl *ids, Node *e)
{
	Node *n = mkdeclname(ids);
	if(ids->next == nil)
		return mkn(Oas, n, e);
	return mkn(Oas, n, varinit(ids->next, e));
}

Decl*
concatdecl(Decl *d1, Decl *d2)
{
	if(d1 == nil)
		return d2;
	Decl *t = d1;
	for(; t->next; t=t->next)
		;
	t->next = d2;
	return d1;
}

int
decllen(Decl*d)
{
	int l = 0;
	for(;d;d=d->next)
		++l;
	return l;
}

int
sharelocal(Decl *d)
{
	if(d->store != Dlocal)
		return 0;
	Type *t = d->ty;
	for(Decl *dd = fndecls; dd; dd=dd->next){
		assert(d != dd);
		Type *tt = dd->ty;
		if(dd->store != Dlocal || dd->link || dd->tref != 0)
			continue;
		if(t->size == tt->size){
			d->link = dd;
			dd->tref = 1;
			return 1;
		}
	}
	return 0;
}

static void
freeloc(Decl *d)
{
	if(d->link)
		d->link->tref = 0;	
}

static void
recdecl(Node *n ,int store, int *id)
{
	Decl *d;

	switch(n->op){
	case Otup:
		for(n = n->l; n; n=n->r)
			recdecl(n->l, store, id);	
		return;
	case Oname:
		assert(n->decl != nil);
		d = mkids(n->decl->sym, nil, nil);
		installids(store, d);
		// old = d->old; assert(old == nil); warn shadwoing
		n->decl = d;	
		d->das = 1;
		if(*id >= 0)
			*id += 1;
		return;
	default:
		assert(0);
	}
}

static int
recmark(Node *n, int id)
{
	switch(n->op){
	case Otup:
		for(n = n->l; n; n = n->r)
			id = recmark(n->l, id);
		break;
	case Oname:
		n->decl->nid = id;
		id = 0;
		break;
	default:
		assert(0);
	}	
	return id;
}
