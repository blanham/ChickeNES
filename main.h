/*
 * main.h
 *
 *  Created on: May 29, 2009
 *      Author: blanham
 */
#ifdef __APPLE__
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#else
#include <SDL.h>
#include <SDL/SDL_opengl.h>
#endif
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <stdint.h>

SDL_Event g_Event;
SDL_WindowID mainwindow;

extern SDL_Surface *surface;	// This surface will tell us the details of the image
extern GLuint texture;			// This is a handle to our texture object
extern GLuint pboIds[2];   
typedef unsigned short WORD;
typedef unsigned char BYTE;
#define DEBUG
//#define PRNTSTAT
extern unsigned char *PRG,*CHR,*RAM,VRAM[0x4000],NT[0x1eff], PAL[0xFF],*SPRRAM;;
extern BYTE *pRAM[16];
extern uint8_t *pVRAM[8];
extern unsigned char *pNT[4];
extern unsigned char lRAM[0x800], CHRRAM[0x2000];
#define rdRAM(addr) pRAM [(addr) >> 12] [(addr) & 0x0FFF]
#define rdCHRRAM(addr) pVRAM [(addr) >> 11] [(addr) & 0x07FF]
#define rdNT(addr) pNT [(addr & 0xC00) >> 10] [(addr) & 0x03FF]
extern int NMI;
extern unsigned char mapper;
extern int CPUcycles;
extern int PRG_size;
extern int CHR_size;
extern int ROM_type;
extern int MMC_type;
extern int i;
extern int FirstREAD;
extern SDL_Surface* screen;
extern SDL_Surface* buffer;
extern SDL_Rect g_SrcRect,g_DstRect;
void nmi();
extern BYTE region;

void WriteRAM(WORD addr, BYTE value);
BYTE ReadRAM(WORD addr);

//function prototypes

int OpenROM(char *filename);
int OpenUNIF(char *filename, FILE *unif);
void ReleaseROM();
void ReleaseRAM();
void PrintRAM();
void WriteRAM(WORD addr, BYTE value);
void WriteIO(WORD addr, BYTE value);
BYTE ReadRAM(WORD addr);
BYTE ReadIO(WORD addr);
void OpenRAM();
void printp();
int initSDL(void);
void initCPU(void);
void RunEMU(int scanlines);
int RunCPU(int scanlines);
