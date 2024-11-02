#include "yo.h"

Sym *anontupsym;
Type *tnone;
Type *tbool;
Type *tint;
Type *tstring;
Type *tany; /* nil ptr */
Type *tptr;

Type*
mktype(int kind, int size, Type *tof, Decl *args)
{
	Type *t = new(sizeof(Type));
	t->kind = kind;
	t->size = size;	
	t->tof = tof;
	t->ids = args;
	return t;
}

Decl*
mkdecl(int store, Type *t)
{
	Decl *d = new(sizeof(Decl));
	d->store = store;
	d->ty = t;
	d->nid = 1;
	return d;
}

Decl*
mkids(Sym *s, Type *t, Decl *next)
{
	Decl *d = mkdecl(Dundef, t);	
	d->next = next;
	d->sym = s;
	return d;
}

Type*
mkidtype(Sym *s)
{
	Type *t = mktype(Tid, 0, nil, nil);	
	if(s->unbound == 0){
		s->unbound = mkdecl(Dunbound, nil);
		s->unbound->sym = s;
	}
	t->decl = s->unbound;
	return t;
}

Node*
structdecl(Decl *id, Node *f)
{
	Node *n = mkn(Ostrdecl, nil, nil);	
	Type *t = mktype(Tstruct, 0, nil, nil);
	id->ty = t;
	n->decl = id;
	n->l = f;
	n->ty = t;
	t->decl = id;
	return n;
}

void
typebuiltin(Decl *d, Type *t)
{
	d->ty = t;
	t->decl = d;
	installids(Dtype, d);
}

void
typeinit(void)
{
	anontupsym = enter(".tuple", 0);

	tint = mktype(Tint, IBY2WD, nil, nil);
	tint->offset = IBY2WD;

	tbool = mktype(Tbool, IBY2WD, nil, nil);
	tbool->offset = IBY2WD;

	tnone = mktype(Tnone, 0, nil, nil);
	tany = mktype(Tany, IBY2WD, nil, nil);


	tstring = mktype(Tstring, IBY2WD, nil, nil);
	tstring->offset = IBY2WD;
}

void
typestart(void)
{
	Sym *s;
	Decl *d;

	s = enter("int", 0);
	d = mkids(s, nil, nil);
	typebuiltin(d, tint);

	s = enter("bool", 0);
	d = mkids(s, nil, nil);
	typebuiltin(d, tbool);

	s = enter("string", 0);
	d = mkids(s, nil, nil);
	typebuiltin(d, tstring);
}

Node*
fielddecl(int store, Decl *ids)
{
	Node *n = mkn(Ofielddecl, nil, nil);
	n->decl = ids;
	for(; ids; ids=ids->next)	
		ids->store = store;
	return n;
}

Decl*
typeids(Decl *ids, Type *t)
{
	if(ids == nil)
		return nil;
	ids->ty = t;
	for(Decl *id = ids->next; id; id=id->next)
		id->ty = t;
	return ids;
}

int
idssize(Decl *d, int off)
{
	for(; d; d=d->next){
		bindsize(d->ty);
		off = align(off);
		d->offset = off;
		off += d->ty->size;
	}	
	return off;
}

int
bindsize(Type *t)
{
	if(t == nil)
		return 0;
	switch(t->kind){
	case Tnone:
	case Tint:
	case Tbool:
		break;
	case Tslice: t->size=ArrHead; break;		
	case Tarray: t->size=t->len*bindsize(t->tof)+ArrHead; break;
	case Tref:
	case Tptr:
		t->size = IBY2WD;
		bindsize(t->tof);
		break;
	case Tfn:
		t->size = 0;
		idssize(t->ids, 56);
		bindsize(t->tof);
		break;
	case Tpkg:
		t->size = IBY2WD;
		idssize(t->ids,0);
		break;
	case Tstruct:
	case Ttup:
		t->size = idssize(t->ids, 0);
		t->size = align(t->size);
		t->offset = t->size;
		break;
	default:
		assert(0);
	}
	return t->size;
}

void
bindtypes(Type *t)
{
	Decl *id = nil;
	switch(t->kind){
	case Tnone:
	case Tpkg:
	case Tpfn:
	case Tint:
	case Tbool:
		break;
	case Tslice:
	case Tarray:
	case Tref:
	case Tptr:
		bindtypes(t->tof);
		break;
	case Tid:
		id = t->decl->sym->decl;
		assert(id != nil);
		id->sym->unbound = nil;
		t->decl = id;
		*t = *id->ty;
		break;
	case Tfn:
		for(id = t->ids; id; id=id->next)	
			bindtypes(id->ty);
		bindtypes(t->tof);
		break;
	case Ttup:
	case Tstruct:
		for(id = t->ids; id; id=id->next)	
			bindtypes(id->ty);
		break;
	default:
		assert(0);
	}
}

void
gbind(Node *n)
{
	for(; ; n=n->r){
		if(n==nil)
			return;
		if(n->op != Oseq)
			break;
		gbind(n->l);
	}
	Decl *d = nil;
	switch(n->op){
	case Odas:
	case Oas:
	case Ovardecl:
	case Ofn: break;
	case Oexport: gbind(n->l); break;
	case Ofielddecl: bindtypes(n->decl->ty); break;
	case Ostrdecl:
		bindtypes(n->ty);
		repushids(n->ty->ids);
		gbind(n->l);
		d = popids(n->ty->ids);
		assert(d == nil);
		break;
	default:
		assert(0);
	}
}

Type*
resolvetype(Type *t)
{
	Decl *d = t->decl;
	assert(d->store != Dunbound);
	if(d->store != Dtype){
		assert(d->store != Dundef);	
		assert(d->store != Dwundef);	
	}
	assert(d->ty != nil);
	return d->ty;
}

void
assertstruct(Type *t)
{
	Decl *p = t->decl;
	for(Decl *id = t->ids; id; id=id->next){
		assert(id->store == Dfield);
		id->dot = p;
	}	
	for(Decl *id = t->ids; id; id=id->next){
		id->dot = p;
		id->ty = isvalidty(id->ty);
	}	
}

Type*
asserttype(Type *t)
{
	int i = 0;
	Decl *id = nil;
	switch(t->kind){
	case Tnone:
	case Tint:
	case Tpkg:
	case Tpfn:
	case Tbool:
	case Tslice:
		break;
	case Tarray: assert(t->len != 0);
	case Tref:
	case Tptr:
		t->tof = asserttype(t->tof);
		break;
	case Tid:t = asserttype(resolvetype(t)); break;
	case Tstruct: assertstruct(t); break;
	case Tfn:
		for(id = t->ids; id; id=id->next){
			id->store = Darg;
			id->ty = asserttype(id->ty);
		}
		t->tof = asserttype(t->tof);
		break;
	case Ttup:
		if(t->decl == nil){
			t->decl = mkdecl(Dtype, t);
			t->decl->sym = enter(".tuple", 0);
		}
		for(Decl *id=t->ids; id; id=id->next){
			id->store = Dfield;
			if(id->sym == nil){
				char buf[64] = {0};
				sprintf(buf, "t%d", i);
				id->sym = enter(buf, 0);
			}
			i += 1;
			id->ty = asserttype(id->ty);
		}
		break;
	default:
		assert(0);
	}
	return t;
}

Type*
isvalidty(Type *t)
{
	bindtypes(t);
	t = asserttype(t);
	if(t->kind != Tpkg && t->kind!=Tpfn)
		bindsize(t);	
	return t;
}

Type*
usetype(Type *t)
{
	if(t == nil)
		return nil;
	t = isvalidty(t);
	return t;	
}

int
eqtup(Type *t1, Type *t2)
{
	Decl *d1 = t1->ids, *d2 = t2->ids;
	for(; d1 && d2; d1=d1->next, d2=d2->next){
		if(eqtype(d1->ty, d2->ty) == 0)
			return 0;
	}
	return d1 == d2;
}

int
eqtype(Type *t1, Type *t2)
{
	if(t1 == t2)
		return 1;
	if(t1 == nil || t2 == nil)
		return 0;
	if(t1->kind != t2->kind){
		if(t1->kind != Tptr || t2->kind != Tref)
			return 0;
	}
	switch(t1->kind){
	case Tslice:
		return eqtype(t1->tof, t2->tof);
	case Tarray:
		return t1->len == t2->len && eqtype(t1->tof, t2->tof);
	case Tptr:
		return eqtype(t1->tof, t2->tof);
	case Tnone:
	case Tint:
	case Tbool:
		return 1;
	case Ttup:
	case Tstruct:
		return eqtup(t1, t2);
	}
	return 0;
}

Decl*
isinids(Decl *ids, Sym *s)
{
	for(; ids; ids=ids->next)	
		if(ids->sym == s)
			return ids;
	return nil;
}

Decl*
isinpkg(Decl *ids, Sym *s)
{
	for(; ids; ids=ids->next){
		if(ids->sym->len-1 == s->len && memcmp(s->name, ids->sym->name+1, s->len)==0)
			return ids;
	}
	return nil;
}