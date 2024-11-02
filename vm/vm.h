#include "isa.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct Inst Inst;
typedef union Adr Adr;
typedef struct REG REG;
typedef struct Array Array;
typedef struct Frame Frame;
typedef union Stack Stack;

union Adr
{
	WORD imm;
	WORD ind;
	Inst*	ins;
	struct {
		u16	f;	/* First  */	
		u16	s;	/* Second  */
	};
};

struct Inst
{
	u8	op;
	u8	add;
	u16	reg;
	Adr	s;
	Adr	d;
};

struct Frame
{
	Inst *lr;
	u8 *fp;
};

union Stack
{
	u8 stack[1];
	struct{
		int sz;
		u8 *SP;
		u8 *TS;
		u8 *EX;
		union{
			u8 fu[1];
			Frame fr[1];
		};
	};
};

struct REG
{
	Inst *PC;
	u8 *FP;
	u8 *SP;
	u8 *TS;
	u8 *EX;
	void *s, *d, *m;
	int t, dt ,mt;
};

struct Array
{
	u8 *arr;
	WORD len;
	WORD cap;
	WORD size;
};

extern REG R;
extern	void	(*dec[])(void);
