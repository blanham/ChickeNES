#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "main.h"
#include "B6502.h"
#include "pad.h"
#include "PPU.h"
#include "APU.h"
#include "mapper.h"
#include "config.h"
#include "int2bin.h"

extern int multiplier;

//mapper number, mirroring, region, battery
unsigned char mapper, mirror, region, battery;
//switches, need to have better names
int flip = 0;
//establish pointers for rom and ram address space
unsigned char *PRG, *CHR, *RAM, *SPRRAM;
unsigned char lRAM[0x800],CHRRAM[0x2000],VRAM[0x4000];
unsigned char NT[0x1eff], PAL[0xFF];

//establish array of pointers for swapping
uint8_t *pRAM[16]={lRAM,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
uint8_t *pVRAM[8]={&VRAM[0],&VRAM[0x800],&VRAM[0x800*2],&VRAM[0x800*3]};
uint8_t *pNT[4]={&NT[0x0],&NT[0x400],&NT[0x400*2],&NT[0x400*3]};
/* OpenRom opens up file, processes iNES header, and places the PRG and
 * CHR roms in their respective address spaces.
 */
int OpenROM(char *filename)
{
    typedef struct
    {
        char NES[4];
        char PRG_size;
        char CHR_size;
        char ROM_info;
        char ROM_info2;
        char Region;
        char INES_reserved[7];

    } iNES;

    FILE *fp;
    iNES header;

    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("File not found!\n");
        fclose(fp);
        exit(1);
    }

    fread(&header, 1, sizeof(header), fp);
    if (memcmp(header.NES, "NES\x1A", 4) == 0)
    {
        printf("Detected iNES file: %s\n", filename);
    }
    else if (memcmp(header.NES, "UNIF", 4) == 0)
    {
        OpenUNIF(filename,fp);
    }
    else
    {
        printf("Not a valid iNES rom image!\n");
        exit(1);
    }

    //16kb PRG pages and 8kb CHR pages
    PRG_size = header.PRG_size * 0x4000;
    CHR_size = header.CHR_size * 0x2000;
    mapper = (header.ROM_info >> 4) + (header.ROM_info2 &= 0xf0);
    battery = header.ROM_info & 0x02;
    region = header.Region & 0x01;
    //for verification

    printf("PRG size: 0x%X\n", PRG_size);
    printf("CHR size: 0x%X\n", CHR_size);
    //reads region
    printf("Region:");
    if (region) printf("\tPAL\n");
    else printf("\tNTSC\n");

    //16 for PAL, 15 for NTSC
    if (region) multiplier =  16;
    else multiplier = 15;

    printf("Mapper: %i\n",mapper);
    if (battery) printf("Battery Save\n");
    if (header.ROM_info & 0x1) printf("Mirroring: Vertical\n");
    else if (header.ROM_info & 0x8) printf("Mirroring: Four Screen\n");
    else printf("Mirroring: Horizontal\n");

    mirror = header.ROM_info;


    //now we allocate the proper PRG memory
    if (PRG_size == 0x4000)
    {
        PRG = malloc(0x8000);
    }
    else
    {
        PRG = malloc(PRG_size);
    }
    // then read data into it:
    if (PRG_size >= 0x8000) fread(PRG, PRG_size, sizeof(char), fp);
    else
    {
        fread(PRG, PRG_size, sizeof(char), fp);
        memcpy(&PRG[0x4000], PRG, 0x4000);
    }
    //if CHR rom is present, allocate memory for it
    if (CHR_size != 0)
    {
        CHR = malloc(CHR_size);
        fread(CHR, CHR_size, sizeof(char), fp);
    }
    else CHR = NULL;


    //as we now have our needed data, we can close the rom file
    fclose(fp);
    return 0;
}

int OpenUNIF(char *filename, FILE *unif){
        char revision[4];
        printf("Detected UNIF file: %s\n", filename);
        fseek(unif, 0x20, SEEK_SET);
        fread(revision, 0x4, sizeof(char), unif);
        printf("UNIF Rev: \t%i\n", (int)revision);
        printf("UNIF not yet supported\n");
        exit(1);
}

void OpenRAM()
{
    RAM = malloc(0x10000);
    //VRAM = malloc(0x4000);
    SPRRAM = malloc(0x100);
    memset(RAM, 0x00, 0x10000);
    memset(CHR, 0x00, 0x800*4);

    //memset(VRAM, 0x00, 0x4000);
    memset(SPRRAM, 0x00, 0xff);
    if (!mapper)
    {
        memcpy(&RAM[0x8000],PRG,0x8000);
        pVRAM[0]=&CHR[0];
        pVRAM[1]= &CHR[0x800];
        pVRAM[2]=&CHR[0x800*2];
        pVRAM[3]= &CHR[0x800*3];


        //memcpy(VRAM, CHR, 0x2000);

    }
    else LoadMapper(mapper);
}

void ReleaseROM()
{
   // free(PRG);
    free(CHR);
}

void ReleaseRAM()
{
    free(RAM);
    //free(VRAM);
    free(SPRRAM);
}

void PrintRAM()
{
    FILE *cout;
    cout = fopen("spr.dmp", "wr");
 	fwrite(SPRRAM,0x100,1,cout);
    fwrite(&ppu.OAM_2,0x30,1,cout);
    fwrite(&spr_buf,0x100,1,cout);
    fclose(cout);
   
 	FILE *vout;
    vout = fopen("vram.dmp", "wr");
    //memcpy(&RAM[0],&lRAM[0],0x200);
    fwrite(pVRAM[0],0x2000,1,vout);
    fwrite(NT,0x1F00,1,vout);
    fwrite(PAL,0x100,1,vout);
    fclose(vout);

    FILE *out;
    out = fopen("ram.dmp", "wr");
    fwrite(RAM,0x10000,1,out);
    fclose(out);

}


void WriteRAM(uint16_t addr, uint8_t value)
{
    uint16_t Addr = addr >> 13;
    switch (Addr)
    {
    case 0:
        addr &= 0x7ff;
        RAM[addr] = value;
        break;
    case 1:
    case 2:
        WriteIO(addr, value);
        break;
    default:
        RAM[addr] = value;//what the fuck is this!?
        WriteMapper(addr, value);
        break;

    }


}

void WriteIO(uint16_t addr, uint8_t value)
{   int i, k, tmp;

    switch (addr)
    {

    case 0x2000:
        RAM[0x2000] = value;
        T &= 0x73FF;
        T |= (value & 0x03) << 10;

			//if( (value & 0x80) && !(RAM[0x2000] & 0x80) && (RAM[0x2000] & 0x80) )
			//nmi();
       // printf("PPU init: %.2x\n", value);
        break;
    case 0x2001:
        RAM[0x2001] = value;
        break;
    case 0x2002:
        RAM[0x2002] = value;
        printf("shouldn't write here!\n");
        exit(1);
        break;
    case 0x2003:
        RAM[0x2003] = value;
        //printf("\tSPRaddr: %.2x\n", RAM[0x2003]);

        break;
    case 0x2004:
        if(ppu.rendering) value = 0xFF;
        if((RAM[0x2003]&3)==2)
            value &= 0xE3;
        SPRRAM[RAM[0x2003]++] = value;

        break;
    case 0x2005:

        if (flip == 0)
        {
            lpyX = (value & 0x7) ;
            T &= 0x7FE0;
            T |= ((value&0xF8)>>3);
        }
        else if (flip == 1)
        {
          T &= 0xC1F;
				T |= ((value & 0xF8)<<2) | ((value & 0x07)<<12);
        }
        flip ^= 1;

        break;
    case 0x2006:
        if (flip == 0)
        {
            T&=0x00FF;
            T |= (value & 0x3F) << 8;
        }
        else if (flip == 1)
        {
               T&=0x7F00;
               T |= value;
               V=T;
        }
        flip ^= 1;
        break;

    case 0x2007:
        if (RAM[0x2000] & 0x80) break;
        wrVRAM(V&0x3FFF,value);

        //}
        //printf("\nPPUaddr:%.4x\n", V);

        if (RAM[0x2000] & 0x4)
        {
            V = V + 32;

        }
        else V = V + 1;
         if(ppu.rendering)       RunPPU(PPUcycles);

        break;

    case 0x4004:
        square2reg1 = value;
        break;
    case 0x4005:
        square2reg2 = value;
        break;
    case 0x4006:
        square2reg3 = value;
        break;
    case 0x4007:
        square2reg4 = value;
		break;
    case 0x4011:
       // printf("DMC1 init!\n");
       // RAM[0x4011] = value;
        break;
    case 0x4014:
        tmp = value <<8;
        for (i=0; i != 0x100; ++i)
        {
            k = ReadRAM(tmp|i);
            SPRRAM[(RAM[0x2003] + i)&0xFF] = k;


        }

        //CPUcycles = CPUcycles - 513;
        //RunPPU(204);
               //  if(ppu.rendering)       RunPPU(PPUcycles);

        break;
    case 0x4015:
        apuchannel = value;
       // printf("APU status! %x\n", RAM[addr]);
        break;
    case 0x4016:
        padwrite(value);
        break;
    case 0x4017:
        padwrite2(value);
        break;
    default:
        RAM[addr]=value;
        //printf("%x\n", RAM[addr]);
        break;
    }
}


int foo = 0;
uint8_t ReadRAM(uint16_t addr)
{
    uint8_t value = 0;
    uint16_t Addr = addr >> 13;
    switch (Addr)
    {
    case 0:
        addr &= 0x7ff;
        value = RAM[addr];
        break;
    case 1:
    case 2:
        value = ReadIO(addr);
        break;
    case 4:
        value = RAM[addr];
        break;
    default:
        value = RAM[addr];
        break;

    }
    //printf("RR: %.4x\n",addr);
    return value;
}

uint8_t ReadIO(uint16_t addr)
{
    uint8_t value = 0;
    switch (addr)
    {
    case 0x2002:
        //RunPPU(Mcycles);
        value = RAM[0x2002];
        flip = 0;
        RAM[0x2002] &= ~(0x80|0x40);
        break;
    case 0x2004:
        //value = SPRRAM[RAM[0x2003]];
        RAM[0x2003]++;
        break;

    case 0x2007:
        if (FirstREAD)
        {
            FirstREAD=0;
            value = 0x00;
        }else{



        value = rdVRAM(V&0x3FFF);



        if (RAM[0x2000] & 0x4)
        {
            V = V + 32;
        }
        else
        {
            V = V + 1;
        }
#ifdef PRNTSTAT
            printf("%.4x\n",V);
#endif
        }
        break;
    case 0x4000: case 0x4001: case 0x4002: case 0x4003:
    case 0x4004: case 0x4005: case 0x4006: case 0x4007:
    case 0x4008: case 0x4009: case 0x400A: case 0x400B:
    case 0x400C: case 0x400D: case 0x400E: case 0x400F:
        value = 0x40;
        break;

    case 0x4015:
        value = apustatus;
        break;

    case 0x4016:
        value = padread();
        break;
    case 0x4017:
        //value = padread2();
        value = 0x40;
        break;
    default:
        value = RAM[addr];
        break;
    }

    return value;
}