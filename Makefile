#glBNES Makefile
LDFLAGS=$(shell sdl2-config --libs) -framework OpenGL -framework Foundation 
CFLAGS=$(shell sdl2-config --cflags) -I./include -g -std=gnu11 -s -Wall# -O3 -fexpensive-optimizations 

SOURCES= main.c cart.c apu.c memory.c b6502.c ppu.c config.c mapper.c pad.c
OBJECTS = $(patsubst %.c,obj/%.o,$(SOURCES))

all: chickenes
chickenes: $(OBJECTS)
	$(CC) $(LDFLAGS) -o chickenes $(OBJECTS)

$(OBJECTS): | obj
obj:
	@mkdir -p $@

obj/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ 

.PHONY: clean
clean:
	rm -f obj/*.o
	rm -f chickenes
	rmdir obj
