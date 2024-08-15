#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "bstrlib.h"
#include "error.h"

#include "allocator.h"
#include "test_types.h"
#include "bitmap.h"

int global_verbosity = DEBUGLEV_DEVELOP;

#define SEPARATOR "---------------------------------------\n"

typedef struct{
    int expected_result;
    RuntimeStreamConfig config;
} TestStreamConfig;

static struct tagbstring default_stream_name = bsStatic("TESTSTREAM");

int ival = 10;
double dval = 7.123456;
float fval = 7.123456f;
int64_t i64val = LONG_MAX;
#ifdef WITH_HALF_PRECISION
_Float16 hval = 7.123456;
#endif

const char* get_stream_typename(TestConfigStreamType type)
{
    switch (type)
    {
        case TEST_STREAM_TYPE_INT: return "INT";
        case TEST_STREAM_TYPE_DOUBLE: return "DOUBLE";
        case TEST_STREAM_TYPE_SINGLE: return "SINGLE";
        case TEST_STREAM_TYPE_INT64: return "INT64";
#ifdef WITH_HALF_PRECISION
        case TEST_STREAM_TYPE_HALF: return "HALF";
#endif
        default: return "unknown";
    }
}

void print_array(int64_t* arr, int size)
{
    printf("[");
    for (int i = 0; i < size; i++)
    {
        printf("%" PRId64, arr[i]);
        if (i < size-1) printf(", ");
    }
    printf("]");
}

int test_stream_config(TestStreamConfig* tconfig)
{
    printf(SEPARATOR);
    printf("Name of str: %s\n", bdata(tconfig->config.name));
    int result = allocate_arrays(&tconfig->config);
    if (result == tconfig->expected_result)
    {
        if (result == 0) //positive tests
        {
            if (tconfig->config.ptr != NULL) // Allocation check and printing fields of tconfig
            {
                printf("%dD array of Datatype: %s allocated succesfully.\n", tconfig->config.dims, get_stream_typename(tconfig->config.type));
                printf("Allocated memory address: %p\n", tconfig->config.ptr);
                printf("Dimensions: %d\n", tconfig->config.dims);
                printf("Dimension Sizes: ");
                print_array(tconfig->config.dimsizes, tconfig->config.dims);
                printf("\nOffsets: ");
                print_array(tconfig->config.offsets, tconfig->config.dims);
                printf("\nId: %d\n", tconfig->config.id);
                printf("Array Datatype: %s\n", get_stream_typename(tconfig->config.type));
                tconfig->config.init = init_function;
                initialize_arrays(&tconfig->config);
                print_arrays(&tconfig->config);
                release_arrays(&tconfig->config);
                printf("%dD array of Datatype: %s deallocated succesfully.\n", tconfig->config.dims, get_stream_typename(tconfig->config.type));
            }
            else // in case of allocation failure
            {
                fprintf(stderr, "Memory not allocated!\n");
                printf("Failed to allocate %dD array of type: %s (Error: %d).\n", tconfig->config.dims, get_stream_typename(tconfig->config.type), result);
                return -EINVAL;
            }
        }
        else if (result == -ENOMEM) // if out of mem cases
        {
            printf("Out of Memory for %dD %s array (Error: %d).\n", tconfig->config.dims, get_stream_typename(tconfig->config.type), result);
            return -ENOMEM;
        }
        else if (result == -EINVAL) // for negative tests
        {
            printf("Issue in test config, either of the config's is not allowed for id: %d\n", tconfig->config.id);    
        }
        return 0; //already expected_result validated and only for tests calculation
    }
    else // typical error case of any error type
    {
        printf("Unexpected result for %dD %s array (Error expected: %d, actual: %d).\n", tconfig->config.dims, get_stream_typename(tconfig->config.type), tconfig->expected_result, result);
        return -EINVAL;
    }

    return 0;
}

/* Test Configuration: 0 - Positive tests, -EINVAL - Negative tests*/
static TestStreamConfig testconfigs[] = {
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {40},
            .offsets = {0},
            .dims = 1,
            .flags = 0,
            .id = 1,
            .type = TEST_STREAM_TYPE_INT,
            .init_val = &ival,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {80},
            .offsets = {0},
            .dims = 1,
            .flags = 0,
            .id = 2,
            .type = TEST_STREAM_TYPE_DOUBLE,
            .init_val = &dval,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {40},
            .offsets = {0},
            .dims = 1,
            .flags = 0,
            .id = 3,
            .type = TEST_STREAM_TYPE_SINGLE,
            .init_val = &fval,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {40, 40},
            .offsets = {0, 0},
            .dims = 2,
            .flags = 0,
            .id = 4,
            .type = TEST_STREAM_TYPE_INT,
            .init_val = &ival,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {80, 80},
            .offsets = {0, 0},
            .dims = 2,
            .flags = 0,
            .id = 5,
            .type = TEST_STREAM_TYPE_DOUBLE,
            .init_val = &dval,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {20, 20},
            .offsets = {0, 0},
            .dims = 2,
            .flags = 0,
            .id = 6,
            .type = TEST_STREAM_TYPE_SINGLE,
            .init_val = &fval,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {40, 20},
            .offsets = {0, 0},
            .dims = 2,
            .flags = 0,
            .id = 7,
            .type = TEST_STREAM_TYPE_INT,
            .init_val = &ival,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {40, 80},
            .offsets = {0, 0},
            .dims = 2,
            .flags = 0,
            .id = 8,
            .type = TEST_STREAM_TYPE_DOUBLE,
            .init_val = &dval,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {8, 40},
            .offsets = {0, 0},
            .dims = 2,
            .flags = 0,
            .id = 9,
            .type = TEST_STREAM_TYPE_SINGLE,
            .init_val = &fval,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {-2},
            .offsets = {0},
            .dims = 1,
            .flags = 0,
            .id = 10,
            .type = TEST_STREAM_TYPE_INT,
        },
        .expected_result = -EINVAL
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {-10, -10},
            .offsets = {0, 0},
            .dims = 2,
            .flags = 0,
            .id = 11,
            .type = TEST_STREAM_TYPE_DOUBLE,
        },
        .expected_result = -EINVAL
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {40, 40, 40},
            .offsets = {0, 0, 0},
            .dims = 3,
            .flags = 0,
            .id = 12,
            .type = TEST_STREAM_TYPE_INT,
            .init_val = &ival,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {80, 80, 80},
            .offsets = {0, 0, 0},
            .dims = 3,
            .flags = 0,
            .id = 13,
            .type = TEST_STREAM_TYPE_DOUBLE,
            .init_val = &dval,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {20, 20, 20},
            .offsets = {0, 0, 0},
            .dims = 3,
            .flags = 0,
            .id = 14,
            .type = TEST_STREAM_TYPE_SINGLE,
            .init_val = &fval,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {8, 40, 40},
            .offsets = {0, 0, 0},
            .dims = 3,
            .flags = 0,
            .id = 15,
            .type = TEST_STREAM_TYPE_INT,
            .init_val = &ival,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {40, 8, 80},
            .offsets = {0, 0, 0},
            .dims = 3,
            .flags = 0,
            .id = 16,
            .type = TEST_STREAM_TYPE_DOUBLE,
            .init_val = &dval,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {20, 8, 40},
            .offsets = {0, 0, 0},
            .dims = 3,
            .flags = 0,
            .id = 17,
            .type = TEST_STREAM_TYPE_SINGLE,
            .init_val = &fval,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {10, 2, 10},
            .offsets = {0, 0, 0},
            .dims = 3,
            .flags = 0,
            .id = 18,
            .type = TEST_STREAM_TYPE_HALF,
#ifdef WITH_HALF_PRECISION
            .init_val = &hval,
#endif
        },
#ifdef WITH_HALF_PRECISION
        .expected_result = 0
#else
        .expected_result = -EINVAL
#endif
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {40, 80},
            .offsets = {0, 0},
            .dims = 2,
            .flags = 0,
            .id = 19,
            .type = TEST_STREAM_TYPE_INT64,
            .init_val = &i64val,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {100},
            .offsets = {0},
            .dims = 1,
            .flags = 0,
            .id = 20,
            .type = TEST_STREAM_TYPE_HALF,
#ifdef WITH_HALF_PRECISION
            .init_val = &hval,
#endif
        },
#ifdef WITH_HALF_PRECISION
        .expected_result = 0
#else
        .expected_result = -EINVAL
#endif
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {100, 100},
            .offsets = {0, 0},
            .dims = 2,
            .flags = 0,
            .id = 21,
            .type = TEST_STREAM_TYPE_HALF,
#ifdef WITH_HALF_PRECISION
            .init_val = &hval,
#endif
        },
#ifdef WITH_HALF_PRECISION
        .expected_result = 0
#else
        .expected_result = -EINVAL
#endif
    },
    {   
        .config = { 
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {10000},
            .offsets = {0},
            .dims = 1,
            .flags = 0,
            .id = 22,
            .type = TEST_STREAM_TYPE_INT,
            .init_val = &ival,
        },
        .expected_result = 0
    },
    {   
        .config = { 
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {100},
            .offsets = {0},
            .dims = 2,
            .flags = 0,
            .id = 23,
            .type = TEST_STREAM_TYPE_INT,
        },
        .expected_result = -EINVAL
    },
    {   
        .config = { 
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {0},
            .offsets = {0},
            .dims = 0,
            .flags = 0,
            .id = 24,
            .type = TEST_STREAM_TYPE_INT,
        },
        .expected_result = -EINVAL
    },
    {   
        .config = { 
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {100},
            .offsets = {0},
            .dims = -1,
            .flags = 0,
            .id = 25,
            .type = TEST_STREAM_TYPE_INT,
        },
        .expected_result = -EINVAL
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {100},
            .offsets = {1000},
            .dims = 1,
            .flags = 0,
            .id = 26,
            .type = TEST_STREAM_TYPE_INT,
        },
        .expected_result = -EINVAL
    },
    {
       .config = {
            .name = NULL,
            .ptr = NULL,
            .dimsizes = {100},
            .offsets = {0},
            .dims = 1,
            .flags = 0,
            .id = 27,
            .type = TEST_STREAM_TYPE_INT,
       },
       .expected_result = -EINVAL
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {INT64_MAX + 1},
            .offsets = {0},
            .dims = 1,
            .flags = 0,
            .id = 28,
            .type = TEST_STREAM_TYPE_INT,
        },
        .expected_result = -EINVAL
    },
   {
       .config = {
           .name = &default_stream_name,
           .ptr = NULL,
           .dimsizes = {100},
           .offsets = {0},
           .dims = 1,
           .flags = 0,
           .id = 29,
           .type = 10000,
       },
       .expected_result = -EINVAL
   },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {100, 0},
            .offsets = {0, 0},
            .dims = 2,
            .flags = 0,
            .id = 30,
            .type = TEST_STREAM_TYPE_INT,
        },
        .expected_result = -EINVAL
    },
/*    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {100},
            .offsets = {0},
            .dims = 1,
            .flags = (1 << TEST_STREAM_ALLOC_TYPE_1DIM),
            .id = 31,
            .type = TEST_STREAM_TYPE_INT,
            .init_val = &ival,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {100},
            .offsets = {0},
            .dims = 1,
            .flags = 999,
            .id = 32,
            .type = TEST_STREAM_TYPE_INT,
        },
        .expected_result = -EINVAL
    },*/
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {100},
            .offsets = {0},
            .dims = 1,
            .flags = 0,
            .id = -33,
            .type = TEST_STREAM_TYPE_INT,
        },
        .expected_result = -EINVAL
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {100},
            .offsets = {0},
            .dims = 1,
            .flags = 0,
            .id = 0,
            .type = TEST_STREAM_TYPE_INT,
            .init_val = &ival,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {40},
            .offsets = {1},
            .dims = 1,
            .flags = 0,
            .id = 34,
            .type = TEST_STREAM_TYPE_INT,
            .init_val = &ival,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {40, 40},
            .offsets = {1, 2},
            .dims = 2,
            .flags = 0,
            .id = 35,
            .type = TEST_STREAM_TYPE_INT64,
            .init_val = &i64val,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {8, 16, 40},
            .offsets = {1, 1, 1},
            .dims = 3,
            .flags = 0,
            .id = 36,
            .type = TEST_STREAM_TYPE_DOUBLE,
            .init_val = &dval,
        },
        .expected_result = 0
    },
    {
        .config = {
            .name = &default_stream_name,
            .ptr = NULL,
            .dimsizes = {40},
            .offsets = {20},
            .dims = 1,
            .flags = 0,
            .id = 37,
            .type = TEST_STREAM_TYPE_SINGLE,
            .init_val = &fval,
        },
        .expected_result = 0
    }
};


int main()
{
    printf("==>Testing allocator implementation\n");
    size_t num_configs = sizeof(testconfigs) / sizeof(testconfigs[0]);
    int ok = 0;
    int err = 0;
    int output = 0;
    for (size_t i = 0; i < num_configs; i++)
    {
        output = test_stream_config(&testconfigs[i]);
        if (output == 0)
        {
            ok++;
        }
        else
        {
            printf("Check Test id: %d\n", testconfigs[i].config.id);
            err++;
        }

    }


    printf(SEPARATOR);
    printf("==>Testing allocator done\n");
    printf("Results: Total %ld, ok %d, err %d\n", num_configs, ok ,err);
    if (ok != num_configs || err != 0) printf("Error in Tests!\n"); // num_configs = Positive Tests + Negative Tests 


    return 0;
}
