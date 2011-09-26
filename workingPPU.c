/*
 * PPU.c
 *
 *  Created on: Jun 5, 2009
 *  Started again Sept 8, 2009
 *      Author: blanham
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "B6502.h"
#include "main.h"
#include "PPU.h"



struct	tPPU	ppu;

//not happy with how ppu.OAM_2 is working
BYTE OAM3[8][3];


BYTE chr_buf[0x200][8][8];
BYTE pal_buf[0x2000];

//loopy regs
WORD V, T = 0;
BYTE lpyX;

BYTE spr_buf[0x100];

GLfloat vertices[][3]={
	{-1.0,-1.0,1.0},
	{-1.0,1.0,1.0},
	{1.0,1.0,1.0},
	{1.0,-1.0,1.0},
	{-1.0,-1.0,-1.0},
	{-1.0,1.0,-1.0},
	{1.0,1.0,-1.0},
	{1.0,-1.0,-1.0}
};
unsigned char sprpointer;

void sprrun(void);

GLbyte nes_pal[64][4] = {
	{0x75, 0x75, 0x75, 0},
	{0x21, 0x1b, 0x8f, 0},
	{0x00, 0x00, 0xab, 0},
	{0x47, 0x00, 0x9f, 0},
	{0x8f, 0x00, 0x77, 0},
	{0xab, 0x00, 0x13, 0},
	{0xa7, 0x00, 0x00, 0},
	{0x7f, 0x0b, 0x00, 0},
	{0x43, 0x2f, 0x00, 0},
	{0x00, 0x47, 0x00, 0},
	{0x00, 0x51, 0x00, 0},
	{0x00, 0x3f, 0x17, 0},
	{0x1b, 0x3f, 0x5f, 0},
	{0x00, 0x00, 0x00, 0},
	{0x00, 0x00, 0x00, 0},
	{0x00, 0x00, 0x00, 0},
	{0xbc, 0xbc, 0xbc, 0},
	{0x00, 0x73, 0xef, 0},
	{0x23, 0x3b, 0xef, 0},
	{0x83, 0x00, 0xf3, 0},
	{0xbf, 0x00, 0xbf, 0},
	{0xe7, 0x00, 0x5b, 0},
	{0xdb, 0x2b, 0x00, 0},
	{0xcb, 0x4f, 0x0f, 0},
	{0x8b, 0x73, 0x00, 0},
	{0x00, 0x97, 0x00, 0},
	{0x00, 0xab, 0x00, 0},
	{0x00, 0x93, 0x3b, 0},
	{0x00, 0x83, 0x8b, 0},
	{0x00, 0x00, 0x00, 0},
	{0x00, 0x00, 0x00, 0},
	{0x00, 0x00, 0x00, 0},
	{0xff, 0xff, 0xff, 0},
	{0x3f, 0xbf, 0xff, 0},
	{0x5f, 0x97, 0xff, 0},
	{0xa7, 0x8b, 0xfd, 0},
	{0xf7, 0x7b, 0xff, 0},
	{0xff, 0x77, 0xb7, 0},
	{0xff, 0x77, 0x63, 0},
	{0xff, 0x9b, 0x3b, 0},
	{0xf3, 0xbf, 0x3f, 0},
	{0x83, 0xd3, 0x13, 0},
	{0x4f, 0xdf, 0x4b, 0},
	{0x58, 0xf8, 0x98, 0},
	{0x00, 0xeb, 0xdb, 0},
	{0x00, 0x00, 0x00, 0},
	{0x00, 0x00, 0x00, 0},
	{0x00, 0x00, 0x00, 0},
	{0xff, 0xff, 0xff, 0},
	{0xab, 0xe7, 0xff, 0},
	{0xc7, 0xd7, 0xff, 0},
	{0xd7, 0xcb, 0xff, 0},
	{0xff, 0xc7, 0xff, 0},
	{0xff, 0xc7, 0xdb, 0},
	{0xff, 0xbf, 0x83, 0},
	{0xff, 0xdb, 0xab, 0},
	{0xff, 0xe7, 0xa3, 0},
	{0xe3, 0xff, 0xa3, 0},
	{0xab, 0xf3, 0xbf, 0},
	{0xb3, 0xff, 0xcf, 0},
	{0x9f, 0xff, 0xf3, 0},
	{0x00, 0x00, 0x00, 0},
	{0x00, 0x00, 0x00, 0},
	{0x00, 0x00, 0x00, 0}
};

unsigned char pixels[240][256][4];

//cycle for cycle variables
unsigned char attrshifttable[0x400];
unsigned char attrloc[0x400];

WORD attribshift;


const	unsigned char	ReverseCHR[256] =
{
	0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0,
	0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,
	0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
	0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,
	0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,
	0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
	0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
	0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
	0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
	0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9,
	0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
	0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
	0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,
	0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
	0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
	0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF
};


unsigned char sprcount[240];


const	unsigned long	CHRLoBit[16] =
{
	0x00000000,0x00000001,0x00000100,0x00000101,0x00010000,0x00010001,0x00010100,0x00010101,
	0x01000000,0x01000001,0x01000100,0x01000101,0x01010000,0x01010001,0x01010100,0x01010101
};
const	unsigned long	CHRHiBit[16] =
{
	0x00000000,0x00000002,0x00000200,0x00000202,0x00020000,0x00020002,0x00020200,0x00020202,
	0x02000000,0x02000002,0x02000200,0x02000202,0x02020000,0x02020002,0x02020200,0x02020202
};

const	unsigned char	attrloc1[256] =
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
	0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
	0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
	0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
	0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
	0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
	0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F
};
const	unsigned long	AttribBits[4] =
{
	0x00000000,0x04040404,0x08080808,0x0C0C0C0C
};
const	unsigned char	AttribShift[128] =
{
	0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,
	4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6,4,4,6,6
};



static const unsigned char attrlut[] =
{
	0x03, 0x07, 0x0B, 0x0F
};


void init_chr_buffer(){
	int i;
	int j;
	int k;
	BYTE pixel;
	unsigned char layerA;
	unsigned char layerB;
	for (i=0;i<256;i++){

			for (j=0;j<8;j++){
					k = 0;
					int l = 7;
					while (k<8)
						{

							layerA = ((CHR[(i*0x10)+j] >> k) & 1);
							layerB = ((CHR[(i*0x10)+0x8+j] >> k) & 1);



							if (layerA & layerB)
								{
									pixel = 0x3;
								}
							else  if (layerA)
								{
									pixel = 0x1;
								}
							else if (layerB)
								{
									pixel = 0x2;
								}
							else
								{
									pixel = 0x00;
								}

							chr_buf[i][j][l] = pixel;
							l--;
							k++;

						}
				}
		}

}

GLvoid *colors;

void initPPU()
{
	RAM[0x2002] = 0x10;
	ppu.scanline = -1;
	ppu.rendering = 0;
	ppu.endcycle = 341;
	ppu.cycle = 0;
	//builds shift tables
	int i;
	for (i = 0; i != 0x400; ++i)
		{
			attrshifttable[i] = ((i >> 4) & 0x04) | (i & 0x02);
			attrloc[i] =   (((i >> 4) & 0x38) | (i >> 2))  &  7;
		}

		
	init_chr_buffer();
}



void nmi()
{
#ifdef PRNTSTAT
	printf("\n\nNMI!\n\n");
#endif
	RAM[0x0100+STACK--]= (PC >> 8);
	RAM[0x0100+STACK--]= (PC & 0xff);
	RAM[0x0100+STACK--]= P;
	PC = RAM[0xfffa] + (RAM[0xfffb]<<8);
	RAM[0x2000] |= 0x80;
	NMI=0;
}

inline unsigned char rdVRAM(WORD addr){

	BYTE value= 0;

	if (addr>=0x2000&&addr<0x3F00){

			value = rdNT(addr);

		}else if (addr>=0x3F00){

			if (addr == 0x4 || addr == 0x8 || addr == 0xc) addr = 0x0;
			value = PAL[addr&0xFF];

		}else    if (addr<0x2000){

			value = rdCHRRAM(addr);

		}


	return value;


}

inline void wrVRAM(WORD addr, BYTE value){
	addr &= 0x3FFF;
	if (addr>=0x2000 && addr<0x3F00){
			//printf("ADDR:%.4X\n",addr);
			rdNT(addr) = value;
		}else if (addr>=0x3F00)
		{
			if (addr==0x3f10  ||addr == 0x3f14 || addr == 0x3f18 || addr == 0x3f1c) addr -= 0x10;
			PAL[addr&0x3F] = value;
		}else     if (addr<0x2000){
			rdCHRRAM(addr)=value;
		}
}



int fuck;


unsigned char chicken;

static int sprpos, sprnum, sprsub, sprtmp, sprstate;

unsigned char rndrdata[4];
int extra;




void RunPPU(int cycles)
{

	unsigned short pataddr;
	unsigned short attrbaddr;
	unsigned short rndraddr;
	unsigned short sprtpataddr;
	WORD attribute;
	BYTE pindex;
	register BYTE tile;
	int sprnum;
	//static struct sSprState spr;



	register int i, j;
	for (i = 0; i < cycles; i++)
		{
			ppu.cycle++;
			if (ppu.cycle == 256)
				{
				//	if (ppu.scanline==30) RAM[0x2002] |=0x40;

					if (ppu.scanline > -1)
						ppu.endcycle = 341;
					else if (ppu.shortline && ppu.rendering)
						ppu.endcycle = 340;
				}
			else if (ppu.cycle == 304)
				{
					if ((ppu.scanline < 0) && ppu.rendering){V = T;FirstREAD = 1;}
				}
			else if (ppu.cycle == ppu.endcycle)
				{
					++ppu.scanline;

					ppu.cycle = 0;
					if (ppu.scanline == 0)
						ppu.onscreen = 1;
					else if (ppu.scanline == 240){
							ppu.onscreen=ppu.rendering = 0;

						}
					else if (ppu.scanline == 241)
					{

					glClear(GL_COLOR_BUFFER_BIT);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 240, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
					glBindTexture(GL_TEXTURE_2D, texture );
					glBegin(GL_QUADS);
						glTexCoord2d(0,0); glVertex2i(0, 0);
						glTexCoord2d(1,0); glVertex2i(256, 0);
						glTexCoord2d(1,1); glVertex2i(256, 240);
						glTexCoord2d(0,1); glVertex2i(0, 240);
					glEnd();
					SDL_GL_SwapBuffers();
					SDL_GL_SwapWindow(mainwindow);
					//wait (slows down)
					//	usleep(5000);
					ppu.frames++;
						
					//we are in vblank!
					RAM[0x2002] |= 0x80;
					//sprcounter reset
					RAM[0x2003] = 0;
					//Set NMI
					if (RAM[0x2000] & 0x80)
						NMI = 1;
				}
		}
		else if (ppu.scanline == 341)
		{
			ppu.scanline = -1;
			if (RAM[0x2001] & 0x18) ppu.rendering =1;

			if (!region)
				ppu.shortline ^= 1;
			else
			ppu.shortline = 0;
		}
		else if ((ppu.scanline < 0) && (ppu.cycle == 1))
		{
			RAM[0x2002] = 0;
		}
/*
		if (ppu.rendering)
		{

			int k;
			if (ppu.cycle & 1)
			{
				rndrdata[(ppu.cycle >> 1) & 3] = rdVRAM(V);
			}
				//	if(ppu.cycle==0)printf("SL: %3i OAM:",ppu.scanline);
			switch (ppu.cycle)
			{


					//thanks to qeed, and by relation, wesnesday and disch,
					//First Phase of Rendering (Clearing the Secon
				case   0:   case   8:   case  16:   case  24:
				case  32:   case  40:   case  48:   case  56:		
					//now we fetch a tile from the name table
					//using the pointer directly
					tile = pNT [(V & 0xC00) >> 10] [(V) & 0x03FF];
					//we start zeroing the secondary OAM
					ppu.sprstate = ppu.OAM_2[ppu.cycle >> 1] == 0xFF;
					break;
				case   1:     case  9:     case  17:    case  25:
				case  33:    case 41:       case 49:    case 57:
					//and use it and $2006 to get the pattern address
					pataddr = (tile << 4) | (V>>12) | ((RAM[0x2000] & 0x10) << 8);
					//we start zeroing the secondary OAM
					ppu.sprstate = ppu.OAM_2[ppu.cycle >> 1] == 0xFF;
					break;
				case 2:     case 10:    case 18:    case 26:
				case 34:    case 42:    case 50:    case 58:
					attrbaddr  = 0x23C0 | (V & 0xC00) | (attrloc1[(V>>2) & 0xFF]);
					attribshift = attrshifttable[V & 0x7F];
					//we start zeroing the secondary OAM
					ppu.sprstate = ppu.OAM_2[ppu.cycle >> 1] == 0xFF;
					break;
				case 3:     case 11:    case 19:    case 27:
				case 35:    case 43:    case 51:    case 59:
					rndraddr = attrbaddr;
					// attribute = attrlut[((rdNT(rndraddr) >> attribshift )& 3)];
					attribute = ((rdNT(rndraddr) >> attribshift )& 3) << 2;

					for (k=0; k != 8; ++k)
						pal_buf[ppu.cycle+13+k] = attribute;
					if ((V & 0x1F) == 0x1F)
						V ^= 0x41F;
					else
						V++;
					//we start zeroing the secondary OAM
					ppu.sprstate = ppu.OAM_2[ppu.cycle >> 1] == 0xFF;
					break;
				case   4:   case  12:   case  20:   case  28:
				case  36:   case  44:   case  52:   case  60:
					rndraddr = pataddr;
					//we start zeroing the secondary OAM
					ppu.sprstate = ppu.OAM_2[ppu.cycle >> 1] == 0xFF;
					break;
				case   5:	case  13:	case  21:	case  29:
				case  37:	case  45:	case  53:	case  61:
						for (k = 0; k != 8; ++k)
							pal_buf[ppu.cycle+11+k] |=  ((rdCHRRAM(rndraddr) >> (k ^ 7)) & 1);
						//we start zeroing the secondary OAM
						ppu.sprstate = ppu.OAM_2[ppu.cycle >> 1] == 0xFF;
						break;
				case   6:	case  14:	case  22:	case  30:
				case  38:	case  46:	case  54:	case  62:
						rndraddr = pataddr | 8;
						//we start zeroing the secondary OAM
						//	ppu.sprstate = ppu.OAM_2[ppu.cycle >> 1] == 0xFF;
						break;
				case   7:	case  15:	case  23:	case  31:
				case  39:	case  47:	case  55:	case  63:
						for (k = 0; k != 8; ++k){
							pal_buf[ppu.cycle+9+k] |= ((((rdCHRRAM(pataddr | 8) << 1) >> (k ^ 7)) & 2) );
						}
						//we start zeroing the secondary OAM
						ppu.sprstate = ppu.OAM_2[ppu.cycle >> 1] == 0xFF;
						break;
                case  64: 	case  72:   case  80:   case  88:
				case  96:   case 104:   case 112:   case 120:
				case 128:   case 136:   case 144:   case 152:
				case 160:   case 168:   case 176:   case 184:
				case 192:   case 200:   case 208:   case 216:
				case 224:   case 232:   case 240:   case 248:
						tile = pNT [(V & 0xC00) >> 10] [(V) & 0x03FF];
						break;
				case 65:    case 73:    case 81:    case 89:
				case 97:    case 105:   case 113:   case 121:
				case 129:   case 137:   case 145:   case 153:
				case 161:   case 169:   case 177:   case 185:
				case 193:   case 201:   case 209:   case 217:
				case 225:   case 233:   case 241:   case 249:
						pataddr = (tile << 4) | (V>>12) | ((RAM[0x2000] & 0x10) << 8);
						break;
				case 66:    case 74:    case 82:    case 90:
				case 98:    case 106:   case 114:   case 122:
				case 130:   case 138:   case 146:   case 154:
				case 162:   case 170:   case 178:   case 186:
				case 194:   case 202:   case 210:   case 218:
				case 226:   case 234:   case 242:   case 250:
						attrbaddr  = 0x23C0 | (V & 0xC00) | (attrloc1[(V>>2) & 0xFF]);
						attribshift = attrshifttable[V & 0x7F];
						break;
				case 67:    case 75:    case 83:    case 91:
				case 99:    case 107:   case 115:   case 123:
				case 131:   case 139:   case 147:   case 155:
				case 163:   case 171:   case 179:   case 187:
				case 195:   case 203:   case 211:   case 219:
                case 227:   case 235:   case 243:
						pataddr = (tile << 4) | (V>>12) | ((RAM[0x2000] & 0x10) << 8);
						rndraddr = attrbaddr;
						// attribute = attrlut[((rdNT(rndraddr) >> attribshift )& 3)];
						attribute = ((rdNT(rndraddr) >> attribshift )& 3) << 2;

						for (k=0; k != 8; ++k)	
							pal_buf[ppu.cycle+13+k] = attribute;
						if ((V & 0x1F) == 0x1F)
							V ^= 0x41F;
						else
							V++;
						break;
				case  68:   case  76:   case  84:   case  92:
				case 100:   case 108:   case 116:   case 124:
				case 132:   case 140:   case 148:   case 156:
				case 164:   case 172:   case 180:   case 188:
				case 196:   case 204:   case 212:   case 220:
				case 228:   case 236:   case 244:   case 252:
								rndraddr = pataddr;
								break;
				case  69:	case  77:	case  85:	case  93:
				case 101:	case 109:	case 117:	case 125:
				case 133:	case 141:	case 149:	case 157:
				case 165:	case 173:	case 181:	case 189:
				case 197:	case 205:	case 213:	case 221:
				case 229:	case 237:	case 245:	case 253:

								pataddr = (tile << 4) | (V>>12) | ((RAM[0x2000] & 0x10) << 8);

								for (k = 0; k != 8; ++k)
									pal_buf[ppu.cycle+11+k] |=  ((rdCHRRAM(rndraddr) >> (k ^ 7)) & 1);
								break;

				case  70:	case  78:	case  86:	case  94:
				case 102:	case 110:	case 118:	case 126:
				case 134:	case 142:	case 150:	case 158:
				case 166:	case 174:	case 182:	case 190:
				case 198:	case 206:	case 214:	case 222:
				case 230:	case 238:	case 246:	case 254:
						rndraddr = pataddr | 8;
						break;
				case  71:	case  79:	case  87:	case  95:
				case 103:	case 111:	case 119:	case 127:
				case 135:	case 143:	case 151:	case 159:
				case 167:	case 175:	case 183:	case 191:
				case 199:	case 207:	case 215:	case 223:
				case 231:	case 239:	case 247:	case 255:

					pataddr = (tile << 4) | (V>>12) | ((RAM[0x2000] & 0x10) << 8);
					for (k = 0; k != 8; ++k)
						pal_buf[ppu.cycle+9+k] |= ((((rdCHRRAM(pataddr | 8) << 1) >> (k ^ 7)) & 2) );
					break;
             	case 251:
					pataddr = (tile << 4) | (V>>12) | ((RAM[0x2000] & 0x10) << 8);
					rndraddr = attrbaddr;
					attribute = ((rdNT(rndraddr) >> attribshift) & 3) << 2;
					for (k = 0; k != 8; ++k) pal_buf[ppu.cycle+13+k] = attribute;
					if ((V & 0x1F) == 0x1F)
						V ^= 0x41F;
					else
						++V;

					// This is the same as above when applying the
					//   attribute data 

					if ((V & 0x7000) == 0x7000)
					{
						int tmp = V & 0x3E0;
						//reset tile y offset 12 - 14 in addr
						V &= 0xFFF;
						switch (tmp)
						{
						//29, flip bit 11
							case 0x3A0:
								V ^= 0xBA0;
							break;
							case 0x3E0: //31, back to 0
								V ^= 0x3E0;
							break;
							default: //inc y scroll if not reached
								V += 0x20;
						}
					}
					else //inc fine y
						V += 0x1000;
					break;
				case 256:   case 264:   case 272:   case 280:
				case 288:   case 296:   case 304:   case 312:
					rndraddr = 0x2000 | (V & 0xFFF);
					break;
				case 320:   case 328:
					//name table
					tile = pNT [(V & 0xC00) >> 10] [(V) & 0x03FF];
					break;
				case 321:   case 329:
					pataddr = (tile << 4) | (V>>12) | ((RAM[0x2000] & 0x10) << 8);
					break;
				case 322:   case 330:
					attrbaddr  = 0x23C0 | (V & 0xC00) |	(attrloc1[(V>>2) & 0xFF]);
					attribshift = attrshifttable[V & 0x7F];
					break;
				case 323:    case 331:

					rndraddr = attrbaddr;
					attribute = ((rdNT(rndraddr) >> attribshift) & 3) << 2;
					for (k=0; k != 8; ++k)
						pal_buf[ppu.cycle-323+k] = attribute;

					if ((V & 0x1F) == 0x1F)
						V ^= 0x41F;
					else
						V++;
					break;
				case 324:   case 332:
					rndraddr = pataddr;
					break;
				case 326:	case 334:
					rndraddr = pataddr | 8;
					break;
				case 325: case 333:
					for (k = 0; k != 8; ++k)
						pal_buf[ppu.cycle-325+k] |= ( (rdCHRRAM(pataddr) >> (k ^ 7)) & 1);
					break;
				case 327:	case 335:
					for (k = 0; k != 8; ++k)
						pal_buf[ppu.cycle-327+k] |= (((rdCHRRAM(pataddr | 8) << 1) >> (k ^ 7)) & 2);
					break;
								//The upper bit of the 2 bit plane that is in
								//the pattern table for the last 2 tiles.
				case 257:
					V &= 0xFBE0;
					V |= (T & 0x41F);
					rndraddr = 0x2000 | (V & 0xFFF);
					break;
				case 258:   case 266:   case 274:   case 282:
				case 290:   case 298:   case 306:   case 314:
					rndraddr = 0x2000 | (V & 0xFFF);
					break;
				case 265:   case 273:   case 281:   case 289:
                case 297:   case 305:   case 313:
					break;
				case 259:   case 267:   case 275:   case 283:
				case 291:   case 299:   case 307:   case 315:
					fuck = (ppu.cycle >> 1) & 0x1C;					
					
				 	if ((ppu.OAM_2[fuck] <= ppu.scanline) && (ppu.OAM_2[fuck]+8 > ppu.scanline + ((RAM[0x2000] & 0x20) >> 2)))
				//if ((ppu.scanline == ppu.OAM_2[fuck]+1))
					{
						tile= ppu.OAM_2[fuck|1];
						extra=ppu.OAM_2[fuck];
						//	printf("\nX: %x Y: %x Tile: %x Fuck: %i Scanline:%x\n",ppu.OAM_2[fuck+1],ppu.OAM_2[fuck],tile,fuck,ppu.scanline);
						sprite.inrange = 1;
					}else{
						tile=0x00;
						extra=0;
						sprite.inrange=0;
					}
					break;
				case 260:   case 268:   case 276:   case 284:
				case 292:   case 300:   case 308:   case 316:
					rndraddr == pataddr;
					break;

				case 261:	case 269:	case 277:	case 285:
				case 293:	case 301:	case 309:	case 317:
					rndraddr = attrbaddr;
					fuck= (ppu.cycle >> 1) & 0x1E;

					attribute = AttribBits[ppu.OAM_2[fuck] & 0x3];
								

					if (sprite.inrange){
						if ((ppu.OAM_2[fuck] & 0x40)){
							for (k = 0; k != 8; ++k){
								spr_buf[ppu.cycle-261+k] = attribute;
								spr_buf[ppu.cycle-261+k] |= chr_buf[tile][ppu.scanline-extra][k^7]&3;
                            }
						}else if((ppu.OAM_2[fuck] & 0x80)){
							for (k = 0; k != 8; ++k){
								spr_buf[ppu.cycle-261+k] = attribute;
								spr_buf[ppu.cycle-261+k] |= chr_buf[tile][7-(ppu.scanline-extra)][k]&3;
							}
						}else if((ppu.OAM_2[fuck] & 0xC0)){
							for (k = 0; k != 8; ++k){
								spr_buf[ppu.cycle-261+k] = attribute;
								spr_buf[ppu.cycle-261+k] |= chr_buf[tile][7-(ppu.scanline-extra)][k^7]&3;
							}
						}else{
							for (k = 0; k != 8; ++k){
								spr_buf[ppu.cycle-261+k] = attribute;
								spr_buf[ppu.cycle-261+k] |= chr_buf[tile][ppu.scanline-extra][k]&3;
							}
						}
					}
					//we start zeroing the secondary OAM

					//spr_pixel_index_buf is a 64 byte buf,
					//  since it is 8 tiles. (64 pixels)
					break;
				case 262:	case 270:	case 278:	case 286:
				case 294:	case 302:	case 310:	case 318:
					rndraddr = pataddr|8;
					//get the hi chr now...
					break;
				case 263:	case 271:	case 279:	case 287:
				case 295:	case 303:	case 311:	case 319:
					if (sprite.inrange){
					//	printf("FUCKHI: %i -4:%i\n",fuck, ((ppu.scanline%8)-4));
							
						if ((ppu.OAM_2[fuck] & 0x40))
						{
							for (k = 0; k != 8; ++k)
							{				
								spr_buf[ppu.cycle-263+k] |= chr_buf[tile][ppu.scanline-extra][k^7]&1;
                               }

						}else if((ppu.OAM_2[fuck] & 0x80)){
							for (k = 0; k != 8; ++k){
								spr_buf[ppu.cycle-263+k] |= chr_buf[tile][7-(ppu.scanline-extra)][k]&1;
							}
						}else if((ppu.OAM_2[fuck] & 0xC0)){
							for (k = 0; k != 8; ++k){
									spr_buf[ppu.cycle-263+k] |= chr_buf[tile][7-(ppu.scanline-extra)][k^7]&1;
								}
						}else{
							for (k = 0; k != 8; ++k){
								spr_buf[ppu.cycle-263+k] |= chr_buf[tile][ppu.scanline-extra][k]&1;
							}
						}
						
					}
					break;
				case 336:	case 338:
					rndraddr = 0x2000 | (V & 0xFFF);
					break;
				case 337:	case 339:
					break;
				case 340:
				
					break;
				default:
					break;
								// Now one line of scanline has been complete,
								//   341 cycles [0, 340] has been done.
								//   Another note, the render_addr = ...
								//   is not certain that the real NES
								//   reads like this, but it does adhere
								//   to the 2 cycles per fetch basis, also
								//  no games does sprites to this accurate fetching,
								//   one can just speed up the sprite code by writing
								//   all the sprite data at once at cycle 319, since
								//   we already have all the data by cycle 255.
								

						}
					
					if (ppu.onscreen && (ppu.cycle < 256))
					{
						sprrun();
						if ((RAM[0x2001]&0x08) && ((ppu.cycle >= 8) || (RAM[0x2001] & 0x02))) pindex = pal_buf[ppu.cycle + lpyX]; else
								pindex = 0; //if not, use master palette color.

					// this code checks if the sprite is visible and no
					//  sprite clipping
						//if(ppu.scanline==30) RAM[0x2002] |= 0x40;
						int j, tmp = 0;
							
						if ((RAM[0x2001] & 0x10) && ((ppu.cycle >= 8) && (RAM[0x2001] & 0x4) ))
						{	
							int q =0;
							for (q= 0; q < sprcount[ppu.scanline-1]; q+=4)
							{
								tmp = q >> 1;					
								j = ppu.cycle -  ppu.OAM_2[q|3];
								BYTE new;
								//printf("sfd");
								if (j & ~7) continue;
							
								new =  spr_buf[(q<<1)+j];
										
								if (new&3) 
								{
							
								if ((ppu.sprite0) && (!q) && (pindex) && (ppu.cycle< 255)&& (RAM[0x2001] & 0x8))
								{
									RAM[0x2002] |= 0x40;
									ppu.sprite0 = 0;
								}
												
								//do if sprite priority is greater than bg
								if (!((pindex&0x3 ) && (ppu.OAM_2[q | 2] & 0x20)))
								//	if (!(pindex && ppu.OAM_2[0x20 | (tmp | 1)]))
								pindex = new | 0x10;
					
								break;
							}
						}
										
						}



						if (!ppu.rendering)
						{
							if ((V & 0x3F00) == 0x3F00)
							pindex = V & 0x1F;
						}
							
						//pindex &= (RAM[0x2001] & 1); //deal with grayscale
							//set in 0x2001

						pindex |= ((RAM[0x2001] & 0xE0)>>8); //color emphasis
							//set in 0x2001.
						if (pindex == 0x4 || pindex == 0x8 || pindex == 0xc) pindex = 0x0;

						if(ppu.onscreen && (ppu.cycle<256)){								
								pixels[ppu.scanline][ppu.cycle][0] = nes_pal[PAL[pindex]][0];
								pixels[ppu.scanline][ppu.cycle][1] = nes_pal[PAL[pindex]][1];
								pixels[ppu.scanline][ppu.cycle][2] = nes_pal[PAL[pindex]][2];

							 }

						}





				}



*/

		}
		



}



void sprrun(void){
	
	if (ppu.cycle < 64)
	{
		static int sprpos;
		if (ppu.cycle == 0)
			sprpos = 0;
		if (ppu.cycle & 1)
			sprpos++;
	}
	else if (ppu.cycle<256 && ppu.cycle >=64)
		{

				
				if (ppu.cycle ==64)
					{
						
						sprpos = 0;
						sprnum = 0;
						sprsub = 0;
						sprstate = 0;
						sprite.inrange=0;
					//	sprtmp=0;
					
						sprcount[ppu.scanline]=0;
					}
				switch (sprstate)
					{
						case 0:	// evaluate current Y coordinate
							if (ppu.cycle & 1)
								{
									if (sprpos < 0x40) ppu.OAM_2[sprpointer = sprpos] = sprtmp;
									if ((sprtmp  >= ppu.scanline-7) && (sprtmp < ppu.scanline  +1 +((RAM[0x2000] & 0x20) >> 2)))
								
									//if ((sprtmp >= ppu.scanline) && (sprtmp < (ppu.scanline +7  +8*((RAM[0x2000] & 0x20) >> 5))))
										{
											if(sprnum == 0) ppu.sprite0=ppu.scanline;
											
											sprstate = 1;	// sprite is in range, fetch the rest of it
											sprsub = 1;
											sprpos++;
											sprcount[ppu.scanline]+=4;
										}
									else
										{
											//
											if (++sprnum == 64){
													sprstate = 4;
												}
											else if (sprpos == 32)
												sprstate = 2;
											else	sprstate = 0;
											//
										}
								}
							else	sprtmp = SPRRAM[sprpointer = sprnum << 2];
							break;
						case 1:	// Y-coordinate is in range, copy remaining bytes
							if (ppu.cycle & 1)
								{
									ppu.OAM_2[sprpointer = sprpos++] = sprtmp;
									
									if (sprsub++ == 3)
										{
											//

											if (++sprnum == 64)
												sprstate = 4;
											else if (sprpos == 32)
												sprstate = 2;
											else	sprstate = 0;
											//
										}
								}
							else	sprtmp = SPRRAM[sprpointer = (sprnum << 2) | sprsub];
							break;
						case 2:	// exactly 8 sprites detected, go through 'weird' evaluation
							if (ppu.cycle & 1)
								{
									sprpointer = 0;	// failed write
									if ((sprtmp >= ppu.scanline-7) && (sprtmp < (ppu.scanline+1  + ((RAM[0x2000] & 0x20) >> 2))))
										{	// 9th sprite found "in range"
											sprstate = 3;
											sprsub = 1;
											sprpos = 1;	// set counter
											RAM[0x2000] |= 0x20;	// set sprite overflow flag
										}
									else
										{
											sprsub = (sprsub + 1) & 3;
											//
											if (++sprnum == 64)
												sprstate = 4;
											//
										}
								}
							else	sprtmp = SPRRAM[sprpointer = (sprnum << 2) | sprsub];
							break;
						case 3:	// 9th sprite detected, fetch next 3 bytes
							if (ppu.cycle & 1)
								{
									sprpointer = sprpos;
									sprsub = (sprsub + 1) & 3;
								//	sprcount[ppu.scanline]+=4;
									
									if (!sprsub)
										sprnum = (sprnum + 1) & 63;
									if (sprpos++ == 3)
										{
											if (sprsub == 3)
												sprnum = (sprnum + 1) & 63;
											sprstate = 4;
										}
								}
							else	sprtmp = SPRRAM[sprpointer = (sprnum << 2) | sprsub];
							break;
						case 4:	// no more sprites to evaluate, thrash until HBLANK
							if (ppu.cycle & 1)
								{
									
									sprpointer = 0;
									sprnum = (sprnum + 1) & 63;
								}
							else	sprtmp = SPRRAM[sprpointer = sprnum << 2];
							break;
					}
		}
	else if (ppu.cycle< 320)
		{
			if ((ppu.cycle & 7) < 4)
				sprpointer = (ppu.cycle & 3) | (((ppu.cycle - 256) & 0x38) >> 1);
			else	sprpointer = (((ppu.cycle - 256) & 0x38) >> 1) | 3;
		}
	else
		{
			//sprcount[ppu.scanline]=0;
			sprpointer = 0;
		}


}
