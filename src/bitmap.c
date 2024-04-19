#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "bitmap.h"
int createBitmap(size_t size, size_t alignment, Bitmap* bitmap)
{
    if (bitmap == NULL || size <= 0) return -EINVAL;

    if (bitmap->data != NULL) return -EFAULT;
    
    if (alignment <= 0 || (alignment & (alignment - 1)) != 0) return -EFAULT;
    
    size_t data = (size + BITS_PER_ELEMENT - 1)/ BITS_PER_ELEMENT;
    bitmap->data = (BitmapDataType *)aligned_alloc(alignment, data * sizeof(BitmapDataType));
    if (bitmap->data == NULL)
    {
        free(bitmap);
        return -ENOMEM;
    }

    bitmap->size = size;
    bitmap->alignment = alignment;
    for (size_t i = 0; i < data; i++)
    {
        if (i == data - 1)
        {
            bitmap->data[i] = (1ULL << (size % BITS_PER_ELEMENT)) - 1;
        }
        else
        {
            bitmap->data[i] = (BitmapDataType) - 1;
        }

    }

    return 0;
}

void destroyBitmap(Bitmap *bitmap)
{
    if (bitmap != NULL && bitmap->data != NULL)
    {
        free(bitmap->data);
        bitmap->data = NULL;
        bitmap->alignment = 0;
        bitmap->size = 0;
    }
}

int setBit(Bitmap *bitmap, size_t index)
{
   if (bitmap == NULL || bitmap->data == NULL || index >= bitmap->size) return -EINVAL;

   bitmap->data[index / BITS_PER_ELEMENT] |= (1ULL << (index % BITS_PER_ELEMENT));

   if (bitmap->data[index / BITS_PER_ELEMENT] & (1ULL << (index % BITS_PER_ELEMENT)) == 0ULL) return -EINVAL;

   return 0;
}

int clearBit(Bitmap *bitmap, size_t index)
{
    if (bitmap == NULL || bitmap->data == NULL || index >= bitmap->size) return -EINVAL;

    bitmap->data[index / BITS_PER_ELEMENT] &= ~(1ULL << (index % BITS_PER_ELEMENT));

    if (bitmap->data[index / BITS_PER_ELEMENT] & (1ULL << (index % BITS_PER_ELEMENT)) == 0ULL) return -EINVAL;

    return 0;
}

int isBitSet(Bitmap *bitmap, size_t index)
{
    if (bitmap == NULL || bitmap->data == NULL || index >= bitmap->size) return 0;

    return (bitmap->data[index / BITS_PER_ELEMENT] & (1ULL << (index % BITS_PER_ELEMENT))) != 0ULL ? 1 : 0;
}
