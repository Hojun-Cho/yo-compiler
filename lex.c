#include "yo.h"

#define isend(x) (*(x)==0)

static void popimport(void);

typedef struct
{
	char *name;
	int tok;
}Keywd;

static Keywd keywd[] = {
	{"len", Llen},
	{"return", Lret},
	{"fn", Lfn},
	{"for", Lfor},
	{"if", Lif},
	{"else", Lelse},
	{"var", Lvar},
	{"type", Ltype},
	{"struct", Lstruct},
	{"export", Lexport},
	{"import", Limport},
	{"package", Lpackage},
};

static int bstack;
static FILE *bin;
static Token un;
FILE *bins[1024];

static Sym **syms;
static int nsym;
static int isym;
static int ineof;

static int
Getc(void)
{
	if(ineof)
		return EOF;
	int c = fgetc(bin);
	if(c == EOF)
		ineof = 1;
	return c;
}

static void 
Ungetc(int c)
{
	if(ineof)
		return;	
	ungetc(c, bin);
}

static int
isident(int c)
{
	return isdigit(c) || isalpha(c) || c == '_';
}

static Token 
lexstr(void)
{
	char id[StrSize + 1];
	char *p = id;
	int i = 0;
	for(;;){
		int c = Getc();
		assert(c != EOF);
		if(c == '\"')
			break;
		assert(i++ < StrSize);
		*p++ = c;	
	}
	*p = '\0';
	Sym *sym = new(sizeof(Sym));
	sym->len = i;
	sym->name = new(p - id + 1);
	memcpy(sym->name, id, p-id+1);
	return (Token){.kind=Lsconst, .sym=sym};
}

static Token 
lexid(void)
{
	char id[StrSize + 1];
	char *p = id;
	int i = 0, c = Getc();
	for(;;){
		assert(i++ < StrSize);
		*p++ = c; *p = '\0';
		c = Getc();
		if(c==EOF|| isident(c) == 0){
			Ungetc(c);
			break;
		}
	}
	Sym *sym = enter(id, Lid);
	Token tok = (Token){.kind=sym->tok};
	if(tok.kind == Lid ||tok.kind == Ltid || tok.kind==Lconst)
		tok.sym = sym;
	return tok;
}

static Token 
lexnum(void)
{
	int v = Getc() - '0'; 
	for(;;){
		int c = Getc();
		if(c==EOF || isdigit(c) == 0){
			Ungetc(c);
			break;
		}
		v = v * 10 + c - '0';
	}
	return (Token){.kind=Lconst, .num=v};	
}

static Token 
lexing(void)
{
	for(;;){
	int c = Getc();
	if(c == EOF){
		popimport();
		if(bstack == -1)
			return (Token){.kind=Leof};
		return (Token){.kind=Leof};
	}
	if(isspace(c) || c =='\n')
		continue;
	if(isdigit(c)){
		Ungetc(c);
		return lexnum();
	}
	if(isident(c)){
		Ungetc(c);
		return lexid();
	}
	switch(c){
	case '\"':
		return lexstr();
	case '(': case ')': case '[': case ']':
	case '}': case '{': case ',': case ';':
		return (Token){.kind=c};
	case '|':
		if((c = Getc()) == '|')
			return (Token){.kind=Loror};
		Ungetc(c);
		return (Token){.kind='|'};
	case '&':
		if((c = Getc()) == '&')
			return (Token){.kind=Landand};
		Ungetc(c);
		return (Token){.kind='&'};
	case '!':
		if((c = Getc()) == '=')
			return (Token){.kind=Lneq};
		Ungetc(c);
		return (Token){.kind='!'};
	case '=':
		if((c = Getc()) == '=')
			return (Token){.kind=Leq};
		Ungetc(c);
		return (Token){.kind='='};
	case '>':
		if((c = Getc()) == '=')
			return (Token){.kind=Lgeq};
		Ungetc(c);
		return (Token){.kind='>'};
	case '<':
		if((c = Getc()) == '=')
			return (Token){.kind=Lleq};
		Ungetc(c);
		return (Token){.kind='<'};
	case '.':
		return (Token){.kind=c};
	case ':':
		if((c = Getc()) == '=')
			return (Token){.kind=Ldas};
		Ungetc(c);
		return (Token){.kind=':'};
	case '+':case '-': case '*':
		return (Token){.kind=c};
	default:
		assert(0);
	}
	}
}

void
unlex(Token tok)
{
	un = tok;
}

Token
lex(void)
{
	if(un.kind != EOF){
		Token t = un;
		un = (Token){.kind = EOF};
		return t;
	}	
	Token t = lexing();
	if(t.kind == EOF){
		return (Token){.kind=EOF};
	}
	return t;
}

Token
peek(int *arr)
{
	Token t = lex();
	unlex(t);
	while(!isend(arr))
	if(*arr++ == t.kind)
		return t;
	return (Token){.kind=Lnone};
}

Token
try(int *arr)
{
	Token t = lex();
	while(!isend(arr))
		if(t.kind == *arr++)
			return t;
	unlex(t);
	return (Token){.kind=Lnone};
}

Token
want(int *arr)
{
	Token t = lex();
	while(!isend(arr))
	if(t.kind == *arr++)
		return t;
	assert(0);
}

void
lexinit(void)
{
	bstack = -1;
	for(int i = 0; i < elem(keywd); ++i){
		enter(keywd[i].name, keywd[i].tok);
	}
}

Sym*
dotsym(void)
{
	char id[StrSize + 1];
	char *p = id;
	int c;
	while(isspace(c=Getc()))
		;
	int i = 0;
	for(;;){
		assert(i++ < StrSize);
		*p++ = c; *p = '\0';
		c = Getc();
		if(c==EOF||(isident(c)==0&&c!='.')){
			Ungetc(c);
			break;
		}
	}
	Sym *s = new(sizeof(Sym));
	s->len = i;
	s->name = new(i+1);
	memcpy(s->name, id, i+1);
	un = (Token){.kind=-1};
	return s;
}

void
lexstart(char *name)
{
	FILE *f = fopen(name, "r");
	assert(f!=nil);
	un = (Token){.kind = EOF};
	ineof = 0;
	bin = bins[++bstack] = f;
}

Sym*
enter(char *name, int tok)
{
	int l = strlen(name);
	for(int i = 0; i < isym; ++i){
		Sym *s = syms[i];
		if(s && l==s->len && memcmp(name, s->name, l)==0)
			return s;
	}
	Sym *s = new(sizeof(Sym));
	s->name = new(l+1);
	memcpy(s->name, name, l+1);
	s->len = l;
	if(tok == 0)
		tok = Lid;
	s->tok = tok;
	if(isym + 1 >= nsym){
		nsym += 16;
		syms = realloc(syms, nsym*sizeof(Sym*));
	}
	return syms[isym++] = s;
}

static void
popimport(void)
{
	ineof = 0;
	fclose(bin);
	bstack -= 1;
	bin = bins[bstack];
}

void
importdef(Sym *file)
{
	char buf[1024]={0};
	sprintf(buf, "%s/exported", file->name);	
	FILE *b = fopen(buf, "r");
	assert(b != nil);
	assert(bstack + 1 < sizeof(bins));
	bin = bins[++bstack] = b;
}
