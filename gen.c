#include "yo.h"

static	int	addrmode[Rend] =
{
	/* Rreg */	Afp,
	/* Rmreg */	Amp,
	/* Roff */	Aoff,
	/* Rnoff */	Anoff,
	/* Rdesc */	Adesc,
	/* Rdescp */	Adesc,
	/* Rconst */	Aimm,
	/* Ralways */	Aerr,
	/* Radr */	Afpind,
	/* Rmadr */	Ampind,
	/* Rcant */	Aerr,
	/* Rpc */	Apc,
	/* Rmpc */	Aerr,
	/* Rareg */	Aerr,
	/* Ramreg */	Aerr,
	/* Raadr */	Aerr,
	/* Ramadr */	Aerr,
	/* Rldt */	Aldt,
};

static int blockstklen;
static Inst zinst;
static	Decl	*wtemp;
static	Node	*ntoz;
static	int	ntemp;

int *blockstk;
int blockdep;
int nblock;
int blocks;

Node retnode;
Inst *lastinst, *firstinst;

int
pushblock(void)
{
	if(blockdep >= blockstklen){
		blockstklen = blockdep += 32;
		blockstk = realloc(blockstk, blockstklen*sizeof(blockstk));
	}
	blockstk[blockdep++] = blocks;
	return blocks = nblock++;
}

void
repushblock(int b)
{
	blockstk[blockdep++] = blocks;	
	blocks = b;
}

void
popblock(void)
{
	blocks = blockstk[--blockdep];
}

Inst*
nextinst(void)
{
	Inst *in = lastinst->next;
	if(in)
		return in;
	in = new(sizeof(Inst));
	*in = zinst;
	lastinst->next = in;
	return in;
}


Inst*
mkinst(void)
{
	Inst *in = lastinst->next;
	if(in == nil){
		in = new(sizeof(Inst));
		*in = zinst;
		lastinst->next = in;
	}
	lastinst = in;
	assert(blocks >= 0);
	in->block = blocks;
	return in;
}

void
tinit(void)
{
	wtemp = nil;
}

void
genstart(void)
{
	ntoz = nil;
	Decl *d = mkdecl(Dlocal, tint);
	d->sym = enter(".ret", 0);
	d->offset = IBY2WD * REGRET;

	zinst = (Inst){0};
	zinst.op = INOP;
	zinst.sm = Anone;
	zinst.dm = Anone;
	zinst.mm = Anone;
	
	firstinst = new(sizeof(Inst));	
	*firstinst = zinst;
	lastinst = firstinst;

	retnode.op = Oname;
	retnode.addable = Rreg;	
	retnode.ty = tint;
	retnode.decl = d;

	blocks = -1;
	blockdep = 0;
	nblock = 0;	
}

Addr
genaddr(Node *n)
{
	Addr a = {0};
	if(n == nil)
		return a;
	switch(n->addable){
	case Rareg:
		a = genaddr(n->l);
		if(n->op == Oadd)
			a.reg += n->r->val;	
		break;
	case Rreg:
		if(n->decl)
			a.decl = n->decl;
		else
			a = genaddr(n->l);	
		break;
	case Rconst:
		a.offset = n->val;
		break;
	case Radr:
		a = genaddr(n->l);
		break;
	case Raadr:
		a = genaddr(n->l);	
		if(n->op == Oadd)
			a.offset += n->r->val;
		break;
	default:
		assert(0);
	}
	return a;
}

int
sameaddr(Node *n, Node *m)
{
	if(n->addable != m->addable)
		return 0;
	Addr a = genaddr(n);
	Addr b = genaddr(m);
	return a.offset==b.offset && a.reg==b.reg && a.decl == b.decl;
}

Inst*
genmove(Type *t, Node *s, Node *d)
{
	static u8 mvtab[] = {
		[Tptr] = IMOVW,
		[Tint] = IMOVW,
		[Tbool] = IMOVW,
		[Ttup] = IMOVM,
		[Tarray] = IARRAY,
		[Tslice] = IMOVM,
		[Tstruct] = IMOVM,
	};	
	Inst *in = nil;
	Addr reg = {0};
	int regm = Anone;
	u8 op = mvtab[t->kind];
	assert(op!=0);
	switch(t->kind){
	case Tarray:
	case Tslice:
	case Tstruct:
	case Ttup:
		assert(t->size != 0);
		regm = Aimm;
		reg.offset = t->size;
		break;
	}
	in = mkinst();
	in->op = op;
	if(s){
		in->s = genaddr(s);
		in->sm = addrmode[s->addable];	
	}	
	in->m = reg;
	in->mm = regm;
	if(d){
		in->d = genaddr(d);
		in->dm = addrmode[d->addable];
	}
	return in;
}

Inst*
genrawop(int op, Node *s, Node *m, Node *d)
{
	Inst *in = mkinst();
	in->op = op;
	if(s){
		in->s = genaddr(s);
		in->sm = addrmode[s->addable];	
	}	
	if(m){
		in->m = genaddr(m);
		in->mm = addrmode[m->addable];
		assert(in->mm != Ampind);
		assert(in->mm != Afpind);
	}
	if(d){
		in->d = genaddr(d);
		in->dm = addrmode[d->addable];
	}
	return in;
}

Inst*
genop(int op, Node *s, Node *m, Node *d)
{
	static u8 disoptab[Oend][Tend] = 
	{
		[Olt] = {[Tint]=ILTW,[Tbool]=ILTW},
		[Oeq] = {[Tint]=IEQW,[Tbool]=IEQW},
		[Oneq] = {[Tint]=INEQW,[Tbool]=INEQW},
		[Oleq] ={[Tint]=ILEQW,[Tbool]=ILEQW},
		[Oandand] = {[Tint]=IEQW,[Tbool]=IEQW},
		[Ooror] = {[Tint]=IADDW,[Tbool]=IADDW},
		[Oadd] = {[Tint]=IADDW,},
		[Osub] = {[Tint]=ISUBW,},
		[Omul] = {[Tint]=IMULW,},
	};
	Inst *in = mkinst();
	int iop = disoptab[op][d->ty->kind];
	assert(iop != 0);
	in->op = iop;
	if(s){
		in->s = genaddr(s);
		in->sm = addrmode[s->addable];	
	}	
	if(m){
		in->m = genaddr(m);
		in->mm = addrmode[m->addable];
		assert(in->mm != Ampind);
		assert(in->mm != Afpind);
	}
	if(d){
		in->d = genaddr(d);
		in->dm = addrmode[d->addable];
	}
	return in;
}

void
addprint(FILE *f, int am, Addr *a)
{
	switch(am){
	case Anone:
		return;	
	case Afp:
		fprintf(f, "%d(fp)", a->reg);
		break;
	case Aimm:
		fprintf(f, "$%d", a->offset);
		break;
	case Afpind:
		fprintf(f, "%d(%d(fp))", a->offset, a->reg);
		break;
	case Adesc:
		fprintf(f, "$%d", a->offset+a->reg);
		break;	
	case Apc:
		fprintf(f, "$%d", a->reg+a->offset);
		break;
	case Amp:
		fprintf(f, "%d(mp)",a->reg);
		break;
	case Aldt:
		fprintf(f, "$%d", a->reg);	
		break;
	default:
		assert(0);
	}
	return;
}

void
instconv(FILE *f, Inst *in)
{
	char *comma = "";

	if(in == nil)
		return;
	char* op = instname[in->op];	
	fprintf(f, "\t%s\t", op);
	if(op == INOP)
		return;
	if(in->sm != Anone){
		addprint(f, in->sm, &in->s);
		comma = ", ";
	}
	if(in->mm != Anone){
		fprintf(f, "%s", comma);
		addprint(f, in->mm, &in->m);
		comma = ", ";
	}
	if(in->dm != Anone){
		fprintf(f, "%s", comma);
		addprint(f, in->dm, &in->d);
		comma = "";
	}
	fprintf(f, "\n");
}

Node*
talloc(Node *n, Type *t, Node *nok)
{
	Decl *ok = nok == nil ? nil : nok->decl;

	if(ok==nil||ok->tref==0)
		ok = nil;
	*n = (Node){0};
	n->op = Oname;
	n->addable = Rreg;
	n->ty = t;
	if(ok != nil 
	&&ok->ty->kind == t->kind
	&&ok->ty->size == t->size){
		ok->tref++;
		n->decl = ok;
		return n;
	}
	for(Decl *d = wtemp; d != nil; d = d->next){
		if(d->tref == 1
		&& d->ty->kind == t->kind
		&& d->ty->size == t->size){
			d->tref++;
			n->decl = d;
			return n;
		}
	}
	char buf[64] = {0};
	sprintf(buf, ".t%d", ntemp++);
	Decl *d = mkdecl(Dlocal, t);
	d->sym = enter(buf, 0);
	d->tref = 2;
	n->decl = d;
	d->next = wtemp;
	wtemp = d;

	return n;
}

void
tfree(Node *n)
{
	if(n == nil || n->decl == nil || n->decl->tref == 0)
		return;
	assert(n->decl->tref > 1);
	if(--n->decl->tref == 1){
		return;
	}
}

void
tfreenow(void)
{
	assert(ntoz == nil);
}

int
align(int off)
{
	while(off % IBY2WD)
		off++;
	return off;
}

int
idoffsets(Decl *id, long offset, int al)
{
	if(id==nil || id->flag)
		return align(offset);
	for(; id; id = id->next){
		id->flag = 1;
		if(id->store == Dlocal && id->link){
			if(id->link->flag == 0)
				idoffsets(id->link, offset, al);
			id->offset = id->link->offset;
			continue;
		}
		offset = align(offset);
		id->offset = offset;
		offset += id->ty->size;
		if(id->nid == 0 && (id->next == nil || id->next->nid != 0))
			offset = align(offset);
	}
	return align(offset);
}

Decl*
tdecls(void)
{
	for(Decl *d = wtemp; d; d=d->next)
		assert(d->tref == 1);
	return wtemp;
}

int
resolvepcs(Inst *inst)
{
	int pc = 0;
	for(Inst *in=inst; in; in=in->next){
		Decl *d = in->s.decl;
		if(d){
			in->s.reg += d->offset;
		}
		d = in->m.decl;
		if(d){
			in->m.reg += d->offset;
		}
		d = in->d.decl;
		if(d){
			if(in->dm == Apc){
				in->d.offset = d->pc->pc;
			}else{
				in->d.reg += d->offset;	
			}
		}
		in->pc = pc++;
	}
	for(Inst *in=inst; in; in=in->next){
		Decl *d  =in->s.decl;
		if(d && in->sm == Apc)
			in->s.offset = d->pc->pc;
		d = in->d.decl;
		if(d != nil && in->dm == Apc)
			in->d.offset = d->pc->pc;
		if(in->branch){
			in->dm = Apc;
			in->d.offset = in->branch->pc;
		}
	}
	return pc;
}

int
idindices(Decl *id)
{
	int i = 0;
	for(; id; id=id->next){
		usetype(id->ty);
		id->offset = i++;
	}
	return i;
}

void
ipatch(Inst *b, Inst *d)
{
	Inst *n;
	for(; b; b = n){
		n = b->branch;
		b->branch = d;
	}	
}