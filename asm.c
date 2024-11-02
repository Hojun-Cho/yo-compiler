#include "yo.h"

char* instname[] = {
	[INOP] = "nop",
	[IADDW] = "addw",
	[ISUBW] = "subw",
	[IMULW] = "mulw",
	[IRET] = "ret",
	[IMOVW] = "movw",
	[IMOVM] = "movm",
	[ILEA] = "lea",
	[ICALL] = "call",
	[IJMP] = "jmp",
	[PCALL] = "pcall",
	[IFRAME] = "frame",
	[IARRAY] = "array",
	[ISLICE] = "slice",
	[ILEN] = "len",
	[IBEQW] = "beqw",
	[IBNEQW] = "bneqw",
	[ILTW] = "lt",
	[ILEQW] = "leq",
	[IEQW] = "eq",
};

void
asminst(FILE *f, Inst *in)
{
	for(; in; in=in->next)
		instconv(f, in);
}

void
asmexport(FILE *f, Sym *pkg, Decl **arr, int n)
{	
	fprintf(f, "\tpackage\t");
	fprintf(f, "%s\n", pkg->name);
	fprintf(f, "\texported %d\n", n);
	for(int i = 0; i < n; ++i){
		Decl *d = arr[i];
		switch(d->store){
		default:
			break;
		// case Dfn:
		// 	fprintf(f, "\tlink\t%u,%u,\"",d->desc->id, d->pc->pc);
		// 	fprintf(f, "%s\"\n", d->sym->name);
		// 	break;
		}
	}
}