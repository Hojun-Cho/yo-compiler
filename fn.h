/* lex.c parse.c */
void lexinit(void);
void lexstart(char*);
Sym* enter(char *name, int tok);
Token peek(int *arr);
Token try(int *arr);
Token want(int *arr);
Token lex(void);
void importdef(Sym *file);
Sym* dotsym(void);
void unlex(Token tok);
int isimported(Sym *path);
int isexported(Decl *d);
Node *parse(char*);
Node** parsefiles(char **srcs, int n);

/* node.c */
void* new(int s);
Node* mkn(int op, Node *l, Node *r);
Node* mkconst(int v);
Node* dupn(Node *n);
Node* retalloc(Node *n, Node *nn);
Node* mkname(Sym *s);
Decl* mkids(Sym *s, Type *t, Decl *next);
Node* mkunray(int op, Node *l);
Node* mksconst(Sym *s);
Node* mkscope(Node *body); 
Node* mkbool(int v);

/* com.c */
void fncom(Decl *);

/* assert.c */
void gassert(Node *n);
void gdecl(Node *n);
void gbind(Node *n);
void assertfn(Decl *d);
void gentry(Node *tree);

/* type.c */
void typeinit(void);
void typestart(void);
Decl* mkdecl(int store, Type *t);
Type* mktype(int kind, int size, Type *tof, Decl *args);
Decl* typeids(Decl *ids, Type *t);
Type* usetype(Type *t);
Type* asserttype(Type *t);
int eqtype(Type *t1, Type *t2);
int eqtup(Type *t1, Type *t2);
Node* structdecl(Decl *id, Node *f);
void bindtypes(Type *t);
Type* mkidtype(Sym *s);
Node* fielddecl(int store, Decl *ids);
Type* isvalidty(Type *t);
Decl* isinids(Decl *ids, Sym *s);
Decl* isinpkg(Decl *ids, Sym *s);
int bindsize(Type *t); 

/* decl.c */
Node* fndecl(Node *n, Type *t, Node *body);
Inst* nextinst(void);
Inst* mkinst(void);
void installids(int store, Decl *ids);
void fndecled(Node *n);
void declstart(void);
void pushscope(Node *n, int kind);
void repushids(Decl *d);
Decl* popids(Decl *d);
Decl* popscope(void);
int resolvedesc(int l, Decl *d);
Decl* resolveldts(Decl *d, Decl **dd);
void dasdecl(Node *n);
Decl* concatdecl(Decl *d1, Decl *d2);
Inst* genmove(Type *t, Node *s, Node *d);
int align(int off);
Node* vardecl(Decl *ids, Type *t);
void vardecled(Node *n);
Node* varinit(Decl *ids, Node *e);
void structdecled(Node *n);
int decllen(Decl*d); 
int sharelocal(Decl *d);

/* dis.c */
int disinst(FILE *out, Inst *in);
void wr1(FILE *f, u8 v);
void wr4(FILE *f, u32 v);
void disaddr(FILE* f, u32 m, Addr *a);

/* gen.c */
void genstart(void);
Inst* genop(int op, Node *s, Node *m, Node *d);
Inst* genrawop(int op, Node *s, Node *m, Node *d);
Addr genaddr(Node *n);
void instconv(FILE *f, Inst *i);
int sameaddr(Node *n, Node *m);
Node* talloc(Node *n, Type *t, Node *nok);
void tfree(Node *n);
void tfreenow(void);
int resolvepcs(Inst *inst);
Decl* tdecls(void);
void tinit(void);
int idoffsets(Decl *id, long offset, int al);
int pushblock(void);
void repushblock(int b);
void popblock(void);
void ipatch(Inst *b, Inst *d);

/* asm */
void asminst(FILE *f, Inst *in);
void asmexport(FILE *f, Sym *pkg, Decl **arr, int n);
void genexports(char *);
void genbin(char *);

/* file.c */
char* getpath(void);
int issufix(char *a, char *sufix);
char** getfiles(char *path, char *sufix);
Node** parsefiles(char **srcs, int n);
Sym* assertpkgdecl(Node **trees, int n);