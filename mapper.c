/*
 * mapper.c
 *
 *  Created on: June 27, 2009
 *      Author: blanham
 */
#include <stdio.h>
#include "main.h"
#include "B6502.h"
int data, reset = 0;
WORD mmc1;
int count=0;
int LoadMapper(int mapper){
    switch(mapper){

        case 1:
            memcpy(&RAM[0x8000],PRG,0x4000);
            memcpy(&RAM[0xc000],&PRG[0x4000],0x4000);
            if(CHR_size) memcpy(VRAM, CHR, 0x2000);
            count=1;
            break;

        case 66:
            memcpy(&RAM[0x8000],&PRG[0x8000],0x4000);
            memcpy(&RAM[0xc000],&PRG[0xc000],0x4000);
            memcpy(VRAM, CHR, 0x2000);
            break;

        default:
            printf("Unimplemented mapper!\n");
            exit(1);
            break;

    }

return 0;

}
int chrm,prgm = 0;
void WriteMapper(WORD addr, BYTE value){
   // printf("\nMapper Write!\n");
    //printf("asfasdf");
    switch(mapper){

        case 1:
        printf("Count:%i\n",count);
            if(count>5) count=1;
            if(value & 0x1){
                mmc1 = 0;

            }else if(value & 0x1){
            }

            if(count==5){
                if(addr >=0x8000 && addr<=0x9FFF){

                }

            }
            count++;
            break;

        case 66:
            value &= 0x33;

            if(value&0x30){
	memcpy(&RAM[0x8000],&PRG[(value>>4)*0x8000],0x8000);
//	printf("PC: %4x\n",PC);
	
}else{
	memcpy(&RAM[0x8000],&PRG[0x8000],0x8000);
//	printf("PCf: %4x\n",PC);
}

         //   memcpy(&VRAM[0], &CHR[((value))*0x2000], 0x2000);

if(value&0x3){
	value = value&0x3;
	pVRAM[0]=&CHR[0x2000*value];
    pVRAM[1]= &CHR[0x800+value*0x2000];
    pVRAM[2]=&CHR[0x800*2+value*0x2000];
    pVRAM[3]= &CHR[0x800*3+value*0x2000];
       

}else{


	 pVRAM[0]=&CHR[0];
        pVRAM[1]= &CHR[0x800];
        pVRAM[2]=&CHR[0x800*2];
        pVRAM[3]= &CHR[0x800*3];

}
            break;

        default:
            printf("Unimplemented mapper!\n");
            exit(1);
            break;

    }

}

