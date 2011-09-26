/**
 `* config.c
 `*
 `*  Created on: Jun 24, 2009
 `*      Author: blanham
 `*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "config.h"
//will use libxml2 eventually
int openconfig(void);
void newconfig(void);
FILE *cfg;
CONFIG config;

int openconfig(void)
{

    cfg = fopen("bnes.cfg", "r+");
    if (cfg == NULL)
    {
        fclose(cfg);
        printf("bnes.cfg not found!\n");
        newconfig();
    }
    fread(&config, 1, sizeof(config), cfg);
    fclose(cfg);
    if(memcmp(config.BNES, "BNES\x1A",5) == 0){
        printf("bnes.cfg found!\n");
    }else{
        printf("invalid cfg!");

        exit(1);
    }

    return 0;
}

void newconfig(void)
{
    cfg = fopen("bnes.cfg", "wr");
    if (cfg == NULL)
    {
        fclose(cfg);
        printf("bnes.cfg cannot be created!\n");
        exit(1);
    }
    fprintf(cfg, "BNES\x1A\n");

}

void configwrite(void){
    cfg = fopen("bnes.cfg", "r+");
    fwrite(&config, 1, sizeof(config), cfg);
    fclose(cfg);
}
