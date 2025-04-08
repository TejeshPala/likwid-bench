// bitmask.h
#ifndef BITMASK_H
#define BITMASK_H


#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>


static inline int is_multipleof_pow2(const uint64_t in, const uint64_t nbits)
{
    if (nbits == 0 || (nbits & (nbits - 1)) != 0)
    {
        return -EINVAL;
    }
    return ((in & (nbits - 1)) == 0);
}

static inline int is_multipleof_nbits(const uint64_t in, const uint64_t nbits)
{
    if (nbits == 0)
    {
        return -EINVAL;
    }
    return (in % nbits == 0);
}

static inline int roundup_nbits(uint64_t* out, const uint64_t in, const uint64_t nbits)
{
    if (!out || nbits == 0)
    {
        return -EINVAL;
    }
    if (in > UINT64_MAX - (nbits - 1))
    {
        return -EOVERFLOW;
    }
    *out = ((in + nbits - 1) / nbits) * nbits;
    return 0;
}

static inline int rounddown_nbits(uint64_t* out, const uint64_t in, const uint64_t nbits)
{
    if (!out || nbits == 0)
    {
        return -EINVAL;
    }
    *out = (in / nbits ) * nbits;
    return 0;
}

static inline int roundup_nbits_pow2(uint64_t* out, const uint64_t in, const uint64_t nbits)
{
    if (!out || nbits == 0 || (nbits & (nbits - 1)) != 0)
    {
        return -EINVAL;
    }
    if (in > UINT64_MAX - (nbits - 1))
    {
        return -EOVERFLOW;
    }
    *out = (in + nbits - 1) & ~(nbits - 1);
    return 0;
}

static inline int rounddown_nbits_pow2(uint64_t* out, const uint64_t in, const uint64_t nbits)
{
    if (!out || nbits == 0 || (nbits & (nbits - 1)) != 0)
    {
        return -EINVAL;
    }
    *out = in & ~(nbits - 1);
    return 0;
}

static inline int roundnearest_nbits_pow2(uint64_t* out, const uint64_t in, const uint64_t nbits)
{
    if (!out || nbits == 0 || (nbits & (nbits - 1)) != 0)
    {
        return -EINVAL;
    }
    if (in > UINT64_MAX - (nbits >> 1))
    {
        return -EOVERFLOW;
    }
    *out = (in + (nbits >> 1)) & ~(nbits - 1);
    return 0;
}

static inline int roundnearest_nbits(uint64_t* out, const uint64_t in, const uint64_t nbits)
{
    if (!out || nbits == 0)
    {
        return -EINVAL;
    }
    if (nbits > UINT64_MAX / 2 || in >= UINT64_MAX - (nbits / 2))
    {
        return -EOVERFLOW;
    }
    *out = ((in + (nbits / 2)) / nbits) * nbits;
    return 0;
}

#endif /* BITMASK_H */
