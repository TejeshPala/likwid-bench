#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "bitmap.h"
#include <error.h>

int create_bitmap(size_t size, size_t alignment, Bitmap* bitmap)
{
    if (bitmap == NULL || size == 0) return -EINVAL;

    if (bitmap->data != NULL) return -EFAULT;
    
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) return -EINVAL;
    
    size_t data_size = (size + BITS_PER_ELEMENT - 1)/ BITS_PER_ELEMENT;
    BitmapDataType* data = (BitmapDataType *)aligned_alloc(alignment, data_size * sizeof(BitmapDataType));
    if (data == NULL) return -ENOMEM;

    bitmap->data = data;
    bitmap->size = size;
    bitmap->alignment = alignment;
    memset(bitmap->data, 0, data_size * sizeof(BitmapDataType));

    return 0;
}

void destroy_bitmap(Bitmap *bitmap)
{
    if (bitmap != NULL && bitmap->data != NULL)
    {
        free(bitmap->data);
        bitmap->data = NULL;
        bitmap->alignment = 0;
        bitmap->size = 0;
    }
}

int set_bit(Bitmap *bitmap, size_t index)
{
    if (bitmap == NULL || bitmap->data == NULL || index >= bitmap->size) return -EINVAL;

    bitmap->data[index / BITS_PER_ELEMENT] |= (1ULL << (index % BITS_PER_ELEMENT));

   return 0;
}

int clear_bit(Bitmap *bitmap, size_t index)
{
    if (bitmap == NULL || bitmap->data == NULL || index >= bitmap->size) return -EINVAL;

    bitmap->data[index / BITS_PER_ELEMENT] &= ~(1ULL << (index % BITS_PER_ELEMENT));

    return 0;
}

int is_bit_set(Bitmap *bitmap, size_t index)
{
    if (bitmap == NULL || bitmap->data == NULL || index >= bitmap->size) return 0;

    return (bitmap->data[index / BITS_PER_ELEMENT] & (1ULL << (index % BITS_PER_ELEMENT))) != 0ULL;
}

void print_set_bits(const Bitmap *bitmap)
{
    if (bitmap == NULL || bitmap->data == NULL)
    {
        ERROR_PRINT("Bitmap invalid or data not found");
        return;
    }

    int found = 0;
    printf("[");
    for (size_t i = 0; i < bitmap->size; ++i)
    {
        if (is_bit_set((Bitmap*) bitmap, i))
        {
            if (!found) printf("%zu ", i);
            else printf(",%zu", i);
            found = 1;
        }
    }
    printf("]\n");
}
