#pragma once

#include "spectre.h"

namespace spectre {
    class Meltdown : public spectre::Base {
        virtual void before();

        virtual void after();

        virtual void probe_bit(int tries, uint8_t mask, const uint8_t* data, const uint8_t* cache_probe);
    };
};
