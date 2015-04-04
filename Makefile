#glBNES Makefile
CC=clang
#LINKER=gcc
#LDFLAGS=$(shell sdl-config --libs) -arch i386 -g -framework GLUT -framework OpenGL -framework Foundation -lpthread -lncurses
#CFLAGS=$(shell sdl-config --cflags) -arch i386 -std=c99 -s -Wall -O3 -fexpensive-optimizations 
                                                                
LDFLAGS=$(shell sdl2-config --libs) -framework OpenGL -framework Foundation 
CFLAGS=$(shell sdl2-config --cflags) -g -std=gnu11 -s -Wall# -O3 -fexpensive-optimizations 

SOURCES=  main.o cart.o apu.o memory.o b6502.o ppu.o config.o mapper.o pad.o

                                      
bnes: $(SOURCES)
	$(CC) $(LDFLAGS) -o bnes $(SOURCES)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# DO NOT DELETE

APU.o: main.h
B6502.o: main.h B6502.h int2bin.h
PPU.o: B6502.h main.h PPU.h
config.o: main.h config.h
main.o: main.h B6502.h pad.h PPU.h APU.h mapper.h config.h int2bin.h
mapper.o: main.h
pad.o: main.h
                                                                                                   
clean:
	rm *.o bnes

