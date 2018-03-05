#pragma once

#include "common.h"
#include "libflush++.h"

namespace spectre {

    class Base {
    public:
        uint8_t ALIGN_CACHE cache_probe[256 * CACHE_LINE_SIZE];

        LibFlush libflush;

        /**
         * Tiny delay to wait for the cache flush to complete (Could also use mfence)
         */
        static inline void fence() {
            for (volatile int z = 0; z < 100; z++) {}
        }

        virtual ~Base();

        /**
        * Performs any initialization before probing.
        * Raises a string with an error message if anything went wrong.
        */
        virtual void before();

        /**
        * Finalize probing, reverses before()
        */
        virtual  void after();

        /**
        * Accesses the value in cache_probe[(*data & mask) * CACHE_LINE_SIZE]
        *
        * The gotcha is that access should be made speculatively, so that we can fetch priviledged data without the CPU realizing it 😈
        */
        virtual void probe_bit(int tries, uint8_t mask, const uint8_t* data, const uint8_t* cache_probe) = 0;

        /* Report best guess in value[0] and runner-up in value[1] */
        void readMemoryByte(const uint8_t* target_ptr, uint8_t value[2], int score[2]);


        void dump(const uint8_t* target_ptr, size_t target_len);
    };
}
