// bitmap.h
#ifndef BITMAP_H
#define BITMAP_H

#include <inttypes.h>
#include <stdint.h>
#include <stddef.h>


#if defined(__LP64__) || defined(_LP64)
    typedef uint64_t BitmapDataType;
    #define BITS_PER_ELEMENT 64
#else
    typedef uint32_t BitmapDataType;
    #define BITS_PER_ELEMENT 32
#endif

typedef struct {
    BitmapDataType *data;
    uint64_t          size;
    uint64_t          alignment;
} Bitmap;

int create_bitmap(uint64_t size, uint64_t alignment, Bitmap* bitmap);
void destroy_bitmap(Bitmap *bitmap);
int set_bit(Bitmap *bitmap, uint64_t index);
int clear_bit(Bitmap *bitmap, uint64_t index);
int is_bit_set(Bitmap *bitmap, uint64_t index);
void print_set_bits(const Bitmap *bitmap);

#endif /* BITMAP_H */
