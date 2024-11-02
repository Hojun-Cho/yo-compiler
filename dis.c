#include "yo.h"

static int dismode[Aend] = {
	/* Aimm */	AIMM,
	/* Amp */	AMP,
	/* Ampind */	AMP|AIND,
	/* Afp */	AFP,
	/* Afpind */	AFP|AIND,
	/* Apc */	AIMM,
	/* Adesc */	AIMM,
	/* Aoff */	AIMM,
	/* Anoff */	AIMM,
	/* Aerr */	AXXX,
	/* Anone */	AXXX,
	/* Aldt */	AIMM,
};

static int disregmode[Aend] = {
	/* Aimm */	AXIMM,
	/* Amp */	AXINM,
	/* Ampind */	AXNON,
	/* Afp */	AXINF,
	/* Afpind */	AXNON,
	/* Apc */	AXIMM,
	/* Adesc */	AXIMM,
	/* Aoff */	AXIMM,
	/* Anoff */	AXIMM,
	/* Aerr */	AXNON,
	/* Anone */	AXNON,
	/* Aldt */	AXIMM,
};

void
wr1(FILE *f, u8 v)
{
	fwrite(&v, 1, 1, f);
}
void
wr2(FILE *f, u16 v)
{
	fwrite(&v, 1, 2, f);
}
void
wr4(FILE *f, u32 v)
{
	fwrite(&v, 1, 4, f);
}

void
disaddr(FILE* f, u32 m, Addr *a)
{
	u32 val = 0;
	switch(m){
	case Anone:
		return;
	case Aimm:
		val = a->offset;
		break;
	case Afp:
		val = a->reg;
		break;
	case Afpind:
		wr2(f, a->reg);
		wr2(f, a->offset);
		return;
	case Adesc:
		val = a->offset+a->reg;	
		break;
	case Apc:
		val = a->offset;
		break;
	default:
		assert(0);
	}	
	wr4(f, val);
}

int
disinst(FILE *f, Inst *in)
{
	int n = 0;
	for(; in != nil; in = in->next){
		n += 1;
		wr1(f, in->op);
		wr1(f, SRC(dismode[in->sm]) | DST(dismode[in->dm]) | disregmode[in->mm]);
		disaddr(f, in->mm, &in->m);
		disaddr(f, in->sm, &in->s);
		disaddr(f, in->dm, &in->d);
	}
	return n;
}