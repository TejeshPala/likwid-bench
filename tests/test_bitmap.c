#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "bitmap.h"

int global_verbosity = DEBUGLEV_DEVELOP;

#define SEPARATOR "---------------------------------------\n"

typedef struct {
    int expected_result;
    size_t size;
    size_t alignment;
} TestBitmap;

void print_bitmap(Bitmap *bitmap)
{
    printf("Bitmap size: %lu.\n", bitmap->size);
    printf("Bitmap alignment: %lu.\n", bitmap->alignment);
    printf("Bitmap address: %p.\n", (void*)bitmap->data);
}

int test_bitmap(TestBitmap *test)
{
    printf(SEPARATOR);
    printf("Testing bitmap with size: %lu, alignment: %lu.\n", test->size, test->alignment);

    Bitmap bitmap = {NULL, 0, 0};
    int result = create_bitmap(test->size, test->alignment, &bitmap);
    if (result == test->expected_result)
    {
        if (result == 0) 
        {
            if (bitmap.data != NULL)
            {
                printf("Bitmap created successfully\n");
                print_bitmap(&bitmap);
                destroy_bitmap(&bitmap);
                printf("Bitmap destroyed successfully\n");
            }
            else
            {
                printf("Failed to create Bitmap\n");
                return -ENOMEM;
            }

        }
        else if (result == -EINVAL)
        {
            printf("Invalid test.\n");
        }
        else if (result == -ENOMEM)
        {
            printf("Out of Memory.\n");
        }
        else if (result == -EFAULT)
        {
            printf("Alignment issue.\n");
        }
        return result;
    }
    else
    {
        printf("Unexpected result for the test. Expected: %d, Actual: %d.\n", test->expected_result, result);
        return -EINVAL;
    }

    return 0;
}

static TestBitmap tests[] = 
{
    {.size = 0, .alignment = 8, .expected_result = -EINVAL},
    {.size = 100, .alignment = 0, .expected_result = -EINVAL},
    {.size = 128, .alignment = 8, .expected_result = 0},
#if !defined(__SANITIZE_ADDRESS__)
    {.size = 64, .alignment = 16, .expected_result = 0},
#endif
    {.size = 1024, .alignment = 32, .expected_result = 0},
    {.size = 1024, .alignment = 64, .expected_result = 0},
    {.size = SIZE_MAX, .alignment = 8, .expected_result = 0},
    {.size = 10000, .alignment = SIZE_MAX, .expected_result = -EINVAL},
};

int main()
{
    printf("==> Testing Bitmap Implementation\n");

    size_t num_tests = sizeof(tests) / sizeof(tests[0]);
    int ok = 0;
    int err = 0;
    
    for (int i = 0; i < num_tests; i++)
    {
        int result = test_bitmap(&tests[i]);
        if (result == tests[i].expected_result)
        {
            ok++;
        }
        else
        {
            err++;
        }

    }

    printf(SEPARATOR);
    printf("==>Testing Bitmap done\n");
    printf("Results: Total %ld, ok %d, err %d\n", num_tests, ok ,err);
    if (ok != num_tests || err != 0) printf("Error in Tests!\n"); // num_tests = Positive Tests + Negative Tests

    return 0;
}
