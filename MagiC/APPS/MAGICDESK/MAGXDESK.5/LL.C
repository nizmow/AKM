#include "MAGXDESK_GEM.H"

void ullmul(unsigned long m1, unsigned long m2, unsigned long result[2])
{
    uint64_t product = (uint64_t) m1 * (uint64_t) m2;

    result[0] = (unsigned long) (product >> 32);
    result[1] = (unsigned long) product;
}
