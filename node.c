#include "yo.h"

Node*
mkscope(Node *body)
{
	Node *n = mkn(Oscope, nil, body);
	return n;	
}

Node*
mkunray(int op, Node *l)
{
	Node *n = mkn(op, l, nil);
	return n;	
}

Node*
retalloc(Node *n, Node *nn)
{
	if(nn->ty == tnone)
		return nil;
	*n = (Node){0};
	n->op = Oxref;
	n->addable = Radr;
	n->l = dupn(&retnode);
	n->ty = nn->ty;	
	return n;
}

Node*
dupn(Node *n)
{
	Node *nn = new(sizeof(Node));
	*nn = *n;	
	if(n->l)
		nn->l = dupn(nn->l);
	if(n->r)
		nn->r = dupn(nn->r);
	return nn;
}

void*
new(int s)
{
	void *r = calloc(1, s);
	assert(r != 0);
	return r;
}

Node*
mkn(int op, Node *l, Node *r)
{
	Node *n = new(sizeof(Node));
	n->op = op;
	n->l = l;
	n->r = r;	
	return n;
}

Node*
mksconst(Sym *s)
{
	Node *n = mkn(Oconst, NULL, NULL);
	n->ty = tstring;
	n->decl = mkdecl(Dconst, tstring);
	n->decl->sym = s;
	return n;	
}

Node*
mkbool(int v)
{
	Node *n = mkn(Oconst, nil, nil);
	n->ty = tbool;
	n->val = v != 0;
	return n;
}

Node*
mkconst(int v)
{
	Node *n = mkn(Oconst, NULL, NULL);
	n->ty = tint;
	n->val = v;
	return n;	
}

Node*
mkname(Sym *s)
{
	Node *n = mkn(Oname, nil, nil);	
	if(s->unbound == nil){
		s->unbound = mkdecl(Dunbound, nil);	
		s->unbound->sym = s;
	}
	n->decl = s->unbound;
	return n;
}

Node*
fndecl(Node *n, Type *t, Node *body)
{
	Node *fn = mkn(Ofn, n, body);
	fn->ty = t;	
	return fn;
}