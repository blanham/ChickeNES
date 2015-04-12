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

#include "main.h"
#include "b6502.h"

#ifdef DEBUG
# define DPRINTF(...) do { fprintf(stderr, __VA_ARGS__);} while (0)
#else
# define DPRINTF(...) 
#endif

//counting variable
int sum; //used in ADC/SBC
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

//for signed operations - (Because past Bryce didn't understand casting)
int8_t pSigned;
//some functions need to save P
uint8_t saveP;

mos6502 *mos6502_alloc()
{
	mos6502 *ret = calloc(sizeof(*ret), 1);
	RAM = calloc(65536, 1);
	ret->sp = 0xFD;
	ret->flags = 0x24;
	ret->pc = 0xC000;
	ret->ram = RAM;
	return ret;
}

void printp(mos6502 *cpu)
{
    printf("P:");
	//printf(cpu->flags & 0x80 ? "N" : "n");
    if (cpu->flags & 0x80)printf("N");
    else printf("n");
    if (cpu->flags & 0x40)printf("V");
    else printf("v");
    if (cpu->flags & 0x20)printf("U");
    else printf("u");
    if (cpu->flags & 0x10)printf("B");
    else printf("b");
    if (cpu->flags & 0x08)printf("D");
    else printf("d");
    if (cpu->flags & 0x04)printf("I");
    else printf("i");
    if (cpu->flags & 0x02)printf("Z");
    else printf("z");
    if (cpu->flags & 0x01)printf("C");
    else printf("c");
}

void nmi(mos6502 *cpu)
{
	DPRINTF("\n\nNMI!\n\n");
	RAM[0x0100+cpu->sp--] = (cpu->pc >> 8);
	RAM[0x0100+cpu->sp--] = (cpu->pc & 0xff);
	RAM[0x0100+cpu->sp--] = cpu->flags;
	cpu->pc = RAM[0xfffa] + (RAM[0xfffb]<<8);
	RAM[0x2000] |= 0x80;
	NMI=0;
}


void initCPU(void)
{
	//read sarting vector from 0x7ffd
//	cpu->pc = RAM[0xfffc] + (RAM[0xfffd]<<8);
//	printf("Starting execution at: %.4x\n", cpu->pc);
	//set CPU flags and registers clear
	
//	A=X=Y=P=0;
//	cpu->flags |= 0x4;
}


//would probably be better as a macro
void chknegzero(mos6502 *cpu, uint8_t chk) {
	if (chk & 0x80) cpu->flags |= 0x80;
	else cpu->flags &= 0x7f;

	if (chk == 0) cpu->flags |= 0x02;
	else cpu->flags &= 0xfd;
}

void pushstack(mos6502 *cpu) {
	RAM[0x0100+cpu->sp] = cpu->pc;
	cpu->sp--;
	RAM[0x0100+cpu->sp] = cpu->pc+1;
	cpu->sp--;
	RAM[0x0100+cpu->sp] = cpu->flags;
	cpu->sp--;
}

//Memory Read Functions
void RdImd(mos6502 *cpu) {
	DPRINTF("%.2X %.2X", RAM[cpu->pc], RAM[cpu->pc+1]);
	val = RAM[cpu->pc+1];
	cycles = 2;
	cpu->pc++;
}

void RdAcc(mos6502 *cpu) {
	DPRINTF("%.2X", RAM[cpu->pc]);
	val = cpu->a;
	cycles = 2;
}

void RdZp(mos6502 *cpu) {
	DPRINTF("%.2X %.2X", RAM[cpu->pc], RAM[cpu->pc+1]);
	savepc = RAM[cpu->pc+1];
	val = RAM[savepc];
	cycles = 3;
	cpu->pc++;
}

void RdZpX(mos6502 *cpu) {
	DPRINTF("%.2X %.2X", RAM[cpu->pc], RAM[cpu->pc+1]);
	savepc = RAM[cpu->pc+1] + cpu->x;
	savepc &= 0x00ff;
	val = RAM[savepc];
	cpu->pc++;
	cycles = 4;
}

void RdZpY(mos6502 *cpu) {
	DPRINTF("%.2X %.2X", RAM[cpu->pc], RAM[cpu->pc+1]);
	savepc = RAM[cpu->pc+1] + cpu->y;
	savepc &= 0x00ff;
	val = RAM[savepc];
	cpu->pc++;
	cycles = 4;
}

void RdAbs(mos6502 *cpu) {
	DPRINTF("%.2X %.2X %.2X", RAM[cpu->pc], RAM[cpu->pc+1], RAM[cpu->pc+2]);
	savepc = RAM[cpu->pc+1] + (RAM[cpu->pc+1+1]<<8);
 	val = ReadRAM(savepc);
	cpu->pc += 2;
	cycles = 4;
}

void RdAbsX(mos6502 *cpu) {
	DPRINTF("%.2X %.2X %.2X", RAM[cpu->pc], RAM[cpu->pc+1], RAM[cpu->pc+2]);
	savepc = RAM[cpu->pc+1] + (RAM[cpu->pc+1+1]<<8);
	cpu->pc += 2;
	cycles = 4;
	if ((savepc>>8) != ((savepc + cpu->x)>>8)) {
		cycles++;
	}
	savepc += cpu->x;
	val = ReadRAM(savepc);
}

void RdAbsY(mos6502 *cpu) {
	DPRINTF("%.2X %.2X %.2X", RAM[cpu->pc], RAM[cpu->pc+1], RAM[cpu->pc+2]);
	savepc = RAM[cpu->pc+1] + (RAM[cpu->pc+1+1]<<8);
	cpu->pc += 2;
	cycles = 4;
	if ((savepc>>8) != ((savepc + cpu->y)>>8))
		cycles++;
	savepc +=cpu->y;
	val = ReadRAM(savepc);
}

void RdInd(mos6502 *cpu) {
	DPRINTF("%.2X %.2X %.2X \tJMP", RAM[cpu->pc], RAM[cpu->pc+1], RAM[cpu->pc+2]);
	DPRINTF(" $(%.2X%.2X)", RAM[cpu->pc+2], RAM[cpu->pc+1]);

	/*  savepc = RAM[cpu->pc+1] + (RAM[cpu->pc+2] << 8);

		helper = RAM[savepc];

		if (RAM[cpu->pc+1]==0xFF){
		DPRINTF("%.4X",savepc);
		savepc -= 0xFF;
		helper = helper + (RAM[savepc+1] << 8);
		DPRINTF("%.4X",savepc);
		}
		else {helper = helper + (RAM[savepc+1] << 8); }
		*/
	savepc = RAM[cpu->pc+1] + (RAM[cpu->pc+2] << 8);
	supersavepc = RAM[savepc];
	if(RAM[cpu->pc+1]==0xFF){
		savepc &= 0xFF00;
		savepc -= 1;
	}
	supersavepc |= (RAM[savepc +1] << 8);
	//DPRINTF("%.4X\n", RAM[savepc] );

	savepc = supersavepc;

	cycles = 5;
}

void RdIndX(mos6502 *cpu) {
	DPRINTF("%.2X %.2X", RAM[cpu->pc], RAM[cpu->pc+1]);

	val = RAM[cpu->pc+1] + cpu->x;

	savepc = RAM[val];
	savepc |= (RAM[++val] << 8);

	val = RAM[savepc];
	cpu->pc++;
	cycles = 6;
}

void RdIndY(mos6502 *cpu) {
	DPRINTF("%.2X %.2X", RAM[cpu->pc], RAM[cpu->pc+1]);

	cycles = 5;
	val = RAM[cpu->pc+1];
	savepc = RAM[val];

	savepc |= (RAM[++val] << 8);
	savepc +=cpu->y;

	val = RAM[savepc];
	cpu->pc++;
}

//Memory Write Functions, needs cycles added

void WrAcc(mos6502 *cpu) {
	DPRINTF("");
	cpu->a=val;
}

void WrZp(mos6502 *cpu) {
	DPRINTF(" $%.2X = #$%.2X", savepc, val);
	RAM[savepc] = val;
	cpu->pc++;
}

void WrZpX(mos6502 *cpu) {
	DPRINTF(" $%.2X = #$%.2X", savepc, val);
	RAM[savepc] = val;
	cpu->pc++;
}

void WrZpY(mos6502 *cpu) {
	RAM[savepc] = val;

	DPRINTF(" $%.2X = #$%.2X", savepc, val);

	cpu->pc++;
}

void WrAbs(mos6502 *cpu) {
	DPRINTF(" %.2X @ %.4x", val, savepc);
	WriteRAM(savepc, val);
	cpu->pc++;cpu->pc++;
}

void WrAbsX(mos6502 *cpu) {
	DPRINTF(" %.2X @ %.4x", val, savepc);
	WriteRAM(savepc, val);
	cpu->pc++;cpu->pc++;
}

void WrAbsY(mos6502 *cpu) {
	DPRINTF(" %.2X @ %.4x", val, savepc);
	WriteRAM(savepc, val);
	cpu->pc++;cpu->pc++;
}

void WrIndX(mos6502 *cpu) {
	DPRINTF(" %.2X @ %.4x", val, savepc);
	WriteRAM(savepc, val);
	cpu->pc++;
}

void WrIndY(mos6502 *cpu) {
	DPRINTF(" %.2X @ %.4x", val, savepc);
	WriteRAM(savepc, val);
	cpu->pc++;
}

void SAbs(mos6502 *cpu) {
	DPRINTF("%.2X %.2X %.2X", RAM[cpu->pc], RAM[cpu->pc+1], RAM[cpu->pc+2]);
	cycles = 4;
	savepc = RAM[cpu->pc+1] + (RAM[cpu->pc+1+1]<<8);
	val = RAM[savepc];
	cpu->pc++;cpu->pc++;
}

void SAbsX(mos6502 *cpu){
	DPRINTF("%.2X %.2X %.2X", RAM[cpu->pc], RAM[cpu->pc+1], RAM[cpu->pc+2]);
	cycles = 4;
	savepc = RAM[cpu->pc+1] + (RAM[cpu->pc+1+1]<<8) + cpu->x;
	cpu->pc++;cpu->pc++;
}

void SAbsY(mos6502 *cpu){
	DPRINTF("%.2X %.2X %.2X", RAM[cpu->pc], RAM[cpu->pc+1], RAM[cpu->pc+2]);
	cycles = 4;
	savepc = RAM[cpu->pc+1] + (RAM[cpu->pc+1+1]<<8) + cpu->y;
	cpu->pc++;cpu->pc++;
}

//Memory Modify Functions, needs cycles added
void MAcc(mos6502 *cpu) {
	cpu->a = val;
}

void MZp(mos6502 *cpu) {
	DPRINTF(" $%.2X = #$%.2X", savepc, val);
	RAM[savepc] = val;
}

void MZpX(mos6502 *cpu) {
	DPRINTF(" $%.2X = #$%.2X", savepc, val);
	RAM[savepc] = val;
}

void MZpY(mos6502 *cpu) {
	DPRINTF(" $%.2X = #$%.2X", savepc, val);
	RAM[savepc] = val;
}

void MAbs(mos6502 *cpu) {
	DPRINTF(" %.2X @ %.4x", val, savepc);
	WriteRAM(savepc, val);
}

void MAbsX(mos6502 *cpu) {
	DPRINTF(" %.2X @ %.4x", val, savepc);
	WriteRAM(savepc, val);
}

void MAbsY(mos6502 *cpu) {
	DPRINTF(" %.2X @ %.4x", val, savepc);
	WriteRAM(savepc, val);
}

void MIndX(mos6502 *cpu) {
	DPRINTF(" %.2X @ %.4x", val, savepc);
	WriteRAM(savepc, val);
}

void MIndY(mos6502 *cpu) {
	DPRINTF(" %.2X @ %.4x", val, savepc);
	WriteRAM(savepc, val);
}

//Operations

//ADC
void ADC(mos6502 *cpu) {
	DPRINTF("\tADC");

	saveP = (cpu->flags & 0x01);

	//thanks to blargg, from parodius for these lines:
	sum = ((signed char) cpu->a) + ((signed char)val) + saveP;

	if ((sum + 128) & 0x100) cpu->flags |= 0x40;
	else cpu->flags &= 0xbf;

	sum = cpu->a + saveP + val;

	//if overflow, set carry
	if (sum > 0xFF) cpu->flags |= 0x01;
	else cpu->flags &= 0xfe;

	cpu->a =  (sum & 0xFF);

	if (cpu->flags & 0x08) {
		/* Fuck if I know what this does, I think it's DEC code that nestest doesn't like

		   cpu->flags &= 0xfe;

		   if ((cpu->a & 0x0f)>0x09) A = A + 0x06;

		   if ((cpu->a & 0xf0)>0x90) {
		   A = A + 0x60;
		   cpu->flags |= 0x01;
		   }
		   */
	}else{
		cycles++;
	}

	chknegzero(cpu, cpu->a);
}

//AND
void AND(mos6502 *cpu) {
	DPRINTF("\tAND #$%.2X", val);
	cpu->a &= val;
	chknegzero(cpu, cpu->a);
}

//ASL
void ASL(mos6502 *cpu) {
	DPRINTF("\tASL");
	cpu->flags = (cpu->flags & 0xfe) | ((val >>7) & 0x01);
	val = val << 1;
	chknegzero(cpu, val);
}

//BIT
void BIT(mos6502 *cpu) {
	if (val & cpu->a) cpu->flags &= 0xfd;
	else cpu->flags |= 0x02;

	cpu->flags = (cpu->flags & 0x3f) | (val & 0xc0);

	DPRINTF("\tBIT $%.2X = #$%.2X", RAM[cpu->pc], val);
}

void mos6502_branch(mos6502 *cpu, enum mos6502_flags flag, bool condition) {
	cycles = 2;
	if (!(cpu->flags & flag) ^ condition) {
		cycles++;
		uint8_t page = (cpu->pc >> 8) & 0xFF;
		cpu->pc += (signed)cpu->ram[cpu->pc+1];
		if (page != ((cpu->pc >> 8) & 0xFF)) {
			cycles += 2;
		}
	}
	cpu->pc++;
}

//CMP
void CMP(mos6502 *cpu) {

	DPRINTF("\tCMP #$%.2X", val);

	if (cpu->a == val) cpu->flags |= 0x02;
	else cpu->flags &= 0xfd;

	if (cpu->a >= val) cpu->flags |= 0x01;
	else cpu->flags &= 0xfe;

	if ((cpu->a - (signed char)val) & 0x80) cpu->flags |= 0x80;
	else cpu->flags &= 0x7f;
}

//CPX
void CPX(mos6502 *cpu) {
	DPRINTF("\tCPX");

	if (cpu->x == val) cpu->flags |= 0x02;
	else cpu->flags &= 0xfd;

	if (cpu->x >= val) cpu->flags |= 0x01;
	else cpu->flags &= 0xfe;

	if ((cpu->x - (signed char)val) & 0x80) cpu->flags |= 0x80;
	else cpu->flags &= 0x7f;
}

//CPY
void CPY(mos6502 *cpu) {
	DPRINTF("\tCPY");

	//we use the helper variable
	helper = cpu->y - val;
	//if Y = cpu->pc+1, we set zero
	if (helper == 0) cpu->flags |= 0x02;
	else cpu->flags &= 0xfd;
	if (cpu->y+0x100 - val>0xff) cpu->flags |= 0x01;
	else cpu->flags &= 0xfe;
	//if Y < cpu->pc+1, set negative bit
	if (helper & 0x80) cpu->flags |= 0x80;
	else cpu->flags &= 0x7f;
}

//DEC
void DEC(mos6502 *cpu) {
	DPRINTF("\tDEC");
	val--;
	chknegzero(cpu, val);
}

//DEX
void DEX(mos6502 *cpu) {
	DPRINTF("\tDEX");
	cpu->x--;
	chknegzero(cpu, cpu->x);
	cycles=2;
}

//DEY
void DEY(mos6502 *cpu) {
	DPRINTF("\tDEY");
	cpu->y--;
	chknegzero(cpu, cpu->y);
	cycles=2;
}

//EOR
void EOR(mos6502 *cpu) {
	DPRINTF("\tEOR");
	cpu->a ^= val;
	chknegzero(cpu, cpu->a);
}

//INC
void INC(mos6502 *cpu) {
	DPRINTF("\tINC");
	val++;
	chknegzero(cpu, val);
}

//INX
void INX(mos6502 *cpu) {
	DPRINTF("\tINX");
	cpu->x++;
	chknegzero(cpu, cpu->x);
	cycles=2;
}

//INY
void INY(mos6502 *cpu) {
	DPRINTF("\tINY");
	cpu->y++;
	chknegzero(cpu, cpu->y);
	cycles=2;
}

//JMP
void JMP(mos6502 *cpu) {
	DPRINTF("\tJMP $%.2X%.2X", RAM[cpu->pc], RAM[cpu->pc-1]);

	cpu->pc = savepc;
	cpu->pc--;
	cycles = 3;
}

//JSR
void JSR(mos6502 *cpu) {
	savepc = cpu->pc + 0x2;
	RAM[0x0100+cpu->sp--]= ((savepc >> 8));
	RAM[0x0100+cpu->sp--]= ((savepc & 0xff));

	DPRINTF("%.2X %.2X %.2X \tJSR", RAM[cpu->pc], RAM[cpu->pc+1], RAM[cpu->pc+2]);
	DPRINTF(" $%.2X%.2X\t", RAM[cpu->pc+2], RAM[cpu->pc+1]);
	DPRINTF("Jump to Subroutine!");

	savepc = RAM[cpu->pc+1] + (RAM[cpu->pc + 2] << 8);
	savepc = savepc - 0x1;
	cpu->pc = savepc;
	cycles = 6;
}

//LDA
void LDA(mos6502 *cpu) {
	DPRINTF("\tLDA ");
	cpu->a = val;
	chknegzero(cpu, cpu->a);
}

//LDX
void LDX(mos6502 *cpu) {
	DPRINTF("\tLDX");
	cpu->x = val;
	chknegzero(cpu, cpu->x);
}

//LDY
void LDY(mos6502 *cpu) {
	DPRINTF("\tLDY");
	cpu->y = val;
	chknegzero(cpu, cpu->y);
}

//LSR
void LSR(mos6502 *cpu) {
	DPRINTF("\tLSR");
	cpu->flags = (cpu->flags & 0xfe) | (val & 0x01);
	val = val >> 1;
	chknegzero(cpu, val);
}

//ORA
void ORA(mos6502 *cpu) {
	DPRINTF("\tORA #$%.2X", val);
	cpu->a |= val;
	chknegzero(cpu, cpu->a);
}

//PHA
void PHA(mos6502 *cpu) {
	DPRINTF("\tPHA");
	RAM[0x0100+cpu->sp--] = cpu->a;
	cycles = 3;
}

//PHP
void PHP(mos6502 *cpu) {
	DPRINTF("%.2X\tPHP", RAM[cpu->pc]);
	val=cpu->flags;
	val |= 0x10;
	val |= 0x20;
	RAM[0x0100+cpu->sp--]=val;
	cycles = 3;
}

//PLA
void PLA(mos6502 *cpu) {
	DPRINTF("\tPLA");
	cpu->a = RAM[0x0100 + ++cpu->sp];
	chknegzero(cpu, cpu->a);
	cycles = 4;
}

//PLP
void PLP(mos6502 *cpu) {
	DPRINTF("\tPLP");
	cpu->flags = RAM[0x0100 + ++cpu->sp];
	cycles = 4;
}

//ROL
void ROL(mos6502 *cpu) {
	DPRINTF("\tROL");
	helper = (cpu->flags & 0x01);
	cpu->flags = (cpu->flags & 0xfe) | ((val >>7) & 0x01);
	val = val << 1;
	val |= helper;
	chknegzero(cpu, val);
}

//ROR
void ROR(mos6502 *cpu) {
	DPRINTF("\tROR");
	saveP = (cpu->flags & 0x01);
	cpu->flags = (cpu->flags & 0xfe) | (val & 0x01);
	val = val >> 1;
	if (saveP) val |= 0x80;
	chknegzero(cpu, val);
}

//RTI
void RTI(mos6502 *cpu) {
	DPRINTF("%.2X \tRTI", RAM[cpu->pc]);
	DPRINTF("Return from Interrupt!");
	cpu->flags = RAM[0x0100 + ++cpu->sp] | 0x20;
	savepc = RAM[0x0100 + ++cpu->sp];
	savepc |= (RAM[0x0100 + ++cpu->sp] << 8 );
	cpu->pc = savepc - 1;
}

//RTS
void RTS(mos6502 *cpu) {
	DPRINTF("%.2X \tRTS", RAM[cpu->pc]);
	DPRINTF("Return to Subroutine!");
	savepc = RAM[0x0100 + ++cpu->sp] ;
	savepc = savepc + (((RAM[0x0100 + ++cpu->sp])<< 8 ));
	cpu->pc = savepc;
	cycles = 6;
}

//SBC
void SBC(mos6502 *cpu) {
	DPRINTF("\tSBC");

	saveP = (cpu->flags & 0x01);

	//thanks to blargg, from parodius for these lines:
	sum = ((signed char) cpu->a) + (((signed char)val)) + (saveP << 4);

	sum = cpu->a + (val^0xff) + saveP;
	if ((cpu->a ^ sum) & (cpu->a ^ val) & 0x80) cpu->flags |= 0x40;
	else cpu->flags &= 0xbf;

	if (sum>0xff) cpu->flags |= 0x01; else cpu->flags &= 0xfe;

	cpu->a = sum;

	if ( cpu->flags & 0x08) {
		/*  A = A - 0x66;
			cpu->flags &= 0xfe;

			if ((cpu->a & 0x0f)>0x09) A = A + 0x06;

			if ((cpu->a & 0xf0)>0x90) {
			A = A + 0x60;
			cpu->flags |= 0x01;
			}
			*/
	} else {
		cycles++;
	}

	chknegzero(cpu, cpu->a);
}

//STA
void STA(mos6502 *cpu) {
	DPRINTF("\tSTA");
	val = cpu->a;
}

//STX
void STX(mos6502 *cpu) {
	DPRINTF("\tSTX");
	val = cpu->x;
}

//STY
void STY(mos6502 *cpu) {
	DPRINTF("\tSTY");
	val = cpu->y;
}

//TAX
void TAX(mos6502 *cpu) {
	DPRINTF("\tTAX");
	cpu->x = cpu->a;
	chknegzero(cpu, cpu->x);
	cycles = 2;
}

//TAY
void TAY(mos6502 *cpu) {
	DPRINTF("\tTAY");
	cpu->y = cpu->a;
	chknegzero(cpu, cpu->y);
	cycles = 2;
}

//TSX
void TSX(mos6502 *cpu) {
	DPRINTF("\tTSX");
	cpu->x = cpu->sp;
	chknegzero(cpu, cpu->x);
	cycles = 2;
}

//TXA
void TXA(mos6502 *cpu) {
	DPRINTF("\tTXA");
	cpu->a = cpu->x;
	chknegzero(cpu, cpu->a);
	cycles = 2;
}

//TXS
void TXS(mos6502 *cpu) {
	DPRINTF("\tTXS");
	cpu->sp = cpu->x;
	cycles = 2;
}

//TYA
void TYA(mos6502 *cpu) {
	DPRINTF("\tTYA");
	cpu->a = cpu->y;
	chknegzero(cpu, cpu->a);
	cycles = 2;
}

void BRK(mos6502 *cpu) {
			DPRINTF("%.2X \tBRK", RAM[cpu->pc]);
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

int mos6502_doop(mos6502 *cpu) {
	cycles = 0;
	uint8_t op = cpu->ram[cpu->pc];

	switch (op) {

			//Flags
		case 0x18: cpu->flags &= 0xFE; cycles = 2;break;
		case 0x58: cpu->flags &= 0xFB; cycles = 2;break;
		case 0xB8: cpu->flags &= 0xBF; cycles = 2;break;
		case 0xD8: cpu->flags &= 0xF7; cycles = 2;break;
		case 0x38: cpu->flags |= 0x01; cycles = 2;break;
		case 0x78: cpu->flags |= 0x04; cycles = 2;break;
		case 0xF8: cpu->flags |= 0x08; cycles = 2;break;

			//ADC
		case 0x69: RdImd(cpu);  ADC(cpu); break;
		case 0x65: RdZp(cpu);   ADC(cpu); break;
		case 0x75: RdZpX(cpu);  ADC(cpu); break;
		case 0x6D: RdAbs(cpu);  ADC(cpu); break;
		case 0x7D: RdAbsX(cpu); ADC(cpu); break;
		case 0x79: RdAbsY(cpu); ADC(cpu); break;
		case 0x61: RdIndX(cpu); ADC(cpu); break;
		case 0x71: RdIndY(cpu); ADC(cpu); break;

			//AND
		case 0x29: RdImd(cpu);  AND(cpu); break;
		case 0x25: RdZp(cpu);   AND(cpu); break;
		case 0x35: RdZpX(cpu);  AND(cpu); break;
		case 0x2D: RdAbs(cpu);  AND(cpu); break;
		case 0x3D: RdAbsX(cpu); AND(cpu); break;
		case 0x39: RdAbsY(cpu); AND(cpu); break;
		case 0x21: RdIndX(cpu); AND(cpu); break;
		case 0x31: RdIndY(cpu); AND(cpu); break;

			//ASL
		case 0x0A: RdAcc(cpu); ASL(cpu); MAcc(cpu);  break;
		case 0x06: RdZp(cpu);  ASL(cpu); MZp(cpu);   break;
		case 0x16: RdZpX(cpu); ASL(cpu); MZpX(cpu);  break;
		case 0x0E: SAbs(cpu);  ASL(cpu); MAbs(cpu);  break;
		case 0x1E: RdAbsX(cpu);ASL(cpu); MAbsX(cpu); break;

			//Branches
		case 0x90: mos6502_branch(cpu, FLAG_CARRY, false);	break; //BCC
		case 0xB0: mos6502_branch(cpu, FLAG_CARRY, true);	break; //BCS
		case 0xF0: mos6502_branch(cpu, FLAG_ZERO, true);	break;
		case 0x30: mos6502_branch(cpu, FLAG_NEG, true);		break;
		case 0xD0: mos6502_branch(cpu, FLAG_ZERO, false);	break;
		case 0x10: mos6502_branch(cpu, FLAG_NEG, false);	break;
		case 0x50: mos6502_branch(cpu, FLAG_OVER, false);	break;
		case 0x70: mos6502_branch(cpu, FLAG_OVER, true);	break;

			//BIT
		case 0x24: RdZp(cpu);  BIT(cpu); break;
		case 0x2C: RdAbs(cpu); BIT(cpu); break;
			
		case 0x00: BRK(cpu); break;

				   //CMP
		case 0xC9: RdImd(cpu);  CMP(cpu); break;
		case 0xC5: RdZp(cpu);   CMP(cpu); break;
		case 0xD5: RdZpX(cpu);  CMP(cpu); break;
		case 0xCD: RdAbs(cpu);  CMP(cpu); break;
		case 0xDD: RdAbsX(cpu); CMP(cpu); break;
		case 0xD9: RdAbsY(cpu); CMP(cpu); break;
		case 0xC1: RdIndX(cpu); CMP(cpu); break;
		case 0xD1: RdIndY(cpu); CMP(cpu); break;

				   //CPX
		case 0xE0: RdImd(cpu); CPX(cpu); break;
		case 0xE4: RdZp(cpu);  CPX(cpu); break;
		case 0xEC: RdAbs(cpu); CPX(cpu); break;

				   //CPY
		case 0xC0: RdImd(cpu); CPY(cpu); break;
		case 0xC4: RdZp(cpu);  CPY(cpu); break;
		case 0xCC: RdAbs(cpu); CPY(cpu); break;

				   //DEC
		case 0xC6: RdZp(cpu);  DEC(cpu); MZp(cpu);   break;
		case 0xD6: RdZpX(cpu); DEC(cpu); MZpX(cpu);  break;
		case 0xCE: SAbs(cpu);  DEC(cpu); MAbs(cpu);  break;
		case 0xDE: RdAbsX(cpu);DEC(cpu); MAbsX(cpu); break;

				   //DEX
		case 0xCA: DEX(cpu); break;

				   //DEY
		case 0x88: DEY(cpu); break;

				   //EOR
		case 0x49: RdImd(cpu);  EOR(cpu); break;
		case 0x45: RdZp(cpu);   EOR(cpu); break;
		case 0x55: RdZpX(cpu);  EOR(cpu); break;
		case 0x4D: RdAbs(cpu);  EOR(cpu); break;
		case 0x5D: RdAbsX(cpu); EOR(cpu); break;
		case 0x59: RdAbsY(cpu); EOR(cpu); break;
		case 0x41: RdIndX(cpu); EOR(cpu); break;
		case 0x51: RdIndY(cpu); EOR(cpu); break;

				   //INC
		case 0xE6: RdZp(cpu);   INC(cpu); MZp(cpu);   break;
		case 0xF6: RdZpX(cpu);  INC(cpu); MZpX(cpu);  break;
		case 0xEE: SAbs(cpu);   INC(cpu); MAbs(cpu);  break;
		case 0xFE: RdAbsX(cpu); INC(cpu); MAbsX(cpu); break;

				   //INX
		case 0xE8: INX(cpu); break;

				   //INY
		case 0xC8: INY(cpu); break;

				   //JMP
		case 0x4C: SAbs(cpu);  JMP(cpu); break;
		case 0x6C: RdInd(cpu); JMP(cpu); break;

				   //JSR
		case 0x20: JSR(cpu); break;

				   //LDA
		case 0xA9: RdImd(cpu); LDA(cpu); break;
		case 0xA5: RdZp(cpu);  LDA(cpu); break;
		case 0xB5: RdZpX(cpu); LDA(cpu); break;
		case 0xAD: RdAbs(cpu); LDA(cpu); break;
		case 0xBD: RdAbsX(cpu);LDA(cpu); break;
		case 0xB9: RdAbsY(cpu);LDA(cpu); break;
		case 0xA1: RdIndX(cpu);LDA(cpu); break;
		case 0xB1: RdIndY(cpu);LDA(cpu); break;

				   //LDX
		case 0xA2: RdImd(cpu); LDX(cpu); break;
		case 0xA6: RdZp(cpu);  LDX(cpu); break;
		case 0xB6: RdZpY(cpu); LDX(cpu); break;
		case 0xAE: RdAbs(cpu); LDX(cpu); break;
		case 0xBE: RdAbsY(cpu);LDX(cpu); break;

				   //LDY
		case 0xA0: RdImd(cpu); LDY(cpu); break;
		case 0xA4: RdZp(cpu);  LDY(cpu); break;
		case 0xB4: RdZpX(cpu); LDY(cpu); break;
		case 0xAC: RdAbs(cpu); LDY(cpu); break;
		case 0xBC: RdAbsX(cpu);LDY(cpu); break;

				   //LSR
		case 0x4A: RdAcc(cpu);  LSR(cpu); WrAcc(cpu); break;
		case 0x46: RdZp(cpu);   LSR(cpu); MZp(cpu);   break;
		case 0x56: RdZpX(cpu);  LSR(cpu); MZpX(cpu);  break;
		case 0x4E: SAbs(cpu);   LSR(cpu); MAbs(cpu);  break;
		case 0x5E: RdAbsX(cpu); LSR(cpu); MAbsX(cpu); break;


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
		case 0x09: RdImd(cpu);  ORA(cpu); break;
		case 0x05: RdZp(cpu);   ORA(cpu); break;
		case 0x15: RdZpX(cpu);  ORA(cpu); break;
		case 0x0D: RdAbs(cpu);  ORA(cpu); break;
		case 0x1D: RdAbsX(cpu); ORA(cpu); break;
		case 0x19: RdAbsY(cpu); ORA(cpu); break;
		case 0x01: RdIndX(cpu); ORA(cpu); break;
		case 0x11: RdIndY(cpu); ORA(cpu); break;

				   //PHA
		case 0x48: PHA(cpu); break;

				   //PHP
		case 0x08: PHP(cpu); break;

				   //PLA
		case 0x68: PLA(cpu); break;

				   //PLP
		case 0x28: PLP(cpu); break;

				   //ROL
		case 0x2A: RdAcc(cpu);  ROL(cpu); WrAcc(cpu); break;
		case 0x26: RdZp(cpu);   ROL(cpu); MZp(cpu);   break;
		case 0x36: RdZpX(cpu);  ROL(cpu); MZpX(cpu);  break;
		case 0x2E: SAbs(cpu);   ROL(cpu); MAbs(cpu);  break;
		case 0x3E: RdAbsX(cpu); ROL(cpu); MAbsX(cpu); break;

				   //ROR
		case 0x6A: RdAcc(cpu);  ROR(cpu); WrAcc(cpu); break;
		case 0x66: RdZp(cpu);   ROR(cpu); MZp(cpu);   break;
		case 0x76: RdZpX(cpu);  ROR(cpu); MZpX(cpu);  break;
		case 0x6E: SAbs(cpu);   ROR(cpu); MAbs(cpu);  break;
		case 0x7E: RdAbsX(cpu); ROR(cpu); MAbsX(cpu); break;

				   //RTI
		case 0x40: RTI(cpu); break;

				   //RTS
		case 0x60: RTS(cpu); break;

				   //SBC
		case 0xE9: RdImd(cpu);  SBC(cpu); break;
		case 0xe5: RdZp(cpu);   SBC(cpu); break;
		case 0xF5: RdZpX(cpu);  SBC(cpu); break;
		case 0xED: RdAbs(cpu);  SBC(cpu); break;
		case 0xFD: RdAbsX(cpu); SBC(cpu); break;
		case 0xF9: RdAbsY(cpu); SBC(cpu); break;
		case 0xE1: RdIndX(cpu); SBC(cpu); break;
		case 0xF1: RdIndY(cpu); SBC(cpu); break;

				   //STA
		case 0x85: RdZp(cpu);   STA(cpu); MZp(cpu);   break;
		case 0x95: RdZpX(cpu);  STA(cpu); MZpX(cpu);  break;
		case 0x8D: SAbs(cpu);   STA(cpu); MAbs(cpu);  break;
		case 0x9D: RdAbsX(cpu); STA(cpu); MAbsX(cpu); break;
		case 0x99: RdAbsY(cpu); STA(cpu); MAbsY(cpu); break;
		case 0x81: RdIndX(cpu); STA(cpu); MIndX(cpu); break;
		case 0x91: RdIndY(cpu); STA(cpu); MIndY(cpu); break;

				   //STX
		case 0x86: RdZp(cpu);  STX(cpu); MZp(cpu);  break;
		case 0x96: RdZpY(cpu); STX(cpu); MZpY(cpu); break;
		case 0x8E: SAbs(cpu);  STX(cpu); MAbs(cpu); break;

				   //STY
		case 0x84: RdZp(cpu);  STY(cpu); MZp(cpu);  break;
		case 0x94: RdZpX(cpu); STY(cpu); MZpX(cpu); break;
		case 0x8C: SAbs(cpu);  STY(cpu); MAbs(cpu); break;

				   //TAX
		case 0xAA: TAX(cpu); break;

				   //TAY
		case 0xA8: TAY(cpu); break;

				   //TSX
		case 0xBA: TSX(cpu); break;

				   //TXA
		case 0x8A: TXA(cpu); break;

				   //TXS
		case 0x9A: TXS(cpu); break;

				   //TYA
		case 0x98: TYA(cpu); break;

		default:
			printf("%.2X \tUnknown Opcode!\n", op);
			printf("nestest: %.2X %.2X %.2X %.2X\n", RAM[0], RAM[1], RAM[2], RAM[3]);
			exit(EXIT_FAILURE);
	}

	cpu->pc++;

	cpu->cycles += cycles;
	return cycles;
}

#include "b6502_ops.h"
int mos6502_logger(mos6502 *cpu) {
	int ret = 0;
	static int lcycles = 0;
	uint8_t opcode = cpu->ram[cpu->pc];
	struct op *op = &ops[opcode];

	fprintf(stderr, "%.4X  ", cpu->pc);

	switch(op->bytes) {
		case 1:
			fprintf(stderr, "%.2X        ", opcode);
			break;
		case 2:
			fprintf(stderr, "%.2X %.2X     ", opcode, cpu->ram[cpu->pc + 1]);
			break;
		case 3:
			fprintf(stderr, "%.2X %.2X %.2X  ",
				   	opcode, cpu->ram[cpu->pc + 1], cpu->ram[cpu->pc + 2]);
	}

	fprintf(stderr, "%s ", op->name);

	switch(op->mode) {
		case MODE_ABS:
			fprintf(stderr, "$%.2X%.2X   ", cpu->ram[cpu->pc + 2], cpu->ram[cpu->pc + 1]);
			break;
		case MODE_IMD:
			fprintf(stderr, "#$%.2X    ", cpu->ram[cpu->pc+1]);
			break;
		case MODE_REL:
			fprintf(stderr,"$%.4X   ", cpu->pc + 2 + (signed)cpu->ram[cpu->pc+1]);
			break;
		case MODE_IMP:
			fprintf(stderr, "          ");
			break;
		case MODE_ZP:
			{
			uint8_t zp = cpu->ram[cpu->pc + 1];
			fprintf(stderr, "$%.2X = %.2X", zp, cpu->ram[zp]);
			}
			break;
		default:
			fprintf(stderr, "Unimplemented mode %i\n", op->mode);
			exit(EXIT_FAILURE);
			//fprintf(stderr, "%2i        ", op->mode);
	}

	fprintf(stderr, "\t\t\t\t\tA:%.2X X:%.2X Y:%.2X P:%.2X SP:%.2X CYC:%3i\n",
			cpu->a, cpu->x, cpu->y, cpu->flags, cpu->sp, lcycles);

	ret = mos6502_doop(cpu);

	lcycles += ret * 3;
	if (lcycles > 341) {
		lcycles -= 341;
	}
	
	return ret;
}
