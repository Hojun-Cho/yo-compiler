#include "vm.h"

static u8 end[1024];
u32 ninst;
Inst *inst;
REG R;

Inst **modules;
u32 *modsizes;
int nmodules;

#define OP(fn)	void fn(void)
#define B(r)	*((i8*)(R.r))
#define F(r)	((WORD*)(R.r))
#define A(r)	((Array*)(R.r))
#define P(r)	*((WORD**)(R.r))
#define W(r)	*((WORD*)(R.r))
#define T(r)	*((void**)(R.r))
#define JMP(r)	R.PC = *(Inst**)(R.r)

void
printbin(u8 x)
{
	u8 arr[8] = {
		[0] = x & 128,
		[1] = x & 64,
		[2] = x & 32,
		[3] = x & 16,
		[4] = x & 8,
		[5] = x & 4,
		[6] = x & 2,
		[7] = x & 1,
	};
	printf("op %x %d: ",x, x);
	printf("%d%d ", arr[0]!=0,arr[1]!=0);
	printf("%d%d%d ", arr[2]!=0,arr[3]!=0,arr[4]!=0);
	printf("%d%d%d\n", arr[5]!=0,arr[6]!=0,arr[7]!=0);
}

u8
rd1(FILE *f)
{
	u8 v = 0;
	assert(fread(&v, 1, 1, f) == 1);
	return v;
}
u16
rd2(FILE *f)
{
	u16 v = 0;
	assert(fread(&v, 1, 2, f) == 2);
	return v;
}
WORD
rd4(FILE *f)
{
	i32 v = 0;
	assert(fread(&v, 1, 4, f) == 4);
	return v;
}

void
newstack(int sz)
{
	Stack *s = calloc(1, sizeof(s)+sz);
	R.EX = s->stack;
	R.TS = s->stack + sz;
	R.SP = s->fu + sz;
	R.FP = s->fu;
}

void
initprog(int ss)
{
	newstack(ss);
	WORD *p = (WORD*)&R.FP[32];
	*p = (WORD)&end;

}
OP(nop) {}
OP(jmp) {JMP(d);}
OP(lea) {W(d) = (WORD)R.s;}
OP(movw) {W(d) = W(s);}
OP(movm) {memmove(R.d,R.s,W(m));}
OP(addw) {W(d) = W(m) + W(s);} 
OP(subw) {W(d) = W(m) - W(s);} 
OP(mulw) {W(d) = W(m) * W(s);} 
OP(beqw) {if(W(s) == W(m)) JMP(d);}
OP(bneqw) {if(W(s) != W(m)) JMP(d);}
OP(ltw) {W(d) = (W(s) < W(m));}
OP(leqw) {W(d) = (W(s) <= W(m));}
OP(eqw) {W(d) = (W(s) == W(m));}
OP(neqw) {W(d) = (W(s) != W(m));}
OP(frame){
	Stack *s;
	Frame *f;
	int sz;

	sz = W(s);
	s = calloc(1, sizeof(Stack) + sz);
	s->sz = sz;
	s->SP = R.SP;
	s->TS = R.TS;
	s->EX = R.EX;
	f = s->fr;
	R.s = f;
	R.EX = s;
	R.TS = s->stack + sizeof(Stack)+sz;
	R.SP = s->fu + sz;
	T(d) = R.s;
}
OP(call){
	Frame *f = T(s);
	f->lr = R.PC;
	f->fp = R.FP;
	R.FP = (u8*)f;
	JMP(d);
}
OP(pcall){
	Frame *f;
	int mod, pc;

	f = T(s);
	f->lr = R.PC;
	f->fp = R.FP;
	R.FP = (u8*)f;
	mod = W(m);
	pc = W(d);
	R.PC = &modules[mod][pc];
}
OP(ret) {
	Frame *f = (Frame*)R.FP;
	R.FP = f->fp;
	if(R.FP == NULL){
		printf("result %ld\n", W(d));
		WORD *p = end;
		exit(0);
	}
	R.SP = (u8*)f;
	R.PC = f->lr;

	u8 *x = (u8*)f-IBY2WD*4;
	Stack *s = x;
	R.SP = s->SP;
	R.TS = s->TS;
	R.EX = s->EX;
	free(s);
}
OP(len){
	Array *a = A(s);	
	W(d) = a->len;
}
OP(array){
	WORD *s = R.s;
	WORD m = W(m);
	WORD *d = R.d;
	if(m){
		*d = (WORD)&d[ArrHead/IBY2WD];	
		memcpy(d+1, s+1, m-IBY2WD);	
	}else
		assert(0);
}
OP(slice){
	WORD s1 = W(s);	
	WORD s2 = W(m);	
	Array *a = A(d);	

	if(s2 == -1)
		s2 = a->len;
	assert(s1 >= 0 && s1 < a->len);
	assert(s2 >= 0 && s2 <= a->len);
	assert(s1 < s2);
	Array d = *a;
	d.len = s2 - s1;	
	d.cap = s2 - s1;
	d.arr = a->arr + s1*a->size;
	*a = d; 
}
static void (*optab[])(void) = {
	[INOP] = nop,
	[ILEA] = lea,
	[IFRAME] = frame,
	[ICALL] = call,
	[PCALL] = pcall,
	[IJMP] = jmp,
	[IRET] = ret,
	[ILEN] = len,
	[ISLICE] = slice,
	[IARRAY] = array,
	[IADDW] = addw,
	[ISUBW] = subw,
	[IMOVW] = movw,
	[IMULW] = mulw,
	[IMOVM] = movm,
	[IBEQW] = beqw,
	[IBNEQW] = bneqw,
	[IEQW] = eqw,
	[INEQW] = neqw,
	[ILEQW] = leqw,
	[ILTW] = ltw,
};

void
xec(void)
{
	while(1){
		Inst in = *R.PC;
		u8 op = in.op;
		if(dec[in.add] == 0){
			printbin(in.add);
			exit(1);
		}
		if(optab[in.op]==0){
			printf("op 0X%x %d\n", in.op, in.op);
			exit(1);
		}
		dec[in.add]();
		R.PC++;
		optab[op]();
	}
}

void
bpatch(Inst *i, Inst *base, u32 n)
{
	static int tab[IEND] = {
		[ICALL]=1,[IBEQW]=1,[IBNEQW]=1,[IJMP]=1,
	};
	if(tab[i->op] == 0)
		return;
	assert(i->d.imm >= 0 && i->d.imm < n);
	i->d.imm = (WORD)&base[i->d.imm];
	return;
}

void
rdinst(FILE *f, Inst *in)
{
	in->op = rd1(f);
	assert(in->op != EOF);
	in->add = rd1(f);
	switch(in->add & ARM) {
	case AXIMM:
	case AXINF:
	case AXINM:
		in->reg = rd4(f);
		break;
	}
	switch(UXSRC(in->add)) {
	case SRC(AFP):
	case SRC(AMP):	
	case SRC(AIMM):
		in->s.ind = rd4(f);
		break;
	case SRC(AIND|AFP):
	case SRC(AIND|AMP):
		in->s.f = rd2(f);
		in->s.s = rd2(f);
		break;
	}
	switch(UXDST(in->add)) {
	case DST(AFP):
	case DST(AMP):
		in->d.ind = rd4(f);
		break;
	case DST(AIMM):
		in->d.ind = rd4(f);
		break;
	case DST(AIND|AFP):
	case DST(AIND|AMP):
		in->d.f = rd2(f);
		in->d.s = rd2(f);
		break;
	}
}

Inst*
loadmod(char *fname, u32 *outn)
{
	FILE *f = fopen(fname, "r");
	assert(f != 0);
	u32 nim = rd4(f);
	for(u32 i = 0; i < nim; i++){
		u32 len = rd4(f);
		fseek(f, len, SEEK_CUR);
	}
	u32 n = rd4(f);
	Inst *ins = calloc(sizeof(Inst), n);
	for(u32 i = 0; i < n; i++)
		rdinst(f, ins+i);
	for(u32 i = 0; i < n; i++)
		bpatch(ins+i, ins, n);
	fclose(f);
	*outn = n;
	return ins;
}

void
load(char *fname)
{
	FILE *f = fopen(fname, "r");
	assert(f != 0);
	initprog(1024);

	nmodules = rd4(f);
	if(nmodules > 0){
		modules = calloc(sizeof(Inst*), nmodules);
		modsizes = calloc(sizeof(u32), nmodules);
		for(int i = 0; i < nmodules; i++){
			u32 len = rd4(f);
			char *path = calloc(1, len+1);
			fread(path, 1, len, f);
			modules[i] = loadmod(path, &modsizes[i]);
			free(path);
		}
	}

	ninst = rd4(f);
	inst = calloc(sizeof(Inst), ninst);
	for(u32 i = 0; i < ninst; i++)
		rdinst(f, inst+i);
	for(u32 i = 0; i < ninst; i++)
		bpatch(inst+i, inst, ninst);
	R.PC = inst;
	fclose(f);
}

int
main(int argc, char *argv[])
{
	load(argv[1]);
	xec();
}
