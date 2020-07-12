#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "util.h"
#include "macro.h"
#include "fb.h"

int debug_switch = 0;
#define LOGD(...) (debug_switch ? printf(__VA_ARGS__) : 1)
#define LOGI(...) printf(__VA_ARGS__)

#define LOGD(...)
#define LOGI(...)

#define RV64
#define RV32M
#define RV32F
#define RV32D
#define RV32A

#ifdef RV64
#define RV64I
#define RV64M
#define RV64A
#define RV64F
#define RV64D
#endif

#define DEFAULT(x, ...)    \
	default:               \
	{                      \
		__VA_ARGS__ break; \
	}

#define CASE(x, ...)       \
	case x:                \
	{                      \
		__VA_ARGS__ break; \
	}

typedef __int128 s128_t;
typedef unsigned __int128 u128_t;
typedef unsigned long long u64_t;
typedef long long s64_t;
typedef unsigned int u32_t;
typedef unsigned short u16_t;
typedef unsigned char u8_t;
typedef signed long long s64_t;
typedef signed int s32_t;
typedef signed short s16_t;
typedef signed char s8_t;
typedef u32_t reg_t;
typedef s32_t imm_t;

#ifdef RV64
#define XLEN 64
typedef u64_t addr_t;
typedef u64_t x_t;
#elif define(RV32)
#define XLEN 32
typedef u32_t *addr_t;
typedef u32_t x_t;
#endif

u8_t memory[1024 * 1024 * (16 + 8 + 16)];
u8_t uart[8];
u8_t mtimer[0xc000];
u8_t fb[0x008];
x_t CSRs[0x1000];
x_t pc = 0x80000000U, x[33]; // virtual machine registers

union fr {
	double d;
	float f;
	u32_t u32;
	u64_t u64;
	s32_t s32;
	s64_t s64;
};

union fr f[33];

#define X(r) (x[((r - 1) & 0b11111) + 1]) // for write
#define XS128(r) ((s128_t)x[r])
#define XU128(r) ((u128_t)x[r])
#define XS64(r) ((s64_t)x[r])
#define XU64(r) ((u64_t)x[r])
#define XS32(r) ((s32_t)x[r])
#define XU32(r) ((u32_t)x[r])
#define XU16(r) ((u16_t)x[r])
#define XU8(r) ((u8_t)x[r])
#define XS16(r) ((s16_t)x[r])
#define XS8(r) ((s8_t)x[r])
#ifdef RV64
#define XS(r) ((s64_t)x[r])
#define XU(r) ((u64_t)x[r])
#define S2LEN(x) ((s128_t)x)
#define U2LEN(x) ((u128_t)x)

#elif define(RV32)
#define XS(r) ((s32_t)x[r])
#define XU(r) ((u32_t)x[r])
#define S2LEN(x) ((s64_t)x)
#define U2LEN(x) ((u64_t)x)
#endif

#define F32(r) (f[r].f)
#define F64(r) (f[r].d)
#define F32U(r) (f[r].u32)
#define F64U(r) (f[r].u64)
#define F32S(r) (f[r].s32)
#define F64S(r) (f[r].s64)

#define F2U32(x) (*(u32_t *)&(x))
#define F2U64(x) (*(u64_t *)&(x))
#define U2F32(x) (*(float *)&(x))
#define U2F64(x) (*(double *)&(x))

static struct
{
	u64_t addr;
	const char *name;
} function_map[] = {
#include "nm.c"
};

static char *search_function(u64_t addr)
{
	int low = 0;
	int high = (sizeof(function_map) / sizeof(function_map[0])) - 1;
	while (low <= high)
	{
		int middle = (low + high) / 2;
		if (addr == function_map[middle].addr)
		{
			return function_map[middle].name;
		}
		else if (addr > function_map[middle].addr)
		{
			low = middle + 1;
		}
		else if (addr < function_map[middle].addr)
		{
			high = middle - 1;
		}
	}
	return "NULL";
}

static u64_t pc_count = 0;
static u64_t M64(addr_t addr)
{
	if (addr - 0x80000000U < 0x2800000U)
		return *(u64_t *)(memory + addr - 0x80000000U);
	else if (addr - 0x02000000U < 0xc000U)
	{
		addr_t m = mtimer + addr - 0x02000000U;
		if (addr == (0x02000000U + 0x4000U))
		{
		}
		else if (addr == (0x02000000U + 0xbff8U))
		{
		}
		return *(u64_t *)(mtimer + addr - 0x02000000U);
	}
	else
	{
 		exit(0);
		return 0;
	}
}

static u32_t M32(addr_t addr)
{
	if (addr - 0x80000000U < 0x2800000U)
		return *(u32_t *)(memory + addr - 0x80000000U);
	else if (addr - 0x02000000U < 0xc000U)
	{
		if (addr == (0x02000000U + 0x4000U))
		{
		}
		return *(u32_t *)(mtimer + addr - 0x02000000U);
	}
	else
	{
		LOGI("Out of memory! [Addr:%#016llx]\n", addr);
		exit(0);
		return 0;
	}
}

static u16_t M16(addr_t addr)
{
	if (addr - 0x80000000U < 0x2800000U)
		return *(u16_t *)(memory + addr - 0x80000000U);
	else
	{
		LOGI("Out of memory! [Addr:%#016llx]\n", addr);
		exit(0);
		return 0;
	}
}

static u8_t M8(addr_t addr)
{
	if (addr - 0x80000000U < 0x2800000U)
	{
		return *(u8_t *)(memory + addr - 0x80000000U);
	}
	else if (addr - 0x10000000U < 0x00000008U)
	{
		addr_t m = uart + addr - 0x10000000U;
		if (addr == 0x10000000U)
		{
			int ret = peekchar();
			return (ret != -1) ? ret : 0;
		}
		else if (addr == (0x10000000U + 0x05U))
		{
			int ret = probchar();
			return 0x1 << 6 | ((ret > 0) << 0);
		}
	}
	else
	{
		LOGI("Out of memory! [Addr:%#016llx]\n", addr);
		exit(0);
		return 0;
	}
}

static u64_t WM64(addr_t addr, u64_t value)
{
	addr_t m;
	if (addr - 0x80000000U < 0x2800000U)
		m = memory + addr - 0x80000000U;
	else if (addr - 0x02000000U < 0xc000U)
	{
		m = mtimer + addr - 0x02000000U;

		if (addr == (0x02000000U + 0x4000U))
		{
			*(u64_t *)m = value;
		}
		else if (addr == (0x02000000U + 0xbff8U))
		{

			// printf("%p",clock());
		}
	}
	else
	{
		LOGI("Out of memory! [Addr:%#016llx]\n", addr);
		exit(0);
	}

	*(u64_t *)m = value;
}

static u32_t WM32(addr_t addr, u32_t value)
{
	addr_t m;
	if (addr - 0x80000000U < 0x2800000U)
		m = memory + addr - 0x80000000U;
	else if (addr - 0x03000000U < 0x008U)
	{	
		m = fb + addr - 0x03000000U;
		*(u32_t *)m = value;
		if (addr == (0x03000000U + 0x004U))
		{
			addr_t fb_addr = *(u32_t *)fb;
			framebuffer_present(memory + fb_addr - 0x80000000U,value);
		}
		return;
	}
	else
	{
		LOGI("Out of memory! [Addr:%#016llx]\n", addr);
		exit(0);
	}

	*(u32_t *)m = value;
}

static u16_t WM16(addr_t addr, u16_t value)
{
	addr_t m;
	if (addr - 0x80000000U < 0x2800000U)
		m = memory + addr - 0x80000000U;
	else
	{
		LOGI("Out of memory! [Addr:%#016llx]\n", addr);
		exit(0);
	}

	*(u16_t *)m = value;
}

static void WM8(addr_t addr, u8_t value)
{
	addr_t m;
	if (addr - 0x80000000U < 0x2800000U)
		m = memory + addr - 0x80000000U;
	else if (addr - 0x10000000U < 0x00000008U)
	{
		if (addr == (0x10000000U))
		{
			printf("%c", value);
		}
		m = uart + addr - 0x10000000U;
	}
	else
	{
		LOGI("Out of memory! [Addr:%#016llx]\n", addr);
		exit(0);
	}
	*(u8_t *)m = value;
}

#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

enum REGISTER
{
	x0 = 0,
	X0 = 0,
	zero = 0,
	x1 = 1,
	X1 = 1,
	ra = 1,
	x2 = 2,
	X2 = 2,
	sp = 2,
	x3 = 3,
	X3 = 3,
	gp = 3,
	x4 = 4,
	X4 = 4,
	tp = 4,
	x5 = 5,
	X5 = 5,
	t0 = 5,
	x6 = 6,
	X6 = 6,
	t1 = 6,
	x7 = 7,
	X7 = 7,
	t2 = 7,
	x8 = 8,
	X8 = 8,
	s0 = 8,
	fp = 8,
	x9 = 9,
	X9 = 9,
	s1 = 9,
	x10 = 10,
	X10 = 10,
	a0 = 10,
	x11 = 11,
	X11 = 11,
	a1 = 11,
	x12 = 12,
	X12 = 12,
	a2 = 12,
	x13 = 13,
	X13 = 13,
	a3 = 13,
	x14 = 14,
	X14 = 14,
	a4 = 14,
	x15 = 15,
	X15 = 15,
	a5 = 15,
	x16 = 16,
	X16 = 16,
	a6 = 16,
	x17 = 17,
	X17 = 17,
	a7 = 17,
	x18 = 18,
	X18 = 18,
	s2 = 18,
	x19 = 19,
	X19 = 19,
	s3 = 19,
	x20 = 20,
	X20 = 20,
	s4 = 20,
	x21 = 21,
	X21 = 21,
	s5 = 21,
	x22 = 22,
	X22 = 22,
	s6 = 22,
	x23 = 23,
	X23 = 23,
	s7 = 23,
	x24 = 24,
	X24 = 24,
	s8 = 24,
	x25 = 25,
	X25 = 25,
	s9 = 25,
	x26 = 26,
	X26 = 26,
	s10 = 26,
	x27 = 27,
	X27 = 27,
	s11 = 27,
	x28 = 28,
	X28 = 28,
	t3 = 28,
	x29 = 29,
	X29 = 29,
	t4 = 29,
	x30 = 30,
	X30 = 30,
	t5 = 30,
	x31 = 31,
	X31 = 31,
	t6 = 31,
};

enum
{
	OPCODE_R,
	OPCODE_I,
	OPCODE_S,
	OPCODE_U,
	OPCODE_SB,
	OPCODE_UI
};

#define U8(x) ((u8_t)(x))
#define S8(x) ((s8_t)(x))
#define U16(x) ((u16_t)(x))
#define S16(x) ((s16_t)(x))
#define U32(x) ((u32_t)(x))
#define S32(x) ((s32_t)(x))
#define U64(x) ((u64_t)(x))
#define S64(x) ((s64_t)(x))
#define U128(x) ((u128_t)(x))
#define S128(x) ((s128_t)(x))

#define GENERIC_1(x) (_Generic((x),u32_t:1U,u64_t:1ULL,default:1))

#define GET(x, offset, len) (((x) >> (offset)) & ((GENERIC_1(x)  << (len)) - 1))
#define GET_FIXED(x, offset, len) ((x) & (((GENERIC_1(x)  << (len)) - 1) << (offset)))
#define GET_FIXED_L(x, offset, len, offset2) (GET_FIXED((x), (offset), (len)) << ((offset2) - (offset)))
#define GET_FIXED_R(x, offset, len, offset2) (GET_FIXED((x), (offset), (len)) >> ((offset) - (offset2)))
#define GET_FIXED_SHIFT(x, offset, len, offset2) ((((offset2) > (offset))) ? (GET_FIXED_L(x, offset, len, offset2)) : (GET_FIXED_R(x, offset, len, offset2)))
#define OPCODE(ins) GET(U32(ins), 0, 7)

#define GET_EXT(ins, width) U32(S32(GET_FIXED((ins), 31, 1)) >> ((width)-1))

#define GET_I_IMM(ins) S32(GET_EXT((ins), 21) | GET_FIXED_SHIFT((ins), 25, 6, 5) | GET_FIXED_SHIFT((ins), 21, 4, 1) | GET_FIXED_SHIFT((ins), 20, 1, 0))
#define GET_S_IMM(ins) S32(GET_EXT((ins), 21) | GET_FIXED_SHIFT((ins), 25, 6, 5) | GET_FIXED_SHIFT((ins), 8, 4, 1) | GET_FIXED_SHIFT((ins), 7, 1, 0))
#define GET_B_IMM(ins) S32(GET_EXT((ins), 20) | GET_FIXED_SHIFT((ins), 7, 1, 11) | GET_FIXED_SHIFT((ins), 25, 6, 5) | GET_FIXED_SHIFT((ins), 8, 4, 1))
#define GET_U_IMM(ins) S32(GET_FIXED((ins), 12, 20))
#define GET_J_IMM(ins) S32(GET_EXT(ins, 12) | GET_FIXED((ins), 12, 8) | GET_FIXED_SHIFT((ins), 20, 1, 11) | GET_FIXED_SHIFT((ins), 25, 6, 5) | GET_FIXED_SHIFT((ins), 21, 4, 1))

#define GET_I_INS(imm) (GET_FIXED_SHIFT((imm), 0, 12, 20))
#define GET_S_INS(imm) (GET_FIXED_SHIFT((imm), 5, 7, 25) | GET_FIXED_SHIFT((imm), 0, 5, 7))
#define GET_B_INS(imm) (GET_FIXED_SHIFT((imm), 12, 1, 31) | GET_FIXED_SHIFT((imm), 5, 6, 25) | GET_FIXED_SHIFT((imm), 1, 4, 8) | GET_FIXED_SHIFT((imm), 11, 1, 7))
#define GET_U_INS(imm) (GET_FIXED((imm), 12, 20))
#define GET_J_INS(imm) (GET_FIXED_SHIFT((imm), 20, 1, 31) | GET_FIXED_SHIFT((imm), 1, 10, 21) | GET_FIXED_SHIFT((imm), 11, 1, 20) | GET_FIXED((imm), 12, 8))

#define INS_R(opcode, funct3, funct7, rd, rs1, rs2) (GET((opcode), 0, 7) | GET_FIXED_SHIFT((funct3), 0, 3, 12) | GET_FIXED_SHIFT((funct7), 0, 7, 25) | GET_FIXED_SHIFT((rd), 0, 5, 7) | GET_FIXED_SHIFT((rs1), 0, 5, 15) | GET_FIXED_SHIFT((rs2), 0, 5, 20))
#define INS_I(opcode, funct3, rd, rs1, imm) (GET((opcode), 0, 7) | GET_FIXED_SHIFT((funct3), 0, 3, 12) | GET_FIXED_SHIFT((rd), 0, 5, 7) | GET_FIXED_SHIFT((rs1), 0, 5, 15) | GET_I_INS((imm)))
#define INS_S(opcode, funct3, rs1, rs2, imm) (GET((opcode), 0, 7) | GET_FIXED_SHIFT((funct3), 0, 3, 12) | GET_FIXED_SHIFT((rs1), 0, 5, 15) | GET_FIXED_SHIFT((rs2), 0, 5, 20) | GET_S_INS((imm)))
#define INS_B(opcode, funct3, rs1, rs2, imm) (GET((opcode), 0, 7) | GET_FIXED_SHIFT((funct3), 0, 3, 12) | GET_FIXED_SHIFT((rs1), 0, 5, 15) | GET_FIXED_SHIFT((rs2), 0, 5, 20) | GET_B_INS((imm)))
#define INS_U(opcode, rd, imm) (GET((opcode), 0, 7) | GET_FIXED_SHIFT((rd), 0, 5, 7) | GET_U_INS((imm)))
#define INS_J(opcode, rd, imm) (GET((opcode), 0, 7) | GET_FIXED_SHIFT((rd), 0, 5, 7) | GET_J_INS((imm)))

#define GET_SHIFT5(x) GET((x), 0, 5)
#define GET_SHIFT6(x) GET((x), 0, 6)

#ifdef RV64
#define GET_SHIFT(x) GET((x), 0, 6)
#elif define(RV32)
#define GET_SHIFT(x) GET((x), 0, 5)

#endif

#define GET_C_EXT(ins, width) U32((S32(GET(U32(ins), 12, 1)) << 31) >> ((width)-1))

int isnumber(float f)
{
	return (f == f);
}

int classify_float(float f)
{
	u32_t t = F2U32(f);
	if (isnumber(f))
	{
		if (isinf(f))
			return isinf(f) == 1 ? 7 : 0; //
		else if (f == 0.0f || f == -0.0f)
			return GET(t, 31, 1) ? 3 : 4; //-0 / +0
		else if (GET(t, 23, 8) == 0)	  //非规格化数
			return GET(t, 31, 1) ? 2 : 5; //负的非规格化数 / 正的非规格化数
		else							  //规格化数
			return GET(t, 31, 1) ? 1 : 6; //负的规格化数 / 正的规格化数
	}
	else // NaN
	{
		//https://www.wikiwand.com/en/NaN
		//https://www.thinbug.com/q/25050133
		//https://cwiki.apache.org/confluence/display/stdcxx/FloatingPoint
		//TODO:如何区分quiet 和 signalling ?
		return 8; //signalling
				  //return 8  //quiet
	}
}

int classify_double(double d)
{
	u64_t t = F2U64(d);
	if (isnumber(d))
	{
		if (isinf(d))
			return isinf(d) == 1 ? 7 : 0; //
		else if (d == 0.0f || d == -0.0f)
			return GET(t, 63, 1) ? 3 : 4; //-0 / +0
		else if (GET(t, 52, 11) == 0)	  //非规格化数
			return GET(t, 63, 1) ? 2 : 5; //负的非规格化数 / 正的非规格化数
		else							  //规格化数
			return GET(t, 63, 1) ? 1 : 6; //负的规格化数 / 正的规格化数
	}
	else // NaN
	{
		//https://www.wikiwand.com/en/NaN
		//https://www.thinbug.com/q/25050133
		//https://cwiki.apache.org/confluence/display/stdcxx/FloatingPoint
		//TODO:如何区分quiet 和 signalling ?
		return 8; //signalling
				  //return 8  //quiet
	}
}

void classify_print(int i)
{
	switch (i)
	{
	case 0:
		LOGD("[rs1]为−∞");
		break;
	case 1:
		LOGD("[rs1]是负规格化数");
		break;
	case 2:
		LOGD("[rs1]是负的非规格化数");
		break;
	case 3:
		LOGD("[rs1]是-0");
		break;
	case 4:
		LOGD("[rs1]是+0");
		break;
	case 5:
		LOGD("[rs1]是正的非规格化数");
		break;
	case 6:
		LOGD("[rs1]是正的规格化数");
		break;
	case 7:
		LOGD("[rs1]为+∞");
		break;
	case 8:
		LOGD("[rs1]是信号(signaling)NaN");
		break;
	case 9:
		LOGD("[rs1]是一个安静(quiet)NaN");
		break;

	default:
		break;
	}
	LOGD("\n");
}

//Machine Trap Setup
#define MSTATUS (0x300)
#define MISA (0x301)
#define MEDELEG (0x302)
#define MIDELEG (0x303)
#define MIE (0x304)
#define MTVEC (0x305)
#define MCOUNTEREN (0x305)

//Machine Trap Handling
#define MSCRATCH (0x340)
#define MEPC (0x341)
#define MCAUSE (0x342)
#define MTVAL (0x343)
#define MIP (0x344)

#define MSTATUS_UIE (1 << 0)
#define MSTATUS_SIE (1 << 1)
#define MSTATUS_MIE (1 << 3)
#define MSTATUS_UPIE (1 << 4)
#define MSTATUS_SPIE (1 << 5)
#define MSTATUS_MPIE (1 << 7)
#define MSTATUS_SPP (1 << 8)
#define MSTATUS_MPP (3 << 11)
#define MSTATUS_FS (3 << 13)
#define MSTATUS_XS (3 << 15)
#define MSTATUS_MPRV (1 << 17)
#define MSTATUS_SUM (1 << 18)
#define MSTATUS_MXR (1 << 19)
#define MSTATUS_TVM (1 << 20)
#define MSTATUS_TW (1 << 21)
#define MSTATUS_TSR (1 << 22)
#define MSTATUS32_SD (1 << 31)
#define MSTATUS_UXL (3ULL << 32)
#define MSTATUS_SXL (3ULL << 34)
#define MSTATUS64_SD (1ULL << 63)

#define MIP_USIP (1 << 0)
#define MIP_SSIP (1 << 1)
#define MIP_MSIP (1 << 3)
#define MIP_UTIP (1 << 4)
#define MIP_STIP (1 << 5)
#define MIP_MTIP (1 << 7)
#define MIP_UEIP (1 << 8)
#define MIP_SEIP (1 << 9)
#define MIP_MEIP (1 << 11)

#define MIE_USIE (1 << 0)
#define MIE_SSIE (1 << 1)
#define MIE_MSIE (1 << 3)
#define MIE_UTIE (1 << 4)
#define MIE_STIE (1 << 5)
#define MIE_MTIE (1 << 7)
#define MIE_UEIE (1 << 8)
#define MIE_SEIE (1 << 9)
#define MIE_MEIE (1 << 11)

typedef struct bit64_t
{
	x_t
		_0 : 1,
		_1 : 1, _2 : 1, _3 : 1, _4 : 1, _5 : 1, _6 : 1, _7 : 1, _8 : 1, _9 : 1, _10 : 1,
		_11 : 1, _12 : 1, _13 : 1, _14 : 1, _15 : 1, _16 : 1, _17 : 1, _18 : 1, _19 : 1, _20 : 1,
		_21 : 1, _22 : 1, _23 : 1, _24 : 1, _25 : 1, _26 : 1, _27 : 1, _28 : 1, _29 : 1, _30 : 1,
		_31 : 1, _32 : 1, _33 : 1, _34 : 1, _35 : 1, _36 : 1, _37 : 1, _38 : 1, _39 : 1, _40 : 1,
		_41 : 1, _42 : 1, _43 : 1, _44 : 1, _45 : 1, _46 : 1, _47 : 1, _48 : 1, _49 : 1, _50 : 1,
		_51 : 1, _52 : 1, _53 : 1, _54 : 1, _55 : 1, _56 : 1, _57 : 1, _58 : 1, _59 : 1, _60 : 1,
		_61 : 1, _62 : 1, _63 : 1;
} bit64_t;

static x_t CSR(addr_t addr)
{
	addr_t m;
	m = CSRs + addr;
	return *(x_t *)m;
}

static void WCSR(addr_t addr, x_t value)
{
	addr_t m;
	m = CSRs + addr;
	*(x_t *)m = value;
}

static inline void check_exception()
{
	if ((CSR(MSTATUS) & MSTATUS_MIE) && (CSR(MIE) & MIE_MTIE))
	{
		u64_t mtime = *(u64_t *)(mtimer + 0xbff8U);
		u64_t mtimecmp = *(u64_t *)(mtimer + 0x4000U);
		if (mtime >= mtimecmp)
		{
			WCSR(MCAUSE, (1UL << (XLEN - 1)) | 7);
			WCSR(MIP, CSR(MIP) | MIP_MTIP);
			WCSR(MEPC, pc);

			x_t t = CSR(MSTATUS);
			bit64_t b = *(bit64_t *)&t;
			b._7 = b._3; //MPIE=MIE
			b._3 = 0;	 //MIE=0
			t = *(x_t *)&b;
			WCSR(MSTATUS, t);
			pc = CSR(MTVEC);
		}
	}
}

static void RVG(u32_t ins)
{
	switch (OPCODE(ins))
	{
	case 0b0110111: //[U] LUI
	{
		reg_t rd = GET(ins, 7, 5);
		X(rd) = GET_U_IMM(ins);
		break;
	}
	case 0b0010111: //[U] [AUIPC]
	{
		reg_t rd = GET(ins, 7, 5);
		X(rd) = pc + GET_U_IMM(ins);
		break;
	}
	case 0b1101111: //[J] [JAL]
	{
		reg_t rd = GET(ins, 7, 5);
		X(rd) = pc + 4;
		pc += GET_J_IMM(ins);
		// char *s = search_function(pc);

		// if (strcmp(s, "NULL") && strcmp(s, "blk_romdisk_read"))
		// {
		// 	if (rd == 0)
		// 		printf("%s ret\r\n", s);
		// 	else
		// 	{
		// 		printf("%s\r\n", s);
		// 	}
		// }
		LOGD("jal %08llX X[%d]=%lld offset=%08X\n", pc, rd, x[rd], GET_J_IMM(ins));
		return;
		break;
	}
	case 0b1100111: //[I] [JALR]
	{
		reg_t rd = GET(ins, 7, 5);
		reg_t rs1 = GET(ins, 15, 5);
		u32_t t = pc + 4;
		imm_t offset = GET_I_IMM(ins);
		pc = (x[rs1] + offset) & ~1;
		X(rd) = t;
		// char *s = search_function((x[rs1] + offset) & ~1);

		// if (strcmp(s, "NULL") && strcmp(s, "blk_romdisk_read"))
		// {
		// 	if (rd == 0)
		// 		printf("%s ret\r\n", s);
		// 	else
		// 	{
		// 		printf("%s\r\n", s);
		// 	}
		// }
		return;
		break;
	}
	case 0b1100011: //[B]
	{
		int funct3 = GET(ins, 12, 3);
		reg_t rs1 = GET(ins, 15, 5);
		reg_t rs2 = GET(ins, 20, 5);
		imm_t offset = GET_B_IMM(ins);
		switch (funct3)
		{
		case 0b000: //[BEQ]
		{
			if (x[rs1] == x[rs2])
			{
				pc += offset;
				return;
			}
			break;
		}
		case 0b001: //[BNE]
		{
			LOGD("bne offset=%08X, x[%d]=%lld, x[%d]=%lld\n", offset, rs1, (x[rs1]), rs2, (x[rs2]));
			if (x[rs1] != x[rs2])
			{
				pc += offset;
				return;
			}
			break;
		}
		case 0b100: //[BLT]
		{
			if (XS(rs1) < XS(rs2))
			{
				pc += offset;
				return;
			}
			break;
		}
		case 0b101: //[BGE]
		{
			// LOGD("bge offset=%08X, x[%d]=%d, x[%d]=%d\n", offset, rs1,S32(x[rs1]), rs2,S32(x[rs2]));
			if (XS(rs1) >= XS(rs2))
			{
				pc += offset;
				return;
			}
			break;
		}
		case 0b110: //[BLTU]
		{
			// LOGD("bltu offset=%08X, x[%d]=%llu x[%d]=%llu\n", offset, rs1,U32(x[rs1]), rs2,U32(x[rs2]));
			if (XU(rs1) < XU(rs2))
			{
				pc += offset;
				return;
			}
			break;
		}
		case 0b111: //[BGEU]
		{
			if (XU(rs1) >= XU(rs2))
			{
				pc += offset;
				return;
			}
			break;
		}
		default:
			break;
		}
		break;
	}
	case 0b0000011: //[I]
	{
		int funct3 = GET(ins, 12, 3);
		reg_t rs1 = GET(ins, 15, 5);
		imm_t offset = GET_I_IMM(ins);
		reg_t rd = GET(ins, 7, 5);
		switch (funct3)
		{
		case 0b000: //[LB]
		{
			LOGD("x%d=s8[x%d(%llu) + offset(%d)](%d)\n", rd, rs1, x[rs1], offset, S8(M8(x[rs1] + offset)));
			X(rd) = S8(M8(x[rs1] + offset));
			break;
		}
		case 0b001: //[LH]
		{
			LOGD("x%d=s16[x%d(%llu) + offset(%d)](%d)\n", rd, rs1, x[rs1], offset, S16(M16(x[rs1] + offset)));
			X(rd) = S16(M16(x[rs1] + offset));
			break;
		}
		case 0b010: //[LW]
		{
			LOGD("x%d=s32[x%d(%llu) + offset(%d)](%d)\n", rd, rs1, x[rs1], offset, S32(M32(x[rs1] + offset)));
			X(rd) = S32(M32(x[rs1] + offset));
			break;
		}
		case 0b100: //[LBU]
		{
			LOGD("x%d=u8[x%d(%llu) + offset(%d)](%d)\n", rd, rs1, x[rs1], offset, U8(M8(x[rs1] + offset)));
			X(rd) = M8(x[rs1] + offset);
			break;
		}
		case 0b101: //[LHU]
		{
			LOGD("x%d=u16[x%d(%llu) + offset(%d)](%d)\n", rd, rs1, x[rs1], offset, U16(M16(x[rs1] + offset)));
			X(rd) = M16(x[rs1] + offset);
			break;
		}
#ifdef RV64I
		case 0b110: //[LWU]
		{
			LOGD("x%d=u32[x%d(%llu) + offset(%d)](%d)\n", rd, rs1, x[rs1], offset, U32(M32(x[rs1] + offset)));
			X(rd) = M32(x[rs1] + offset);
			break;
		}
		case 0b011: //[LD]
		{
			LOGD("x%d=s64[x%d(%llu) + offset(%d)](%lld)\n", rd, rs1, x[rs1], offset, S64(M64(x[rs1] + offset)));
			X(rd) = (s64_t)(M64(x[rs1] + offset));
			break;
		}
#endif
		default:
			break;
		}
		break;
	}
	case 0b0100011: //[S]
	{
		int funct3 = GET(ins, 12, 3);
		reg_t rs1 = GET(ins, 15, 5);
		reg_t rs2 = GET(ins, 20, 5);
		imm_t offset = GET_S_IMM(ins);
		switch (funct3)
		{
		case 0b000: //[SB]
		{
			LOGD("8[x%d(%llu) + (%d)] = x%d(%llu)\n", rs1, x[rs1], offset, rs2, x[rs2]);

			WM8(x[rs1] + offset, XU8(rs2));
			break;
		}
		case 0b001: //[SH]
		{
			LOGD("16[x%d(%llu) + (%d)] = x%d(%llu)\n", rs1, x[rs1], offset, rs2, x[rs2]);

			WM16(x[rs1] + offset, XU16(rs2));
			break;
		}
		case 0b010: //[SW]
		{
			LOGD("32[x%d(%llu) + offset(%d)] = x%d(%llu)\n", rs1, x[rs1], offset, rs2, x[rs2]);
			if (U32(x[rs1] + offset) == 0xffffffff)
				exit(0);
			else if (U32(x[rs1] + offset) == 0xfffffffe)
				LOGI("%c", U32(x[rs2]) & 0xff);
			else if (U32(x[rs1] + offset) == 0xfffffff0)
				debug_switch = x[rs2];
			else
				WM32(x[rs1] + offset, XU32(rs2));
			break;
		}
#ifdef RV64I
		case 0b011: //[SD]
		{
			LOGD("64[x%d(%llu) + offset(%d)] = x%d(%llu)\n", rs1, x[rs1], offset, rs2, x[rs2]);
			WM64(x[rs1] + offset, XU64(rs2));
			break;
		}
#endif
		default:
			break;
		}
		break;
	}
	case 0b0010011: //[I]
	{
		int funct3 = GET(ins, 12, 3);
		reg_t rs1 = GET(ins, 15, 5);
		imm_t imm = GET_I_IMM(ins);
		reg_t rd = GET(ins, 7, 5);
		switch (funct3)
		{
		case 0b000: //[ADDI]
		{
			LOGD("x%d=x%d(%llu)+imm(%d)\n", rd, rs1, x[rs1], imm);
			X(rd) = x[rs1] + imm;
			break;
		}
		case 0b010: //[SLTI]
		{
			X(rd) = XS(rs1) < imm;
			break;
		}
		case 0b011: //[SLTIU]
		{
			X(rd) = XU(rs1) < imm;
			break;
		}
		case 0b100: //[XORI]
		{
			X(rd) = (x[rs1] ^ imm);
			break;
		}
		case 0b110: //[ORI]
		{
			X(rd) = (x[rs1] | imm);
			break;
		}
		case 0b111: //[ANDI]
		{
			X(rd) = (x[rs1] & imm);
			break;
		}
		case 0b001: //[SLLI] (Shift Left Logical Immediate)
		{

			int shamt = GET(ins, 20, 6);
#if !defined(RV64I) && defined(RV32I)
			if (GET(shamt, 5, 1))
			{
				//TODO: 仅当shamt[5]=0时，指令才是有效的
				SLLILOGI("error");
				exit(0);
			}
#endif
			X(rd) = x[rs1] << shamt;
			break;
		}
		case 0b101:
		{
			int funct6 = GET(ins, 26, 6);
			int shamt = GET(ins, 20, 6);
#if !defined(RV64I) && defined(RV32I)
			if (GET(shamt, 5, 1))
			{
				//TODO: 仅当shamt[5]=0时，指令才是有效的
				LOGI("error");
				exit(0);
			}
#endif
			if (funct6 == 0b000000) //[SRLI] (Shift Right Logical Immediate)
			{
				X(rd) = XU(rs1) >> shamt;
			}
			else if (funct6 == 0b010000) //[SRAI] (Shift Right Arithmetic Immediate)
			{
				X(rd) = XS(rs1) >> shamt;
			}
			break;
		}
		default:
			break;
		}
		break;
	}
#ifdef RV64I
	case 0b0011011:
	{
		reg_t rd = GET(ins, 7, 5);
		reg_t rs1 = GET(ins, 15, 5);

		int funct3 = GET(ins, 12, 3);
		switch (funct3)
		{
		case 0b000: //[I] [ADDIW]
		{
			imm_t imm = GET_I_IMM(ins);
			X(rd) = S32(x[rs1] + imm);
			break;
		}
		case 0b001: //[R] [SLLIW]
		{
			reg_t shamt = GET(ins, 20, 6);

			if (GET(shamt, 5, 1))
			{
				//仅当 shamt[5]=0 时指令有效
				LOGI("error");
				exit(0);
				break;
			}
			X(rd) = S32(x[rs1] << shamt);
			break;
		}
		case 0b101: //[R]
		{
			reg_t shamt = GET(ins, 20, 6);
			int funct3 = GET(ins, 26, 6);
			if (GET(shamt, 5, 1))
			{
				//仅当 shamt[5]=0 时指令有效
				LOGI("error");
				exit(0);
				break;
			}

			switch (funct3)
			{
			case 0b010000: //[SRAIW]
			{
				X(rd) = S32(XS32(rs1) >> shamt);

				break;
			}
			case 0b000000: //[SRLIW]
			{
				X(rd) = S32(XU32(rs1) >> shamt);

				break;
			}
			default:
				break;
			}
			break;
		}
		default:
			break;
		}
		break;
	}
#endif
	case 0b0110011: //[R]
	{
		int funct3 = GET(ins, 12, 3);
		int funct7 = GET(ins, 25, 7);

		reg_t rs1 = GET(ins, 15, 5);
		reg_t rs2 = GET(ins, 20, 5);
		reg_t rd = GET(ins, 7, 5);

		if (funct7 != 0b000001)
		{
			switch (funct3)
			{
			case 0b000:
			{
				if (funct7 == 0b0000000) //[ADD]
				{
					X(rd) = x[rs1] + x[rs2];
				}
				else if (funct7 == 0b0100000) //[SUB]
				{
					X(rd) = x[rs1] - x[rs2];
				}
				break;
			}
			case 0b001: //[SLL]
			{
				X(rd) = XU(rs1) << XU(rs2);
				break;
			}
			case 0b010: //[SLT]
			{
				X(rd) = XS(rs1) < XS(rs2);
				break;
			}
			case 0b011: //[SLTU]
			{
				X(rd) = XU(rs1) < XU(rs2);
				break;
			}
			case 0b100: //[XOR]
			{
				X(rd) = x[rs1] ^ x[rs2];
				break;
			}
			case 0b101:
			{
				if (funct7 == 0b0000000) //[SRL]
				{
					X(rd) = XU(rs1) >> GET_SHIFT(x[rs2]);
				}
				else if (funct7 == 0b0100000) //[SRA] (Shift Right Arithmetic)
				{
					X(rd) = XS(rs1) >> GET_SHIFT(x[rs2]);
				}
				break;
			}
			case 0b110: //[OR]
			{
				X(rd) = x[rs1] | x[rs2];
				break;
			}
			case 0b111: //[AND]
			{
				X(rd) = x[rs1] & x[rs2];
				break;
			}

			default:
				break;
			}
		}

#ifdef RV32M

		if (funct7 == 0b000001)
		{
			reg_t rs1 = GET(ins, 15, 5);
			reg_t rs2 = GET(ins, 20, 5);
			reg_t rd = GET(ins, 7, 5);
			switch (funct3)
			{
			case 0b000: //[MUL]
			{
				X(rd) = x[rs1] * x[rs2];
				break;
			}
			case 0b001: //[MULH]
			{
				X(rd) = (S2LEN(XS(rs1)) * S2LEN(XS(rs2))) >> XLEN;
				break;
			}
			case 0b010: //[MULHSU]
			{
				X(rd) = (S2LEN(XS(rs1)) * U2LEN(U32(rs2))) >> XLEN;
				break;
			}
			case 0b011: //[MULHU]
			{
				X(rd) = (U2LEN(XU(rs1)) * U2LEN(XU(rs2))) >> XLEN;
				break;
			}
			case 0b100: //[DIV]
			{
				X(rd) = XS(rs1) / XS(rs2);
				break;
			}
			case 0b101: //[DIVU]
			{
				X(rd) = XU(rs1) / XU(rs2);
				break;
			}
			case 0b110: //[REM]
			{
				X(rd) = XS(rs1) % XS(rs2);
				break;
			}
			case 0b111: //[REMU]
			{
				X(rd) = XU(rs1) % XU(rs2);
				break;
			}
			default:
				break;
			}
		}
#endif
		break;
	}
	case 0b0111011: //[R]
	{
		int funct3 = GET(ins, 12, 3);
		int funct7 = GET(ins, 25, 7);

		reg_t rs1 = GET(ins, 15, 5);
		reg_t rs2 = GET(ins, 20, 5);
		reg_t rd = GET(ins, 7, 5);
		switch (funct3)
		{
		case 0b000:
		{
			switch (funct7)
			{
			case 0b0000000: //[ADDW]
			{
				X(rd) = S32(x[rs1] + x[rs2]);
				break;
			}
			case 0b0100000: //[SUBW]
			{
				X(rd) = S32(x[rs1] - x[rs2]);
				break;
			}
			case 0b0000001: //[MULW]
			{
				X(rd) = S32(x[rs1] * x[rs2]);
				break;
			}
			default:
				break;
			}
			break;
		}
		case 0b001: //[SLLW]
		{
			X(rd) = S32((x[rs1] << GET_SHIFT5(x[rs2])));
			break;
		}
		case 0b101:
		{
			switch (funct7)
			{
			case 0b0000000: //[SRLW]
			{
				X(rd) = S32((XU32(rs1) >> GET_SHIFT5(x[rs2])));
				break;
			}
			case 0b0100000: //[SRAW]
			{
				X(rd) = S32((XS32(rs1) >> GET_SHIFT5(x[rs2])));
				break;
			}
			case 0b0000001: //[DIVUW]
			{
				X(rd) = S32(XU32(rs1) / XU32(rs2));
				break;
			}
			default:
				break;
			}
			break;
		}
		case 0b100: //[DIVW]
		{
			X(rd) = S32(XS32(rs1) / XS32(rs2));
			break;
		}
		case 0b110: //[REMW]
		{
			X(rd) = S32(XS32(rs1) % XS32(rs2));
			break;
		}
		case 0b111: //[REMUW]
		{
			X(rd) = S32(XU32(rs1) % XU32(rs2));
			break;
		}
		default:
			break;
		}
		break;
	}
	case 0b0001111: //[I]
	{
		int funct3 = GET(ins, 12, 3);
		switch (funct3)
		{
		case 0b000: //[FENCE]
		{
			break;
		}
		case 0b001: //[FENCE.I]
		{
			break;
		}
		default:
			break;
		}
		break;
	}
	case 0b1110011: //[I]
	{
		int funct3 = GET(ins, 12, 3);
		reg_t rs1 = GET(ins, 15, 5);
		reg_t rs2 = GET(ins, 20, 5);
		imm_t offset = GET_I_IMM(ins);
		reg_t rd = GET(ins, 7, 5);
		u32_t csr = GET(ins, 20, 12);
		switch (funct3)
		{
		case 0b000:
		{
			int funct7 = GET(ins, 25, 7);
			if (funct7 == 0b0000000) //[ECALL] (Environment Call)
			{
			}
			else if (funct7 == 0b0100000) //[EBREAK] (Environment Breakpoint)
			{
			}
			else if (funct7 == 0b0011000) //[MRET] (Machine-mode Exception Return)
			{
				pc = CSR(MEPC);
				// 特权级 = CSRs[mstatus].MPP
				x_t t = CSR(MSTATUS);
				bit64_t b = *(bit64_t *)&t;
				b._3 = b._7; //MIE=MPIE
				b._7 = 1;	 //MPIE=1
				t = *(x_t *)&b;
				WCSR(MSTATUS, t);
				return;
			}
			break;
		}
		case 0b001: //[CSRRW] (Control and Status Register Read and Write)
		{
			x_t t = CSR(csr);
			WCSR(csr, x[rs1]);
			X(rd) = t;
			break;
		}
		case 0b010: //[CSRRS] (Control and Status Register Read and Set)
		{
			x_t t = CSR(csr);
			WCSR(csr, t | x[rs1]);
			X(rd) = t;
			break;
		}
		case 0b011: //[CSRRC] (Control and Status Register Read and Clear)
		{
			x_t t = CSR(csr);
			WCSR(csr, t & (~x[rs1]));
			X(rd) = t;
			break;
		}
		case 0b101: //[CSRRWI] (Control and Status Register Read and Write Immediate)
		{
			imm_t zimm = GET(ins, 15, 5);
			X(rd) = CSR(csr);
			WCSR(csr, zimm);
			break;
		}
		case 0b110: //[CSRRSI]
		{
			imm_t zimm = GET(ins, 15, 5);
			x_t t = CSR(csr);
			WCSR(csr, t | zimm);
			X(rd) = t;
			break;
		}
		case 0b111: //[CSRRCI]
		{
			imm_t zimm = GET(ins, 15, 5);
			x_t t = CSR(csr);
			WCSR(csr, t & (~zimm));
			X(rd) = t;
			break;
		}
		default:
			break;
		}
		break;
	}
#if defined(RV32F) || defined(RV32D)
	case 0b0000111: //[I]
	{
		int funct3 = GET(ins, 12, 3);
		imm_t offset = GET_I_IMM(ins);
		reg_t rs1 = GET(ins, 15, 5);
		reg_t rd = GET(ins, 7, 5);

		switch (funct3)
		{
		case 0b010: //[FLW]
		{
			F32U(rd) = M32(x[rs1] + offset);
			break;
		}
		case 0b011: //[FLD]
		{
			F64U(rd) = M64(x[rs1] + offset);
			break;
		}
		default:
			break;
		}
		break;
	}
	case 0b0100111: //[S]
	{
		int funct3 = GET(ins, 12, 3);
		imm_t offset = GET_S_IMM(ins);
		reg_t rs1 = GET(ins, 15, 5);
		reg_t rs2 = GET(ins, 20, 5);
		switch (funct3)
		{
		case 0b010: //[FSW]
		{
			WM32(x[rs1] + offset, F32U(rs2));
			break;
		}
		case 0b011: //[FSD]
		{
			WM64(x[rs1] + offset, F64U(rs2));
			break;
		}
		default:
			break;
		}
		break;
	}
	case 0b1000011: //[R]
	{
		int funct2 = GET(ins, 25, 2);
		reg_t rd = GET(ins, 7, 5);
		reg_t rs1 = GET(ins, 15, 5);
		reg_t rs2 = GET(ins, 20, 5);
		reg_t rs3 = GET(ins, 27, 5);

		switch (funct2)
		{
		case 0b00: //[FMADD.S]
		{
			F32(rd) = F32(rs1) * F32(rs2) + F32(rs3);
			break;
		}
		case 0b01: //[FMADD.D]
		{
			F64(rd) = F64(rs1) * F64(rs2) + F64(rs3);
			break;
		}
		default:
			break;
		}
		break;
	}
	case 0b1000111: //[R]
	{
		int funct2 = GET(ins, 25, 2);
		reg_t rd = GET(ins, 7, 5);
		reg_t rs1 = GET(ins, 15, 5);
		reg_t rs2 = GET(ins, 20, 5);
		reg_t rs3 = GET(ins, 27, 5);

		switch (funct2)
		{
		case 0b00: //[FMSUB.S]
		{
			F32(rd) = F32(rs1) * F32(rs2) - F32(rs3);
			break;
		}
		case 0b01: //[FMSUB.D]
		{
			F64(rd) = F64(rs1) * F64(rs2) - F64(rs3);
			break;
		}
		default:
			break;
		}
		break;
	}
	case 0b1001011: //[R]
	{
		int funct2 = GET(ins, 25, 2);
		reg_t rd = GET(ins, 7, 5);
		reg_t rs1 = GET(ins, 15, 5);
		reg_t rs2 = GET(ins, 20, 5);
		reg_t rs3 = GET(ins, 27, 5);
		switch (funct2)
		{
		case 0b00: //[FNMSUB.S]
		{
			F32(rd) = -F32(rs1) * F32(rs2) + F32(rs3);
			break;
		}
		case 0b01: //[FNMSUB.D]
		{
			F64(rd) = -F64(rs1) * F64(rs2) + F64(rs3);
			break;
		}
		default:
			break;
		}
		break;
	}
	case 0b1001111: //[R]
	{
		int funct2 = GET(ins, 25, 2);
		reg_t rd = GET(ins, 7, 5);
		reg_t rs1 = GET(ins, 15, 5);
		reg_t rs2 = GET(ins, 20, 5);
		reg_t rs3 = GET(ins, 27, 5);
		switch (funct2)
		{
		case 0b00: //[FNMADD.S]
		{
			F32(rd) = -F32(rs1) * F32(rs2) - F32(rs3);
			break;
		}
		case 0b01: //[FNMADD.D]
		{
			F64(rd) = -F64(rs1) * F64(rs2) - F64(rs3);
			break;
		}
		default:
			break;
		}
		break;
	}
	case 0b1010011: //[R]
	{
		int funct7 = GET(ins, 25, 7);
		int funct5 = GET(ins, 20, 5);
		int funct3 = GET(ins, 12, 3);
		reg_t rd = GET(ins, 7, 5);
		reg_t rs1 = GET(ins, 15, 5);
		reg_t rs2 = GET(ins, 20, 5);
		switch (funct7)
		{
#ifdef RV32F
		case 0b0000000: //[FADD.S]
		{
			F32(rd) = F32(rs1) + F32(rs2);
			break;
		}
		case 0b0000100: //[FSUB.S]
		{
			F32(rd) = F32(rs1) - F32(rs2);
			break;
		}
		case 0b0001000: //[FMUL.S]
		{
			F32(rd) = F32(rs1) * F32(rs2);
			break;
		}
		case 0b0001100: //[FDIV.S]
		{
			F32(rd) = F32(rs1) / F32(rs2);
			break;
		}
		case 0b0101100: //[FSQRT.S]
		{
			F32(rd) = sqrtf(F32(rs1));
			break;
		}
		case 0b0010000:
		{
			switch (funct3)
			{
			case 0b000: //[FSGNJ.S]
			{
				F32U(rd) = GET_FIXED(F32U(rs2), 31, 1) | GET_FIXED(F32U(rs1), 0, 31);
				break;
			}
			case 0b001: //[FSGNJN.S]
			{
				F32U(rd) = (GET_FIXED(F32U(rs2), 31, 1) ^ (1U << 31)) | GET_FIXED(F32U(rs1), 0, 31);
				break;
			}
			case 0b010: //[FSGNJX.S]
			{
				F32U(rd) = (GET_FIXED(F32U(rs1), 31, 1) ^ GET_FIXED(F32U(rs2), 31, 1)) | GET_FIXED(F32U(rs1), 0, 31);
				break;
			}

			default:
				break;
			}
			break;
		}
		case 0b0010100:
		{
			switch (funct3)
			{
			case 0b000: //[FMIN.S]
			{
				F32(rd) = fminf(F32(rs1), F32(rs2));
				break;
			}
			case 0b001: //[FMAX.S]
			{
				F32(rd) = fmaxf(F32(rs1), F32(rs2));
				break;
			}

			default:
				break;
			}
			break;
		}
		case 0b1100000:
		{
			switch (funct5)
			{
			case 0b00000: //[FCVT.W.S]
			{
				X(rd) = S32(F32(rs1));
				break;
			}
			case 0b00001: //[FCVT.WU.S]
			{
				X(rd) = S32(U32(F32(rs1)));
				break;
			}
#ifdef RV64F
			case 0b00010: //[FCVT.L.S]
			{
				X(rd) = S64(F32(rs1));
				break;
			}
			case 0b00011: //[FCVT.LU.S]
			{
				X(rd) = U64(F32(rs1));
				break;
			}
#endif
			default:
				break;
			}
			break;
		}
		case 0b1110000:
		{
			switch (funct5)
			{
			case 0b00000: //[FMV.X.W]
			{
				X(rd) = S32(F32S(rs1));
				break;
			}
			case 0b00001: //[FCLASS.S]
			{
				X(rd) = U32(1 << classify_float(F32(rs1)));
				break;
			}

			default:
				break;
			}
			break;
		}
		case 0b1010000:
		{
			switch (funct3)
			{
			case 0b010: //[FEQ.S]
			{
				X(rd) = F32(rs1) == F32(rs2);
				break;
			}
			case 0b001: //[FLT.S]
			{
				X(rd) = F32(rs1) < F32(rs2);
				break;
			}
			case 0b000: //[FLE.S]
			{
				X(rd) = F32(rs1) <= F32(rs2);
				break;
			}

			default:
				break;
			}
			break;
		}
		case 0b1101000:
		{
			switch (funct5)
			{
			case 0b00000: //[FCVT.S.W]
			{
				F32(rd) = XS32(rs1);
				break;
			}
			case 0b00001: //[FCVT.S.WU]
			{
				F32(rd) = XU32(rs1);
				break;
			}
#ifdef RV64F
			case 0b00010: //[FCVT.S.L]
			{
				F32(rd) = XS64(rs1);
				break;
			}
			case 0b00011: //[FCVT.S.LU]
			{
				F32(rd) = XU64(x[rs1]);
				break;
			}
#endif
			default:
				break;
			}
			break;
		}
		case 0b1111000: //[FMV.W.X]
		{
			F32U(rd) = x[rs1];
			break;
		}
#endif
#ifdef RV32D
		case 0b0000001: //[FADD.D]
		{
			F64(rd) = F64(rs1) + F64(rs2);
			break;
		}
		case 0b0000101: //[FSUB.D]
		{
			F64(rd) = F64(rs1) - F64(rs2);
			break;
		}
		case 0b0001001: //[FMUL.D]
		{
			F64(rd) = F64(rs1) * F64(rs2);
			break;
		}
		case 0b0001101: //[FDIV.D]
		{
			F64(rd) = F64(rs1) / F64(rs2);
			break;
		}
		case 0b0101101: //[FSQRT.D]
		{
			F64(rd) = sqrt(F64(rs1));
			break;
		}
		case 0b0010001:
		{
			switch (funct3)
			{
			case 0b000: //[FSGNJ.D]
			{
				F64U(rd) = GET_FIXED(F64U(rs2), 63, 1) | GET_FIXED(F64U(rs1), 0, 63);
				break;
			}
			case 0b001: //[FSGNJN.D]
			{
				F64U(rd) = (GET_FIXED(F64U(rs2), 63, 1) ^ (1ULL << 63)) | GET_FIXED(F64U(rs1), 0, 63);
				break;
			}
			case 0b010: //[FSGNJX.D]
			{
				F64U(rd) = (GET_FIXED(F64U(rs1), 63, 1) ^ GET_FIXED(F64U(rs2), 63, 1)) | GET_FIXED(F64U(rs1), 0, 63);
				break;
			}

			default:
				break;
			}
			break;
		}
		case 0b0010101:
		{
			switch (funct3)
			{
			case 0b000: //[FMIN.D]
			{
				F64(rd) = fmin(F64(rs1), F64(rs2));
				break;
			}
			case 0b001: //[FMAX.D]
			{
				F64(rd) = fmax(F64(rs1), F64(rs2));
				break;
			}

			default:
				break;
			}
			break;
		}
		case 0b1100001:
		{
			switch (funct5)
			{
			case 0b00000: //[FCVT.W.D]
			{
				X(rd) = S32(F64(rs1));
				break;
			}
			case 0b00001: //[FCVT.WU.D]
			{
				X(rd) = S32(U32(F64(rs1)));
				break;
			}
#ifdef RV64D
			case 0b00010: //[FCVT.L.D]
			{
				X(rd) = S64(F64(rs1));
				break;
			}
			case 0b00011: //[FCVT.LU.D]
			{
				X(rd) = U64(F64(rs1));
				break;
			}
#endif
			default:
				break;
			}
			break;
		}
		case 0b0100000:
		{
			switch (funct5)
			{
			case 0b00001: //[FCVT.S.D]
			{
				F32(rd) = F64(rs1);
				break;
			}

			default:
				break;
			}
			break;
		}
		case 0b0100001:
		{
			switch (funct5)
			{
			case 0b00000: //[FCVT.D.S]
			{
				F64(rd) = F32(rs1);
				break;
			}

			default:
				break;
			}
			break;
		}
		case 0b1110001:
		{
			switch (funct5)
			{
			case 0b00000: //
			{
				int funt3 = GET(ins, 12, 3);
				switch (funt3)
				{
#ifdef RV64D
				case 0b000: //[FMV.X.D]
				{
					X(rd) = F64U(rs1);
					break;
				}
#endif
				case 0b001: //[FCLASS.D]
				{
					X(rd) = U32(1 << classify_double(F64(rs1)));
					break;
				}

				default:
					break;
				}

				break;
			}

			default:
				break;
			}
			break;
		}
		case 0b1010001:
		{
			switch (funct3)
			{
			case 0b010: //[FEQ.D]
			{
				X(rd) = F64(rs1) == F64(rs2);
				break;
			}
			case 0b001: //[FLT.D]
			{
				X(rd) = F64(rs1) < F64(rs2);
				break;
			}
			case 0b000: //[FLE.D]
			{
				X(rd) = F64(rs1) <= F64(rs2);
				break;
			}

			default:
				break;
			}
			break;
		}
		case 0b1101001:
		{
			switch (funct5)
			{
			case 0b00000: //[FCVT.D.W]
			{
				F64(rd) = XS32(rs1);
				break;
			}
			case 0b00001: //[FCVT.D.WU]
			{
				F64(rd) = S32(XU32(rs1));
				break;
			}

#ifdef RV64D
			case 0b00010: //[FCVT.D.L]
			{
				F64(rd) = XS64(rs1);
				break;
			}
			case 0b00011: //[FCVT.D.LU]
			{
				F64(rd) = XU64(rs1);
				break;
			}
#endif
			default:
				break;
			}
			break;
		}
#ifdef RV64D
		case 0b1111001: //[FMV.D.X]
		{
			F64U(rd) = x[rs1];
			break;
		}
#endif
#endif
		default:
			break;
		}
		break;
	}
#endif

#if defined(RV32A) || defined(RV64A)
#define AMO32(rd, rs1, op, rs2)          \
	s32_t t = M32(x[rs1]);               \
	WM32(x[rs1], M32(x[rs1]) op x[rs2]); \
	X(rd) = t
#define AMO64(rd, rs1, op, rs2)          \
	s64_t t = M64(x[rs1]);               \
	WM64(x[rs1], M64(x[rs1]) op x[rs2]); \
	X(rd) = t

	case 0b0101111: //[R]
	{
		int funct5 = GET(ins, 27, 5);
		int funct3 = GET(ins, 12, 3);

		reg_t rd = GET(ins, 7, 5);
		reg_t rs1 = GET(ins, 15, 5);
		reg_t rs2 = GET(ins, 20, 5);
#ifdef RV32A
		if (funct3 == 0b010)
		{
			switch (funct5)
			{
			case 0b00010: //[LR.W]
			{
				X(rd) = M32(x[rs1]);
				break;
			}
			case 0b00011: //[SC.W]
			{
				//如果存入成功，向寄存器 x[rd]中存入 0，否则存入一个非 0 的错误码
				WM32(x[rs1], x[rs2]);
				X(rd) = 0;
				break;
			}
			case 0b00001: //[AMOSWAP.W]
			{
				s32_t t = M32(x[rs1]);
				WM32(x[rs1], x[rs2]);
				X(rd) = t;
				break;
			}
			case 0b00000: //[AMOADD.W]
			{
				AMO32(rd, rs1, +, rs2);
				break;
			}
			case 0b00100: //[AMOXOR.W]
			{
				AMO32(rd, rs1, ^, rs2);
				break;
			}
			case 0b01100: //[AMOAND.W]
			{
				AMO32(rd, rs1, &, rs2);
				break;
			}
			case 0b01000: //[AMOOR.W]
			{
				AMO32(rd, rs1, |, rs2);
				break;
			}
			case 0b10000: //[AMOMIN.W]
			{
				s32_t t = M32(x[rs1]);
				WM32(x[rs1], MIN(S32(M32(x[rs1])), XS32(rs2)));
				X(rd) = t;
				break;
			}
			case 0b10100: //[AMOMAX.W]
			{
				s32_t t = M32(x[rs1]);
				WM32(x[rs1], MAX(S32(M32(x[rs1])), XS32(rs2)));
				X(rd) = t;
				break;
			}
			case 0b11000: //[AMOMINU.W]
			{
				s32_t t = M32(x[rs1]);
				WM32(x[rs1], MIN(U32(M32(x[rs1])), XU32(rs2)));
				X(rd) = t;
				break;
			}
			case 0b11100: //[AMOMAXU.W]
			{
				s32_t t = M32(x[rs1]);
				WM32(x[rs1], MAX(U32(M32(x[rs1])), XU32(rs2)));
				X(rd) = t;
				break;
			}
			default:
				break;
			}
			break;
		}
#endif
#ifdef RV64A
		if (funct3 == 0b011)
		{
			switch (funct5)
			{
			case 0b00010: //[LR.D]
			{
				X(rd) = M64(x[rs1]);
				break;
			}
			case 0b00011: //[SC.D]
			{
				WM64(x[rs1], x[rs2]);
				X(rd) = 0;
				break;
			}
			case 0b00001: //[AMOSWAP.D]
			{
				s64_t t = M64(x[rs1]);
				WM64(x[rs1], x[rs2]);
				X(rd) = t;
				break;
			}
			case 0b00000: //[AMOADD.D]
			{
				AMO64(rd, rs1, +, rs2);
				break;
			}
			case 0b00100: //[AMOXOR.D]
			{
				AMO64(rd, rs1, ^, rs2);
				break;
			}
			case 0b01100: //[AMOAND.D]
			{
				AMO64(rd, rs1, &, rs2);
				break;
			}
			case 0b01000: //[AMOOR.D]
			{
				AMO64(rd, rs1, |, rs2);
				break;
			}
			case 0b10000: //[AMOMIN.D]
			{
				s64_t t = M64(x[rs1]);
				WM64(x[rs1], MIN(S64(M64(x[rs1])), XS64(rs2)));
				X(rd) = t;
				break;
			}
			case 0b10100: //[AMOMAX.D]
			{
				s64_t t = M64(x[rs1]);
				WM64(x[rs1], MAX(S64(M64(x[rs1])), XS64(rs2)));
				X(rd) = t;
				break;
			}
			case 0b11000: //[AMOMINU.D]
			{
				s64_t t = M64(x[rs1]);
				WM64(x[rs1], MIN(U64(M64(x[rs1])), XU64(rs2)));
				X(rd) = t;
				break;
			}
			case 0b11100: //[AMOMAXU.D]
			{
				s64_t t = M64(x[rs1]);
				WM64(x[rs1], MAX(U64(M64(x[rs1])), XU64(rs2)));
				X(rd) = t;
				break;
			}
			default:
				break;
			}
			break;
		}
#endif
	}
#endif
	default:
		LOGI("非法指令 pc:%016llx ins:%08X op:%08x\n", pc, ins, OPCODE(ins));
		exit(1);
	}
	pc += 4;
}

static void RVC(u16_t ins)
{
	switch (GET(ins, 0, 2))
	{
		CASE(0b00,
			u32_t func3 = GET(ins, 13, 3);
			switch (func3){
				CASE(0b000, )
				CASE(0b001, )
				CASE(0b010, )
				CASE(0b011, )
				CASE(0b100, )
				CASE(0b101, )
				CASE(0b110, )
				CASE(0b111, )
			}
		)
		CASE(
			0b01,
			u32_t func3 = GET(ins, 13, 3);
			switch (func3) {
				CASE(
					0b000,
					imm_t imm = S32(
						GET_C_EXT((ins), 32 - 8) |
						GET_FIXED_SHIFT((ins), 2, 5, 0));
					reg_t rd = GET(ins, 7, 5);

					if (rd == 0) {
						// [C.NOP]
					} else { // [C.ADDI]
						X(rd) = x[rd] + imm;
					})
				CASE(0b001, )
				CASE(0b010, )
				CASE(0b011, )
				CASE(0b100, )
				CASE(0b101, )
				CASE(
					0b110,
					imm_t offset = S32(
						GET_C_EXT((ins), 32 - 8) |
						GET_FIXED_SHIFT((ins), 10, 2, 5) |
						GET_FIXED_SHIFT((ins), 5, 2, 6) |
						GET_FIXED_SHIFT((ins), 3, 2, 1) |
						GET_FIXED_SHIFT((ins), 2, 1, 5));
					reg_t rs1 = GET(ins, 7, 3) + 8;
					if (x[rs1] == 0) {
						pc += offset;
						return;
					}) // [C.BEQZ]
				CASE(
					0b111,

					imm_t offset = S32(
						GET_C_EXT((ins), 32 - 8) |
						GET_FIXED_SHIFT((ins), 10, 2, 3) |
						GET_FIXED_SHIFT((ins), 5, 2, 6) |
						GET_FIXED_SHIFT((ins), 3, 2, 1) |
						GET_FIXED_SHIFT((ins), 2, 1, 5));
					reg_t rs1 = GET(ins, 7, 3) + 8;
					// LOGI("[C.BNEZ] x%d %08x\n",rs1,offset);
					if (x[rs1] != 0) {
						pc += offset;
						return;
					}) // [C.BNEZ]
				DEFAULT()
			})
		CASE(
			0b10,
			u32_t func3 = GET(ins, 13, 3);
			switch (func3) {
				CASE(0b000, )
				CASE(0b001, )
				CASE(0b010, )
				CASE(0b011, )
				CASE(
					0b100,
					int func1 = GET(ins, 12, 1);
					reg_t rd = GET(ins, 7, 5);
					reg_t rs1 = rd;
					reg_t rs2 = GET(ins, 2, 5);
					if (func1) {
						if (rs2 != 0 && rd != 0) // [C.ADD]
						{
							X(rd) = x[rd] + x[rs2];
						}
					} else {
						if (rs2 != 0 && rd != 0) // [C.MV]
						{
							X(rd) = x[rs2];
						}
						else if (rs1 != 0 && rs2 == 0) // [C.JR]
						{
							pc = x[rs1];
						}
					}

				)
				CASE(0b101, )
				CASE(0b110, )
				CASE(0b111, )
				DEFAULT()
			})
		DEFAULT()
	}
	pc += 2;
}

void eval()
{
	for (;;)
	{
		u32_t ins = M32(pc);
		if (GET(ins, 0, 2) == 0b11)
		{
			LOGD("PC=%016llx INS=%08X\n", pc, ins);
			RVG(ins);
		}
		else
		{
			LOGD("Compressed PC=%016llx INS=%04X\n", pc, ins);
			RVC(ins);
		}
		check_exception();
	}
}

static void read_file(const char *filename)
{
	struct stat st;
	int fd;
	void *map;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
	{
		fprintf(stderr, "error opening file: ");
		perror(filename);
		exit(2);
	}
	fstat(fd, &st);
	if (st.st_size == 0)
	{
		close(fd);
		return;
	}

	map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if ((long)map == -1)
	{
		perror("mmap");
		close(fd);
		return;
	}
	memcpy(memory, map, st.st_size);

	munmap(map, st.st_size);
	close(fd);
}

static void *timer_function(void *ptr)
{
	struct timespec time1 = {0, 0};
	struct timespec time2 = {0, 0};
	clock_gettime(CLOCK_MONOTONIC, &time1);
	while (1)
	{
		usleep(1);
		clock_gettime(CLOCK_MONOTONIC, &time2);
		u64_t time = (time2.tv_sec - time1.tv_sec) * 1000000 + (time2.tv_nsec - time1.tv_nsec) / 1000;
		WM64(0x02000000U + 0xbff8U, 10 * time);
	}
}

#ifdef PROFILE
#include <gperftools/profiler.h>
#endif
int main(int argc, const char **argv)
{
	if (argc != 2)
		return 1;
#ifdef PROFILE
	ProfilerStart("test.prof");//开启性能分析
	atexit(ProfilerStop);
#endif
	setbuf(stdout, NULL);
	// enableRawMode(STDIN_FILENO);
	pthread_t thread1,thread2;

	pthread_create(&thread1, NULL, timer_function, NULL);
	pthread_create(&thread2, NULL, framebuffer_function, NULL);

	read_file(argv[1]);
	eval();
	pthread_join(thread2, NULL);
	pthread_join(thread1, NULL);
	return 0;
}