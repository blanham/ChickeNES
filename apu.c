/**
 `* APU.c
 `*
 `*  Created on: Sept 12, 2009
 `*      Author: blanham
 `*  Trash at the moment, need to improve timing before I get this to work
 `*/
 #include <stdio.h>
 #include <stdlib.h>
 #include <SDL.h>
 #include "main.h"
 //might want to use Blargg's blipbuffer library
#include "APU.h"
//#include "blip/blip.h"

enum { sample_rate = 44100 };
//static const double clock_rate = (double) sample_rate * blip_max_ratio;

//static blip_buffer_t* blip;

 //APU registers
 //square 2
BYTE square2reg1, square2reg2, square2reg3, square2reg4 = 0;
BYTE apustatus, apuchannel;

void RunSquare2(int something);
void test(void *a, Uint8* out, void *c)
{	

//	blip_read_samples( blip, out, 10000, 0 );
	
}
int initAPU(void)
{
//	blip = blip_new( sample_rate / 10 );
//	if ( blip == 0 )
//		exit( EXIT_FAILURE ); /* out of memory */
//	blip_set_rates( blip, clock_rate, sample_rate );
	
//	static SDL_AudioSpec as = { 44100, AUDIO_S16SYS, 1 };
	

	/* start audio */
//	as.callback =test;
//	as.samples = 1024;
//	if ( SDL_OpenAudio( &as, 0 ) < 0 )
//		printf("fuck");//	exit( EXIT_FAILURE );
//	SDL_PauseAudio( 0 );
    //will need to zero counters here
 //   return 0;
}

void RunAPU(int apucycles)
{
//
	RunSquare2(1);

}

void RunSquare2(int something)
{
	
   // blip_add_delta( blip,  square2reg3+(square2reg4&0x7)<<8 + ppu.cycle,  square2reg1&0xf );
	printf("S2reg1: %x r2: %x r3: %x r4: %x\n", square2reg1,square2reg2,square2reg3,square2reg4);

   // blip_add_delta( blip,  8, -10 );
    //blip_add_delta( blip, 12,  +5 );
    //blip_add_delta( blip, 16,  +5 );
    //blip_add_delta( blip, 20, +10 );
    //blip_add_delta( blip, 24,  +5 );
    //blip_add_delta( blip, 28,  +5 );
	//blip_end_frame( blip,1000);
	//square2reg3+(square2reg4&0x7)<<8);
	
    
}

