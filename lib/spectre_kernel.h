#pragma once

#include "spectre.h"
#include "spectre_boundcheck.h"

namespace spectre {
    class KernelBase : public spectre::Base {
    protected:
        FILE* file;

        FunctionArray::function_ptr_t *function_array;
        size_t function_array_size;
        size_t* function_array_size_ptr;
        uint8_t* phys_to_virt;

        virtual void before();
        virtual void after();
    };

    class KernelDataArrayBoundCheckBypass : public spectre::KernelBase {
        DataArray ALIGN_CACHE data_array;

        virtual void probe_bit(int tries, uint8_t mask, const uint8_t* data, const uint8_t* cache_probe);
    };

    class KernelFunctionArrayBoundCheckBypass : public spectre::KernelBase {
        virtual void probe_bit(int tries, uint8_t mask, const uint8_t* data, const uint8_t* cache_probe);
    };
};
