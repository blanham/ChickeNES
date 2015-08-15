#ifndef C_6502_H
#define C_6502_H
/*
 * B6502.h
 *
 *  Created on: May 27, 2009
 *      Author: blanham
 */
#include <stdint.h>

struct _mos6502_cpu {
	uint8_t a;
	uint8_t x;
	uint8_t y;

	uint8_t sp;
	uint16_t pc;
	uint8_t flags;//Might decompose this into bools

	uint8_t irq;

	uint8_t *stack;
	uint8_t *zero_page;
	uint8_t *ram;

	void *aux;
	void	(*write)(void *, uint16_t, uint8_t);
	uint8_t (*read)(void *, uint16_t);

	uint64_t cycles;
};

enum mos6502_flags {
	FLAG_CARRY	= 0x01,
	FLAG_ZERO	= 0x02,
	FLAG_INT	= 0x04,
	FLAG_DEC	= 0x08,
	FLAG_BRK	= 0x10,
	FLAG_PUSH	= 0x20,
	FLAG_OVER	= 0x40,
	FLAG_NEG	= 0x80
};

/*
 7  bit  0
 ---- ----
 NVss DIZC
 |||| ||||
 |||| |||+- Carry: 1 if last addition or shift resulted in a carry, or if
 |||| |||     last subtraction resulted in no borrow
 |||| ||+-- Zero: 1 if last operation resulted in a 0 value
 |||| |+--- Interrupt: Interrupt inhibit
 |||| |       (0: /IRQ and /NMI get through; 1: only /NMI gets through)
 |||| +---- Decimal: 1 to make ADC and SBC use binary-coded decimal arithmetic
 ||||         (ignored on second-source 6502 like that in the NES)
 ||++------ s: No effect, used by the stack copy, see note below
 |+-------- Overflow: 1 if last ADC or SBC resulted in signed overflow,
 |            or D6 from last BIT
 +--------- Negative: Set to bit 7 of the last operation

   */
typedef struct _mos6502_cpu mos6502;

mos6502 *mos6502_alloc();
void mos6502_reset(mos6502 *cpu);
int mos6502_exec(mos6502 *cpu);
int mos6502_logger(mos6502 *cpu);

#define MOS6502_NMI 0x01
#define MOS6502_IRQ 0x02
#define MOS6502_BRK 0x04
#define MOS6502_RST 0x08

#endif
