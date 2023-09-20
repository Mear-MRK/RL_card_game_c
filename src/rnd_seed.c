#include "rnd_seed.h"

#if defined(_WIN32)
#include <process.h>
#define GETPID _getpid
#else
#include <unistd.h>
#define GETPID getpid
#endif

#if defined(__unix) || defined(__linux__) || defined(__MACH__)
#include <sys/types.h>
#include <sys/random.h>
#endif

#include <stdlib.h>
#include <time.h>

#include "log.h"
#include "pcg.h"

// Robert Jenkins' 96 bit Mix Function
static uint64_t rob_jenk_mix(uint64_t a, uint16_t b, uint64_t c)
{
    a = a - b;
    a = a - c;
    a = a ^ (c >> 13);
    b = b - c;
    b = b - a;
    b = b ^ (a << 8);
    c = c - a;
    c = c - b;
    c = c ^ (b >> 13);
    a = a - b;
    a = a - c;
    a = a ^ (c >> 12);
    b = b - c;
    b = b - a;
    b = b ^ (a << 16);
    c = c - a;
    c = c - b;
    c = c ^ (b >> 5);
    a = a - b;
    a = a - c;
    a = a ^ (c >> 3);
    b = b - c;
    b = b - a;
    b = b ^ (a << 10);
    c = c - a;
    c = c - b;
    c = c ^ (b >> 15);

    return c;
}

uint64_t gen_seed(void)
{
    uint64_t a = clock(), b = time(NULL), c = GETPID();
    log_msg(debug, "time(): %lu, clock(): %ld, CLOCKS_PER_SEC: %ld, pid: %lu\n",
            b, a, CLOCKS_PER_SEC, c);

    pcg_seed(a ^ b ^ c);
    a ^= ((uint64_t)pcg_uint32()) << 32 | ((uint64_t)pcg_uint32());
    b ^= ((uint64_t)pcg_uint32()) << 32 | ((uint64_t)pcg_uint32());
    c ^= ((uint64_t)pcg_uint32()) << 32 | ((uint64_t)pcg_uint32());

    uint64_t res = 0ull;
#if defined(__unix) || defined(__linux__) || defined(__MACH__)
    getrandom((void *)&res, sizeof(res), GRND_RANDOM);
#endif
    res ^= rob_jenk_mix(a, b, c);
    log_msg(debug, "Random SEED: %lu\n", res);
    return res;
}