#include <ctype.h>


#define READ8(cpu, addr) ({ cpu->read(cpu->aux, addr); })
#define PRINT_FLAG(cpu, flag, letter)\
	{ fprintf(stderr, "%c", (cpu->flags & flag) ? letter : tolower(letter)); }

#define PRINT_MOS6502_FLAGS(cpu) do {\
	fprintf(stderr, "P:");\
	PRINT_FLAG(cpu, FLAG_NEG,	'N');\
	PRINT_FLAG(cpu, FLAG_OVER,	'V');\
	PRINT_FLAG(cpu, FLAG_PUSH,	'U');\
	PRINT_FLAG(cpu, FLAG_BRK,	'B');\
	PRINT_FLAG(cpu, FLAG_DEC,	'D');\
	PRINT_FLAG(cpu, FLAG_INT,	'I');\
	PRINT_FLAG(cpu, FLAG_ZERO,	'Z');\
	PRINT_FLAG(cpu, FLAG_CARRY,	'C');\
} while(0)

#define READ_IMM_8(cpu) ({ READ8(cpu, ++cpu->pc); })
//#define READ_IND_8(cpu) ({ READ8(cpu, cpu->hl); })
#define READ_IMM_16(cpu) ({cpu->pc+=2; (READ8(cpu, cpu->pc) << 8) + READ8(cpu, cpu->pc-1);})
#define READ_IND(cpu) ({\
		uint16_t _addr = READ_IMM_8(cpu);\
		uint16_t _addr2 = (READ8(cpu, _addr+1) << 8) + READ8(cpu, _addr);\
		READ8(cpu, _addr2);\
		})
#define READ_IND_X(cpu) ({\
		uint16_t _addr = READ_IMM_8(cpu) + cpu->x;\
		uint16_t _addr2 = (READ8(cpu, _addr+1) << 8) + READ8(cpu, _addr);\
		READ8(cpu, _addr2);\
		})
//TODO: Add page crossing
#define READ_IND_Y(cpu) ({\
		uint16_t _addr = READ_IMM_8(cpu);\
		uint16_t _addr2 = (READ8(cpu, _addr+1) << 8) + READ8(cpu, _addr);\
		_addr2 += cpu->y;\
		READ8(cpu, _addr2);\
		})
#define READ_ZP(cpu) ({ READ8(cpu, READ_IMM_8(cpu)); })
#define READ_ZP_X(cpu) ({ READ8(cpu, (READ_IMM_8(cpu) + cpu->x) & 0xFF); })
#define READ_ZP_Y(cpu) ({ READ8(cpu, (READ_IMM_8(cpu) + cpu->y) & 0xFF); })
#define READ_ABS(cpu) ({ READ8(cpu, READ_IMM_16(cpu)); })
//TODO: Add page crossing logic
#define READ_ABS_X(cpu) ({ READ8(cpu, READ_IMM_16(cpu) + cpu->x); })
#define READ_ABS_Y(cpu) ({ READ8(cpu, READ_IMM_16(cpu) + cpu->y); })
#define WRITE8(cpu, addr, value) { cpu->write(cpu->aux, addr, value); }
#define PUSH(cpu, val) { cpu->stack[cpu->sp--] = (uint8_t)(val); }
#define PUSH_PC(cpu) do {\
	PUSH(cpu, cpu->pc >>8);\
	PUSH(cpu, cpu->pc & 0xFF);\
} while (0)
#define POP(cpu) ({cpu->stack[++cpu->sp];})

//#define READ16(cpu, addr) ({ uint16_t _addr = (addr); cpu->read(cpu, _addr})
//
#define FLAG_CLEAR(cpu, flag) { cpu->flags &= ~flag; }
#define FLAG_SET(cpu, flag) { cpu->flags |= flag; }
#define FLAG_CHECK(cpu, cond, flag) {if(cond){cpu->flags |= flag;}else{cpu->flags &= ~flag;}}
#define CHKNEGZERO(cpu, val) do {\
	FLAG_CHECK(cpu, val & 0x80, FLAG_NEG);\
	FLAG_CHECK(cpu, val == 0, FLAG_ZERO);\
} while (0)

#define AND(cpu, val) do {\
	cpu->a &= val;\
	CHKNEGZERO(cpu, cpu->a);\
} while (0)
#define JSR(cpu) do {\
	uint16_t new_pc = READ_IMM_16(cpu) - 1;\
	PUSH_PC(cpu);\
	cpu->pc = new_pc;\
} while (0)

#define LDA(cpu, val) do {\
	cpu->a = (val);\
	CHKNEGZERO(cpu, cpu->a);\
} while (0)

#define LDX(cpu, val) do {\
	cpu->x = (val);\
	CHKNEGZERO(cpu, cpu->x);\
} while (0)

#define LDY(cpu, val) do {\
	cpu->y = (val);\
	CHKNEGZERO(cpu, cpu->y);\
} while (0)
