#include "yo.h"

void assertexpr(Node *n);

Decl *fndecls;
Node **labstk;
int loopdep;
int maxloopdep;

static void
pushloop(void)
{
	if(loopdep >= maxloopdep)
		maxloopdep += MaxScope;
	loopdep += 1;
}

static Type*
typeofslice(Type *t)
{
	Type *p = t;
	while(p && p->kind == Tslice)
		p = p->tof;	
	assert(p && p->kind == Tarray);
	return p;
}

void
elemfields(Node *n)
{
	Decl *d = nil;
	Type *t = n->ty;
	Node *e = nil;

	switch(t->kind){
	case Tstruct:
		d = t->ids;
		for(e = n->l; e != nil; e = e->r){
			e->l->ty = d->ty;
			assertexpr(e->l);
			d = d->next;
		}
		break;
	case Tarray:
			for(e = n->l; e != nil; e = e->r){
				e->l->ty = t->tof;
				assertexpr(e->l);
			}
		break;
	default:
		assert(0);
	}
}

Decl*
tupfields(Node *n)
{
	Decl *h = nil;
	Decl *d = h;
	Decl **last = &h;
	for(; n; n=n->r){
		d = mkdecl(Dfield, n->l->ty);
		*last = d;
		last = &d->next;
	}
	return h;
}

void
dasinfer(Node *n, Type *t)
{
	switch(n->op){
	case Otup:
		n->ty = t;
		n = n->l;
		Decl *p = t->ids;
		for(; p&&n; p=p->next){
			if(p->store != Dfield)
				continue;
			dasinfer(n->l, p->ty);
			n = n->r;
		}
		for(;p; p=p->next){
			if(p->store == Dfield)
				break;
		}
		assert(n==nil && p==nil);
		return;
	case Oname:
		n->decl->ty = t;
		n->ty = t;
		sharelocal(n->decl);
		return;
	default:
		assert(0);
	}
}

void
assertexpr(Node *n)
{
	if(n == nil)
		return;
	if(n->op == Oseq){
		for(; n && n->op==Oseq; n=n->r){
			assertexpr(n->l);	
			n->ty = tnone;
		}
		if(n == nil)
			return;
	}

	Type *t = nil;
	Decl *d = nil;
	Node * l = n->l, *r = n->r;
	if(n->op != Odas)
		assertexpr(l);
	if(n->op != Ocall &&n->op != Odas && n->op != Odot && n->op != Oindex && n->op != Oindex)
		assertexpr(r);
	switch(n->op){
	case Opkg: break;
	case Oseq: n->ty = tnone; break;
	case Ostruct:
	case Oarray: 
		n->ty = isvalidty(n->ty);
		break;
	case Ooror: case Oandand:
		assert(l->ty == tbool);
		assert(r->ty == tbool);
	case Oeq: case Oneq: case Olt:
	case Oleq: case Ogt: case Ogeq:
		n->ty = tbool;
		break;
	case Oconst:
		assert(n->ty != nil);
		break;
	case Omul: case Oadd: case Osub:
		/* TODO : match type or can addable */
		assertexpr(l);
		assertexpr(r);
		assert(eqtype(l->ty, r->ty));
		n->ty = l->ty;
		break;
	case Olen:
		l->ty = isvalidty(l->ty);
		assert(l->ty->kind==Tarray||l->ty->kind==Tslice);
		n->ty = tint;	
		break;
	case Oas:
		if(r->op == Oname)
			assert(r->decl->store != Dtype);
		assert(eqtype(l->ty, r->ty));
		n->ty = l->ty = r->ty;
		break;
	case Odas:
		assertexpr(r);
		dasdecl(l);
		dasinfer(l, r->ty);
		assert(eqtype(l->ty, r->ty));
		n->ty = l->ty = r->ty;
		usetype(n->ty);
		if(r->op == Oname)
			assert(r->decl->store != Dtype);
		break;
	case Oxref:
		t = l->ty;
		assert(t->kind == Tptr);
		n->ty = usetype(t->tof);
		break;
	case Oref:
		t = l->ty;
		assert(l->op == Oname);
		n->ty = usetype(mktype(Tref, IBY2WD, t, nil));	
		break;
	case Odot:
		t = l->ty;
		switch(t->kind){
		case Tpkg:
			assert(r->op == Oname);
			d = isinpkg(t->ids, r->decl->sym);
			assert(d != nil);
			r->decl = d;
			r->ty = n->ty = d->ty;
			break;
		case Tstruct:
			d = isinids(t->ids, r->decl->sym);
			assert(d != nil);
			d->ty = usetype(isvalidty(d->ty));
			r->decl = d;
			n->ty = d->ty;
			break;
		default:
			assert(0);
		}
		break;
	case Oname:
		assert((d = n->decl) != nil);
		if(d->store == Dunbound){
			Sym *s = d->sym;
			assert((d = s->decl) != nil);
			s->unbound = nil;
			n->decl = d;
		}
		assert(n->ty = d->ty = usetype(d->ty));
		switch(d->store){
		case Darg: case Dfn: case Dlocal: case Dfield:
		case Dtype: case Dglobal: case Dpkg:
			break;
		default:
			assert(0);
		}
		break;
	case Oslice:
		t = usetype(l->ty);
		assert(t->kind == Tarray || t->kind == Tslice);
		if(t->kind == Tslice)
			t = t->tof;
		n->ty = mktype(Tslice, ArrHead, t->tof, nil);
		break;
	case Oindex:
		t = l->ty;
		assert(t->kind == Tarray || t->kind == Tslice);
		assertexpr(r);
		if(r->op == Oconst && t->kind == Tarray)
			assert(r->val < t->len);
		n->ty = t->tof;
		break;
	case Otup:
		d = tupfields(l);
		n->ty = usetype(mktype(Ttup, 0, nil, d));
		break;
	case Ocall:
		assertexpr(r);
		t = l->ty;
		if(l->op==Opkg)
			return;
		usetype(t);
		n->ty = t->tof;
		break;
	default:
		assert(0);
	}	
}

void
assertvar(Node *n)
{
	Type *t = isvalidty(n->ty);	
	Decl *last = n->l->decl;
	for(Decl *d = n->decl; d != last->next; d=d->next){
		assert(d->store != Dtype);
		d->ty = t;
	}
}

Node*
assertstmt(Type *ret, Node *n)
{
	Decl *d = nil;
	Node *top = n, *last = nil, *l = nil, *r=nil;

	for(; n; n=n->r){
		l = n->l;
		r = n->r;
		switch(n->op){
		case Oseq:
			n->l = assertstmt(ret, l);
			break;
		case Ovardecl:
			vardecled(n);
			assertvar(n);
			return top;
		case Oret:
			assertexpr(l);
			if(l == nil)
				assert(ret == tnone);
			else if(ret == tnone)
				assert(l->ty == tnone);
			else
				assert(eqtype(ret, l->ty));
			n->ty = ret;
			return top;
		case Oif:
			n->l = l = assertstmt(ret, l);
			assert(l->ty->kind == Tbool);
			r->l = assertstmt(ret, r->l);
			n = r;
			break;
		case Ofor:
			assertexpr(l);
			assert(l->ty->kind == Tbool);
			pushloop();
			r->r = assertstmt(ret, r->r);
			r->l = assertstmt(ret, r->l);
			loopdep -= 1;
			return top;
		case Oscope:
			pushscope(n, Sother);
			assertstmt(ret, n->l);
			n->r = assertstmt(ret, n->r);
			d = popscope();
			fndecls = concatdecl(fndecls,d);
			return top;
		default:
			assertexpr(n);
			if(last == nil)
				return n;
			last->r = n;
			return top;
		}
		last = n;
	}
	return top;
}

Decl*
assertfndecl(Node* n)
{
	Decl *d = n->l->decl;
	d->store = Dfn;
	d->init = n;
	Type *t = n->ty;
	t = isvalidty(t);
	n->ty = d->ty = t = usetype(t);
	d->offset = idoffsets(t->ids, 56, IBY2WD);	
	d->locals = nil;
	n->decl = d;
	return d;
}

void
assertfn(Decl *d)
{
	Node *n = d->init;
	fndecls = nil;
	repushids(d->ty->ids);
	n->r = assertstmt(n->ty->tof, n->r);
	d->locals = concatdecl(popids(d->ty->ids), fndecls);
	fndecls = nil;
}

void
gdecl(Node *n)
{
	for(;;n=n->r){
		if(n==nil)
			return;
		if(n->op != Oseq)
			break;
		gdecl(n->l);
	}	
	switch(n->op){
	case Oexport:
		gdecl(n->l);
		nexport += 1;
		break;
	case Ovardecl:
		vardecled(n);
		nvar += 1;
		break;
	case Ostrdecl:
		structdecled(n);
		ntype += 1;
		break;
	case Ofn:
		fndecled(n);
		nfn += 1;
		break;
	default:
		assert(0);	
	}
}

void
varassert(Node *n)
{
	Type *t = isvalidty(n->ty);
	Decl *last = n->l->decl;
	for(Decl *ids = n->decl; ids != last->next; ids=ids->next){
		ids->ty = t;
	}
}

void
gassert(Node *n)
{
	for(; ; n=n->r){
		if(n==nil)
			return;
		if(n->op != Oseq)
			break;
		gassert(n->l);
	}
	switch(n->op){
	case Oexport:
		gassert(n->l);
		break;
	case Ostrdecl:
		repushids(n->ty->ids);
		gassert(n->l);
		assert(popids(n->ty->ids) == nil);
		break;
	case Ofn:
		assertexpr(n->l);
		assertfndecl(n);
		break;
	case Ovardecl:
		varassert(n);
		bindsize(n->ty);
		break;
	case Ofielddecl:
		bindsize(n->ty);
		break;
	default:
		assert(0);
	}
	return;
}

void
gentry(Node *tree)
{
	static int ivar = 0;
	static int ifn = 0;
	static int iexport = 0;
	static int itype = 0;

	for(; tree; tree=tree->r){
		assert(tree->op == Oseq);
		Node *n = tree->l;
		Decl *d = nil;
		int exp = 0;
redo:
		switch(n->op){
		case Oexport:
			n = n->l;
			exp = 1;
			goto redo;
			break;
		case Ostrdecl:
			types[itype++]=d=n->ty->decl;
			break;
		case Ofn:
			fns[ifn++]=d=n->decl;
			break;
		case Ovardecl:
			vars[ivar++]=d=n->decl;
			break;
		default:
			break;
		}
		if(exp)
			exports[iexport++] = d;
	}
}

Sym*
assertpkgdecl(Node **trees, int n)
{
	Sym *s = trees[0]->l->decl->sym;
	for(int i = 1; i < n; ++i){
		Sym *ss = trees[i]->l->decl->sym;
		assert(s->len == ss->len);
		assert(memcmp(s->name, ss->name, s->len)==0);
	}
	return s;
}