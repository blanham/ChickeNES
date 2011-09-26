/**
 `* pad.c
 `*
 `*  Created on: Jun 21, 2009
 `*      Author: blanham
 `*/
 #include <stdio.h>
 #include <stdlib.h>
 #include <SDL.h>
 #include "main.h"
int padenabled, pad2enabled, strobe, strobe2, jclk, jclk2 = 0;
//event structure


//joystick
SDL_Joystick* g_pStick;

//number of buttons
int g_nStickButtons;

//axis position(read only first two)
int g_StickAxis[2];

void input_init(void){
    	//check for joysticks
	if(SDL_NumJoysticks()==0)
	{
		printf("No joysticks!\n");
		//need to code keyboard support
	}
    fprintf(stdout,"Number of Joysticks: %d\n\n",SDL_NumJoysticks());

    //grab the first joystick
	g_pStick=SDL_JoystickOpen(0);

	//get the number of buttons
	g_nStickButtons=SDL_JoystickNumButtons(g_pStick);

	//enable joystick events
	SDL_JoystickEventState(SDL_ENABLE);

}


int padwrite(unsigned char input){
    if(input & 0x1){
    strobe = 1;
    }else if(((input & 0x1) == 0) && strobe == 1){
    padenabled = 1;
    //printf("Pad 1 enabled\n");

    jclk = 1;
    //poll SDL
    SDL_PollEvent(&g_Event);
    if(g_Event.type==SDL_QUIT) exit(0);
	switch(g_Event.type)
	{
		case SDL_KEYDOWN:
			if(g_Event.key.keysym.mod & KMOD_RSHIFT || g_Event.key.keysym.mod & KMOD_LSHIFT)
			{
				switch(g_Event.key.keysym.sym)
				{
					case SDLK_3:
					return 0xA3;
					break;
					
					case SDLK_8:
					return 0xAA;
					break;
				}
			}
			else
			switch(g_Event.key.keysym.sym)
			{
				case SDLK_PERIOD:
				initCPU();
				break;
			}
	
	
	}
	
	
	
    strobe = 0;
    }else{
  //  printf("Error in padwrite()!\n");
    //printf("strobe: %.2x jclk: %.2x\n", strobe, jclk);
   // exit(1);
    }
    return 0;
}

unsigned char padread(void)
{

    unsigned char output;
    if(padenabled==0){
    //printf("Error in padread()!\n");
    return 0;
    }
   //SDL_PollEvent(&g_Event);

    switch(jclk)
    {
    //A
    case 1:
        //printf("A!\n");
        if(SDL_JoystickGetButton(g_pStick,1) || ( g_Event.key.keysym.sym == SDLK_a)){
        //printf("PRESSED!\n");
        output = 0x41;
        }else output = 0x40;
        break;
    //B
    case 2:
       // printf("B!\n");
        if(SDL_JoystickGetButton(g_pStick,2) || ( g_Event.key.keysym.sym == SDLK_b)){
       // printf("PRESSED!\n");
        output = 0x41;
        }else output = 0x40;

        break;
    //SELECT
    case 3:
       // printf("SELECT!\n");
        if(SDL_JoystickGetButton(g_pStick,4)|| ( g_Event.key.keysym.sym == SDLK_SPACE)){
       // printf("PRESSED!\n");
        output = 0x41;
        }else output = 0x40;
        break;
    //START
    case 4:
       // printf("START!\n");
        if(SDL_JoystickGetButton(g_pStick,3) || (g_Event.key.keysym.sym == SDLK_RETURN)){
     //   printf("PRESSED!\n");
        output = 0x41;
        }else output = 0x40;
        break;
    //UP
    case 5:
       // printf("UP!\n");
        if((SDL_JoystickGetAxis(g_pStick,1)<128) || ( g_Event.key.keysym.sym == SDLK_UP)){
       // printf("PRESSED!\n");
        output = 0x41;
        }else output = 0x40;
        break;
    //DOWN
    case 6:
       // printf("DOWN!\n");
        if((SDL_JoystickGetAxis(g_pStick,1)>128) || ( g_Event.key.keysym.sym == SDLK_DOWN)){
        //printf("PRESSED!\n");
        output = 0x41;
        }else output = 0x40;
        break;
    //LEFT
    case 7:
      //  printf("LEFT!\n");
        if((SDL_JoystickGetAxis(g_pStick,0)<128) || ( g_Event.key.keysym.sym == SDLK_LEFT)){
        //printf("PRESSED!\n");
        output = 0x41;
        }else output = 0x40;
        break;
    //RIGHT
    case 8:
       // printf("RIGHT!\n");
        if((SDL_JoystickGetAxis(g_pStick,0)>128) || ( g_Event.key.keysym.sym == SDLK_RIGHT)){
      //  printf("PRESSED!\n");
        output = 0x41;
        }else output = 0x40;
        break;
    default:
        printf("pad.c broken\n");
        return 1;
    }
    jclk++;
	g_Event.key.keysym.sym = 0;

    if (jclk == 9)
    {
       // printf("8th bit!\n");
        jclk=0;
    }

    return output;
}

int padwrite2(unsigned char input){

    /*
    if(input & 0x1){
    //strobe2 = 1;
    }else if(((input & 0x1) == 0) && strobe == 1){
    pad2enabled = 1;
    printf("Pad 2 enabled\n");

    jclk2 = 1;
    //poll SDL
    //SDL_PollEvent(&g_Event);
    strobe2 = 0;
    }else{
    printf("Error in padwrite2()!\n");
    printf("strobe: %.2x jclk: %.2x\n", strobe, jclk2);

    }
    */
    return 0;
}

int padread2(void)
{

    unsigned char output;
    if(pad2enabled==0){
    //printf("Error in padread()!\n");
    //return 0;
    }


    switch(jclk)
    {
    //A
    case 1:
        printf("A!\n");

        break;
    //B
    case 2:
        printf("B!\n");


        break;
    //SELECT
    case 3:
        printf("SELECT!\n");

        break;
    //START
    case 4:
        printf("START!\n");

        break;
    //UP
    case 5:
        printf("UP!\n");

        break;
    //DOWN
    case 6:
        printf("DOWN!\n");

        break;
    //LEFT
    case 7:
        printf("LEFT!\n");

        break;
    //RIGHT
    case 8:
        printf("RIGHT!\n");

        break;
    default:
        printf("pad.c broken\n");
        return 1;
    }
    jclk2++;


    if (jclk == 17)
    {
        printf("8th bit!\n");
        jclk=0;
    }
    output = 0x40;
    return output;
}
