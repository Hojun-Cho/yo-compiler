enum TypeKind
{
	Tnone,
	Tint,
	Tbool,
	Tstring,
	Tany,
	Ttup,
	Tid,
	Tfn,
	Tstruct,
	Tarray,
	Tslice,
	Tptr,
	Tref,
	Tpfn,
	Tpkg,
	Tend,
};

enum NodeOp
{
	Onone,
	Oadd,
	Osub,
	Olt,
	Ogt,
	Oneq,
	Oeq,
	Oleq,
	Ogeq,
	Omul,
	Oconst,
	Oindex,
	Oadr,
	Otup,
	Ofn,
	Opkg,
	Orange,
	Otypedecl,
	Ostrdecl,
	Oname,
	Ofor,
	Oif,
	Oandand,
	Ooror,
	Oret,
	Oref,
	Oxref,
	Oseq,
	Oscope,
	Ocall,
	Odas,
	Oas,
	Odot,
	Olen,
	Oslice,
	Ofielddecl,
	Ovardecl,
	Ovardecli,
	Oimport,
	Oexport,
	Opkgdecl,
	Oarray,
	Ostruct,
	Oend,
};

enum
{
	Aimm,				/* immediate */
	Amp,				/* global */
	Ampind,				/* global indirect */
	Afp,				/* activation frame */
	Afpind,				/* frame indirect */
	Apc,				/* branch */
	Adesc,				/* type descriptor immediate */
	Aoff,				/* offset in module description table */
	Anoff,			/* above encoded as -ve */
	Aerr,				/* error */
	Anone,				/* no operand */
	Aldt,				/* linkage descriptor table immediate */
	Aend
};

enum
{
	Rreg,				/* v(fp) */
	Rmreg,				/* v(mp) */
	Roff,				/* $v */
	Rnoff,			/* $v encoded as -ve */
	Rdesc,				/* $v */
	Rdescp,				/* $v */
	Rconst,				/* $v */
	Ralways,			/* preceeding are always addressable */
	Radr,				/* v(v(fp)) */
	Rmadr,				/* v(v(mp)) */
	Rcant,				/* following are not quite addressable */
	Rpc,				/* branch address */
	Rmpc,				/* cross module branch address */
	Rareg,				/* $v(fp) */
	Ramreg,				/* $v(mp) */
	Raadr,				/* $v(v(fp)) */
	Ramadr,				/* $v(v(mp)) */
	Rldt,				/* $v */
	Rend
};

enum
{
	Dtype,
	Dlocal,
	Dglobal,
	Dconst,
	Darg,
	Dfield,
	Dfn,
	Dpkg,
	Dundef,
	Dunbound,
	Dwundef,
};

enum
{
	TopScope = 0,
	MaxScope = 128,
	ScopeNil = 1,
	ScopeGlobal = 2,

	Sother = 0,
	Sloop,
	Sscope,
};

enum LexToken 
{
	Leof = -1,
	Lnone = 0,
	Lfn = 5000,
	Llen,
	Ltid,
	Lid,
	Lconst,
	Lsconst,
	Lret,
	Ldas,
	Ltype,
	Lvar,
	Lstruct,
	Lstring,
	Lfor,
	Lif,
	Lelse,
	Lexport,
	Limport,
	Lpackage,
	Leq,
	Lneq,
	Lleq,
	Lgeq,
	Landand,
	Loror,
};

typedef struct Lexer Lexer;
typedef struct Node Node;
typedef struct Type Type;
typedef struct Decl Decl;
typedef struct Inst Inst;
typedef struct Addr Addr;
typedef struct Sym Sym;
typedef struct Token Token;
typedef struct Import Import;

struct Sym
{
	int tok;
	char *name;
	int len;
	Decl *decl;
	Decl *unbound;
};

struct Decl
{
	Sym *sym;
	int nid;
	int scope;
	int das;
	Type *ty;
	Decl *next;
	Decl *old;
	Decl *dot;
	Node *init;
	int store;
	int offset;
	int tref;
	Inst *pc;
	Decl *locals;
	Decl *link;
	int flag;
};

struct Lexer
{
	int tok;
	int next;
	int cur;
	int eof;
	Sym *sym;
	FILE *f;
	WORD val;
};

struct Type
{
	int kind;
	int size;
	int len;
	int offset;
	Decl *decl;
	Type *tof;
	Decl *ids;
};

struct Node
{
	u8 op;
	u8 addable;
	u8 temps;
	Node *l, *r;
	Type *ty;
	int val;
	Decl *decl;
};

struct Addr
{
	i32 reg;
	i32 offset;
	Decl *decl;
};

struct Inst
{
	u8	op;
	u8	sm;			/* operand addressing modes */
	u8	mm;
	u8	dm;
	Addr	s;			/* operands */
	Addr	m;
	Addr	d;
	Inst	*next;
	Inst *branch;
	int block;
	u32 pc;
};

struct Token
{
	int kind;
	union{
		Sym *sym;
		int num;
		Decl *ids;
		Node *node;
		Type *type;
	};
};

struct Import
{
	Sym *sym;
	Sym *path;
	Decl *decls;
};