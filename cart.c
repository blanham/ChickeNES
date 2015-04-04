#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cart.h"

typedef struct {
    char NES[4];
    char PRG_size;
    char CHR_size;
    char ROM_info;
    char ROM_info2;
    char Region;
    char INES_reserved[7];
} iNES;

/* OpenRom opens up file, processes iNES header, and places the PRG and
 * CHR roms in their respective address spaces.
 */
struct cart *cart_load(char *filename)
{
    FILE *fp;
    iNES header;
    struct cart *cart;

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
    cart->prg_size = header.PRG_size * 0x4000;
    cart->chr_size = header.CHR_size * 0x2000;
    cart->mapper = (header.ROM_info >> 4) + (header.ROM_info2 &= 0xf0);
    cart->battery = header.ROM_info & 0x02;
    cart->region = header.Region & 0x01;
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

    if (header.ROM_info & 0x1)
        printf("Mirroring:\n\tVertical\n");
    else if (header.ROM_info & 0x8)
        printf("Mirroring:\n\tFour Screen\n");
    else
        printf("Mirroring:\n\tHorizontal\n");

    cart->mirroring = header.ROM_info;

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

    //if CHR rom is present, allocate memory for it
    if (cart->chr_size > 0) {
        cart->chr_rom = malloc(cart->chr_size);
        fread(cart->chr_rom, cart->chr_size, 1, fp);
    }

    fclose(fp);

    return cart;
}