/*
 * B6502.c - Bryce's 6502 C interpreter
 *
 *  Created on: May 27, 2009
 *      Author: blanham
 *      I used 6502.c, found online, as an essential reference
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "b6502.h"
#include "b6502_macros.h"

extern uint8_t *RAM;

//Program counter save variables
//supersavepc is used when we have to juggle counters
//should consider rewriting those functions that use it
//but probably won't matter as the core is fast enough anyway
uint16_t savepc, supersavepc;
//helper variables
uint8_t helper = 0;
uint8_t val = 0;
//cycle counter
int cycles = 0;

//some functions need to save P
uint8_t saveP;


mos6502 *mos6502_alloc()
{
	mos6502 *ret = calloc(sizeof(*ret), 1);
	RAM = calloc(65536, 1);
	ret->sp = 0xFD;
	ret->flags = 0x24;
	ret->pc = 0xC000;
	ret->ram = calloc(65536, 1);
	ret->stack = calloc(0x100, 1);
	ret->zero_page = calloc(0x100, 1);
	return ret;
}

void nmi(mos6502 *cpu)
{
	PUSH_PC(cpu);
	PUSH(cpu, cpu->flags);
/*  RAM[0x0100+cpu->sp--] = (uint8_t)(cpu->pc >> 8);
	RAM[0x0100+cpu->sp--] = (uint8_t)(cpu->pc & 0xff);
	RAM[0x0100+cpu->sp--] = cpu->flags;
	*/
	cpu->pc = READ8(cpu, 0xfffa) + (READ8(cpu, 0xfffb)<<8);
	//RAM[0x2000] |= 0x80;
}

void mos6502_reset(mos6502 *cpu)
{
	cpu->pc = cpu->read(cpu->aux, 0xFFFC);
	cpu->pc |= cpu->read(cpu->aux, 0xFFFD) << 8;
}

void pushstack(mos6502 *cpu) {
	RAM[0x0100+cpu->sp] = (uint8_t)(cpu->pc >> 8);
	cpu->sp--;
	RAM[0x0100+cpu->sp] = (uint8_t)(cpu->pc & 0xff);
	cpu->sp--;
	RAM[0x0100+cpu->sp] = cpu->flags;
	cpu->sp--;
}
//Operations

void mos6502_branch(mos6502 *cpu, enum mos6502_flags flag, bool condition) {
	cycles = 2;
	if (!(cpu->flags & flag) ^ condition) {
		cycles++;
		uint8_t page = (cpu->pc >> 8) & 0xFF;
		cpu->pc = cpu->pc + (int8_t)READ8(cpu, cpu->pc+1);
		if (page != ((cpu->pc >> 8) & 0xFF)) {
			//cycles++;
		}
	}
	cpu->pc++;
}

void BRK(mos6502 *cpu) {
			cpu->pc+=2;
			pushstack(cpu);
			cpu->pc -=2;
			cpu->flags |= 0x10;
			cycles = 7;
			printf("nestest: %.2X %.2X", RAM[2], RAM[3]);
			//This is here to prevent executing garbage data as opcodes
			//most games I've seen don't use it
			exit(0);
}

#include "b6502_ops.h"
int mos6502_exec(mos6502 *cpu) {
	cycles = 0;

	if ((cpu->flags & FLAG_INT) && cpu->irq) {
		uint8_t irq = __builtin_ctz(cpu->irq);
		PUSH_PC(cpu);
		PUSH(cpu, cpu->flags);
		switch (1 << irq) {
			case MOS6502_IRQ:
			case MOS6502_BRK:
				cpu->pc = READ8(cpu, 0xFFFE) + (READ8(cpu, 0xFFFF)<<8);
				break;
			case MOS6502_NMI:
				cpu->pc = READ8(cpu, 0xFFFA) + (READ8(cpu, 0xFFFB)<<8);
				break;
			case MOS6502_RST:
				cpu->pc = READ8(cpu, 0xFFFC) + (READ8(cpu, 0xFFFD)<<8);
				break;
		}

		cpu->irq = 0;
	}


	uint8_t opcode = READ8(cpu, cpu->pc);
	struct op *op = &ops[opcode];

	switch (opcode) {
			//Flags
		case 0x18: FLAG_CLEAR(cpu, FLAG_CARRY); cycles = 2; break;
		case 0x58: FLAG_CLEAR(cpu, FLAG_INT); cycles = 2; break;
		case 0xB8: FLAG_CLEAR(cpu, FLAG_OVER); cycles = 2; break;
		case 0xD8: FLAG_CLEAR(cpu, FLAG_DEC); cycles = 2; break;
		case 0x38: FLAG_SET(cpu, FLAG_CARRY); cycles = 2;break;
		case 0x78: FLAG_SET(cpu, FLAG_INT); cycles = 2;break;
		case 0xF8: FLAG_SET(cpu, FLAG_DEC); cycles = 2;break;

			//ADC
		case 0x69: ADC(cpu, READ_IMM_8(cpu)); break;
		case 0x65: ADC(cpu, READ_ZP(cpu));    break;
		case 0x75: ADC(cpu, READ_ZP_X(cpu));  break;
		case 0x6D: ADC(cpu, READ_ABS(cpu));   break;
		case 0x7D: ADC(cpu, READ_ABS_X(cpu)); break;
		case 0x79: ADC(cpu, READ_ABS_Y(cpu)); break;
		case 0x61: ADC(cpu, READ_IND_X(cpu)); break;
		case 0x71: ADC(cpu, READ_IND_Y(cpu)); break;

			//AND
		case 0x29: AND(cpu, READ_IMM_8(cpu)); break;
		case 0x25: AND(cpu, READ_ZP(cpu));    break;
		case 0x35: AND(cpu, READ_ZP_X(cpu));  break;
		case 0x2D: AND(cpu, READ_ABS(cpu));   break;
		case 0x3D: AND(cpu, READ_ABS_X(cpu)); break;
		case 0x39: AND(cpu, READ_ABS_Y(cpu)); break;
		case 0x21: AND(cpu, READ_IND_X(cpu)); break;
		case 0x31: AND(cpu, READ_IND_Y(cpu)); break;

			//ASL
		case 0x0A: cpu->a = ASL(cpu, cpu->a); break;
		case 0x06: {
					   uint16_t addr = READ_IMM_8(cpu);
					   uint8_t val = ASL(cpu, READ8(cpu, addr));
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0x16: {
					   uint16_t addr = ADDR_ZP_X(cpu);
					   uint8_t val = ASL(cpu, READ8(cpu, addr));
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0x0E: {
					   uint16_t addr = ADDR_ABS(cpu);
					   uint8_t val = ASL(cpu, READ8(cpu, addr));
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0x1E: {
					   uint16_t addr = ADDR_ABS_X(cpu);
					   uint8_t val = ASL(cpu, READ8(cpu, addr));
					   WRITE8(cpu, addr, val);
					   break;
				   }

			//Branches
		case 0x90: mos6502_branch(cpu, FLAG_CARRY, false);	break; //BCC
		case 0xB0: mos6502_branch(cpu, FLAG_CARRY, true);	break; //BCS
		case 0xF0: mos6502_branch(cpu, FLAG_ZERO,  true);	break;
		case 0x30: mos6502_branch(cpu, FLAG_NEG,   true);	break;
		case 0xD0: mos6502_branch(cpu, FLAG_ZERO,  false);	break;
		case 0x10: mos6502_branch(cpu, FLAG_NEG,   false);	break;
		case 0x50: mos6502_branch(cpu, FLAG_OVER,  false);	break;
		case 0x70: mos6502_branch(cpu, FLAG_OVER,  true);	break;

			//BIT
		case 0x24: BIT(cpu, READ_ZP(cpu)); break;
		case 0x2C: BIT(cpu, READ_ABS(cpu)); break;

		//case 0x00: BRK(cpu); break;

			//CMP
		case 0xC9: CMP(cpu, READ_IMM_8(cpu)); break;
		case 0xC5: CMP(cpu, READ_ZP(cpu));    break;
		case 0xD5: CMP(cpu, READ_ZP_X(cpu));  break;
		case 0xCD: CMP(cpu, READ_ABS(cpu));   break;
		case 0xDD: CMP(cpu, READ_ABS_X(cpu)); break;
		case 0xD9: CMP(cpu, READ_ABS_Y(cpu)); break;
		case 0xC1: CMP(cpu, READ_IND_X(cpu)); break;
		case 0xD1: CMP(cpu, READ_IND_Y(cpu)); break;

			//CPX
		case 0xE0: CPX(cpu, READ_IMM_8(cpu)); break;
		case 0xE4: CPX(cpu, READ_ZP(cpu)); break;
		case 0xEC: CPX(cpu, READ_ABS(cpu)); break;

			//CPY
		case 0xC0: CPY(cpu, READ_IMM_8(cpu)); break;
		case 0xC4: CPY(cpu, READ_ZP(cpu)); break;
		case 0xCC: CPY(cpu, READ_ABS(cpu)); break;

			//DEC
		case 0xC6: {
					   uint16_t addr = READ_IMM_8(cpu);
					   uint8_t val = READ8(cpu, addr) - 1;
					   CHKNEGZERO(cpu, val);
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0xD6: {
					   uint16_t addr = ADDR_ZP_X(cpu);
					   uint8_t val = READ8(cpu, addr) - 1;
					   CHKNEGZERO(cpu, val);
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0xCE: {
					   uint16_t addr = ADDR_ABS(cpu);
					   uint8_t val = READ8(cpu, addr) - 1;
					   CHKNEGZERO(cpu, val);
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0xDE: {
					   uint16_t addr = ADDR_ABS_X(cpu);
					   uint8_t val = READ8(cpu, addr) - 1;
					   CHKNEGZERO(cpu, val);
					   WRITE8(cpu, addr, val);
					   break;
				   }

			//DEX
		case 0xCA: cpu->x--; CHKNEGZERO(cpu, cpu->x); break;

			//DEY
		case 0x88: cpu->y--; CHKNEGZERO(cpu, cpu->y); break;

			//EOR
		case 0x49: EOR(cpu, READ_IMM_8(cpu)); break;
		case 0x45: EOR(cpu, READ_ZP(cpu));    break;
		case 0x55: EOR(cpu, READ_ZP_X(cpu));  break;
		case 0x4D: EOR(cpu, READ_ABS(cpu));   break;
		case 0x5D: EOR(cpu, READ_ABS_X(cpu)); break;
		case 0x59: EOR(cpu, READ_ABS_Y(cpu)); break;
		case 0x41: EOR(cpu, READ_IND_X(cpu)); break;
		case 0x51: EOR(cpu, READ_IND_Y(cpu)); break;

			//INC
		case 0xE6: {
					   uint16_t addr = READ_IMM_8(cpu);
					   uint8_t val = READ8(cpu, addr) + 1;
					   CHKNEGZERO(cpu, val);
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0xF6: {
					   uint16_t addr = ADDR_ZP_X(cpu);
					   uint8_t val = READ8(cpu, addr) + 1;
					   CHKNEGZERO(cpu, val);
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0xEE: {
					   uint16_t addr = ADDR_ABS(cpu);
					   uint8_t val = READ8(cpu, addr) + 1;
					   CHKNEGZERO(cpu, val);
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0xFE: {
					   uint16_t addr = ADDR_ABS_X(cpu);
					   uint8_t val = READ8(cpu, addr) + 1;
					   CHKNEGZERO(cpu, val);
					   WRITE8(cpu, addr, val);
					   break;
				   }

			//INX
		case 0xE8: cpu->x++; CHKNEGZERO(cpu, cpu->x); break;

			//INY
		case 0xC8: cpu->y++; CHKNEGZERO(cpu, cpu->y); break;

			//JMP
		case 0x4C: cpu->pc = READ_IMM_16(cpu) - 1; break;
		case 0x6C: cpu->pc = READ_IND(cpu) - 1; break;


			//JSR
		case 0x20: JSR(cpu); cycles = 6; break;

			//LDA
		case 0xA9: LDA(cpu, READ_IMM_8(cpu)); break;
		case 0xA5: LDA(cpu, READ_ZP(cpu)); break;
		case 0xB5: LDA(cpu, READ_ZP_X(cpu)); break;
		case 0xAD: LDA(cpu, READ_ABS(cpu)); break;
		case 0xBD: LDA(cpu, READ_ABS_X(cpu)); break;
		case 0xB9: LDA(cpu, READ_ABS_Y(cpu)); break;
		case 0xA1: LDA(cpu, READ_IND_X(cpu)); break;
		case 0xB1: LDA(cpu, READ_IND_Y(cpu)); break;

			//LDX
		case 0xA2: LDX(cpu, READ_IMM_8(cpu)); break;
		case 0xA6: LDX(cpu, READ_ZP(cpu)); break;
		case 0xB6: LDX(cpu, READ_ZP_Y(cpu)); break;
		case 0xAE: LDX(cpu, READ_ABS(cpu)); break;
		case 0xBE: LDX(cpu, READ_ABS_Y(cpu)); break;

			//LDY
		case 0xA0: LDY(cpu, READ_IMM_8(cpu)); break;
		case 0xA4: LDY(cpu, READ_ZP(cpu)); break;
		case 0xB4: LDY(cpu, READ_ZP_X(cpu)); break;
		case 0xAC: LDY(cpu, READ_ABS(cpu)); break;
		case 0xBC: LDY(cpu, READ_ABS_X(cpu)); break;

			//LSR
		case 0x4A: cpu->a = LSR(cpu, cpu->a); break;
		case 0x46: {
					   uint16_t addr = READ_IMM_8(cpu);
					   uint8_t val = LSR(cpu, READ8(cpu, addr));
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0x56: {
					   uint16_t addr = ADDR_ZP_X(cpu);
					   uint8_t val = LSR(cpu, READ8(cpu, addr));
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0x4E: {
					   uint16_t addr = ADDR_ABS(cpu);
					   uint8_t val = LSR(cpu, READ8(cpu, addr));
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0x5E: {
					   uint16_t addr = ADDR_ABS_X(cpu);
					   uint8_t val = LSR(cpu, READ8(cpu, addr));
					   WRITE8(cpu, addr, val);
					   break;
				   }

			//NOP
		case 0xEA:  cycles = 2; break;
/*
		case 0x04: case 0x44: case 0x64:
			cpu->pc++;
			break;
		case 0x0C:
			cpu->pc += 2;
			break;
		case 0x14: case 0x34: case 0x54:
		case 0x74: case 0xD4: case 0xF4:
			cpu->pc++;
			break;
		case 0x1A: case 0x3A: case 0x5A:
		case 0x7A: case 0xDA: case 0xFA:
			break;
		case 0x80:
			cpu->pc++;
			break;
		case 0x1C: case 0x3C: case 0x5C:
		case 0x7C: case 0xDC: case 0xFC:
			cpu->pc += 2;
			break;
*/
				//LAX
		//case 0xa3: RdIndX(cpu); LDA(cpu);LDX(cpu);break;
		//ase 0xA7: RdZp(cpu); LDA(cpu); LDX(cpu);break;


				//ORA
		case 0x09: ORA(cpu, READ_IMM_8(cpu)); break;
		case 0x05: ORA(cpu, READ_ZP(cpu));    break;
		case 0x15: ORA(cpu, READ_ZP_X(cpu));  break;
		case 0x0D: ORA(cpu, READ_ABS(cpu));   break;
		case 0x1D: ORA(cpu, READ_ABS_X(cpu)); break;
		case 0x19: ORA(cpu, READ_ABS_Y(cpu)); break;
		case 0x01: ORA(cpu, READ_IND_X(cpu)); break;
		case 0x11: ORA(cpu, READ_IND_Y(cpu)); break;

			//PHA
		case 0x48: PUSH(cpu, cpu->a); break;

			//PHP
		case 0x08: PUSH(cpu, cpu->flags | 0x30); break;

			//PLA
		case 0x68: cpu->a = POP(cpu); CHKNEGZERO(cpu, cpu->a); break;

			//PLP
		case 0x28: cpu->flags = (POP(cpu) & ~FLAG_BRK) | FLAG_PUSH;  break;

			//ROL
		case 0x2A: cpu->a = ROL(cpu, cpu->a); break;
		case 0x26: {
					   uint16_t addr = READ_IMM_8(cpu);
					   uint8_t val = ROL(cpu, READ8(cpu, addr));
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0x36: {
					   uint16_t addr = ADDR_ZP_X(cpu);
					   uint8_t val = ROL(cpu, READ8(cpu, addr));
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0x2E: {
					   uint16_t addr = ADDR_ABS(cpu);
					   uint8_t val = ROL(cpu, READ8(cpu, addr));
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0x3E: {
					   uint16_t addr = ADDR_ABS_X(cpu);
					   uint8_t val = ROL(cpu, READ8(cpu, addr));
					   WRITE8(cpu, addr, val);
					   break;
				   }

			//ROR
		case 0x6A: cpu->a = ROR(cpu, cpu->a); break;
		case 0x66: {
					   uint16_t addr = READ_IMM_8(cpu);
					   uint8_t val = ROR(cpu, READ8(cpu, addr));
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0x76: {
					   uint16_t addr = ADDR_ZP_X(cpu);
					   uint8_t val = ROR(cpu, READ8(cpu, addr));
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0x6E: {
					   uint16_t addr = ADDR_ABS(cpu);
					   uint8_t val = ROR(cpu, READ8(cpu, addr));
					   WRITE8(cpu, addr, val);
					   break;
				   }
		case 0x7E: {
					   uint16_t addr = ADDR_ABS_X(cpu);
					   uint8_t val = ROR(cpu, READ8(cpu, addr));
					   WRITE8(cpu, addr, val);
					   break;
				   }

			//RTI
		case 0x40: cpu->flags = POP(cpu) | FLAG_PUSH; cpu->pc = POP_16(cpu) - 1; break;

			//RTS
		case 0x60: cpu->pc = POP_16(cpu); break;

			//SBC
		case 0xE9: SBC(cpu, READ_IMM_8(cpu)); break;
		case 0xe5: SBC(cpu, READ_ZP(cpu));    break;
		case 0xF5: SBC(cpu, READ_ZP_X(cpu));  break;
		case 0xED: SBC(cpu, READ_ABS(cpu));   break;
		case 0xFD: SBC(cpu, READ_ABS_X(cpu)); break;
		case 0xF9: SBC(cpu, READ_ABS_Y(cpu)); break;
		case 0xE1: SBC(cpu, READ_IND_X(cpu)); break;
		case 0xF1: SBC(cpu, READ_IND_Y(cpu)); break;

			//STA
		case 0x85: WRITE_ZP(cpu, cpu->a);  break;
		case 0x95: WRITE_ZP_X(cpu, cpu->a);  break;
		case 0x8D: WRITE8(cpu, READ_IMM_16(cpu), cpu->a); break;
		case 0x9D: WRITE8(cpu, ADDR_ABS_X(cpu), cpu->a); break;
		case 0x99: WRITE8(cpu, ADDR_ABS_Y(cpu), cpu->a); break;
		case 0x81: WRITE8(cpu, ADDR_IND_X(cpu), cpu->a); break;
		case 0x91: WRITE8(cpu, ADDR_IND_Y(cpu), cpu->a); break;

			//STX
		case 0x86: WRITE_ZP(cpu, cpu->x);  break;
		case 0x96: WRITE_ZP_Y(cpu, cpu->x);  break;
		case 0x8E: WRITE_ABS(cpu, cpu->x); break;

			//STY
		case 0x84: WRITE_ZP(cpu, cpu->y);  break;
		case 0x94: WRITE_ZP_X(cpu, cpu->y);  break;
		case 0x8C: WRITE_ABS(cpu, cpu->y); break;

			//TAX
		case 0xAA: cpu->x = cpu->a; CHKNEGZERO(cpu, cpu->x); break;

			//TAY
		case 0xA8: cpu->y = cpu->a; CHKNEGZERO(cpu, cpu->y); break;

			//TSX
		case 0xBA: cpu->x = cpu->sp; CHKNEGZERO(cpu, cpu->x); break;

			//TXA
		case 0x8A: cpu->a = cpu->x; CHKNEGZERO(cpu, cpu->a); break;

			//TXS
		case 0x9A: cpu->sp = cpu->x; break;

			//TYA
		case 0x98: cpu->a = cpu->y; CHKNEGZERO(cpu, cpu->a); break;

		default:
			fprintf(stderr, "%.2X \tUnknown Opcode!\n", opcode);
			fprintf(stderr, "nestest: %.2X %.2X %.2X %.2X\n", RAM[0], RAM[1], RAM[2], RAM[3]);
			exit(EXIT_FAILURE);
	}

	cpu->pc++;
	cycles = op->cycles;
	cpu->cycles += cycles;
	return cycles;
}

int mos6502_logger(mos6502 *cpu) {
	int ret = 0;
	static int lcycles = 0;
	fprintf(stderr, "$%.4X:  ", cpu->pc);
	uint8_t opcode = READ8(cpu, cpu->pc);
	struct op *op = &ops[opcode];


	switch(op->bytes) {
		case 1:
			fprintf(stderr, "%.2X        ", opcode);
			break;
		case 2:
			fprintf(stderr, "%.2X %.2X     ", opcode, READ8(cpu, cpu->pc + 1));
			break;
		case 3:
			fprintf(stderr, "%.2X %.2X %.2X  ",
					opcode, READ8(cpu, cpu->pc + 1), READ8(cpu, cpu->pc + 2));
	}

	fprintf(stderr, "%s ", op->name);

	/*
	switch(op->mode) {
		case MODE_ACC:
			fprintf(stderr, "A         ");
			break;
		case MODE_ABS:
			if (op->modify) {
				uint16_t addr = cpu->ram[cpu->pc + 1] +  (cpu->ram[cpu->pc + 2] << 8);
				fprintf(stderr, "$%.4X =  %.2X\t\t\t\t", addr, cpu->ram[addr]);
			} else {
				fprintf(stderr, "$%.2X%.2X   \t\t\t\t", cpu->ram[cpu->pc + 2], cpu->ram[cpu->pc + 1]);
			}
			break;
		case MODE_IND:
			{
				uint16_t addr = cpu->ram[cpu->pc + 1] | (cpu->ram[cpu->pc + 2] << 8);
				fprintf(stderr, "($%.4X) =  %.2X%.2X\t\t\t", addr, cpu->ram[addr+1], cpu->ram[addr]);
			}
			break;
		case MODE_IMD:
			fprintf(stderr, "#$%.2X    \t\t\t\t", cpu->ram[cpu->pc+1]);
			break;
		case MODE_REL:
			fprintf(stderr,"$%.4X   \t\t\t\t", cpu->pc + 2 + (signed)cpu->ram[cpu->pc+1]);
			break;
		case MODE_IMP:
			fprintf(stderr, "          \t\t\t\t");
			break;
		case MODE_ZP:
			{
			uint8_t zp = cpu->ram[cpu->pc + 1];
			fprintf(stderr, "$%.2X = %.2X\t\t\t\t", zp, cpu->ram[zp]);
			}
			break;
		case MODE_ZPX:
			{
				uint8_t val =  cpu->ram[cpu->pc+1];
				fprintf(stderr, "$%.2X,X @ %.2X = %.2X\t\t\t",
						val, (val + cpu->x) & 0xFF, cpu->ram[(val + cpu->x) & 0xFF]);
			}
			break;
		case MODE_ZPY:
			{
				uint8_t val =  cpu->ram[cpu->pc+1];
				fprintf(stderr, "$%.2X,Y @ %.2X = %.2X\t\t\t",
						val, (val + cpu->y) & 0xFF, cpu->ram[(val + cpu->y) & 0xFF]);
			}
			break;
		case MODE_ABSX:
			{
			uint16_t addr = cpu->ram[cpu->pc + 1] + (cpu->ram[cpu->pc+2] << 8);
			fprintf(stderr, "$%.4X,X @ %.4X = %.2X\t\t",
					addr,  (addr + cpu->x) & 0xFFFF, cpu->ram[(addr + cpu->x) & 0xFFFF]);
			}
			break;
		case MODE_ABSY:
			{
			uint16_t addr = cpu->ram[cpu->pc + 1] + (cpu->ram[cpu->pc+2] << 8);
			fprintf(stderr, "$%.4X,Y @ %.4X = %.2X\t\t",
					addr,  (addr + cpu->y) & 0xFFFF, cpu->ram[(addr + cpu->y) & 0xFFFF]);
			}
			break;
		case MODE_INDY:
			{
///
//void RdIndY(mos6502 *cpu) {
//	cycles = 5;
//	val = RAM[cpu->pc+1];
//	savepc = RAM[val];
//
//	savepc |= (RAM[++val] << 8);
//	savepc +=cpu->y;
//
//	val = RAM[savepc];
//	cpu->pc++;
//}/

			//LDA ($89),Y = 0300 @ 0300 = 89
			uint8_t zp = cpu->ram[cpu->pc + 1];
			//fprintf(stderr, " %X %X %X %X %X ", cpu->ram
			uint16_t addr = cpu->ram[zp & 0xFF] | (cpu->ram[(zp+1) & 0xFF] << 8);
			uint16_t val = zp;
			//addr += cpu->y;
			fprintf(stderr, "($%.2X),Y = %.4X @ %.4X = %.2X",
					zp, addr, (addr + cpu->y) & 0xFFFF, cpu->ram[(addr + cpu->y) & 0xFFFF]);
			}
			break;
		case MODE_INDX:
			{
			uint8_t zp = cpu->ram[cpu->pc + 1];
			uint16_t val = zp + cpu->x;
			uint16_t addr = cpu->ram[val & 0xFF] + (cpu->ram[(val+1) & 0xFF] << 8);
			fprintf(stderr, "($%.2X,X) @ %.2X = %.4X = %.2X",
					zp, val & 0xFF, addr, cpu->ram[addr]);
			}
			break;
		default:
		//	fprintf(stderr, "Unimplemented mode %i\n", op->mode);
		//	exit(EXIT_FAILURE);
			fprintf(stderr, "%2i        \t\t\t\t", op->mode);
	}*/

	fprintf(stderr, "\tA:%.2X X:%.2X Y:%.2X P:%.2X SP:%.2X",
			cpu->a, cpu->x, cpu->y, cpu->flags, cpu->sp);
	//fprintf(stderr, "\tA:%.2X X:%.2X Y:%.2X P:%.2X SP:%.2X CYC:%3i ",
	//		cpu->a, cpu->x, cpu->y, cpu->flags, cpu->sp, lcycles);

	//PRINT_MOS6502_FLAGS(cpu);
	fprintf(stderr, "\n");

	ret = mos6502_exec(cpu);

	lcycles += ret * 3;
	if (lcycles >= 341) {
		lcycles -= 341;
	}

	return ret;
}
