#ifndef C_NES_CART_H
#define C_NES_CART_H
#include <stdint.h>

struct _nes_cart {
    char *name;
    size_t prg_size;
    size_t chr_size;
    int region;
    int mapper;
    int battery;
    int mirroring;
    int multiplier;

    uint8_t *prg_rom;
    uint8_t *chr_rom;

	uint8_t *prg_pages[32];
	uint8_t *chr_pages[32];
};
typedef struct _nes_cart nes_cart;

nes_cart *nes_cart_load(char *filename);
#endif
