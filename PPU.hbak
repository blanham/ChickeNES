/*
 * PPU.h
 *
 *  Created on: Jun 5, 2009
 *      Author: blanham
 */
//loopy regs
extern WORD V, T;
extern BYTE lpyX;
extern int PPUcycles;


extern int line;
unsigned char rdVRAM(WORD addr);
void wrVRAM(WORD addr, BYTE value);
extern BYTE chr_buf[0x200][8][8];




struct	tPPU
{
    int cycle;
    int endcycle;
    int shortline;

    unsigned short loopy_V, loopy_T;
    unsigned long renderaddr;
    int scanline;
    int frames;
    int endscanline;
    int sprite0;
    int spr_clip;
    int sprites;
    int spritecount;
    int sprheight;
	int spraddr;
    BYTE sprstate;
    BYTE sprbuff[32];
    BYTE *sprdata, OAM_2[0x30];
    int rendering;
    int onscreen;
    int NMI;
    unsigned short *GfxData;
};

extern struct tPPU ppu;
struct food
{
	int sub,xpos, ypos, state;
	int number, oam,inrange;

}sprite;
void initPPU();
void nmi();
void RunPPU(int cycles);
extern BYTE spr_buf[0x100];

