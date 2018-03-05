#pragma once

#include "spectre.h"

namespace spectre {
    class Direct : public spectre::Base {
        virtual void probe_bit(int tries, uint8_t mask, const uint8_t* data, const uint8_t* cache_probe);
    };
};
