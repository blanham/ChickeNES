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

#include <stdint.h>
#include "main.h"
#include "B6502.h"

#include "int2bin.h"

#ifdef DEBUG
# define DPRINTF(...) do { fprintf(stderr, __VA_ARGS__);} while (0)
#else
# define DPRINTF(...) 
#endif

struct  cpu {
	uint8_t a;
	uint8_t x;
	uint8_t y;

	uint8_t sp;
	uint16_t pc;
	uint8_t flags;//Might decompose this into bools
};

//counting variable
int i;
int sum;
//helper variables
uint8_t helper = 0;
uint8_t val = 0;
//cycle counter
int cycles = 0;
//need to define flags
// one variable processed with bitwise ops
uint8_t P;
//6502 Program counter, indicates current address
uint16_t PC;
//Program counter save variables
//supersavepc is used when we have to juggle counters
//should consider rewriting those functions that use it
//but probably won't matter as the core is fast enough anyway
uint16_t savepc, supersavepc;
//for signed operations
int8_t pSigned;
//some functions need to save P
uint8_t saveP;
//establish registers
uint8_t A,X,Y=0;
//beginning address of the Stack ($0100-$01FFF)
uint8_t STACK = 0xFF;


struct cpu *cpu_alloc()
{
	struct cpu *ret = calloc(sizeof(*ret), 1);
	return ret;
}


void cpu_reset(struct cpu *cpu)
{

}

void initCPU(void)
{
	//read sarting vector from 0x7ffd
	PC = RAM[0xfffc] + (RAM[0xfffd]<<8);
	printf("Starting execution at: %.4x\n", PC);
	//set CPU flags and registers clear
	A=X=Y=P=0;
	P |= 0x4;
}


//would probably be better as a macro
void chknegzero(uint8_t chk) {
	if (chk & 0x80) P |= 0x80;
	else P &= 0x7f;

	if (chk == 0) P |= 0x02;
	else P &= 0xfd;
}

char *printstats[2]={"hello, PC:%.4x","PC"};


void pushstack() {
	RAM[0x0100+STACK]=PC;
	STACK--;
	RAM[0x0100+STACK]=PC+1;
	STACK--;
	RAM[0x0100+STACK]=P;
	STACK--;
}

//Memory Read Functions
void RdImd(void) {
	DPRINTF("%.2x %.2x", RAM[PC], RAM[PC+1]);
	val = RAM[PC+1];
	cycles = 2;
	PC++;
}

void RdAcc(void) {
	DPRINTF("%.2x", RAM[PC]);
	val = A;
	cycles = 2;
}

void RdZp(void) {
	DPRINTF("%.2x %.2x", RAM[PC], RAM[PC+1]);
	savepc = RAM[PC+1];
	val = RAM[savepc];
	cycles = 3;
	PC++;
}

void RdZpX(void) {
	DPRINTF("%.2x %.2x", RAM[PC], RAM[PC+1]);
	savepc = RAM[PC+1]+X;
	savepc &= 0x00ff;
	val = RAM[savepc];
	PC++;
	cycles = 4;
}

void RdZpY(void) {
	DPRINTF("%.2x %.2x", RAM[PC], RAM[PC+1]);
	savepc = RAM[PC+1]+Y;
	savepc &= 0x00ff;
	val = RAM[savepc];
	PC++;
	cycles = 4;
}

void RdAbs(void) {
	DPRINTF("%.2x %.2x %.2x", RAM[PC], RAM[PC+1], RAM[PC+2]);
	savepc = RAM[PC+1] + (RAM[PC+1+1]<<8);
	val = ReadRAM(savepc);
	PC += 2;
	cycles = 4;
}

void RdAbsX(void) {
	DPRINTF("%.2x %.2x %.2x", RAM[PC], RAM[PC+1], RAM[PC+2]);
	savepc = RAM[PC+1] + (RAM[PC+1+1]<<8);
	PC += 2;
	cycles = 4;
	if ((savepc>>8) != ((savepc+X)>>8)) {
		cycles++;
	}
	savepc +=X;
	val = ReadRAM(savepc);
}

void RdAbsY(void) {
	DPRINTF("%.2x %.2x %.2x", RAM[PC], RAM[PC+1], RAM[PC+2]);
	savepc = RAM[PC+1] + (RAM[PC+1+1]<<8);
	PC += 2;
	cycles = 4;
	if ((savepc>>8) != ((savepc+Y)>>8))
		cycles++;
	savepc +=Y;
	val = ReadRAM(savepc);
}

void RdInd(void) {
	DPRINTF("%.2x %.2x %.2x \tJMP", RAM[PC], RAM[PC+1], RAM[PC+2]);
	DPRINTF(" $(%.2x%.2x)", RAM[PC+2], RAM[PC+1]);

	/*  savepc = RAM[PC+1] + (RAM[PC+2] << 8);

		helper = RAM[savepc];

		if (RAM[PC+1]==0xFF){
		DPRINTF("%.4X",savepc);
		savepc -= 0xFF;
		helper = helper + (RAM[savepc+1] << 8);
		DPRINTF("%.4X",savepc);
		}
		else {helper = helper + (RAM[savepc+1] << 8); }
		*/
	savepc = RAM[PC+1] + (RAM[PC+2] << 8);
	supersavepc = RAM[savepc];
	if(RAM[PC+1]==0xFF){
		savepc &= 0xFF00;
		savepc -= 1;
	}
	supersavepc |= (RAM[savepc +1] << 8);
	//DPRINTF("%.4X\n", RAM[savepc] );

	savepc = supersavepc;

	cycles = 5;
}

void RdIndX(void) {
	DPRINTF("%.2x %.2x", RAM[PC], RAM[PC+1]);

	val = RAM[PC+1]+X;

	savepc = RAM[val];
	savepc |= (RAM[++val] << 8);

	val = RAM[savepc];
	PC++;
	cycles = 6;
}

void RdIndY(void) {
	DPRINTF("%.2x %.2x", RAM[PC], RAM[PC+1]);

	cycles = 5;
	val = RAM[PC+1];
	savepc = RAM[val];

	savepc |= (RAM[++val] << 8);
	savepc +=Y;

	val = RAM[savepc];
	PC++;
}

//Memory Write Functions, needs cycles added

void WrAcc(void) {
	DPRINTF("\n");
	A=val;
}

void WrZp(void) {
	DPRINTF(" $%.2x = #$%.2X\n", savepc, val);
	RAM[savepc] = val;
	PC++;
}

void WrZpX(void) {
	DPRINTF(" $%.2X = #$%.2X\n", savepc, val);
	RAM[savepc] = val;
	PC++;
}

void WrZpY(void) {
	RAM[savepc] = val;

	DPRINTF(" $%.2x = #$%.2X\n", savepc, val);

	PC++;
}

void WrAbs(void) {
	DPRINTF(" %.2x @ %.4x\n", val, savepc);
	WriteRAM(savepc, val);
	PC++;PC++;
}

void WrAbsX(void) {
	DPRINTF(" %.2x @ %.4x\n", val, savepc);
	WriteRAM(savepc, val);
	PC++;PC++;
}

void WrAbsY(void) {
	DPRINTF(" %.2x @ %.4x\n", val, savepc);
	WriteRAM(savepc, val);
	PC++;PC++;
}

void WrIndX(void) {
	DPRINTF(" %.2x @ %.4x\n", val, savepc);
	WriteRAM(savepc, val);
	PC++;
}

void WrIndY(void) {
	DPRINTF(" %.2x @ %.4x\n", val, savepc);
	WriteRAM(savepc, val);
	PC++;
}

void SAbs(void) {
	DPRINTF("%.2x %.2x %.2x", RAM[PC], RAM[PC+1], RAM[PC+2]);
	cycles = 4;
	savepc = RAM[PC+1] + (RAM[PC+1+1]<<8);
	val = RAM[savepc];
	PC++;PC++;
}

void SAbsX(void){
	DPRINTF("%.2x %.2x %.2x", RAM[PC], RAM[PC+1], RAM[PC+2]);
	cycles = 4;
	savepc = RAM[PC+1] + (RAM[PC+1+1]<<8) + X;
	PC++;PC++;
}

void SAbsY(void){
	DPRINTF("%.2x %.2x %.2x", RAM[PC], RAM[PC+1], RAM[PC+2]);
	cycles = 4;
	savepc = RAM[PC+1] + (RAM[PC+1+1]<<8) + Y;
	PC++;PC++;
}

//Memory Modify Functions, needs cycles added
void MAcc(void) {
	A=val;
}

void MZp(void) {
	DPRINTF(" $%.2x = #$%.2x\n", savepc, val);
	RAM[savepc] = val;
}

void MZpX(void) {
	DPRINTF(" $%.2x = #$%.2x\n", savepc, val);
	RAM[savepc] = val;
}

void MZpY(void) {
	DPRINTF(" $%.2x = #$%.2x\n", savepc, val);
	RAM[savepc] = val;
}

void MAbs(void) {
	DPRINTF(" %.2x @ %.4x\n", val, savepc);
	WriteRAM(savepc, val);
}

void MAbsX(void) {
	DPRINTF(" %.2x @ %.4x\n", val, savepc);
	WriteRAM(savepc, val);
}

void MAbsY(void) {
	DPRINTF(" %.2x @ %.4x\n", val, savepc);
	WriteRAM(savepc, val);
}

void MIndX(void) {
	DPRINTF(" %.2x @ %.4x\n", val, savepc);
	WriteRAM(savepc, val);
}

void MIndY(void) {
	DPRINTF(" %.2x @ %.4x\n", val, savepc);
	WriteRAM(savepc, val);
}

//Operations

//ADC
void ADC(void) {
	DPRINTF("\tADC\n");

	saveP = (P & 0x01);

	//thanks to blargg, from parodius for these lines:
	sum = ((signed char) A) + ((signed char)val) + saveP;

	if ((sum + 128) & 0x100) P |= 0x40;
	else P &= 0xbf;

	sum = A + saveP + val;

	//if overflow, set carry
	if (sum > 0xFF) P |= 0x01;
	else P &= 0xfe;

	A =  (sum & 0xFF);

	if ( P & 0x08) {
		/* Fuck if I know what this does, I think it's DEC code that nestest doesn't like

		   P &= 0xfe;

		   if ((A & 0x0f)>0x09) A = A + 0x06;

		   if ((A & 0xf0)>0x90) {
		   A = A + 0x60;
		   P |= 0x01;
		   }
		   */
	}else{
		cycles++;
	}

	chknegzero(A);
}

//AND
void AND(void) {
	DPRINTF("\tAND #$%.2X\n", val);
	A &= val;
	chknegzero(A);
}


//ASL
void ASL(void) {
	DPRINTF("\tASL\n");
	P = (P & 0xfe) | ((val >>7) & 0x01);
	val = val << 1;
	chknegzero(val);
}

//Branches

//BCC
void BCC(void) {

	if ((RAM[PC+1] & 0x80)) {
		pSigned = (signed char)RAM[PC+1];
	} else {
		pSigned = RAM[PC+1];
	}

	DPRINTF("%.2x %.2x %.2x", RAM[PC], RAM[PC+1], RAM[PC+2]);
	DPRINTF("\tBCC");
	DPRINTF(" $%.4x\n", (PC + 2) + pSigned);

	if ((P & 0x01)==0){
		PC = PC + pSigned + 1;

		DPRINTF("Branch!\n");

	} else {
		PC++;
	}

	cycles = 2;
}

//BCS
void BCS(void) {

	//converts unsigned to signed
	if ((RAM[PC+1] & 0x80)) {
		pSigned = RAM[PC+1] - 0x100;
	} else
		pSigned = RAM[PC+1];

	DPRINTF("%.2x %.2x %.2x", RAM[PC], RAM[PC+1], RAM[PC+2]);
	DPRINTF("\tBCS");
	DPRINTF(" $%.4x\n", (PC + 2) + pSigned);

	if (P & 0x01) {
		PC = PC + pSigned;
		PC++;

		DPRINTF("Branch!\n");

	} else
		PC++;

	cycles = 2;
}

//BEQ
void BEQ(void) {

	if ((RAM[PC+1] & 0x80)) {
		pSigned = RAM[PC+1] - 0x100;
	} else pSigned = RAM[PC+1];


	DPRINTF("%.2x %.2x %.2x", RAM[PC], RAM[PC+1], RAM[PC+2]);
	DPRINTF("\tBEQ");

	DPRINTF(" $%.4x\n", (PC + 2) + pSigned);


	if (P & 0x02) {
		PC = PC + pSigned;
		cycles = 3;

		DPRINTF("Branch!\n");

		PC++;
	} else PC++;

	cycles = 2;
}

//BIT
void BIT(void) {

	if (val & A) P &= 0xfd;
	else P |= 0x02;

	P = (P & 0x3f) | (val & 0xc0);


	DPRINTF("\tBIT");

	DPRINTF(" $%.2X = #$%.2X\n", RAM[PC], val);


}

//BMI
void BMI(void) {

	cycles = 2;
	//converts unsigned to signed

	if ((RAM[PC+1] & 0x80)) {
		pSigned = RAM[PC+1] - 0x100;
	} else
		pSigned = RAM[PC+1];


	DPRINTF("%.2x %.2x %.2x", RAM[PC], RAM[PC+1], RAM[PC+2]);
	DPRINTF("\tBMI");

	DPRINTF(" $%.4x\n", (PC + 2) + pSigned);


	if ((RAM[PC+1]>>8) != (PC>>8))
		cycles++;

	if ((P & 0x80)) {

		DPRINTF("Branch!\n");

		PC = PC + pSigned;
		++cycles;
		PC++;
	} else

		PC++;

}

//BNE
void BNE(void) {

	cycles =2 ;
	//converts unsigned to signed

	if ((RAM[PC+1] & 0x80)) {
		pSigned = RAM[PC+1] - 0x100;
	} else
		pSigned = RAM[PC+1];


	DPRINTF("%.2x %.2x %.2x", RAM[PC], RAM[PC+1], RAM[PC+2]);
	DPRINTF("\tBNE");

	DPRINTF(" $%.4x\n", (PC + 2) + pSigned);


	if ((RAM[PC+1]>>8) != (PC>>8))
		cycles = cycles + 2;

	if ((P & 0x02)==0) {

		DPRINTF("Branch!\n");


		PC = PC + pSigned;
		++cycles;
		PC++;
	} else

		PC++;

}

//BPL
void BPL(void) {

	//converts unsigned to signed
	if ((RAM[PC+1] & 0x80))pSigned = (signed char)RAM[PC+1];
	else pSigned = RAM[PC+1];

	cycles = 2;


	DPRINTF("%.2x %.2x \tBPL", RAM[PC], RAM[PC+1]);

	DPRINTF(" $%.4x\n", PC + 2 + pSigned);


	if (P & 0x80) {

		PC++;
	} else {
		PC = PC + 2 + pSigned -1;


		DPRINTF("Branch!\n");

	}
}

//BVC
void BVC(void) {

	if ((RAM[PC+1] & 0x80))pSigned = (signed char)RAM[PC+1];
	else pSigned = RAM[PC+1];

	cycles = 2;


	DPRINTF("%.2x %.2x \tBVC", RAM[PC], RAM[PC+1]);

	DPRINTF(" $%.4x\n", PC + 2 + pSigned);


	if (P & 0x40) {
		PC++;
	} else {
		PC = PC + 2 + pSigned -1;

		DPRINTF("Branch!\n");
	}
}

//BVS
void BVS(void) {

	//converts unsigned to signed
	if ((RAM[PC+1] & 0x80))pSigned = (signed char)RAM[PC+1];
	else pSigned = RAM[PC+1];

	cycles = 2;


	DPRINTF("%.2x %.2x \tBVS", RAM[PC], RAM[PC+1]);
	DPRINTF(" $%.4x\n", PC + 2 + pSigned);

	if (P & 0x40) {
		PC = PC + 2 + pSigned -1;

		DPRINTF("Branch!\n");

	} else {
		PC++;
	}

}

//CMP
void CMP(void) {

	DPRINTF("\tCMP #$%.2X\n", val);

	if (A == val) P |= 0x02;
	else P &= 0xfd;

	if (A >= val) P |= 0x01;
	else P &= 0xfe;

	if ((A - (signed char)val) & 0x80) P |= 0x80;
	else P &= 0x7f;
}

//CPX
void CPX(void) {

	DPRINTF("\tCPX\n");

	if (X == val) P |= 0x02;
	else P &= 0xfd;

	if (X >= val) P |= 0x01;
	else P &= 0xfe;

	if ((X - (signed char)val) & 0x80) P |= 0x80;
	else P &= 0x7f;
}

//CPY
void CPY(void) {

	DPRINTF("\tCPY\n");

	//we use the helper variable
	helper = Y - val;
	//if Y = PC+1, we set zero
	if (helper == 0) P |= 0x02;
	else P &= 0xfd;
	if (Y+0x100 - val>0xff) P |= 0x01;
	else P &= 0xfe;
	//if Y < PC+1, set negative bit
	if (helper & 0x80) P |= 0x80;
	else P &= 0x7f;
}

//DEC
void DEC(void) {

	DPRINTF("\tDEC");

	val--;
	chknegzero(val);
}

//DEX
void DEX(void) {
	DPRINTF("\tDEX\n");

	X--;
	chknegzero(X);
	cycles=2;
}

//DEY
void DEY(void) {
	DPRINTF("\tDEY\n");

	Y--;
	chknegzero(Y);
	cycles=2;
}

//EOR
void EOR(void) {
	DPRINTF("\tEOR\n");

	A ^= val;
	chknegzero(A);
}

//INC
void INC(void) {

	DPRINTF("\tINC\n");

	val++;
	chknegzero(val);
}

//INX
void INX(void) {
	DPRINTF("\tINX\n");

	X++;
	chknegzero(X);
	cycles=2;
}

//INY
void INY(void) {
	DPRINTF("\tINY\n");

	Y++;
	chknegzero(Y);
	cycles=2;
}

//JMP
void JMP(void) {
	DPRINTF("\tJMP");
	DPRINTF(" $%.2x%.2x\n", RAM[PC], RAM[PC-1]);

	PC = savepc;
	PC--;
	cycles = 5;
}

//JSR
void JSR(void) {
	savepc = PC + 0x2;
	RAM[0x0100+STACK--]= ((savepc >> 8));
	RAM[0x0100+STACK--]= ((savepc & 0xff));

	DPRINTF("%.2x %.2x %.2x \tJSR", RAM[PC], RAM[PC+1], RAM[PC+2]);
	DPRINTF(" $%.2X%.2X\t", RAM[PC+2], RAM[PC+1]);
	DPRINTF("Jump to Subroutine!\n");

	savepc = RAM[PC+1] + (RAM[PC + 2] << 8);
	savepc = savepc - 0x1;
	PC = savepc;
	cycles = 6;
}

//LDA
void LDA(void) {
	DPRINTF("\tLDA ");
	A = val;
	chknegzero(A);
}

//LDX
void LDX(void) {
	DPRINTF("\tLDX\n");
	X = val;
	chknegzero(X);
}

//LDY
void LDY(void) {
	DPRINTF("\tLDY\n");
	Y = val;
	chknegzero(Y);
}

//LSR
void LSR(void) {
	DPRINTF("\tLSR\n");
	P = (P & 0xfe) | (val & 0x01);
	val = val >> 1;
	chknegzero(val);
}

//ORA
void ORA(void) {
	DPRINTF("\tORA #$%.2X\n", val);
	A |= val;
	chknegzero(A);
}

//PHA
void PHA(void) {
	DPRINTF("\tPHA\n");
	RAM[0x0100+STACK--] = A;
	cycles = 3;
}

//PHP
void PHP(void) {
	DPRINTF("%.2x\tPHP\n", RAM[PC]);
	val=P;
	val |= 0x10;
	val |= 0x20;
	RAM[0x0100+STACK--]=val;
	cycles = 3;
}

//PLA
void PLA(void) {
	DPRINTF("\tPLA\n");
	A = RAM[0x0100 + ++STACK];
	chknegzero(A);
	cycles = 4;
}

//PLP
void PLP(void) {
	DPRINTF("\tPLP\n");
	P = RAM[0x0100 + ++STACK];
	cycles = 4;
}

//ROL
void ROL(void) {
	DPRINTF("\tROL\n");
	helper = (P & 0x01);
	P = (P & 0xfe) | ((val >>7) & 0x01);
	val = val << 1;
	val |= helper;
	chknegzero(val);
}

//ROR
void ROR(void) {

	DPRINTF("\tROR");

	saveP = (P & 0x01);
	P = (P & 0xfe) | (val & 0x01);
	val = val >> 1;
	if (saveP) val |= 0x80;
	chknegzero(val);
}

//RTI
void RTI(void) {
	DPRINTF("%.2x \tRTI\n", RAM[PC]);
	DPRINTF("\n\nReturn from Interrupt!\n\n");
	P = RAM[0x0100 + ++STACK] | 0x20;
	savepc = RAM[0x0100 + ++STACK];
	savepc |= (RAM[0x0100 + ++STACK] << 8 );
	PC = savepc - 1;
}

//RTS
void RTS(void) {
	DPRINTF("%.2x \tRTS\n", RAM[PC]);
	DPRINTF("\n\nReturn to Subroutine!\n\n");
	savepc = RAM[0x0100 + ++STACK] ;
	savepc = savepc + (((RAM[0x0100 + ++STACK])<< 8 ));
	PC = savepc;
	cycles = 6;
}

//SBC
void SBC(void) {
	DPRINTF("\tSBC\n");

	saveP = (P & 0x01);

	//thanks to blargg, from parodius for these lines:
	sum = ((signed char) A) + (((signed char)val)) + (saveP << 4);

	sum = A + (val^0xff) + saveP;
	if ((A ^ sum) & (A ^ val) & 0x80) P |= 0x40;
	else P &= 0xbf;

	if (sum>0xff) P |= 0x01; else P &= 0xfe;

	A =  sum;

	if ( P & 0x08) {
		/*  A = A - 0x66;
			P &= 0xfe;

			if ((A & 0x0f)>0x09) A = A + 0x06;

			if ((A & 0xf0)>0x90) {
			A = A + 0x60;
			P |= 0x01;
			}
			*/
	} else {
		cycles++;
	}

	chknegzero(A);
}

//STA
void STA(void) {
	DPRINTF("\tSTA");
	val = A;
}

//STX
void STX(void) {
	DPRINTF("\tSTX");
	val = X;
}

//STY
void STY(void) {
	DPRINTF("\tSTY");
	val=Y;
}

//TAX
void TAX(void) {
	DPRINTF("\tTAX\n");
	X = A;
	chknegzero(X);
	cycles=2;
}

//TAY
void TAY(void) {
	DPRINTF("\tTAY\n");
	Y = A;
	chknegzero(Y);
	cycles=2;
}

//TSX
void TSX(void) {
	DPRINTF("\tTSX\n");
	X = STACK;
	chknegzero(X);
	cycles = 2;
}

//TXA
void TXA(void) {
	DPRINTF("\tTXA\n");
	A = X;
	chknegzero(A);
	cycles = 2;
}

//TXS
void TXS(void) {
	DPRINTF("\tTXS\n");
	STACK = X;
	cycles = 2;
}

//TYA
void TYA(void) {
	DPRINTF("\tTYA\n");
	A = Y;
	chknegzero(A);
	cycles = 2;
}

int DoOP(int op) {

	cycles = 0;

	DPRINTF("$%.4X:", PC);

	switch (op) {
			//BRK, 7 cycles, 1 byte, needs to push PC+2 and P into stack
		case 0x0:
			DPRINTF("%.2x \tBRK\n", RAM[PC]);
			PC+=2;
			pushstack();
			PC -=2;
			P |= 0x10;
			cycles = 7;
			//This is here to prevent executing garbage data as opcodes
			//most games I've seen don't use it
			exit(0);
			break;

			//Flags
		case 0x18: P &= 0xFE; cycles = 2;break;
		case 0x38: P |= 0x01; cycles = 2;break;
		case 0x58: P &= 0xFB; cycles = 2;break;
		case 0x78: P |= 0x04; cycles = 2;break;
		case 0xB8: P &= 0xBF; cycles = 2;break;
		case 0xD8: P &= 0xF7; cycles = 2;break;
		case 0xF8: P |= 0x08; cycles = 2;break;

			/* General Ops */

			//ADC
		case 0x69: RdImd();  ADC(); break;
		case 0x65: RdZp();   ADC(); break;
		case 0x75: RdZpX();  ADC(); break;
		case 0x6D: RdAbs();  ADC(); break;
		case 0x7D: RdAbsX(); ADC(); break;
		case 0x79: RdAbsY(); ADC(); break;
		case 0x61: RdIndX(); ADC(); break;
		case 0x71: RdIndY(); ADC(); break;

			//AND
		case 0x29: RdImd();  AND(); break;
		case 0x25: RdZp();   AND(); break;
		case 0x35: RdZpX();  AND(); break;
		case 0x2D: RdAbs();  AND(); break;
		case 0x3D: RdAbsX(); AND(); break;
		case 0x39: RdAbsY(); AND(); break;
		case 0x21: RdIndX(); AND(); break;
		case 0x31: RdIndY(); AND(); break;

				   //ASL
		case 0x0A: RdAcc(); ASL(); MAcc();  break;
		case 0x06: RdZp(); 	ASL(); MZp();   break;
		case 0x16: RdZpX(); ASL(); MZpX();  break;
		case 0x0E: SAbs();  ASL(); MAbs();  break;
		case 0x1E: RdAbsX();ASL(); MAbsX(); break;

				   //Branches
		case 0x90: BCC(); break;
		case 0xB0: BCS(); break;
		case 0xF0: BEQ(); break;
		case 0x30: BMI(); break;
		case 0xD0: BNE(); break;
		case 0x10: BPL(); break;
		case 0x50: BVC(); break;
		case 0x70: BVS(); break;

				   //BIT
		case 0x24: RdZp();  BIT(); break;
		case 0x2C: RdAbs(); BIT(); break;

				   //CMP
		case 0xC9: RdImd();  CMP(); break;
		case 0xC5: RdZp();   CMP(); break;
		case 0xD5: RdZpX();  CMP(); break;
		case 0xCD: RdAbs();  CMP(); break;
		case 0xDD: RdAbsX(); CMP(); break;
		case 0xD9: RdAbsY(); CMP(); break;
		case 0xC1: RdIndX(); CMP(); break;
		case 0xD1: RdIndY(); CMP(); break;

				   //CPX
		case 0xE0: RdImd(); CPX(); break;
		case 0xE4: RdZp();  CPX(); break;
		case 0xEC: RdAbs(); CPX(); break;

				   //CPY
		case 0xC0: RdImd(); CPY(); break;
		case 0xC4: RdZp();  CPY(); break;
		case 0xCC: RdAbs(); CPY(); break;

				   //DEC
		case 0xC6: RdZp();  DEC(); MZp();   break;
		case 0xD6: RdZpX(); DEC(); MZpX();  break;
		case 0xCE: SAbs();  DEC(); MAbs();  break;
		case 0xDE: RdAbsX();DEC(); MAbsX(); break;

				   //DEX
		case 0xCA: DEX(); break;

				   //DEY
		case 0x88: DEY(); break;

				   //EOR
		case 0x49: RdImd();  EOR(); break;
		case 0x45: RdZp();   EOR(); break;
		case 0x55: RdZpX();  EOR(); break;
		case 0x4D: RdAbs();  EOR(); break;
		case 0x5D: RdAbsX(); EOR(); break;
		case 0x59: RdAbsY(); EOR(); break;
		case 0x41: RdIndX(); EOR(); break;
		case 0x51: RdIndY(); EOR(); break;

				   //INC
		case 0xE6: RdZp();   INC(); MZp();   break;
		case 0xF6: RdZpX();  INC(); MZpX();  break;
		case 0xEE: SAbs();   INC(); MAbs();  break;
		case 0xFE: RdAbsX(); INC(); MAbsX(); break;

				   //INX
		case 0xE8: INX(); break;

				   //INY
		case 0xC8: INY(); break;

				   //JMP
		case 0x4C: SAbs();  JMP(); break;
		case 0x6C: RdInd(); JMP(); break;

				   //JSR
		case 0x20: JSR(); break;

				   //LDA
		case 0xA9: RdImd(); LDA(); break;
		case 0xA5: RdZp();  LDA(); break;
		case 0xB5: RdZpX(); LDA(); break;
		case 0xAD: RdAbs(); LDA(); break;
		case 0xBD: RdAbsX();LDA(); break;
		case 0xB9: RdAbsY();LDA(); break;
		case 0xA1: RdIndX();LDA(); break;
		case 0xB1: RdIndY();LDA(); break;

				   //LDX
		case 0xA2: RdImd(); LDX(); break;
		case 0xA6: RdZp();  LDX(); break;
		case 0xB6: RdZpY(); LDX(); break;
		case 0xAE: RdAbs(); LDX(); break;
		case 0xBE: RdAbsY();LDX(); break;

				   //LDY
		case 0xA0: RdImd(); LDY(); break;
		case 0xA4: RdZp();  LDY(); break;
		case 0xB4: RdZpX(); LDY(); break;
		case 0xAC: RdAbs(); LDY(); break;
		case 0xBC: RdAbsX();LDY(); break;

				   //LSR
		case 0x4A: RdAcc();  LSR(); WrAcc(); break;
		case 0x46: RdZp();   LSR(); MZp();   break;
		case 0x56: RdZpX();  LSR(); MZpX();  break;
		case 0x4E: SAbs();   LSR(); MAbs();  break;
		case 0x5E: RdAbsX(); LSR(); MAbsX(); break;


		case 0xEA:  break;
					/*    //NOPs

						  case 0x04: case 0x44: case 0x64: PC++; break;
						  case 0x0C: PC++; PC++; break;
						  case 0x14: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4: PC++; break;
						  case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA: break;
						  case 0x80: PC++; break;
						  case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC: PC++; PC++; break;
					//LAX
					case 0xa3: RdIndX(); LDA();LDX();break;
					case 0xA7: RdZp(); LDA(); LDX();break;
					*/

					//ORA
		case 0x09: RdImd();  ORA(); break;
		case 0x05: RdZp();   ORA(); break;
		case 0x15: RdZpX();  ORA(); break;
		case 0x0D: RdAbs();  ORA(); break;
		case 0x1D: RdAbsX(); ORA(); break;
		case 0x19: RdAbsY(); ORA(); break;
		case 0x01: RdIndX(); ORA(); break;
		case 0x11: RdIndY(); ORA(); break;

				   //PHA
		case 0x48: PHA(); break;

				   //PHP
		case 0x08: PHP(); break;

				   //PLA
		case 0x68: PLA(); break;

				   //PLP
		case 0x28: PLP(); break;

				   //ROL
		case 0x2A: RdAcc();  ROL(); WrAcc(); break;
		case 0x26: RdZp();   ROL(); MZp();   break;
		case 0x36: RdZpX();  ROL(); MZpX();  break;
		case 0x2E: SAbs();   ROL(); MAbs();  break;
		case 0x3E: RdAbsX(); ROL(); MAbsX(); break;

				   //ROR
		case 0x6A: RdAcc();  ROR(); WrAcc(); break;
		case 0x66: RdZp();   ROR(); MZp();   break;
		case 0x76: RdZpX();  ROR(); MZpX();  break;
		case 0x6E: SAbs();   ROR(); MAbs();  break;
		case 0x7E: RdAbsX(); ROR(); MAbsX(); break;

				   //RTI
		case 0x40: RTI(); break;

				   //RTS
		case 0x60: RTS(); break;

				   //SBC
		case 0xE9: RdImd();  SBC(); break;
		case 0xe5: RdZp();   SBC(); break;
		case 0xF5: RdZpX();  SBC(); break;
		case 0xED: RdAbs();  SBC(); break;
		case 0xFD: RdAbsX(); SBC(); break;
		case 0xF9: RdAbsY(); SBC(); break;
		case 0xE1: RdIndX(); SBC(); break;
		case 0xF1: RdIndY(); SBC(); break;

				   //STA
		case 0x85: RdZp();   STA(); MZp();   break;
		case 0x95: RdZpX();  STA(); MZpX();  break;
		case 0x8D: SAbs();   STA(); MAbs();  break;
		case 0x9D: RdAbsX(); STA(); MAbsX(); break;
		case 0x99: RdAbsY(); STA(); MAbsY(); break;
		case 0x81: RdIndX(); STA(); MIndX(); break;
		case 0x91: RdIndY(); STA(); MIndY(); break;


				   //STX
		case 0x86: RdZp();  STX(); MZp();  break;
		case 0x96: RdZpY(); STX(); MZpY(); break;
		case 0x8E: SAbs();  STX(); MAbs(); break;

				   //STY
		case 0x84: RdZp();  STY(); MZp();  break;
		case 0x94: RdZpX(); STY(); MZpX(); break;
		case 0x8C: SAbs();  STY(); MAbs(); break;

				   //TAX
		case 0xAA: TAX(); break;

				   //TAY
		case 0xA8: TAY(); break;

				   //TSX
		case 0xBA: TSX(); break;

				   //TXA
		case 0x8A: TXA(); break;

				   //TXS
		case 0x9A: TXS(); break;

				   //TYA
		case 0x98: TYA(); break;

		default:
			printf("%.2x \tUnknown OPCode!\n", op);
			printf("nestest: %.2x\n",RAM[0]);
			exit(0);
	}

	return cycles;
}






