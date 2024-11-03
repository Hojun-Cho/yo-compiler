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
	[INEQW] = "neq",
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
	fprintf(f, "\n\tpackage\t");
	fprintf(f, "%s\n", pkg->name);
	fprintf(f, "\tfn %d\n", n);
	for(int i = 0; i < n; ++i){
		Decl *d = arr[i];
		switch(d->store){
		default:
			break;
		case Dfn:
			fprintf(f, "\tfn\t%s,%d,\n",d->sym->name, d->pc->pc);
			break;
		}
	}
}