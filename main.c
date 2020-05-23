#include <stdio.h>
#include <math.h>
#include <float.h>

#define PSEUDO_INSTRUCTIONS

#define RV32M
#define RV32F
#define RV32D
#define RV32A

typedef unsigned long long u64_t;
typedef long long s64_t;
typedef unsigned int u32_t;
typedef unsigned short u16_t;
typedef unsigned char u8_t;
typedef signed long long s64_t;
typedef signed int s32_t;
typedef signed short s16_t;
typedef signed char s8_t;
typedef unsigned int register_t;

u32_t text[1024], // text segment
	stack[1024];  // stack

u8_t memory[1024]; // data segment
u32_t *CSRs;
u32_t pc = 0, x[33]; // virtual machine registers

union fr {
	double d;
	float f;
	u32_t u32;
	u64_t u64;
	s32_t s32;
	s64_t s64;
};

union fr f[33];

#define M(x) (*(u32_t *)(memory + (x)))
#define M64(x) (*(u64_t *)(memory + (x)))
#define M32(x) (*(u32_t *)(memory + (x)))
#define M16(x) (*(u16_t *)(memory + (x)))
#define M8(x) (*(u8_t *)(memory + (x)))

#define X(r) (x[((r - 1) & 0b11111) + 1]) // for write

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

// instructions
enum RV32I
{
	LUI,
	AUIPC,
	JAL,
	JALR,
	BEQ,
	BNE,
	BLT,
	BGE,
	BLTU,
	BGEU,
	LB,
	LH,
	LW,
	LBU,
	LHU,
	SB,
	SH,
	SW,
	ADDI,
	SLTI,
	SLTIU,
	XORI,
	ORI,
	ANDI,
	SLLI,
	SRLI,
	SRAI,
	ADD,
	SUB,
	SLL,
	SLT,
	SLTU,
	XOR,
	SRL,
	SRA,
	OR,
	AND,
	FENCE,
	FENCE_I,
	ECALL,
	EBREAK,
	CSRRW,
	CSRRS,
	CSRRC,
	CSRRWI,
	CSSRRSI,
	CSRRCI,
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

#define GET(x, offset, len) (((x) >> (offset)) & ((1 << (len)) - 1))
#define GET_FIXED(x, offset, len) ((x) & (((1 << (len)) - 1) << (offset)))
#define GET_FIXED_L(x, offset, len, offset2) (GET_FIXED((x), (offset), (len)) << ((offset2) - (offset)))
#define GET_FIXED_R(x, offset, len, offset2) (GET_FIXED((x), (offset), (len)) >> ((offset) - (offset2)))

#define OPCODE(ins) GET((ins), 0, 7)

#define UNSIGNED(x) ((u32_t)(x))
#define SIGNED(x) ((s32_t)(x))

#define GET_EXT(ins, width) UNSIGNED(SIGNED(GET_FIXED((ins), 31, 1)) >> ((width)-1))

#define GET_I_IMM(ins) (GET_EXT((ins), 21) | GET_FIXED_R((ins), 25, 6, 5) | GET_FIXED_R((ins), 21, 4, 1) | GET_FIXED_R((ins), 20, 1, 0))
#define GET_S_IMM(ins) (GET_EXT((ins), 21) | GET_FIXED_R((ins), 25, 6, 5) | GET_FIXED_R((ins), 8, 4, 1) | GET_FIXED_R((ins), 7, 1, 0))
#define GET_B_IMM(ins) (GET_EXT((ins), 20) | GET_FIXED_L((ins), 7, 1, 11) | GET_FIXED_R((ins), 25, 6, 5) | GET_FIXED_R((ins), 8, 4, 1))
#define GET_U_IMM(ins) (GET_FIXED((ins), 12, 20))
#define GET_J_IMM(ins) (GET_EXT(ins, 12) | GET_FIXED((ins), 12, 8) | GET_FIXED_R((ins), 20, 11, 11) | GET_FIXED_R((ins), 25, 6, 5) | GET_FIXED_R((ins), 21, 4, 1))

#define GET_I_INS(imm) (GET_FIXED_L((imm), 0, 12, 20))
#define GET_S_INS(imm) (GET_FIXED_L((imm), 5, 7, 25) | GET_FIXED_L((imm), 0, 5, 7))
#define GET_B_INS(imm) (GET_FIXED_L((imm), 12, 1, 31) | GET_FIXED_L((imm), 5, 6, 25) | GET_FIXED_L((imm), 1, 4, 8) | GET_FIXED_R((imm), 11, 1, 7))
#define GET_U_INS(imm) (GET_FIXED((imm), 12, 20))
#define GET_J_INS(imm) (GET_FIXED_L((imm), 20, 1, 31) | GET_FIXED_L((imm), 1, 10, 21) | GET_FIXED_L((imm), 11, 1, 20) | GET_FIXED((imm), 12, 8))

#define INS_R(opcode, funct3, funct7, rd, rs1, rs2) (GET((opcode), 0, 7) | GET_FIXED_L((funct3), 0, 3, 12) | GET_FIXED_L((funct7), 0, 7, 25) | GET_FIXED_L((rd), 0, 5, 7) | GET_FIXED_L((rs1), 0, 5, 15) | GET_FIXED_L((rs2), 0, 5, 20))
#define INS_I(opcode, funct3, rd, rs1, imm) (GET((opcode), 0, 7) | GET_FIXED_L((funct3), 0, 3, 12) | GET_FIXED_L((rd), 0, 5, 7) | GET_FIXED_L((rs1), 0, 5, 15) | GET_I_INS((imm)))
#define INS_S(opcode, funct3, rs1, rs2, imm) (GET((opcode), 0, 7) | GET_FIXED_L((funct3), 0, 3, 12) | GET_FIXED_L((rs1), 0, 5, 15) | GET_FIXED_L((rs2), 0, 5, 20) | GET_S_INS((imm)))
#define INS_B(opcode, funct3, rs1, rs2, imm) (GET((opcode), 0, 7) | GET_FIXED_L((funct3), 0, 3, 12) | GET_FIXED_L((rs1), 0, 5, 15) | GET_FIXED_L((rs2), 0, 5, 20) | GET_B_INS((imm)))
#define INS_U(opcode, rd, imm) (GET((opcode), 0, 7) | GET_FIXED_L((rd), 0, 5, 7) | GET_U_INS((imm)))
#define INS_J(opcode, rd, imm) (GET((opcode), 0, 7) | GET_FIXED_L((rd), 0, 5, 7) | GET_J_INS((imm)))

#define DEBUG(rd) INS_R(1, 0, 0, rd, 0, 0)

#define LUI(rd, imm) INS_U(0b0110111, rd, imm)
#define AUIPC(rd, imm) INS_U(0b0010111, rd, imm)
#define JAL(rd, offset) INS_J(0b1101111, rd, offset)
#define JALR(rd, rs1, offset) INS_I(0b1100111, 0b000, rd, rs1, offset)
#define BEQ(rs1, rs2, offset) INS_B(0b1100011, 0b000, rs1, rs2, offset)
#define BNE(rs1, rs2, offset) INS_B(0b1100011, 0b001, rs1, rs2, offset)
#define BLT(rs1, rs2, offset) INS_B(0b1100011, 0b100, rs1, rs2, offset)
#define BGE(rs1, rs2, offset) INS_B(0b1100011, 0b101, rs1, rs2, offset)
#define BLTU(rs1, rs2, offset) INS_B(0b1100011, 0b110, rs1, rs2, offset)
#define BGEU(rs1, rs2, offset) INS_B(0b1100011, 0b111, rs1, rs2, offset)
#define LB(rd, rs1, imm) INS_I(0b0000011, 0b000, rd, rs1, imm)
#define LH(rd, rs1, imm) INS_I(0b0000011, 0b001, rd, rs1, imm)
#define LW(rd, rs1, imm) INS_I(0b0000011, 0b010, rd, rs1, imm)
#define LBU(rd, rs1, imm) INS_I(0b0000011, 0b100, rd, rs1, imm)
#define LHU(rd, rs1, imm) INS_I(0b0000011, 0b101, rd, rs1, imm)
#define SB(rs2, rs1, imm) INS_S(0b0100011, 0b000, rs1, rs2, imm)
#define SH(rs2, rs1, imm) INS_S(0b0100011, 0b001, rs1, rs2, imm)
#define SW(rs2, rs1, imm) INS_S(0b0100011, 0b010, rs1, rs2, imm)
#define ADDI(rd, rs1, imm) INS_I(0b0010011, 0b000, rd, rs1, imm)
#define SLTI(rd, rs1, imm) INS_I(0b0010011, 0b010, rd, rs1, imm)
#define SLTIU(rd, rs1, imm) INS_I(0b0010011, 0b011, rd, rs1, imm)
#define XORI(rd, rs1, imm) INS_I(0b0010011, 0b100, rd, rs1, imm)
#define ORI(rd, rs1, imm) INS_I(0b0010011, 0b110, rd, rs1, imm)
#define ANDI(rd, rs1, rs2) INS_R(0b0010011, 0b111, 0b0000000, rd, rs1, imm)
#define SLLI(rd, rs1, shamt) INS_R(0b0010011, 0b001, 0b0000000, rd, rs1, GET_FIXED_L(0b0000000, 0, 7, 5) | GET_FIXED(shamt, 0, 5))
#define SRLI(rd, rs1, shamt) INS_R(0b0010011, 0b101, 0b0000000, rd, rs1, GET_FIXED_L(0b0000000, 0, 7, 5) | GET_FIXED(shamt, 0, 5))
#define SRAI(rd, rs1, shamt) INS_R(0b0010011, 0b101, 0b0100000, rd, rs1, GET_FIXED_L(0b0000000, 0, 7, 5) | GET_FIXED(shamt, 0, 5))
#define ADD(rd, rs1, rs2) INS_R(0b0110011, 0b000, 0b0000000, rd, rs1, rs2)
#define SUB(rd, rs1, rs2) INS_R(0b0110011, 0b000, 0b0000000, rd, rs1, rs2)
#define SLL(rd, rs1, rs2) INS_R(0b0110011, 0b001, 0b0000000, rd, rs1, rs2)
#define SLT(rd, rs1, rs2) INS_R(0b0110011, 0b010, 0b0000000, rd, rs1, rs2)
#define SLTU(rd, rs1, rs2) INS_R(0b0110011, 0b011, 0b0000000, rd, rs1, rs2)
#define XOR(rd, rs1, rs2) INS_R(0b0110011, 0b100, 0b0000000, rd, rs1, rs2)
#define SRL(rd, rs1, rs2) INS_R(0b0110011, 0b101, 0b0000000, rd, rs1, rs2)
#define SRA(rd, rs1, rs2) INS_R(0b0110011, 0b101, 0b0100000, rd, rs1, rs2)
#define OR(rd, rs1, rs2) INS_R(0b0110011, 0b110, 0b0000000, rd, rs1, rs2)
#define AND(rd, rs1, rs2) INS_R(0b0110011, 0b111, 0b0000000, rd, rs1, rs2)
#define FENCE(rd, rs1, imm) INS_I(0b0001111, 0b000, rd, rs1, imm)
#define FENCE_I(rd, rs1, imm) INS_I(0b0001111, 0b001, rd, rs1, imm)
#define ECALL(rd, rs1, imm) INS_I(0b1110011, 0b000, rd, rs1, imm)
#define EBREAK(rd, rs1, imm) INS_I(0b1110011, 0b000, rd, rs1, imm)
#define CSRRW(rd, rs1, imm) INS_I(0b1110011, 0b001, rd, rs1, imm)
#define CSRRS(rd, rs1, imm) INS_I(0b1110011, 0b010, rd, rs1, imm)
#define CSRRC(rd, rs1, imm) INS_I(0b1110011, 0b011, rd, rs1, imm)
#define CSRRWI(rd, rs1, imm) INS_I(0b1110011, 0b101, rd, rs1, imm)
#define CSSRRSI(rd, rs1, imm) INS_I(0b1110011, 0b110, rd, rs1, imm)
#define CSRRCI(rd, rs1, imm) INS_I(0b1110011, 0b111, rd, rs1, imm)

#ifdef PSEUDO_INSTRUCTIONS
#define NOP() ADDI(0, 0, 0)			 //No operation
#define LI(rd, imm) ADDI(rd, 0, imm) //Load immediate for RV32I imm[11:0]
// #define LI(rd, imm) LUI(0, imm)      //Load immediate for RV32I imm[31:12]
#define MV(rd, rs) ADDI(rd, rs, 0)	 //Copy register
#define NOT(rd, rs) XORI(rd, rs, -1) //One’s complement
#define NEG(rd, rs) SUB(rd, x0, rs)	 //Two’s complement

#define BEQZ(rs, offset) BEQ(rs, x0, offset)	  //Branch if = zero
#define BNEQZ(rs, offset) BNEQ(rs, x0, offset)	  //Branch if ̸= zero
#define BLEZ(rs, offset) BLE(x0, rs, offset)	  //Branch if ≤ zero
#define BGEZ(rs, offset) BGE(rs, x0, offset)	  //Branch if ≥ zero
#define BLTZ(rs, offset) BLT(rs, x0, offset)	  //Branch if < zero
#define BGTZ(rs, offset) BGT(x0, rs, offset)	  //Branch if > zero
#define BGT(rs, rt, offset) BLT(rt, rs, offset)	  //Branch if >
#define BLE(rs, rt, offset) BGE(rt, rs, offset)	  //Branch if ≤
#define BGTU(rs, rt, offset) BLTU(rt, rs, offset) //Branch if >, unsigned
#define BLEU(rs, rt, offset) BGEU(rt, rs, offset) //Branch if ≤, unsigned

#define J(offset) JAL(0, offset) //Jump
// #define JAL(offset) JAL(1, offset) //Jump and link
#define JR(rs) JALR(0, rs, 0) //Jump register
// #define JALR(rs) JALR(1, rs, 0)	   //Jump and link register
#define RET() JALR(0, 1, 0) // Return from subroutine

#define CSRR(rd, csr) CSRRS(rd, csr, x0)	 //Read CSR
#define CSRW(csr, rs) CSRRW(x0, csr, rs)	 // Write CSR
#define CSRS(csr, rs) CSRRS(x0, csr, rs)	 // Set bits in CSR
#define CSRC(csr, rs) CSRRC(x0, csr, rs)	 //Clear bits in CSR
#define CSRWI(csr, imm) CSRRWI(x0, csr, imm) // Write CSR, immediate
#define CSRSI(csr, imm) CSRRSI(x0, csr, imm) // Set bits in CSR, immediate
#define CSRCI(csr, imm) CSRRCI(x0, csr, imm) // Clear bits in CSR, immediate

#endif

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
		printf("[rs1]为−∞");
		break;
	case 1:
		printf("[rs1]是负规格化数");
		break;
	case 2:
		printf("[rs1]是负的非规格化数");
		break;
	case 3:
		printf("[rs1]是-0");
		break;
	case 4:
		printf("[rs1]是+0");
		break;
	case 5:
		printf("[rs1]是正的非规格化数");
		break;
	case 6:
		printf("[rs1]是正的规格化数");
		break;
	case 7:
		printf("[rs1]为+∞");
		break;
	case 8:
		printf("[rs1]是信号(signaling)NaN");
		break;
	case 9:
		printf("[rs1]是一个安静(quiet)NaN");
		break;

	default:
		break;
	}
	printf("\n");
}

void eval()
{
	for (;;)
	{
		// printf("PC=%08x\n", pc);
		u32_t ins = text[pc >> 2];
		pc += 4;
		switch (OPCODE(ins))
		{
		case 0b0110111: //U lui
		{
			register_t rd = GET(ins, 7, 5);
			X(rd) = GET_U_IMM(ins);
			break;
		}
		case 0b0010111: //U auipc
		{
			register_t rd = GET(ins, 7, 5);
			x[GET(ins, 7, 5)] = pc + GET_U_IMM(ins);
			break;
		}
		case 0b1101111: //J jal
		{
			register_t rd = GET(ins, 7, 5);
			X(rd) = pc + 4;
			pc += GET_J_IMM(ins);
			// printf("jal %08X X[0]=%d\n", pc,x[0]);
			break;
		}
		case 0b1100111: //I jalr
		{
			register_t rd = GET(ins, 7, 5);
			register_t rs1 = GET(ins, 15, 5);
			u32_t t = pc + 4;
			pc = (x[rs1] + GET_I_IMM(ins)) & ~1;
			X(rd) = t;
			break;
		}
		case 0b1100011: //B
		{
			int funct3 = GET(ins, 12, 3);
			register_t rs1 = GET(ins, 15, 5);
			register_t rs2 = GET(ins, 20, 5);
			u32_t offset = GET_B_IMM(ins);
			switch (funct3)
			{
			case 0b000: //beq
			{
				if (x[rs1] == x[rs2])
					pc += offset;
				break;
			}
			case 0b001: //bne
			{
				// printf("bne offset=%08X, x[%d]=%d, x[%d]=%d\n", offset, rs1,(x[rs1]), rs2,(x[rs2]));
				if (x[rs1] != x[rs2])
					pc += offset;
				break;
			}
			case 0b100: //blt
			{
				if (SIGNED(x[rs1]) < SIGNED(x[rs2]))
					pc += offset;
				break;
			}
			case 0b101: //bge
			{
				// printf("bge offset=%08X, x[%d]=%d, x[%d]=%d\n", offset, rs1,SIGNED(x[rs1]), rs2,SIGNED(x[rs2]));
				if (SIGNED(x[rs1]) >= SIGNED(x[rs2]))
					pc += offset;
				break;
			}
			case 0b110: //bltu
			{
				// printf("bltu offset=%08X, x[%d]=%u x[%d]=%u\n", offset, rs1,UNSIGNED(x[rs1]), rs2,UNSIGNED(x[rs2]));
				if (UNSIGNED(x[rs1]) < UNSIGNED(x[rs2]))
					pc += offset;
				break;
			}
			case 0b111: //bgeu
			{
				if (UNSIGNED(x[rs1]) >= UNSIGNED(x[rs2]))
					pc += offset;
				break;
			}
			default:
				break;
			}
			break;
		}
		case 0b0000011: //I
		{
			int funct3 = GET(ins, 12, 3);
			register_t rs1 = GET(ins, 15, 5);
			u32_t offset = GET_I_IMM(ins);
			register_t rd = GET(ins, 7, 5);
			switch (funct3)
			{
			case 0b000: //lb
			{
				X(rd) = (s8_t)(M(x[rs1] + offset));
				break;
			}
			case 0b001: //lh
			{
				X(rd) = (s16_t)(M(x[rs1] + offset));
				break;
			}
			case 0b010: //lw
			{
				// printf("ins = %08X x%d = [x%d(%u) + (%d)](%d)\n", ins,rd, rs1, x[rs1], offset,(s32_t)(M(x[rs1] + offset)));
				X(rd) = (s32_t)(M(x[rs1] + offset));
				break;
			}
			case 0b100: //lbu
			{
				X(rd) = (u8_t)(M(x[rs1] + offset));
				break;
			}
			case 0b101: //lhu
			{
				X(rd) = (u16_t)(M(x[rs1] + offset));
				break;
			}
			default:
				break;
			}
			break;
		}
		case 0b0100011: //S
		{
			int funct3 = GET(ins, 12, 3);
			register_t rs1 = GET(ins, 15, 5);
			register_t rs2 = GET(ins, 20, 5);
			u32_t offset = GET_S_IMM(ins);
			switch (funct3)
			{
			case 0b000: //sb
			{
				M8(x[rs1] + offset) = (u8_t)(x[rs2]);
				break;
			}
			case 0b001: //sh
			{
				M16(x[rs1] + offset) = (u16_t)(x[rs2]);
				break;
			}
			case 0b010: //sw
			{
				// printf("a2=%X j = %d [x%d(%u) + (%d)] = x%d(%u)\n",x[a2],x[a5] ,rs1, x[rs1], offset, rs2, x[rs2]);
				M32(x[rs1] + offset) = (u32_t)(x[rs2]);
				break;
			}
			default:
				break;
			}
			break;
		}
		case 0b0010011: //I
		{
			int funct3 = GET(ins, 12, 3);
			register_t rs1 = GET(ins, 15, 5);
			u32_t imm = GET_I_IMM(ins);
			register_t rd = GET(ins, 7, 5);
			switch (funct3)
			{
			case 0b000: //addi
			{
				X(rd) = x[rs1] + imm;
				break;
			}
			case 0b010: //slti
			{
				X(rd) = (SIGNED(x[rs1]) < SIGNED(imm));
				break;
			}
			case 0b011: //sltiu
			{
				X(rd) = (UNSIGNED(x[rs1]) < UNSIGNED(imm));
				break;
			}
			case 0b100: //xori
			{
				X(rd) = (x[rs1] ^ imm);
				break;
			}
			case 0b110: //ori
			{
				X(rd) = (x[rs1] | imm);
				break;
			}
			case 0b111: //andi
			{
				X(rd) = (x[rs1] & imm);
				break;
			}
			case 0b001: //slli (Shift Left Logical Immediate)
			{
				int shamt = GET(ins, 20, 5);
				X(rd) = (UNSIGNED(x[rs1]) << imm);
				break;
			}
			case 0b101:
			{
				int funct7 = GET(ins, 25, 7);
				int shamt = GET(ins, 20, 5);
				if (funct7 == 0b0000000) //srli (Shift Right Logical Immediate)
				{
					X(rd) = (UNSIGNED(x[rs1]) >> shamt);
				}
				else if (funct7 == 0b0100000) //srai (Shift Right Arithmetic Immediate)
				{
					X(rd) = (SIGNED(x[rs1]) >> shamt);
				}
				break;
			}
			default:
				break;
			}
			break;
		}
		case 0b0110011: //R
		{
			int funct3 = GET(ins, 12, 3);
			int funct7 = GET(ins, 25, 7);

			register_t rs1 = GET(ins, 15, 5);
			register_t rs2 = GET(ins, 20, 5);
			register_t rd = GET(ins, 7, 5);
			switch (funct3)
			{
			case 0b000:
			{
				if (funct7 == 0b0000000) //add
				{
					X(rd) = x[rs1] + x[rs2];
				}
				else if (funct7 == 0b0100000) //sub
				{
				}
				break;
			}
			case 0b001: //sll
			{
				X(rd) = UNSIGNED(x[rs1]) << x[rs2];
				break;
			}
			case 0b010: //slt
			{
				X(rd) = (SIGNED(x[rs1]) < SIGNED(x[rs2]));
				break;
			}
			case 0b011: //sltu
			{
				X(rd) = (UNSIGNED(x[rs1]) < UNSIGNED(x[rs2]));
				break;
			}
			case 0b100: //xor
			{
				X(rd) = x[rs1] ^ x[rs2];
				break;
			}
			case 0b101:
			{
				if (funct7 == 0b0000000) //srl
				{
					X(rd) = (UNSIGNED(x[rs1]) >> x[rs2]);
				}
				else if (funct7 == 0b0100000) //sra (Shift Right Arithmetic)
				{
					X(rd) = (SIGNED(x[rs1]) >> x[rs2]);
				}
				break;
			}
			case 0b110: //or
			{
				X(rd) = x[rs1] | x[rs2];
				break;
			}
			case 0b111: //and
			{
				X(rd) = x[rs1] & x[rs2];
				break;
			}

			default:
				break;
			}

#ifdef RV32M

			if (funct7 == 0b000001)
			{
				register_t rs1 = GET(ins, 15, 5);
				register_t rs2 = GET(ins, 20, 5);
				register_t rd = GET(ins, 7, 5);
				switch (funct3)
				{
				case 0b000: //mul
				{
					X(rd) = x[rs1] * x[rs2];
					break;
				}
				case 0b001: //mulh
				{
					s64_t t = SIGNED(x[rs1]) * SIGNED(x[rs2]);
					X(rd) = t >> 32;
					break;
				}
				case 0b010: //mulhsu
				{
					u64_t t = SIGNED(x[rs1]) * UNSIGNED(x[rs2]);
					X(rd) = t >> 32;
					break;
				}
				case 0b011: //mulhu
				{
					u64_t t = UNSIGNED(x[rs1]) * UNSIGNED(x[rs2]);
					X(rd) = t >> 32;
					break;
				}
				case 0b100: //div
				{
					X(rd) = SIGNED(x[rs1]) / SIGNED(x[rs2]);
					break;
				}
				case 0b101: //divu
				{
					X(rd) = UNSIGNED(x[rs1]) / UNSIGNED(x[rs2]);
					break;
				}
				case 0b110: //rem
				{
					X(rd) = SIGNED(x[rs1]) % SIGNED(x[rs2]);
					break;
				}
				case 0b111: //remu
				{
					X(rd) = UNSIGNED(x[rs1]) % UNSIGNED(x[rs2]);
					break;
				}
				default:
					break;
				}
			}
#endif
			break;
		}
		case 0b0001111: //I
		{
			int funct3 = GET(ins, 12, 3);
			switch (funct3)
			{
			case 0b000: //fence
			{
				break;
			}
			case 0b001: //fence.i
			{
				break;
			}
			default:
				break;
			}
			break;
		}
		case 0b1110011: //I
		{
			int funct3 = GET(ins, 12, 3);
			register_t rs1 = GET(ins, 15, 5);
			register_t rs2 = GET(ins, 20, 5);
			u32_t offset = GET_I_IMM(ins);
			register_t rd = GET(ins, 7, 5);
			u32_t csr = GET(ins, 20, 12);
			switch (funct3)
			{
			case 0b000:
			{
				int funct7 = GET(ins, 25, 7);
				if (funct7 == 0b0000000) //ecall (Environment Call)
				{
				}
				else if (funct7 == 0b0100000) //ebreak (Environment Breakpoint)
				{
				}
				break;
			}
			case 0b001: //csrrw (Control and Status Register Read and Write)
			{
				u32_t t = CSRs[csr];
				CSRs[csr] = x[rs1];
				X(rd) = t;
				break;
			}
			case 0b010: //csrrs (Control and Status Register Read and Set)
			{
				u32_t t = CSRs[csr];
				CSRs[csr] = t | x[rs1];
				X(rd) = t;
				break;
			}
			case 0b011: //csrrc (Control and Status Register Read and Clear)
			{
				u32_t t = CSRs[csr];
				CSRs[csr] = t & (~x[rs1]);
				X(rd) = t;
				break;
			}
			case 0b101: //csrrwi (Control and Status Register Read and Write Immediate)
			{
				u32_t zimm = GET(ins, 15, 5);
				X(rd) = CSRs[csr];
				CSRs[csr] = zimm;
				break;
			}
			case 0b110: //csrrsi
			{
				u32_t zimm = GET(ins, 15, 5);
				u32_t t = CSRs[csr];
				CSRs[csr] = t | zimm;
				X(rd) = t;
				break;
			}
			case 0b111: //csrrci
			{
				u32_t zimm = GET(ins, 15, 5);
				u32_t t = CSRs[csr];
				CSRs[csr] = t & (~zimm);
				X(rd) = t;
				break;
			}
			default:
				break;
			}
			break;
		}
		case 0b0000001: //debug
		{
			register_t rd = GET(ins, 7, 5);
			int funct3 = GET(ins, 12, 3);
			if (funct3 == 0)
				printf("x[%d]=%#X|%#d\n", rd, x[rd], x[rd]);
			break;
		}
#if defined(RV32F) || defined(RV32D)
		case 0b0000111: //I
		{
			int funct3 = GET(ins, 12, 3);
			u32_t offset = GET_I_IMM(ins);
			register_t rs1 = GET(ins, 15, 5);
			register_t rd = GET(ins, 7, 5);

			switch (funct3)
			{
			case 0b010: //FLW
			{
				F32U(rd) = M32(x[rs1] + offset);
				break;
			}
			case 0b011: //FLD
			{
				F64U(rd) = M64(x[rs1] + offset);
				break;
			}
			//FLW
			break;
			}
		}
		case 0b0100111: //S
		{
			int funct3 = GET(ins, 12, 3);
			u32_t offset = GET_S_IMM(ins);
			register_t rs1 = GET(ins, 15, 5);
			register_t rs2 = GET(ins, 20, 5);
			switch (funct3)
			{
			case 0b010: //FSW
			{
				M32(x[rs1] + offset) = F32U(rs2);
				break;
			}
			case 0b011: //FSD
			{
				M64(x[rs1] + offset) = F64U(rs2);
				break;
			}
			break;
			}
		}
		case 0b1000011: //R
		{
			int funct2 = GET(ins, 25, 2);
			register_t rd = GET(ins, 7, 5);
			register_t rs1 = GET(ins, 15, 5);
			register_t rs2 = GET(ins, 20, 5);
			register_t rs3 = GET(ins, 27, 5);

			switch (funct2)
			{
			case 0b00: //FMADD.S
			{
				F32(rd) = F32(rs1) * F32(rs2) + F32(rs3);
				break;
			}
			case 0b01: //FMADD.D
			{
				F64(rd) = F64(rs1) * F64(rs2) + F64(rs3);
				break;
			}
			break;
			}
		}
		case 0b1000111: //R
		{
			int funct2 = GET(ins, 25, 2);
			register_t rd = GET(ins, 7, 5);
			register_t rs1 = GET(ins, 15, 5);
			register_t rs2 = GET(ins, 20, 5);
			register_t rs3 = GET(ins, 27, 5);

			switch (funct2)
			{
			case 0b00: //FMSUB.S
			{
				F32(rd) = F32(rs1) * F32(rs2) - F32(rs3);
				break;
			}
			case 0b01: //FMSUB.D
			{
				F64(rd) = F64(rs1) * F64(rs2) - F64(rs3);
				break;
			}
			break;
			}
		}
		case 0b1001011: //R
		{
			int funct2 = GET(ins, 25, 2);
			register_t rd = GET(ins, 7, 5);
			register_t rs1 = GET(ins, 15, 5);
			register_t rs2 = GET(ins, 20, 5);
			register_t rs3 = GET(ins, 27, 5);
			switch (funct2)
			{
			case 0b00: //FNMSUB.S
			{
				F32(rd) = -F32(rs1) * F32(rs2) + F32(rs3);
				break;
			}
			case 0b01: //FNMSUB.D
			{
				F64(rd) = -F64(rs1) * F64(rs2) + F64(rs3);
				break;
			}
			break;
			}
		}
		case 0b1001111: //R
		{
			int funct2 = GET(ins, 25, 2);
			register_t rd = GET(ins, 7, 5);
			register_t rs1 = GET(ins, 15, 5);
			register_t rs2 = GET(ins, 20, 5);
			register_t rs3 = GET(ins, 27, 5);
			switch (funct2)
			{
			case 0b00: //FNMADD.S
			{
				F32(rd) = -F32(rs1) * F32(rs2) - F32(rs3);
				break;
			}
			case 0b01: //FNMADD.D
			{
				F64(rd) = -F64(rs1) * F64(rs2) - F64(rs3);
				break;
			}
			break;
			}
		}
		case 0b1010011: //R
		{
			int funct7 = GET(ins, 25, 7);
			int funct5 = GET(ins, 20, 5);
			int funct3 = GET(ins, 12, 3);
			register_t rd = GET(ins, 7, 5);
			register_t rs1 = GET(ins, 15, 5);
			register_t rs2 = GET(ins, 20, 5);
			switch (funct7)
			{
#ifdef RV32F
			case 0b0000000: //FADD.S
			{
				F32(rd) = F32(rs1) + F32(rs2);
				break;
			}
			case 0b0000100: //FSUB.S
			{
				F32(rd) = F32(rs1) - F32(rs2);
				break;
			}
			case 0b0001000: //FMUL.S
			{
				F32(rd) = F32(rs1) * F32(rs2);
				break;
			}
			case 0b0001100: //FDIV.S
			{
				F32(rd) = F32(rs1) / F32(rs2);
				break;
			}
			case 0b0101100: //FSQRT.S
			{
				F32(rd) = sqrtf(F32(rs1));
				break;
			}
			case 0b0010000:
			{
				switch (funct3)
				{
				case 0b000: //FSGNJ.S
				{
					F32U(rd) = GET_FIXED(F32U(rs2), 31, 1) | GET_FIXED(F32U(rs1), 0, 31);
					break;
				}
				case 0b001: //FSGNJN.S
				{
					F32U(rd) = (GET_FIXED(F32U(rs2), 31, 1) ^ (1 << 31)) | GET_FIXED(F32U(rs1), 0, 31);
					break;
				}
				case 0b010: //FSGNJX.S
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
				case 0b000: //FMIN.S
				{
					F32(rd) = fminf(F32(rs1), F32(rs2));
					break;
				}
				case 0b001: //FMAX.S
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
				case 0b00000: //FCVT.W.S
				{
					X(rd) = SIGNED(F32(rs1));
					break;
				}
				case 0b00001: //FCVT.WU.S
				{
					X(rd) = UNSIGNED(F32(rs1));
					break;
				}
				default:
					break;
				}
				break;
			}
			case 0b1110000:
			{
				switch (funct5)
				{
				case 0b00000: //FMV.X.W
				{
					X(rd) = SIGNED(F32S(rs1));
					break;
				}
				case 0b00001: //FCLASS.S
				{
					X(rd) = UNSIGNED(1 << classify_float(F32(rs1)));
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
				case 0b000: //FEQ.S
				{
					X(rd) = F32(rs1) == F32(rs2);
					break;
				}
				case 0b001: //FLT.S
				{
					X(rd) = F32(rs1) < F32(rs2);
					break;
				}
				case 0b010: //FLE.S
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
				case 0b00000: //FCVT.S.W
				{
					F32(rd) = SIGNED(x[rs1]);
					break;
				}
				case 0b00001: //FCVT.S.WU
				{
					F32(rd) = UNSIGNED(x[rs1]);
					break;
				}

				default:
					break;
				}
				break;
			}
			case 0b1111000: //FMV.W.X
			{
				F32U(rd) = x[rs1];
				break;
			}
#endif
#ifdef RV32D
			case 0b0000001: //FADD.D
			{
				F64(rd) = F64(rs1) + F64(rs2);
				break;
			}
			case 0b0000101: //FSUB.D
			{
				F64(rd) = F64(rs1) - F64(rs2);
				break;
			}
			case 0b0001001: //FMUL.D
			{
				F64(rd) = F64(rs1) * F64(rs2);
				break;
			}
			case 0b0001101: //FDIV.D
			{
				F64(rd) = F64(rs1) / F64(rs2);
				break;
			}
			case 0b0101101: //FSQRT.D
			{
				F64(rd) = sqrt(F64(rs1));
				break;
			}
			case 0b0010001:
			{
				switch (funct3)
				{
				case 0b000: //FSGNJ.D
				{
					F64U(rd) = GET_FIXED(F64U(rs2), 63, 1) | GET_FIXED(F64U(rs1), 0, 63);
					break;
				}
				case 0b001: //FSGNJN.D
				{
					F64U(rd) = (GET_FIXED(F64U(rs2), 63, 1) ^ (1 << 63)) | GET_FIXED(F64U(rs1), 0, 63);
					break;
				}
				case 0b010: //FSGNJX.D
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
				case 0b000: //FMIN.D
				{
					F64(rd) = fmin(F64(rs1), F64(rs2));
					break;
				}
				case 0b001: //FMAX.D
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
				case 0b00001: //FCVT.WU.D
				{
					X(rd) = UNSIGNED(F32(rs1));
					break;
				}

				default:
					break;
				}
				break;
			}
			case 0b0100000:
			{
				switch (funct5)
				{
				case 0b00001: //FCVT.S.D
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
				case 0b00000: //FCVT.D.S
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
				case 0b00000: //FCLASS.D
				{
					X(rd) = UNSIGNED(1 << classify_double(F64(rs1)));
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
				case 0b000: //FEQ.D
				{
					X(rd) = F64(rs1) == F64(rs2);
					break;
				}
				case 0b001: //FLT.D
				{
					X(rd) = F64(rs1) < F64(rs2);
					break;
				}
				case 0b010: //FLE.D
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
				case 0b00000: //FCVT.D.W
				{
					F64(rd) = SIGNED(x[rs1]);
					break;
				}
				case 0b00001: //FCVT.D.WU
				{
					F64(rd) = UNSIGNED(x[rs1]);
					break;
				}

				default:
					break;
				}
				break;
			}
#endif
			default:
				break;
			}
			break;
		}
#endif
#ifdef RV32A
#define AMO32(x) x

		case 0b0101111: //R
		{
			int funct5 = GET(ins, 27, 31);
			register_t rd = GET(ins, 7, 5);
			register_t rs1 = GET(ins, 15, 5);
			register_t rs2 = GET(ins, 20, 5);
			switch (funct5)
			{
			case 0b00010: //LR.W
			{
				break;
			}
			case 0b00011: //SC.W
			{
				break;
			}
			case 0b00001: //AMOSWAP.W
			{
				X(rd) = M(x[rs1]);
				M(x[rs1]) = x[rs2];
				// X(rd) = AMO32(M(x[rs1]) SWAP x[rs2]);
				break;
			}
			case 0b00000: //AMOADD.W
			{
				X(rd) = AMO32(M(x[rs1]) + x[rs2]);
				break;
			}
			case 0b00100: //AMOXOR.W
			{
				X(rd) = AMO32(M(x[rs1]) ^ x[rs2]);
				break;
			}
			case 0b01100: //AMOAND.W
			{
				X(rd) = AMO32(M(x[rs1]) & x[rs2]);
				break;
			}
			case 0b01000: //AMOOR.W
			{
				X(rd) = AMO32(M(x[rs1]) | x[rs2]);
				break;
			}
			case 0b10000: //AMOMIN.W
			{
				X(rd) = AMO32(MIN(SIGNED(M(x[rs1])), SIGNED(x[rs2])));
				break;
			}
			case 0b10100: //AMOMAX.W
			{
				X(rd) = AMO32(MAX(SIGNED(M(x[rs1])), SIGNED(x[rs2])));
				break;
			}
			case 0b11000: //AMOMINU.W
			{
				X(rd) = AMO32(MIN(UNSIGNED(M(x[rs1])), UNSIGNED(x[rs2])));
				break;
			}
			case 0b11100: //AMOMAXU.W
			{
				X(rd) = AMO32(MAX(UNSIGNED(M(x[rs1])), UNSIGNED(x[rs2])));
				break;
			}
			default:
				break;
			}
			break;
		}
#endif
		default:
			printf("非法指令\n");
			return;
		}
	}
}

#define OFF_CALC(target_addr) (target_addr - index * 4 - 4)

void insertion_sort()
{
	int index = 0;
	// void insertion_sort(long a[], size_t n)
	// {
	// 	for (size_t i = 1, j; i < n; i++)
	// 	{
	// 		long x = a[i];
	// 		for (j = i; j > 0 && a[j - 1] > x; j--)
	// 		{
	// 			a[j] = a[j - 1];
	// 		}
	// 		a[j] = x;
	// 	}
	// }

	for (int i = 0; i < 10; i++)
	{
		M32(i * 4) = 10 - i;
		printf("%d ", M32(i * 4));
	}
	printf("\n");

	X(a3) = 0;
	X(a1) = 10;
	X(x1) = 0x52;
	text[index++] = ADDI(a3, a0, 4); //a3 is pointer to a[i]
	text[index++] = ADDI(a4, x0, 1); //i = 1
	//Outer Loop
	text[index++] = BLTU(a4, a1, OFF_CALC(0x10)); //if i < n, jump to Continue Outer loop
	//Exit Outer Loop
	text[index++] = JALR(x0, x1, 0); //return from function
	//Continue OUter loop
	text[index++] = LW(a6, a3, 0);	 //x = a[i]
	text[index++] = ADDI(a2, a3, 0); //a2 is pointer to a[j]
	text[index++] = ADDI(a5, a4, 0); //j = i
	//Inner Loop:
	text[index++] = LW(a7, a2, -4);				 //a7 = a[j-1]
	text[index++] = BGE(a6, a7, OFF_CALC(0x34)); //if a[j-1] <= a[i], jup to Exit Inner Loop
	text[index++] = SW(a7, a2, 0);				 //a[j]=a[j-1]
	text[index++] = ADDI(a5, a5, -1);			 //j--
	text[index++] = ADDI(a2, a2, -4);			 //decrement a2 to point to a[j]
	text[index++] = BNE(a5, x0, OFF_CALC(0x1c)); //if j != 0, jump to Inner Loop
	//Exit Inner Loop:
	text[index++] = SLLI(a5, a2, 0x2);	  //multiply a5 by 4
	text[index++] = ADD(a5, a0, a5);	  //a5 is now byte address oi a[j]
	text[index++] = SW(a6, a5, 0);		  //a[j] = x
	text[index++] = ADDI(a4, a4, 1);	  //i++
	text[index++] = ADDI(a3, a3, 4);	  //increment a3 to point to a[i]
	text[index++] = JAL(x0, OFF_CALC(8)); //jump to Outer Loop
	eval();
	for (int i = 0; i < 10; i++)
	{
		printf("%d ", M32(i * 4));
	}

	printf("\n ");
}

int main()
{
    return 0;
}