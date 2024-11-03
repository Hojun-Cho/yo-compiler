#include "yo.h"

static Node* ecom(Node *nto, Node *n);

static Inst **breaks;
static Inst **conts;
static Node **bcsps;
static int scp;
static Node *scps[MaxScope];

static void
pushscp(Node *n)
{
	assert(scp < MaxScope);
	scps[scp++] = n;
}

static void
popscp(void)
{
	scp -= 1;	
}

static Node*
curscp(void)
{
	if(scp == 0) return nil;	
	else return scps[scp-1];
}

static Node*
sumark(Node *n)
{
	if(n == nil)
		return nil;
	Node *l = n->l;
	Node *r = n->r;
	n->temps = 0;
	n->addable = Rcant;
	if(l){
		sumark(l);
		n->temps = l->temps;
	}
	if(r){
		sumark(r);
		if(r->temps == n->temps) n->temps++;
		else if(r->temps > n->temps) n->temps = r->temps;
	}
	switch(n->op){
	case Oindex:
		n->addable = Radr;
		break;
	case Oxref:
		switch(l->addable){
		case Rreg: n->addable = Radr; break;
		case Rareg: n->addable = Rreg; break;
		case Raadr: n->addable = Radr; break;
		default: break;
		}
		break;
	case Oadr:
		switch(l->addable){
		case Rreg: n->addable = Rareg;	break;
		case Radr: n->addable = Raadr;	break;
		default: break;
		}
		break;
	case Oname:
		switch(n->decl->store){
		case Dtype: break;
		case Darg:
		case Dlocal: n->addable = Rreg; break;
		case Dpkg:
		case Dfn: n->addable = Rpc; break;	
		case Dglobal: n->addable = Rreg; break;
		default:
			assert(0);
		}	
		break;
	case Oconst:
		switch(n->ty->kind){
		case Tbool:
		case Tint: n->addable = Rconst; break;
		default: assert(0);	
		}
		break;	
	case Omul:
	case Oadd:
	case Osub:
		if(r->addable == Rconst){
			switch(l->addable){
			case Rcant:
			case Rconst: break;
			case Rareg: n->addable = Rareg; break;
			case Rreg:
			case Raadr: n->addable = Raadr; break;
			case Radr: n->addable = Rreg; break;
			default: assert(0);
			}
		}
		break;
	default:
		break;
	}
	if(n->addable < Rcant) n->temps = 0;
	else if(n->temps == 0) n->temps = 1;
	return n;
}

static void
arrcom(Node *nto, Node *n)
{
	Node sz = {.op=Oconst,.val=0,.addable=Rconst,.ty=tint};
	Node toff = {0}, tadd={0}, tadr={0}, fake={0}, *e=nil;
	toff.op = Oconst;
	toff.ty = tint;
	tadr.op = Oadr;
	tadr.l = nto;
	tadr.ty = tint;
	tadd.op = Oadd;
	tadd.l = &tadr;
	tadd.r = &toff;
	tadd.ty = tint;
	fake.op = Oxref;
	fake.l = &tadd;
	sumark(&fake);
	assert(fake.addable < Rcant);

	fake.ty = n->ty->tof;
	genmove(nto->ty, nto, nto);
	toff.val += IBY2WD; sz.val = nto->ty->len;
	genmove(tint, &sz, &fake);
	toff.val += IBY2WD; sz.val = nto->ty->len;
	genmove(tint, &sz, &fake);
	toff.val += IBY2WD; sz.val = nto->ty->tof->size;
	genmove(tint, &sz, &fake);
	for(e = n->l; e != nil; e = e->r){
		toff.val += nto->ty->tof->size;
		ecom(&fake, e->l);
	}
}

static int
tupblk0(Node *n, Decl **dd)
{
	Decl *d = nil;
	switch(n->op){
	case Otup:
		for(n = n->l; n; n=n->r)
			if(tupblk0(n->l, dd) == 0)
				return 0;
		return 1;
	case Oname:
		if(n->decl == nil)
			return 0;
		d = *dd;
		if(d && d->next != n->decl)
			return 0;
		int nid = n->decl->nid;
		if(d == nil && nid == 1)
			return 0;
		if(d != nil && nid != 0)
			return 0;		
		*dd = n->decl;
		return 1;
	}	
	return 0;
}

static Node*
tupblk(Node *n)
{
	if(n->op != Otup)
		return nil;
	Decl *d = nil;
	if(tupblk0(n, &d) == 0)
		return nil;
	while(n->op == Otup)
		n =  n->l->l;
	assert(n->op == Oname);
	return n;
}

static void
tuplcom(Node *n, Node *nto)
{
	Decl *d = nil;
	Node toff = {0}, tadd={0}, tadr={0}, fake={0}, tas={0}, *e=nil, *as;
	toff.op = Oconst;
	toff.ty = tint;
	tadr.op = Oadr;
	tadr.l = n;
	tadr.ty = tint;
	tadd.op = Oadd;
	tadd.l = &tadr;
	tadd.r = &toff;
	tadd.ty = tint;
	fake.op = Oxref;
	fake.l = &tadd;
	sumark(&fake);
	assert(fake.addable < Rcant);
	d = nto->ty->ids;
	for(e = nto->l; e != nil; e = e->r){
		as = e->l;
		if(as->op != Oname || as->decl != nildecl){
			toff.val = d->offset;
			fake.ty = d->ty;
			if(as->addable < Rcant)
				genmove(d->ty, &fake, as);
			else{
				tas.op = Oas;
				tas.ty = d->ty;
				tas.l = as;
				tas.r = &fake;
				tas.addable = Rcant;
				ecom(nil, &tas);
			}
		}
		d = d->next;
	}
}

static void
tuplrcom(Node *n, Node *nto)
{
	Decl *de = nto->ty->ids;
	Node *s = nil, *d = nil, tas ={0};
	for(s=n->l, d=nto->l; s!=nil&&d!=nil; s=s->r, d=d->r){
		if(d->l->op != Oname || d->l->decl != nil){
			tas.op = Oas;
			tas.ty = de->ty;
			tas.l = d->l;
			tas.r = s->l;
			sumark(&tas);
			ecom(nil, &tas);
		}	
		de = de->next;
	}
	assert(s == nil &&d == nil);
}

static void
tupcom(Node *nto, Node *n)
{
	Decl *d = nil;
	Node toff = {0}, tadd={0}, tadr={0}, fake={0}, *e=nil;
	toff.op = Oconst;
	toff.ty = tint;
	tadr.op = Oadr;
	tadr.l = nto;
	tadr.ty = tint;
	tadd.op = Oadd;
	tadd.l = &tadr;
	tadd.r = &toff;
	tadd.ty = tint;
	fake.op = Oxref;
	fake.l = &tadd;
	sumark(&fake);
	assert(fake.addable < Rcant);
	d = n->ty->ids;
	for(e = n->l; e != nil; e = e->r){
		toff.val = d->offset;
		fake.ty = d->ty;
		ecom(&fake, e->l);
		d = d->next;
	}
}

static Node*
eacom(Node *n, Node *reg, Node *t)
{
	Node *l = n->l;
	if(n->op != Oxref){
		Node *tmp = talloc(reg, n->ty, t);
		ecom(tmp, n);
		return reg;
	}
	if(l->op==Oadd && l->r->op == Oconst){
		if(l->l->op == Oadr){
			l->l->l = eacom(l->l->l, reg, t);
			sumark(n);
			assert(n->addable < Rcant);
			return n;
		}else
			assert(0);
	}else if(l->op == Oadr){
		assert(0);
	}else{
		talloc(reg, l->ty, t);
		ecom(reg, l);
		n->l = reg;
		n->addable = Radr;	
	}
	return n;
}

static void
assertcall(Decl *i, Node *j)
{
	assert((i==nil) == (j==nil));
	for(; i && j && j->op==Oseq; i=i->next, j=j->r){
		assert(eqtype(i->ty, j->l->ty));
	}
	assert((j==nil) == (i==nil));
}

static void
callcom(int op, Node *n, Node *ret)
{
	Node *args = n->r, *fn = n->l;
	Node tadd = {0}, pass={0}, toff={0}, frame={0};
	assertcall(fn->ty->ids, args);
	talloc(&frame, tint, nil);
	toff.op = Oconst;
	toff.addable = Rconst;
	toff.ty = tint;
	tadd.op = Oadd;
	tadd.addable = Raadr;
	tadd.l = &frame;
	tadd.r = &toff;
	tadd.ty = tint;
	pass.op = Oxref;
	pass.ty = tint;
	pass.op = Oxref;
	pass.addable = Radr;
	pass.l = &tadd;

	Inst *in = genrawop(IFRAME, nil, nil, &frame);
	in->sm = Adesc;
	in->s.decl = fn->decl;

	Decl *d = fn->ty->ids;
	int off = 0;
	for(Node *a = args; a; a=a->r, d=d->next){
		off = d->offset;
		toff.val = off;
		pass.ty = d->ty;
		ecom(&pass, a->l);
	}

	/* pass return value */
	if(ret && ret->ty != tnone){
		toff.val = REGRET * IBY2WD;
		pass.ty = fn->ty->tof;
	 	in = genrawop(ILEA, ret, nil, &pass);
	}
	
	if(fn->op != Opkg){
		in = genrawop(ICALL, &frame, nil, nil);
		in->dm = Apc;
		in->d.decl = fn->decl;
	}else{
		in = genrawop(PCALL, &frame, nil, nil);
		in->mm = Aimm;
		in->m.offset = fn->decl->pc->m.reg;
		in->dm = Aimm;
		in->d.offset = fn->decl->pc->pc;
	}
	tfree(&frame);
}

static Node*
rewrite(Node *n)
{
	if(n == nil)
		return nil;	
	Type *t  = nil;
	Decl *d = nil;
	Node *l = n->l, *r = n->r;
	switch(n->op){
	case Opkg: return n;
	case Oexport: n = n->l; break;
	case Odas: n->op = Oas; return rewrite(n);
	case Odot:
		d = r->decl;
		switch(d->store){
		case Dpkg: return r;
		case Dfield: break;
		case Dfn:
			assert(r->l == nil);
			n->op = Oname;
			n->decl = d;
			n->r = nil;
			n->l = nil;
			return n;
		default:
			assert(0);
		}
		r->op = Oconst;
		r->val = d->offset;
		r->ty = tint;
		n->l = mkunray(Oadr, l);
		n->l->ty = tint;
		n->op = Oadd;
		n = mkunray(Oxref, n);
		n->ty = n->l->ty;
		n->l->ty = tint;
		n->l = rewrite(n->l);
		return n;
	case Oslice:
		if(r->l == nil)
			r->l = mkconst(0);
		if(r->r == nil)
			r->r = mkconst(t->len);
		n->l = rewrite(n->l);
		n->r = rewrite(n->r);
		break;
	case Oindex:
		t = n->ty;
		n->r = mkn(Omul, mkconst(IBY2WD), n->r);
		n->r->ty = tint;
		n->op = Oadd;
		n->ty = tint;
		n = mkunray(Oxref, n);
		n->ty = t; 
		break;
	default:
		n->l = rewrite(l);
		n->r = rewrite(r);
		break;	
	}
	return n;
}

static Node*
simplifiy(Node *n)
{
	if(n==nil)
		return nil;	
	return rewrite(n);
}

static int
tupaliased(Node *n, Node *e)
{
	for(;;){
		if(e == nil) return 0;
		if(e->op == Oname && e->decl == n->decl) return 1;
		if(tupaliased(n, e->l)) return 1;
		e = e->r;
	}	
}

static int
tupsaliased(Node *n, Node *e)
{
	for(;;){
		if(n == nil) return 0;
		if(n->op == Oname && tupaliased(n, e)) return 1;
		if(tupsaliased(n->l, e)) return 1;
		n = n->r;
	}	
}

static int
swaprelop(int op)
{
	switch(op){
	case Oeq: return Oneq;
	case Oneq: return Oeq;
	case Olt: return Ogt;
	case Ogt: return Olt;
	case Ogeq: return Oleq;
	case Oleq: return Ogeq;
	default: assert(0);
	}	
}

static Inst*
cmpcom(Node *nto, Node *n)
{
	Node tl={0}, tr={0};
	Node *l = n->l;
	Node *r = n->r;
	int op = n->op;
	Inst *i;

	switch(op){
	case Ogt:
	case Ogeq:
		op = swaprelop(op);
		Node *t = l;
		l = r;
		r = t;
	}
	if(r->addable < Ralways){
		if(l->addable >= Rcant)
			l = eacom(l, &tl, nil);
	}else if(l->temps <= r->temps){
		r = ecom(talloc(&tr, r->ty, nil), r);
		if(l->addable >= Rcant)
			l = eacom(l, &tl, nil);
	}else{
		l = eacom(l, &tl, nil);
		r = eacom(l, talloc(&tr, r->ty, nil), r);
	}
	i = genop(op, l, r, nto);
	tfree(&tl);
	tfree(&tr);
	return i;
}

static Inst*
brcom(Node *n, int ift, Inst *b)
{
	Inst *bb = nil;
	int op = n->op;
	Node nto={0};
	Node f = (Node){.op=Oconst,.val=!ift,.addable=Rconst,.ty=tbool};

	sumark(n);
	if(op != Oconst){
		cmpcom(talloc(&nto, tbool, nil), n);
		bb = genrawop(IBEQW, &nto, &f, nil);
	}else{
		bb = genrawop(IBEQW, &f, n, nil);	
	}
	bb->branch = b;
	tfree(&nto);
	return bb;
}

static Node*
ecom(Node *nto, Node *n)
{
	Node *l = n->l; Node *r = n->r;	
	Node tr={0},tl={0},tt={0},*tn=nil;
	int op = n->op;

	if(n->addable < Rcant){
		if(nto)
			genmove(n->ty, n, nto);
		return nto;
	}
	switch(op){
	case Oas:
		if(l->op == Oslice)
			assert(0);
		if(l->op == Otup){
			if(tupsaliased(r, l) == 0){
				if((tn = tupblk(l))){
					tn->ty = n->ty;
					ecom(tn, r);
					if(nto)
						genmove(n->ty, tn, nto);
					break;
				}
				if((tn = tupblk(r))){
					tn->ty = n->ty;
					ecom(tn, l);
					if(nto)
						genmove(n->ty, tn, nto);
					break;
				}
				if(nto==nil && r->op == Otup){
					tuplrcom(r, l);
					break;
				}
			}
			if(r->addable >= Ralways||r->op!=Oname
				||tupaliased(r, l)){
				talloc(&tr, n->ty, nil);
				ecom(&tr, r);
				r = &tr;
			}
			tuplcom(r, n->l);
			if(nto)
				genmove(n->ty, r, nto);
			tfree(&tr);
			break;
		}
		if(l->addable >= Rcant)
			l = eacom(l, &tl, nto);
		ecom(l, r);
		if(nto)
			genmove(nto->ty, l, nto);
		tfree(&tl);
		tfree(&tr);
		break;
	case Olen: genrawop(ILEN, n->l, nil, nto); break;
	case Oarray: arrcom(nto, n); break;
	case Ostruct: tupcom(nto, n); break;
	case Oslice:
		if(l->addable >= Rcant)
			l = eacom(l, &tl, nto);
		if(r->l->addable >= Rcant)
			r->l = ecom(talloc(&tr,r->l->ty,nil), r->l);
		if(r->r->addable >= Rcant)
			r->r = ecom(talloc(&tt,r->r->ty,nil), r->r);
		genmove(n->ty, l, nto);	
		genrawop(ISLICE, r->l, r->r, nto);
		tfree(&tl);
		tfree(&tr);
		tfree(&tt);
		break;
	case Otup:
		if((tn = tupblk(n)) != nil){
			tn->ty = n->ty;
			genmove(n->ty, tn, nto);
			break;
		}
		tupcom(nto, n);
		break;
	case Oxref:
		n = eacom(n, &tl, nto);
		genmove(n->ty, n, nto);
		tfree(&tl);
		break;
	case Oref:
		genrawop(ILEA, l, nil, nto);
		break;
	case Ocall:
		callcom(op, n, nto);
		break;	
	case Oandand: case Ooror:
	case Oeq: case Oneq:
	case Olt: case Ogt:
	case Ogeq: case Oleq: 
		cmpcom(nto, n);		
		break;
	case Omul:
	case Oadd:
	case Osub:
		if(nto==nil)
			break;
		if(l->addable < Ralways){
			if(r->addable >= Rcant)
				r = eacom(r, &tr, nto);
		}else if(r->temps <= l->temps){
			l = ecom(talloc(&tl, l->ty, nto), l);
			if(r->addable >= Rcant)
				r = eacom(r, &tr, nil);
		}else{
			r = eacom(r, &tr, nto);
			l = ecom(talloc(&tl, l->ty, nil), l);
		}
		if(sameaddr(nto, l))
			genop(op, r, nil, nto);
		else if(sameaddr(nto, r))
			genop(op, l, nil, nto);
		else
			genop(op, r, l, nto);
		tfree(&tl);
		tfree(&tr);
		break;
	}
	return nto;
}

static void
scom(Node *n)
{
	Inst *p, *pp;
	Node *l, *r;

	for(; n; n = n->r){
		l = n->l;
		r = n->r;
		switch(n->op){
		case Ovardecl:
			return;
		case Oscope:
			pushscp(n);
			scom(n->l);
			scom(n->r);
			popscp();
			return;
		case Oif:
			pushblock();
			pushblock();
			p = brcom(l, 1, nil);
			tfreenow();
			popblock();
			scom(r->l);
			if(r->r){
				pp = p;
				p = genrawop(IJMP, nil, nil, nil);
				ipatch(pp, nextinst());
				scom(r->r);	
			}
			ipatch(p, nextinst());
			popblock();
			return;
		case Ofor:
			pushblock();
			pp = nextinst();
			n->l = l = simplifiy(n->l);
			sumark(l);
			p = brcom(l, 1, nil);
			tfreenow();
			popblock();
			assert(loopdep < maxloopdep);
			breaks[loopdep] = nil;
			conts[loopdep] = nil;
			bcsps[loopdep] = curscp();
			loopdep += 1;
			scom(n->r->l);
			loopdep -= 1;
			ipatch(conts[loopdep], nextinst());
			if(n->r->r){
				pushblock();
				scom(n->r->r);
				popblock();
			}
			repushblock(lastinst->block);	
			ipatch(genrawop(IJMP,nil,nil,nil),pp);
			popblock();
			ipatch(p, nextinst());
			ipatch(breaks[loopdep], nextinst());
			return;
		case Oret:
			pushblock();
			if(n->l){
				Node ret={0};
				n->l = simplifiy(n->l);
				sumark(n->l);
				ecom(retalloc(&ret, n->l), n->l);
				tfreenow();
			}
			genrawop(IRET, nil, nil, nil);
			popblock();
			break;
		case Oseq:
			scom(n->l);
			break;
		default:
			pushblock();
			n = simplifiy(n);
			sumark(n);
			ecom(nil, n);
			tfreenow();
			popblock();
			return;
		}
	}
}

void
fncom(Decl *d)
{
	Node *n = d->init;
	d->pc = nextinst();
	tinit();

	loopdep = scp = 0;
	breaks = new(sizeof(breaks[0]) * maxloopdep);
	conts = new(sizeof(conts[0]) * maxloopdep);
	bcsps = new(sizeof(bcsps[0]) * maxloopdep);
	for(Node *p=n->r; p; p=p->r){
		if(p->op != Oseq){
			scom(p);
			break;
		}else
			scom(p->l);
	}	
	free(breaks);
	free(conts);
	free(bcsps);

	Decl *locs = concatdecl(d->locals, tdecls());
	d->offset += idoffsets(locs, d->offset, IBY2WD);
	d->locals = locs;
	printf("%d\n", d->offset);
}
