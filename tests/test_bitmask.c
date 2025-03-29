#include "bitmask.h"


#define SEPARATOR "---------------------------------------\n"
#define NUM_TESTS 11

typedef struct {
    size_t  in;
    size_t  nbits;
    size_t  expected_result;
    int     expected_error;
} TestCase;

void run_tests(const char *test_name, TestCase *tests, int num_tests, int (*func)(size_t*, size_t, size_t))
{
    printf(SEPARATOR);
    printf("Running %s tests\n", test_name);
    int pass_count = 0;
    int fail_count = 0;
    int should_fail = 0;

    for (size_t i = 0; i < num_tests; i++)
    {
        size_t out;
        int err = func(&out, tests[i].in, tests[i].nbits);
        if (err == tests[i].expected_error)
        {
            if (out == tests[i].expected_result)
            {
                printf("Test %2zu: PASS (in=%-10zu, nbits=%-5zu, expected=%-10zu, actual=%-10zu)\n",
                    i + 1, tests[i].in, tests[i].nbits, tests[i].expected_result, out);
                pass_count++;
            }
            else
            {
                printf("Test %2zu: SHOULD FAIL (in=%-10zu, nbits=%-5zu, expected=%-10zu, actual=%-10zu)\n",
                    i + 1, tests[i].in, tests[i].nbits, tests[i].expected_result, out);
                should_fail++;
            }
        }
        else
        {
            printf("Test %2zu: FAIL (in=%-10zu, nbits=%-5zu, expected=%-10zu, actual=%-10zu, err=%d)\n",
                i + 1, tests[i].in, tests[i].nbits, tests[i].expected_result, out, err);
            fail_count++;
        }
    }
    
    printf("Total: \t%zu Passed: \t%d Failed: \t%d Should Fail: \t%d\n", num_tests, pass_count, fail_count, should_fail);
}


int main()
{
    TestCase roundup_tests[] =
    {
        {130, 64, 192, 0},
        {64, 64, 64, 0},
        {63, 64, 64, 0},
        {128, 64, 128, 0},
        {200, 128, 256, 0},
        {0, 64, 0, 0},
        {SIZE_MAX - 63, 64, SIZE_MAX, 0},

        {1, 2, 2, 0},
        {5, 4, 8, 0},
        {100, 0, 0, -EINVAL},
        {SIZE_MAX - 30, 64, 0, -EOVERFLOW},
    };

    TestCase rounddown_tests[] =
    {
        {130, 64, 128, 0},
        {64, 64, 64, 0},
        {63, 64, 0, 0},
        {128, 64, 128, 0},
        {200, 128, 128, 0},
        {0, 64, 0, 0},
        {SIZE_MAX, 64, SIZE_MAX & ~63, 0},

        {7, 3, 6, 0},
        {0, 0, 0, -EINVAL},
        {1, 2, 0, 0},
    };

    TestCase roundnearest_tests[] =
    {
        {130, 64, 128, 0},
        {160, 64, 192, 0},
        {96, 64, 128, 0},
        {32, 64, 64, 0},
        {200, 128, 256, 0},
        {0, 64, 0, 0},
        {SIZE_MAX - (SIZE_MAX % 64), 64, SIZE_MAX - (SIZE_MAX % 64), 0},

        {15, 8, 16, 0},
        {100, 0, 0, -EINVAL},
        {63, 64, 64, 0},
    };

    run_tests("roundup_nbits", roundup_tests, NUM_TESTS, roundup_nbits);
    run_tests("rounddown_nbits", rounddown_tests, NUM_TESTS - 1, rounddown_nbits);
    run_tests("roundnearest_nbits", roundnearest_tests, NUM_TESTS - 1, roundnearest_nbits);

    TestCase roundup_pow2_tests[] =
    {
        {130, 64, 192, 0},
        {64, 64, 64, 0},
        {63, 64, 64, 0},
        {128, 64, 128, 0},
        {200, 128, 256, 0},
        {0, 64, 0, 0},
        {SIZE_MAX - 63, 64, SIZE_MAX, 0},

        {1, 1, 1, 0},
        {5, 4, 8, 0},
        {100, 0, 0, -EINVAL},
        {SIZE_MAX - 30, 64, 0, -EOVERFLOW},
    };

    TestCase rounddown_pow2_tests[] =
    {
        {130, 64, 128, 0},
        {64, 64, 64, 0},
        {63, 64, 0, 0},
        {128, 64, 128, 0},
        {200, 128, 128, 0},
        {0, 64, 0, 0},
        {SIZE_MAX & ~63, 64, SIZE_MAX & ~63, 0},

        {7, 3, 6, -EINVAL},
        {0, 0, 0, -EINVAL},
        {1, 2, 0, 0},
    };

    TestCase roundnearest_pow2_tests[] =
    {
        {130, 64, 128, 0},
        {160, 64, 192, 0},
        {96, 64, 128, 0},
        {32, 64, 64, 0},
        {200, 128, 256, 0},
        {0, 64, 0, 0},
        {SIZE_MAX - (SIZE_MAX % 64), 64, SIZE_MAX - (SIZE_MAX % 64), 0},

        {15, 8, 16, 0},
        {100, 0, 0, -EINVAL},
        {63, 64, 64, 0},
    };

    run_tests("roundup_nbits_pow2", roundup_pow2_tests, NUM_TESTS, roundup_nbits_pow2);
    run_tests("rounddown_nbits_pow2", rounddown_pow2_tests, NUM_TESTS - 1, rounddown_nbits_pow2);
    run_tests("roundnearest_nbits_pow2", roundnearest_pow2_tests, NUM_TESTS - 1, roundnearest_nbits_pow2);

    return 0;
}
