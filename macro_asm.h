#define PSEUDO_INSTRUCTIONS

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