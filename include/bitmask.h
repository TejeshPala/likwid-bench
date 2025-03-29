// bitmask.h
#ifndef BITMASK_H
#define BITMASK_H


#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>


static inline int is_multipleof_pow2(const size_t in, const size_t nbits)
{
    if (nbits == 0 || (nbits & (nbits - 1)) != 0)
    {
        return -EINVAL;
    }
    return ((in & (nbits - 1)) == 0);
}

static inline int is_multipleof_nbits(const size_t in, const size_t nbits)
{
    if (nbits == 0)
    {
        return -EINVAL;
    }
    return (in % nbits == 0);
}

static inline int roundup_nbits(size_t* out, const size_t in, const size_t nbits)
{
    if (!out || nbits == 0)
    {
        return -EINVAL;
    }
    if (in > SIZE_MAX - (nbits - 1))
    {
        return -EOVERFLOW;
    }
    *out = ((in + nbits - 1) / nbits) * nbits;
    return 0;
}

static inline int rounddown_nbits(size_t* out, const size_t in, const size_t nbits)
{
    if (!out || nbits == 0)
    {
        return -EINVAL;
    }
    *out = (in / nbits ) * nbits;
    return 0;
}

static inline int roundup_nbits_pow2(size_t* out, const size_t in, const size_t nbits)
{
    if (!out || nbits == 0 || (nbits & (nbits - 1)) != 0)
    {
        return -EINVAL;
    }
    if (in > SIZE_MAX - (nbits - 1))
    {
        return -EOVERFLOW;
    }
    *out = (in + nbits - 1) & ~(nbits - 1);
    return 0;
}

static inline int rounddown_nbits_pow2(size_t* out, const size_t in, const size_t nbits)
{
    if (!out || nbits == 0 || (nbits & (nbits - 1)) != 0)
    {
        return -EINVAL;
    }
    *out = in & ~(nbits - 1);
    return 0;
}

static inline int roundnearest_nbits_pow2(size_t* out, const size_t in, const size_t nbits)
{
    if (!out || nbits == 0 || (nbits & (nbits - 1)) != 0)
    {
        return -EINVAL;
    }
    if (in > SIZE_MAX - (nbits >> 1))
    {
        return -EOVERFLOW;
    }
    *out = (in + (nbits >> 1)) & ~(nbits - 1);
    return 0;
}

static inline int roundnearest_nbits(size_t* out, const size_t in, const size_t nbits)
{
    if (!out || nbits == 0)
    {
        return -EINVAL;
    }
    if (nbits > SIZE_MAX / 2 || in >= SIZE_MAX - (nbits / 2))
    {
        return -EOVERFLOW;
    }
    *out = ((in + (nbits / 2)) / nbits) * nbits;
    return 0;
}

#endif /* BITMASK_H */
