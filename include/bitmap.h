// bitmap.h
#ifndef BITMAP_H
#define BITMAP_H

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
    size_t          size;
    size_t          alignment;
} Bitmap;

int createBitmap(size_t size, size_t alignment, Bitmap* bitmap);
void destroyBitmap(Bitmap *bitmap);
int setBit(Bitmap *bitmap, size_t index);
int clearBit(Bitmap *bitmap, size_t index);
int isBitSet(Bitmap *bitmap, size_t index);

#endif /* BITMAP_H */
