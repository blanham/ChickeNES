/**
 `* APU.h
 `*
 `*  Created on: Sept 17, 2009
 `*      Author: blanham
 `*/
#include "blip/blip.h"
BYTE square2reg1, square2reg2, square2reg3, square2reg4;
BYTE apustatus, apuchannel;
void RunAPU(int apucycles);
static blip_buffer_t *blip;
