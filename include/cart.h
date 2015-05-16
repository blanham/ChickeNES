#include <stdint.h>

struct cart {
    char *name;
    int prg_size;
    int chr_size;
    int region;
    int mapper;
    int battery;
    int mirroring;
    int multiplier;

    uint8_t *prg_rom;
    uint8_t *chr_rom;
};

struct cart *cart_load(char *filename);