#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <string.h>

#include "bstrlib.h"
#include "bstrlib_helper.h"

#define NUM_TESTS 11 // Total Tests
#define SEPARATOR "---------------------------------------\n"

typedef enum {
    INT_TEST,
    FLOAT_TEST,
    DOUBLE_TEST
} TestType;

typedef struct {
    TestType test_type;
    union {
        int iresult;
        float fresult;
        double dresult;
    } expected_result;
    bstring bstr;
} TestBstrHelper;

void print_bstr(bstring bstr)
{
    printf("Testing string: %s\n", bdata(bstr));
}

void run_tests(TestBstrHelper *tests, int num_tests)
{
    printf("Testing bstr helper functions\n");
    int ok = 0;
    int err = 0;
    for (int i = 0; i < num_tests; i++)
    {
        int status;
        printf(SEPARATOR);
        print_bstr(tests[i].bstr);
        switch (tests[i].test_type)
        {
            case INT_TEST:
            {
                int result;
                status = batoi(tests[i].bstr, &result);
                if (status == BSTR_OK && result == tests[i].expected_result.iresult)
                {
                    printf("Test %s passed\n", bdata(tests[i].bstr));
                    ok++;
                }
                else
                {
                    printf("Test failed for %s, Expected: %d, Actual: %d\n", bdata(tests[i].bstr), tests[i].expected_result.iresult, result);
                    err++;
                }
                
                break;
            }
            case FLOAT_TEST:
            {
                bstring btmp1 = bfromcstr("");
                bstring btmp2 = bfromcstr("");
                float result;
                status = batof(tests[i].bstr, &result);
                bformata(btmp1,"%.6f", result);
                bformata(btmp2,"%.6f", tests[i].expected_result.fresult);
                if (status == BSTR_OK && bstrcmp(btmp1, btmp2) == 0)
                {
                    printf("Test %s passed\n", bdata(tests[i].bstr));
                    ok++;
                }
                else
                {
                    printf("Test failed for %s, Expected: %s, Actual: %s\n", bdata(tests[i].bstr), bdata(btmp1), bdata(btmp2));
                    err++;
                }
 
                bdestroy(btmp1);
                bdestroy(btmp2);
                break;
            }
            case DOUBLE_TEST:
            {
                bstring btmp1 = bfromcstr("");
                bstring btmp2 = bfromcstr("");
                double result;
                status = batod(tests[i].bstr, &result);
                bformata(btmp1, "%.15g", result);
                bformata(btmp2, "%.15g", tests[i].expected_result.dresult);
                if (status == BSTR_OK && bstrcmp(btmp1, btmp2) == 0)
                {
                    printf("Test %s passed\n", bdata(tests[i].bstr));
                    ok++;
                }
                else
                {
                    printf("Test failed for %s, Expected: %s, Actual: %s\n", bdata(tests[i].bstr), bdata(btmp1), bdata(btmp2));
                    err++;
                }

                bdestroy(btmp1);
                bdestroy(btmp2);
                break;
            }
            default:
                printf("Invalid Tests\n");
                break;

        }
    }

    printf(SEPARATOR);
    printf("Testing bstr helper functions done\n");
    printf("Results: Total %d, ok %d, err %d\n", num_tests, ok, err);
    if (num_tests != ok || err != 0) printf("Error in tests\n");

}

static struct tagbstring bstrlists[NUM_TESTS] = {
    bsStatic("123"),
    bsStatic("-456"),
    bsStatic("2147483647"),
    bsStatic("-2147483648"),
    bsStatic("4294967295"),
    bsStatic("3.402823"),
    bsStatic("3e+6"),
    bsStatic("-1e-5"),
    bsStatic("12345.678901234567891"),
    bsStatic("1.797693134862315e+15"),
    bsStatic("2.22507e-14"),
};

TestBstrHelper tests[NUM_TESTS] = {
    {.test_type = INT_TEST, .expected_result.iresult = 123, .bstr = &bstrlists[0]},
    {.test_type = INT_TEST, .expected_result.iresult = -456, .bstr = &bstrlists[1]},
    {.test_type = INT_TEST, .expected_result.iresult = 2147483647, .bstr = &bstrlists[2]},
    {.test_type = INT_TEST, .expected_result.iresult = -2147483648, .bstr = &bstrlists[3]},
    {.test_type = INT_TEST, .expected_result.iresult = 4294967295, .bstr = &bstrlists[4]},
    {.test_type = FLOAT_TEST, .expected_result.fresult = 3.402823f, .bstr = &bstrlists[5]},
    {.test_type = FLOAT_TEST, .expected_result.fresult = 3e+6f, .bstr = &bstrlists[6]},
    {.test_type = FLOAT_TEST, .expected_result.fresult = -1e-5f, .bstr = &bstrlists[7]},
    {.test_type = DOUBLE_TEST, .expected_result.dresult = 12345.678901234567891, .bstr = &bstrlists[8]},
    {.test_type = DOUBLE_TEST, .expected_result.dresult = 1.797693134862315e+15, .bstr = &bstrlists[9]},
    {.test_type = DOUBLE_TEST, .expected_result.dresult = 2.22507e-14, .bstr = &bstrlists[10]}
};

int main()
{
    run_tests(tests, NUM_TESTS);

    return 0;
}
