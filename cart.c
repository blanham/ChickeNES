#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cart.h"

typedef struct {
	uint8_t NES[4];
	uint8_t prg_size;
	uint8_t chr_size;
	uint8_t rom_info;
	uint8_t rom_info2;
	uint8_t region;
	uint8_t ines_reserved[7];
} iNES;

/* OpenRom opens up file, processes iNES header, and places the PRG and
 * CHR roms in their respective address spaces.
 */
nes_cart *nes_cart_load(char *filename)
{
	FILE *fp;
	iNES header;
	nes_cart *cart;

	if ((fp = fopen(filename, "rb")) == NULL) {
		printf("File not found!\n");
		return NULL;
	}

	fread(&header, 1, sizeof(header), fp);

	if (memcmp(header.NES, "NES\x1A", 4)) {
		printf("Not a valid iNES rom image!\n");
		return NULL;
	}

	printf("Detected iNES file: %s\n", filename);

	cart = calloc(sizeof(*cart), 1);
	//16kb PRG pages and 8kb CHR pages
	cart->prg_size = header.prg_size * 0x4000;
	cart->chr_size = header.chr_size * 0x2000;
	cart->mapper = (header.rom_info >> 4) + (header.rom_info2 & 0xf0);
	cart->battery = header.rom_info & 0x02;
	cart->region = header.region & 0x01;
	//for verification

	printf("PRG size:\n\t0x%X\n", cart->prg_size);
	printf("CHR size:\n\t0x%X\n", cart->chr_size);

	//reads region
	printf("Region:\n\t%s\n", cart->region ? "PAL" : "NTSC");

	//16 for PAL, 15 for NTSC
	cart->multiplier = cart->region ? 16 : 15;

	printf("Mapper:\n\t%i\n",  cart->mapper);

	if (cart->battery)
		printf("Battery Save\n");

	if (header.rom_info & 0x1)
		printf("Mirroring:\n\tVertical\n");
	else if (header.rom_info & 0x8)
		printf("Mirroring:\n\tFour Screen\n");
	else
		printf("Mirroring:\n\tHorizontal\n");

	cart->mirroring = header.rom_info;

	//these prg conditionals are for the case
	//where the cart needs to be mirrored
	//we really should have a table of pointers
	//to pages instead of a monolithic block
	if (cart->prg_size == 0x4000) {
		cart->prg_rom = malloc(0x8000);
	} else {
		cart->prg_rom = malloc(cart->prg_size);
	}

	if (cart->prg_size >= 0x8000) {
		fread(cart->prg_rom, cart->prg_size, 1, fp);
	} else {
		fread(cart->prg_rom, cart->prg_size, 1, fp);
		memcpy(&cart->prg_rom[0x4000], cart->prg_rom, 0x4000);
	}

	for (int i = 0; i < cart->prg_size / 0x4000; i++) {
		fprintf(stderr, "Time %i\n", i);
		cart->prg_pages[i] = &cart->prg_rom[0x4000 * i];
	}
	fprintf(stderr, "0 %x\n", cart->prg_pages[0][0]);
	//if CHR rom is present, allocate memory for it
	if (cart->chr_size > 0) {
		cart->chr_rom = malloc(cart->chr_size);
		fread(cart->chr_rom, cart->chr_size, 1, fp);
	}

	fclose(fp);

	return cart;
}
