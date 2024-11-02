/* instructions */
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int8_t i8;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;

typedef u8 byte;
typedef intptr_t WORD;

enum
{
	INOP,
	IARRAY,
	ISLICE,
	IFRAME,
	ICALL,
	IJMP,
	PCALL,
	IRET,
	IREF,
	ILEN,
	ILEA,
	IADDW,
	ISUBW,
	IMULW,
	IMULB,
	IMOVW,
	IMOVM,
	ILTW,
	IEQW,
	ILEQW,
	IBEQW,
	IBNEQW,
	IEND,
};

enum
{
	AMP	= 0x00,		/* Src/Dst op addressing */
	AFP	= 0x01,
	AIMM	= 0x02,
	AXXX	= 0x03,
	AIND	= 0x04,
	AMASK	= 0x07,
	AOFF	= 0x08,
	AVAL	= 0x10,

	ARM	= 0xC0,		/* Middle op addressing */
	AXNON	= 0x00,
	AXIMM	= 0x40,
	AXINF	= 0x80,
	AXINM	= 0xC0,

	IBY2WD	= sizeof(WORD),
	REGRET = 4,

	StrSize = 256,
	ArrHead = IBY2WD * 4,
};

#define SRC(x)		((x)<<3)
#define DST(x)		((x)<<0)
#define USRC(x)		(((x)>>3)&AMASK)
#define UDST(x)		((x)&AMASK)
#define UXSRC(x)	((x)&(AMASK<<3))
#define UXDST(x)	((x)&(AMASK<<0))
