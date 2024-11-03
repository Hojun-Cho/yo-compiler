#include "yo.h"

static Node* sourcefile(void);
static Type* parameters(void);
static Node* expr(void);
static Node* exprlist(void);
static Node* topdecl(void);
static Node* decltype(void);
static Node* declvar(void);
static Type* type(void);
static Node* primaryexprs(void);
static void importdecl(void);
static Node* litvalue(void);
static Node* operand(void);
static Node* stmtlist(void);

Node*
parse(char *name)
{
	lexstart(name);
	want((int[]){Lpackage, 0});
	Sym *s = want((int[]){Lid, 0}).sym;
	if(pkgname == nil)
		pkgname = s;
	assert(pkgname==nil || pkgname == s);
	importdecl();
	Node *n = sourcefile();
	return n;
}

Node**
parsefiles(char **srcs, int n)
{
	Node **arr = new(sizeof(Node*)*(n+1));
	for(int i = 0; i < n; ++i){
		arr[i] = parse(srcs[i]);
	}
	return arr;
}

static Sym*
trygetpkg(Sym *s)
{
	if(s->decl&&s->decl->ty&&s->decl->ty->kind == Tpkg){
		if(try((int[]){'.',0}).kind != '.')
			return nil;
		Sym *ss = want((int[]){Lid,0}).sym;
		Decl *d = isinpkg(s->decl->ty->ids, ss);
		assert(d != nil);
		return d->sym;
	}
	return nil;
}

static Node*
sconstval(void)
{
	Token t = want((int[]){Lsconst,0});
	return mksconst(t.sym);
}

static Node*
constval(void)
{
	Node *n;

	Token t = want((int[]){Lconst,0});
	if(t.sym == truedecl->sym){
		n = mkn(Oconst, nil, nil);
		n->ty = tbool;
		n->val = 1;
	}else if(t.sym == falsedecl->sym){
		n = mkn(Oconst, nil, nil);
		n->ty = tbool;
		n->val = 0;
	}else
		n = mkconst(t.num);
	return n;
}

static Decl* 
ident(void)
{
	Token t = want((int[]){Lid,0});
	return mkids(t.sym, nil, nil);
}

// identlist = Lid {',' Lid}
static Decl*
identlist(void)
{
	Decl h={0}, *p=&h;

	p = p->next = ident();
	for(;try((int[]){',',0}).kind;){
		p = p->next = ident();
	}
	return h.next;
}

static Node*
name(void)
{
	Sym *s = want((int[]){Lid,0}).sym;	
	Sym *p = trygetpkg(s);
	if(p){
		Node *n = mkn(Opkg,nil,nil);
		n->decl = p->decl;
		n->ty = p->decl->ty;
		return n;
	}
	return mkname(s);
}

// argments = '(' exprlist ')'
static Node*
argments(void)
{
	want((int[]){'(',0});
	if(try((int[]){')',0}).kind)
		return nil;
	Node *n = exprlist();
	if(n->op != Oseq)
		n = mkn(Oseq, n, nil);
	want((int[]){')',0});
	return n;
}

static Node*
compoundexpr(void)
{
	Node *n;

	want((int[]){'(',0});	
	n = exprlist();
	want((int[]){')',0});
	assert(n->op == Oseq);
	return mkn(Otup, n, nil);
}

static Node*
bltlen(void)
{
	want((int[]){Llen,0});
	want((int[]){'(',0});
	Node *l = name();
	want((int[]){')',0});
	Node *n = mkn(Olen, l, nil);
	return n;
}

// ElementList   = KeyedElement { "," KeyedElement } .
// KeyedElement  = [ Key ":" ] Element .
// Key           = FieldName | Expression | LiteralValue .
// FieldName     = identifier .
// Element       = Expression | LiteralValue .
static Node*
element(void)
{
	if(peek((int[]){'{',0}).kind)
		return litvalue();
	return expr();
}

static Node*
elemlist(void)
{
	Node h={0}, *p=&h;

	p = p->r = mkn(Oseq, element(), nil);
	while(try((int[]){',',0}).kind)
		p = p->r = mkn(Oseq, element(), nil);
	return h.r;
}

// LiteralValue  = "{" [ ElementList [ "," ] ] "}" .
static Node*
litvalue(void)
{
	want((int[]){'{',0});
	if(try((int[]){'}', 0}).kind)
		return nil;
	Node *n = elemlist();
	want((int[]){'}',0});
	return n;
}

static Node*
compositelit(void)
{
	Token t = want((int[]){Lid,0});
	Node *n = litvalue();
 	n = mkn(Ostruct, n, nil);
 	n->ty = mkidtype(t.sym);
 	return n;
}

static int
litlen(Node *n)
{
	int i = 0;
	for(; n; n=n->r)
		++i;
	return i;
}

static Node*
arraylit(void)
{
	Type *t = type();
	Node *n = litvalue();
	int l = litlen(n);
	if(t->len)
		assert(t->len >= l);
	else
		t->len = l;	
	n = mkn(Oarray, n, nil);
	n->ty = t;
 	return n;
}

static Node*
operand(void)
{
	Token t = peek((int[]){Lid,Lstruct,Lsconst,Lconst,Llen,'(','[',0});
	switch(t.kind){
	case '[': return arraylit();
	case '(': return compoundexpr();	
	case Lstruct: return compositelit(); 
	case Lid: return name();
	case Lsconst: return sconstval();
	case Lconst: return constval();
	case Llen: return bltlen();
	default: assert(0);
	}
}

static Node*
index(Node *id)
{
	Node *n=nil, *l=nil, *r=nil;

	want((int[]){'[',0});
	switch(try((int[]){':',']',0}).kind){
	case ']':
		assert(0);
	case ':':
		if(try((int[]){']',0}).kind)
			return mkn(Oslice, id, mkn(Oseq, mkconst(0), mkconst(-1)));
		n = mkn(Oslice, id, mkn(Oseq, mkconst(0), expr()));
		break;
	default:
		l = expr();
		if(peek((int[]){']',0}).kind){
			n = mkn(Oindex, id, l);
			break;
		}
		want((int[]){':',0});
		if(peek((int[]){']',0}).kind){
			n = mkn(Oslice, id, mkn(Oseq, l, mkconst(-1)));
			break;
		}
		r = expr();
		n = mkn(Oslice, id, mkn(Oseq, l, r));
	}
	want((int[]){']',0});
	return n;
}

static Node*
primaryexprs(void)
{
	Node h={0}, *p=&h;

	p = operand();
	for(;;){
		switch(peek((int[]){'.','(','[',0}).kind){
		case '.':
			lex();
			p = mkn(Odot, p, operand());
			break;
		case '(': p = mkn(Ocall, p, argments()); break;
		case '[': p = index(p); break;
		default: return p;
		}
	}
}

// unrayexpr = primaryexprs | unray-op unrayexpr
static Node*
unrayexpr(void)
{
	switch(try((int[]){'+','-','*','/','&',0}).kind){
	default: return primaryexprs();
	case '+':
	case '!':
	case '/':
	case '^': assert(0);
	case '-': return mkn(Omul, mkconst(-1), unrayexpr());
	case '*': return mkn(Oxref, unrayexpr(), nil); 
	case '&': return mkn(Oref,unrayexpr(), nil);
	}
}

// mul-op = ('*'|'/') primary 
static Node*
mulop(void)
{
	int c;
	Node *p = unrayexpr();
	while((c = try((int[]){'*','/',0}).kind))
	switch(c){
	case '*':
		p = mkn(Omul, p, unrayexpr());
		break;
	default:
		assert(0);
	}
	return p;
}

// add-op = [mul-op] ('+'|'-') expr 
static Node*
addop(void)
{
	int c;
	Node *p = mulop();
	while((c = try((int[]){'+','-',0}).kind))
	switch(c){
	case '+': p = mkn(Oadd, p, mulop()); break;
	case '-': p = mkn(Osub, p, mulop()); break;
	default: assert(0);
	}
	return p;
}

static Node*
relop(void)
{
	int c, op;
	Node *p = addop();
	while((c = try((int[]){Leq,Lneq,Lgeq,Lleq,'>','<',0}).kind)){
		switch(c){
		case '<': op = Olt; break;
		case '>': op = Ogt; break;
		case Leq: op = Oeq; break;
		case Lneq: op = Oneq; break;
		case Lgeq: op = Ogeq; break;
		case Lleq: op = Oleq; break;
		default: assert(0);
		}
		p = mkn(op, p, addop());
	}
	return p;
}

static Node*
logicop(void)
{
	int c, op;
	Node *p = relop();
	while((c = try((int[]){Loror,Landand,0}).kind)){
		switch(c){
		case Loror: op = Ooror; break;
		case Landand: op = Oandand; break;
		default: assert(0);
		}
		p = mkn(op, p, relop());
	}
	return p;
}

static Node*
expr(void)
{
	return logicop();
}

// exprlist = expr {',' expr }
static Node*
exprlist(void)
{
	Node h={0}, *p=&h;

	p->r = expr();
	if(peek((int[]){',',0}).kind == 0)
		return p->r;
	p = p->r = mkn(Oseq, p->r, nil);
	for(;try((int[]){',',0}).kind;){
		p = p->r = mkn(Oseq, expr(), nil);
	}
	return h.r;
}

// estmt = expr
// SimpleStmt = Assignment
Node*
sstmt(void)
{
	Node *r = nil;
	Node *l = exprlist();
	switch(try((int[]){'=',Ldas,0}).kind){
	case '=':
		r = exprlist();
		return mkn(Oas, l, r);
	case Ldas:
		r = exprlist();
		return mkn(Odas, l, r);
	default:
		return l;
	}
}

static Node*
retstmt(void)
{
	want((int[]){Lret,0});
	if(peek((int[]){';',0}).kind)
		return mkn(Oret, nil, nil);
	Node *n = expr();
	return mkn(Oret, n, nil);
}

// block = '{' stmtlist '}'
static Node*
block(void)
{
	want((int[]){'{',0});
	Node *n = stmtlist();
	n = mkscope(n);
	want((int[]){'}',0});
	return n;
}

static Node*
ifstmt(void)
{
	Node *n,*l,*p,*r, *c = nil;

	want((int[]){Lif,0});
	l = sstmt();
	if(try((int[]){';',0}).kind)
		c = expr();
	p = mkunray(Oseq, block());
	if(c) n = mkn(Oscope, l, mkn(Oif, c, p));
	else n = mkn(Oif, l, p);

	for(c = nil; try((int[]){Lelse,0}).kind; c = nil){
		if(try((int[]){Lif,0}).kind){
			l = sstmt();
			if(try((int[]){';',0}).kind)
				c = expr();
			r = mkunray(Oseq, block());
			if(c) p->r = mkn(Oscope, l, mkn(Oif, c, r));
			else p->r = mkn(Oif, l, r);
			p = r;
		}else
			p->r = block();
	}
	return n;
}

// for [ cond | forclause | rangeclause ] block
// ForClause = [ InitStmt ] ";" [ Condition ] ";" [ PostStmt ] .
static Node *
mkfor(Node *init, Node *cond, Node *post, Node *body)
{
	Node *n;

	n = mkn(Oseq, body, post);
	n = mkn(Ofor, cond, n);
	if(init)
		return mkn(Oscope, init, n);
	return n;
}

static Node*
forstmt(void)
{
	Node *i=nil, *c=nil, *p=nil, *tmp=nil;

	want((int[]){Lfor,0});
	if(peek((int[]){'{',0}).kind)
		return mkfor(i, mkbool(1), nil, block());

	if(peek((int[]){';',0}).kind == 0)
		tmp = sstmt();
	if(peek((int[]){'{',0}).kind){
		assert((c = tmp) != nil);
		assert(c->op != Odas && c->op != Oas);
		return mkfor(nil, c, nil, block());
	}
	want((int[]){';',0});
	i = tmp;
	tmp = nil;
	if(try((int[]){';',0}).kind)
		c = mkbool(1);
	else if(peek((int[]){'{'}).kind)
		assert(0);
	else{
		c = expr();
		want((int[]){';',0});
	}

	if(peek((int[]){'{'}).kind)
		return mkfor(i, c, nil, block());
	p = sstmt();
	assert(p->op != Odas);
	return mkfor(i, c, p, block());
}

// stmt = simple-stmt | decl 
static Node*
stmt(void)
{
	switch(peek((int[]){Lret,Lif,Lfor,Ltype,Lvar,'{',0}).kind){
	case Lret: return retstmt();
	case Lfor: return forstmt();
	case Lif: return ifstmt();
	case Ltype: return decltype();
	case Lvar: return declvar();
	case '{': return block();
	default: return sstmt();
	}
}

static Node*
stmtlist(void)
{
	Node h={0}, *p=&h;

	for(;peek((int[]){'}',0}).kind==0;){
		p = p->r = mkn(Oseq, stmt(), nil);
		want((int[]){';',0});
	}
	return h.r;
}

// '{' stmtlist '}'
static Node*
fbody(void)
{
	want((int[]){'{',0});
	if((try((int[]){'}',0})).kind)
		return nil;
	Node *n = stmtlist();
	want((int[]){'}',0});
	return n;
}

// StructType    = "struct" "{" { FieldDecl ";" } "}" .
// FieldDecl     = (IdentifierList Type | EmbeddedField) [ Tag ] .
// EmbeddedField = [ "*" ] TypeName [ TypeArgs ] .
// Tag           = string_lit .
static Node*
field(void)
{
	Decl *ids = identlist();
	Decl *t = typeids(ids, type());
	want((int[]){';', 0});
	return fielddecl(Dfield, t);
}

static Node*
fields(void)
{
	Node h={0}, *p=&h;

	for(;;)
	switch(peek((int[]){Lid,Ltid,0}).kind){
	case Lid:
	case Ltid: p = p->r = mkn(Oseq, field(), nil); break;
	default: return h.r;
	}
}

// [identlist] Type
static Decl*
paramdecl(void)
{
	Decl *ids = identlist();
	Type *t = type();
	return typeids(ids, t);
}

// paramdecl {"," paramdecl}
static Decl*
paramlist(void)
{
	Decl *p = paramdecl();
	for(;try((int[]){',',0}).kind==',';){
		 p =concatdecl(p, paramdecl());
	}
	return p;
}

// "(" [parameterlist [ ',' ] ] ")"
static Type*
parameters(void)
{
	Decl h={0}, *p=&h;

	want((int[]){'(',0});
	for(;try((int[]){')',0}).kind==0;)
		p = p->next = paramlist();
	return mktype(Tfn, 0, tnone, h.next);
}

static Type*
arraytype(void)
{
	Type *ty;
	Token t;

	want((int[]){'[',0});
	switch((t = try((int[]){Lid,Lconst,']',0})).kind){
	case Lid:
		assert(t.sym->decl == _decl);
		want((int[]){']',0});
		ty = mktype(Tarray, 0, type(), nil);
		ty->len = 0;
		return ty;
	case Lconst:
		assert(t.num > 0);
		want((int[]){']',0});
		ty = mktype(Tarray, 0, type(), nil);
		assert((ty->len = t.num) > 0);
		return ty;
	case ']':
		ty = mktype(Tslice, 0, type(), nil);
		return ty;
	default : assert(0);
	}
}

static Type*
type(void)
{
	Type h = {0}, *p=&h;
	Sym *s;

	Token t = want((int[]){Lid,Ltid,'*','[',0});
	switch(t.kind){
	case '*':
		do{
			p = p->tof = mktype(Tptr, IBY2WD, nil, nil);
		}while(try((int[]){'*',0}).kind);
		p->tof = type();
		return h.tof;
	case Lid:
	case Ltid:
		if((s = trygetpkg(t.sym))) return s->decl->ty;
		else return mkidtype(t.sym);
	case '[': unlex(t); return arraytype();
	default: assert(0);
	}
}

// type ["," type] 
static Decl*
typelist(void)
{
	Decl h={0}, *p=&h;	

	p = p->next = mkids(nil, type(), nil);
	for(;try((int[]){',',0}).kind==',';)
		p = p->next = mkids(nil, type(), nil);
	return h.next;
}

static Type*
tuptype(void)
{
	want((int[]){'(',0});
	Decl *d = typelist();
	want((int[]){')',0});
	assert(d->next != nil);
	return mktype(Ttup, 0, nil, d);
}

static Type*
fnresult(void)
{
	switch(peek((int[]){Lid, Ltid,'[','*','(',0}).kind){
	case '[':
	case '*':
	case Lid:
	case Ltid: return type();
	case '(': return tuptype();
	default: return tnone;
	}	
}

// parameters [result]
static Type*
signature(void)
{
	Type *param = parameters();
	param->tof = fnresult();
	return param;
}

// funcdecl "fn" Lid signature fbody 
static Node*
declfn(void)
{
	want((int[]){Lfn,0});
	Node *nm = name();
	Type *s = signature();
	Node *b = fbody();
	Node *n = fndecl(nm, s, b);
	return n;
}

// var-spec = identlist (Type ['=' exprlist] | '=' exprlist )
// var-decl = Lvar varspec
static Node*
declvar(void)
{
	want((int[]){Lvar,0});
	Decl *d = identlist();
	Type *t = type();
	switch(peek((int[]){';','=',0}).kind){
	case ';': return vardecl(d, t);
	default:assert(0);
	}
}

// type-decl = Ltype Lid (Lstruct) '{' fields '}'
static Node*
decltype(void)
{
	want((int[]){Ltype,0});
	Token t = want((int[]){Lid,0});
	Decl *id = mkids(t.sym, 0, nil);
	want((int[]){Lstruct,0});
	want((int[]){'{',0});
	Node *f = fields();
	want((int[]){'}',0});
	return structdecl(id, f);
}

static Node*
decl(void)
{
	switch(peek((int[]){Ltype,Lvar,0}).kind){
	case Ltype: return decltype();	
	case Lvar: return declvar();
	default: assert(0);
	}
}

static Node*
declexport(void)
{
	Node *n;

	want((int[]){Lexport,0});
	switch(peek((int[]){Lfn,Lvar,Ltype,0}).kind){
	case Lfn: n=declfn(); break;
	case Ltype: n=decltype(); break;
	case Lvar: n=declvar(); break;
	default: assert(0);
	}
	return mkn(Oexport, n, nil);
}

static Node*
topdecl(void)
{
	switch(peek((int[]){Lexport,Lfn,0}).kind){
	case Lexport: return declexport();
	case Lfn: return declfn();
	default: return decl();
	}
}

// SourceFile = PackageClause ";" { ImportDecl ";" } { TopLevelDecl ";" } .
static Node*
sourcefile(void)
{
	Node h={0}, *p = &h;

	for(;;){
		switch(peek((int[]){EOF,0}).kind){
		case EOF: return h.r;
		default:
			p=p->r=mkn(Oseq, topdecl(), nil);
			want((int[]){';',0});
			break;
		}
	}
	return h.r;
}

static Type*
imtype(void)
{
	Type *t = mkidtype(dotsym());
	t->kind = want((int[]){Lconst,0}).num;
	t->size = want((int[]){Lconst,0}).num;
	t->offset = want((int[]){Lconst,0}).num;
	return t;
}

static Type*
imparam(int n)
{
	if(n == 0)
		return nil;
	Type *res = mktype(Tpfn, 0, nil, nil);
	res->size = want((int[]){Lconst,0}).num;
	Decl h, *p=&h;
	for(int i = 0; i < n; i++){
		Type *t = imtype();
		Decl *d = mkids(nil, t, nil);
		d->offset = t->offset;
		p = p->next = d;
	}
	res->ids = h.next;
	return res;
}

static Type*
imret(int n)
{
	if(n == 0)
		return tnone;
	if(n == 1){
		want((int[]){Lconst,0});
		want((int[]){Lconst,0});
		return imtype();
	}
	Type *res = mktype(Ttup, 0, nil, nil);
	res->size = want((int[]){Lconst,0}).num;
	res->offset = want((int[]){Lconst,0}).num;
	Decl h, *p = &h;
	for(int i = 0; i < n; i++){
		p = p->next = mkids(nil, imtype(), nil);
	}
	res->ids = h.next;
	return res;
}

static Decl*
imdeclfn(void)
{
	want((int[]){Lfn,0});
	Sym *s = dotsym();
	int sz = want((int[]){Lconst,0}).num;
	int pc = want((int[]){Lconst,0}).num;
	int args = want((int[]){Lconst,0}).num;
	int rets = want((int[]){Lconst,0}).num;
	Type *t = imparam(args);
	t->tof = imret(rets);
	Decl *d = mkids(s, t, nil);
	d->pc = new(sizeof(Inst));
	d->store = Dpkg;
	d->offset = sz;
	d->pc->pc = pc;
	d->pc->m.reg = nimport;
	s->decl = d;
	return d;
}

static Decl*
imstruct(void)
{
	want((int[]){Ltype,0});
	Sym *s = dotsym();
	want((int[]){Lstruct,0});
	Type *t = mktype(Tstruct, 0, nil, nil);
	t->size = want((int[]){Lconst,0}).num;
	int n = want((int[]){Lconst,0}).num;
	Decl *res = mkids(s, t, nil);
	res->store = Dtype;
	Decl h={0}, *p=&h;
	for(int i = 0; i < n; ++i){
		Decl *c = ident();
		c->ty = imtype();
		c->dot = res;
		c->store = Dfield;
		p = p->next = c;
	}
	t->ids = h.next; 
	t->decl = res;
	s->decl = res;
	return res;
}

static Decl*
imfields(Import *i)
{
	Decl h, *p=&h, *t;
	for(;;){
		switch(peek((int[]){Lfn,Ltype,Leof,0}).kind){
		default : assert(0);
		case Leof: return h.next;
		case Ltype: t = imstruct(); break;
		case Lfn: 
			t = imdeclfn();
			break;
		}	
		p = p->next = t;
	}
}

static void
entryimport(Import *i, Sym *path)
{
	i->path = path;
	Decl *id = ident();
	i->sym = id->sym;
	i->decls = imfields(i);
	Type *ty = mktype(Tpkg, 0, nil, i->decls);
	id->ty = ty;
	for(Decl *p=ty->ids; p; p=p->next)
		p->dot = id;
	installids(Dpkg, id);
}

static void
importdecl(void)
{
	for(;try((int[]){Limport,0}).kind;){
		Sym *path = want((int[]){Lsconst, 0}).sym;
		if(isimported(path))
			return;
		imports = realloc(imports, (nimport+1)*sizeof(Import));
		importdef(path);
		entryimport(&imports[nimport], path);
		nimport += 1;
	}
}