#pragma once

#include "spectre.h"

namespace spectre {


    class DataArray {
    public:
        size_t ALIGN_CACHE array_size;
        uint8_t ALIGN_CACHE *array;

        DataArray(int n=256);
        ~DataArray();
    };



    class FunctionArray {
        static void noop(uint8_t a, const uint8_t* b, const uint8_t* c);

    public:
        typedef void (*function_ptr_t)(uint8_t, const uint8_t*, const uint8_t*);

        size_t ALIGN_CACHE array_size;
        function_ptr_t ALIGN_CACHE *array;

        FunctionArray(int n=256);
        ~FunctionArray();
    };



    class DataArrayBoundCheckBypass : public spectre::Base {
        DataArray ALIGN_CACHE data_array;
        void victim(size_t index);
    public:
        virtual void probe_bit(int tries, uint8_t mask, const uint8_t* data, const uint8_t* cache_probe);
    };

    class FunctionArrayBoundCheckBypass : public spectre::Base {
        FunctionArray ALIGN_CACHE function_array;

    public:
        static void exploit(uint8_t mask, const uint8_t* target_ptr, const uint8_t* cache_probe);

        virtual void probe_bit(int tries, uint8_t mask, const uint8_t* data, const uint8_t* cache_probe);
    };
};
