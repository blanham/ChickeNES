/*
 * main.c
 *
 *  Created on: May 28, 2009
 *      Author: blanham
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <getopt.h>

#include "cart.h"

#include "main.h"
#include "B6502.h"
#include "pad.h"
#include "PPU.h"
#include "APU.h"
#include "mapper.h"
#include "config.h"
#include "int2bin.h"

#define SCANLINES 1850
int NMI = 0;
int CPUcycles, PPUcycles, Mcycles, line, Scycles = 0;
int FirstREAD = 1;
int multiplier = 15;
int PRG_size, CHR_size;

SDL_Surface* screen = NULL;

static struct option longopts[] = {
	{ "scanlines", required_argument, NULL, 's'},
	{ NULL, 0, NULL, 0}
};
struct nes {
	mos6502 *cpu;
	struct ppu {} ppu;
	struct apu {} apu;
	nes_cart *cart;
};
typedef struct nes nes_t;
void nes_write(void *aux, uint16_t addr, uint8_t val)
{
	nes_t *nes = (nes_t *)aux;
	mos6502 *cpu = nes->cpu;
	switch (addr) {
		case 0x0000 ... 0x00FF:
			cpu->zero_page[addr] = val;
			break;
		case 0x0100 ... 0x01FF:
			cpu->stack[addr] = val;
			break;
		case 0x0200 ... 0x07FF:
			cpu->ram[addr] = val;
			break;
		case 0x2000 ... 0x2007:
			break;
		case 0x2008 ... 0x3FFF:
			break;
		case 0x4000 ... 0x401F:
			break;
		case 0x4020 ... 0x5FFF:
			break;
		case 0x6000 ... 0x7FFF:
			//nes->cart->ram[addr & 0x1FFF] = val;
			break;
		case 0x8000 ... 0xBFFF:
		case 0xC000 ... 0xFFFF:
			break;
		default:
			cpu->ram[addr] = val;
			break;
	}
}

uint8_t nes_read(void *aux, uint16_t addr)
{
	nes_t *nes = (nes_t *)aux;
	mos6502 *cpu = nes->cpu;
	switch (addr) {
		case 0x0000 ... 0x00FF:
			return cpu->zero_page[addr];
		case 0x0100 ... 0x01FF:
			return cpu->stack[addr];
		case 0x0200 ... 0x07FF:
			return cpu->ram[addr];
		case 0x2000 ... 0x2007:
			break;
		case 0x2008 ... 0x3FFF:
			break;
		case 0x4000 ... 0x401F:
			break;

		case 0x4020 ... 0x5FFF:
		case 0x6000 ... 0x7FFF:
			//return nes->cart->ram[addr & 0x1FFF];
			break;
		case 0x8000 ... 0xBFFF:
			return nes->cart->prg_pages[0][addr & 0x3FFF];
		case 0xC000 ... 0xFFFF:
			return nes->cart->prg_pages[1][addr & 0x3FFF];
		default:
			return cpu->ram[addr];
	}
	return 0;
}

int sdl_init()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO)) {
		printf( "Unable to init SDL: %s\n", SDL_GetError());
		return 1;
	}

	// make sure SDL cleans up before exit
	atexit(SDL_Quit);

	SDL_Window *window = SDL_CreateWindow("ChickeNES", 100, 100, 512, 480, SDL_WINDOW_SHOWN);

	// input_init();

	if (!window) {
		printf("Unable to set 256x240 video: %s\n", SDL_GetError());
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int ch, scanlines = 0;
	char *filename = NULL;

	while ((ch = getopt_long(argc, argv, "s:", longopts, NULL)) != -1) {
		switch (ch) {
			case 's':
				fprintf(stderr, "Scanlines: %s", optarg);
				scanlines = atoi(optarg);
				break;
		}
	}

	if ((filename = argv[optind]) == NULL) {
		//This should print usage instead of this error
		fprintf(stderr, "No file specified\n");
		return EXIT_FAILURE;
	}

	printf("ChickeNES v. 0.0.0.1a\n");
	if (openconfig()) {
		return EXIT_FAILURE;
	}

	if (sdl_init()) {
		return EXIT_FAILURE;
	}

	struct nes *nes = calloc(sizeof(*nes), 1);

	if ((nes->cart = nes_cart_load(filename)) == NULL) {
		fprintf(stderr, "Error loading rom!\n");
		return EXIT_FAILURE;
	}

	//init cpu
	nes->cpu = mos6502_alloc();
	nes->cpu->aux = nes;
	nes->cpu->read = nes_read;
	nes->cpu->write = nes_write;

	//init ppu
	//init apu

	//get number of lines/scanlines if specified
	//(use getopt), other options

	//pass cart and options to cpu
	//memcpy(&nes->cpu->ram[0x8000], nes->cart->prg_rom, 0x8000);

	//reset cpu
	mos6502_reset(nes->cpu);

	//enter run loop

	bool running = true;
	SDL_Event e;
	int cycles = 0;
	int dcycles = 0;
	while (running) {
		SDL_PollEvent(&e);
		if (e.type == SDL_QUIT)
			running = false;
		dcycles += cycles * nes->cart->multiplier;

		cycles = mos6502_logger(nes->cpu);
	}

	return 0;
}

int Dcycles;


void RunEMU(int frames)
{
	//sets the number of cycles based on Regional multiplier
	//int Dcycles = cycles * multiplier;
	/*  
		int running = 1;
		initCPU();
		initAPU();
		while (running)
		{

		RunCPU(0);
	//printf("PPU!\n");
	RunPPU(341);
	//	RunPPU2(0);
	PPUcycles=0;
	if (NMI)
	nmi();
	Mcycles=0;
	//if(running == 100) exit(0);
	//  if (ppu.frames == frames) running = 0;
	}
	*/
}

//FIXME Need to figure out what the fuck is going on with cycle counting and fix it

int RunCPU(int runto)
{
	/*  
		CPUcycles = 114;

		while (CPUcycles > 0)
		{

		int help = 0;//DoOP(RAM[PC]);
	//	if(PC>0x9000)printf("PCL: %x\n",PC);
	//  PC++;
	Mcycles = 341;
	PPUcycles += (help * multiplier);
	//printf("PPU: %i\n",PPUcycles);
	CPUcycles -= help;
	//	RunAPU(341);


#ifdef PRNTSTAT
	//  printp();
	//printf(" A:%.2x, X:%.2x, Y:%.2x, SP:%.2x, S2:%.2x, S1:%.2x P:%.2x\n\n", A, X, Y, STACK, RAM[0x0100+STACK+1], RAM[0x0100+STACK],P);
	//   printf(" A:%.2x, X:%.2x, Y:%.2x, S:%.2x, S2:%.2x, S?:%.2x, Cycles:%i PPUCYCLES:%i line:%i\n\n", A, X, Y, STACK, RAM[0x0100+STACK+1], RAM[0x0100+STACK], CPUcycles, ppu.cycle, ppu.scanline);
#endif



}

return Dcycles;*/
return 0;
}








