#include "vm.h"
// 000 LO(MP) offset indirect from MP
// 001 LO(FP) offset indirect from FP
// 010 $OP 30 bit immediate
// 011 none no operand
// 100 SO(SO(MP)) double indirect from MP
// 101 SO(SO(FP)) double indirect from FP
// 110 reserved
// 111 reserved

#define DIND(reg, xxx) (u8*)((*(WORD*)(R.reg+R.PC->xxx.f))+R.PC->xxx.s)

static void
D09(void)
{
	R.s = R.FP+R.PC->s.ind;
	R.d = R.FP+R.PC->d.ind;
	R.m = R.d;
}
static void
D0A(void)
{
	R.s = R.FP+R.PC->s.ind;
	R.d = (u8*)&R.PC->d.imm;
	R.m = R.d;
}
static void
D0B(void)
{
	R.s = R.FP+R.PC->s.ind;
}
static void
D0D(void)
{
	R.s = R.FP+R.PC->s.ind;
	R.d = DIND(FP, d);
	R.m = R.d;
}
static void
D0F(void)
{
	R.s = R.FP+R.PC->s.ind;
}
static void
D11(void)
{
	R.s = (u8*)&R.PC->s.imm;
	R.d = R.FP+R.PC->d.ind;
	R.m = R.d;
}
static void
D15(void)
{
	R.s = (u8*)&R.PC->s.imm;
	R.d = DIND(FP, d);
	R.m = R.d;
}
static void
D1A(void)
{
	R.d = (u8*)&R.PC->d.imm;
	R.m = R.d;
}
static void
D1B(void) /* 11 011 */
{
}
static void
D29(void)
{
	R.s = DIND(FP, s);	
	R.d = R.FP+R.PC->d.ind;
	R.m = R.d;
}
static void
D2D(void)
{
	R.s = DIND(FP, s);	
	R.d = DIND(FP, d);	
	R.m = R.d;
}
static void
D32(void)
{
	R.d = (u8*)&R.PC->d.imm;
	R.m = R.d;
}
static void
D49(void)
{
	R.s = R.FP+R.PC->s.ind;
	R.d = R.FP+R.PC->d.ind;
	R.t = (i16)R.PC->reg;
	R.m = &R.t;
}
static void
D4A(void)
{
	R.s = R.FP+R.PC->s.ind;
	R.d = (u8*)&R.PC->d.imm;
	R.t = (i16)R.PC->reg;
	R.m = &R.t;
}
static void
D4D(void)
{
	R.s = R.FP+R.PC->s.ind;
	R.d = DIND(FP, d);
	R.t = (i16)R.PC->reg;
	R.m = &R.t;
}
static void
D51(void)
{
	R.s = (i8*)&R.PC->s.imm;
	R.d = R.FP+R.PC->d.ind;
	R.t = (i16)R.PC->reg;
	R.m = &R.t;
}
static void
D52(void)
{
	R.s = (u8*)&R.PC->s.imm;
	R.d = (u8*)&R.PC->d.imm;
	R.t = (i16)R.PC->reg;
	R.m = &R.t;
}
static void
D55(void)
{
	R.s = (u8*)&R.PC->s.imm;
	R.d = DIND(FP, d);
	R.t = (i16)R.PC->reg;
	R.m = &R.t;
}
static void
D69(void)
{
	R.s = DIND(FP, s);
	R.d = R.FP+R.PC->d.ind;
	R.t = (i16)R.PC->reg;
	R.m = &R.t;
}
static void
D89(void)
{
	R.s = R.FP+R.PC->s.ind;
	R.d = R.FP+R.PC->d.ind;
	R.m = R.FP+R.PC->reg;
}
static void
D8A(void)
{
	R.s = R.FP+R.PC->s.ind;
	R.d = (u8*)&R.PC->d.imm;
	R.m = R.FP+R.PC->reg;
}
static void
D8D(void)
{
	R.s = R.FP+R.PC->s.ind;
	R.d = DIND(FP, d);
	R.m = R.FP+R.PC->reg;
}
static void
D91(void)
{
	R.s = (u8*)&R.PC->s.imm;
	R.d = R.FP+R.PC->d.ind;
	R.m = R.FP+R.PC->reg;
}
static void
D92(void)
{
	R.s = (u8*)&R.PC->s.imm;
	R.d = (u8*)&R.PC->d.imm;
	R.m = R.FP+R.PC->reg;
}
static void
D93(void)
{
	R.s = (u8*)&R.PC->s.imm;
}
static void
D95(void)
{
	R.s = (u8*)&R.PC->s.imm;
	R.d = DIND(FP, d);
	R.m = R.FP+R.PC->reg;
}
static void
DA9(void)
{
	R.s = DIND(FP, s);
	R.d = R.FP+R.PC->d.ind;
	R.m = R.FP+R.PC->reg;
}
static void
DAD(void)
{
	R.s = DIND(FP, s);
	R.d = DIND(FP, d);
	R.m = R.FP+R.PC->reg;
}

void	(*dec[])(void) =
{
	[0x09] = D09,
	[0x0A] = D0A,
	[0x0B] = D0B,
	[0x0D] = D0D,
	[0x0F] = D0F,
	[0x11] = D11,
	[0x15] = D15,
	[0x1B] = D1B,
	[0x1A] = D1A,
	[0x29] = D29,
	[0x2D] = D2D,
	[0x32] = D32,
	[0x49] = D49,
	[0x4A] = D4A,
	[0x4D] = D4D,
	[0x51] = D51,
	[0x52] = D52,
	[0x55] = D55,
	[0x69] = D69,
	[0x89] = D89,
	[0x8A] = D8A,
	[0x8D] = D8D,
	[0x91] = D91,
	[0x92] = D92,
	[0x93] = D93,
	[0x95] = D95,
	[0xA9] = DA9,
	[0xAD] = DAD,
};
