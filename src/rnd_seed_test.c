#include "rnd_seed.h"

#include <stdio.h>

void rnd_seed_test(void)
{
    uint64_t seeds[10] = {0};
    for(int i = 0; i < 10; i++)
        seeds[i] = gen_seed();
    for(int i = 0; i < 10; i++)
        printf("Seed: %lu\n", seeds[i]);
}