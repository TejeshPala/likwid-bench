#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#include <bstrlib.h>
#include <bstrlib_helper.h>

#define NUM_CONVERT_TESTS 11 // Total Tests
#define SEPARATOR "---------------------------------------\n"

void print_bstr(bstring bstr)
{
    printf("Testing string: %s\n", bdata(bstr));
}

// begin conversion tests

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

int run_conversion_tests(TestBstrHelper *tests, int num_tests)
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
    if (err != 0)
    {
        return -err;
    }
    if (num_tests != ok)
    {
        return -EFAULT;
    }
    return 0;
}

static struct tagbstring bstrlists[NUM_CONVERT_TESTS] = {
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

TestBstrHelper tests[NUM_CONVERT_TESTS] = {
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
// end conversion tests

// struct bstrList* tests

typedef struct {
    struct tagbstring name;
    int (*func)();
    int error;
} BstrListTest;

// Sort function
int bstrlist_sort_lexi_min_size(const struct tagbstring * left, const struct tagbstring * right)
{
    if (blength(left) < blength(right) || (bstrcmp(left, right) < 0))
    {
        return -1;
    }
    else if (blength(left) == blength(right) || (bstrcmp(left, right) == 0))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

int bstrList_create_add_close()
{
    int err = 0;
    struct tagbstring content = bsStatic("NAME");
    struct bstrList* l = bstrListCreate();
    if (!l)
    {
        return -ENOMEM;
    }
    err = bstrListAdd(l, &content);
    if (err != BSTR_OK)
    {
        return -EFAULT;
    }
    bstrListDestroy(l);
    return 0;
}

int bstrList_sort()
{
    int err = 0;
    int idx = 0;
    int input_len = 0;
    int ref_len = 0;
    struct bstrList* l = NULL;
    struct bstrList* r = NULL;
    struct bstrList* s = NULL;

    static struct tagbstring inputs[] = {
        bsStatic("N"),
        bsStatic("M"),
        bsStatic("NAME"),
        bsStatic("SIZE_STREAM_0_DIM_0"),
        bsStatic("NUM_THREADS"),
        bsStatic("NUM_STREAMS"),
        bsStatic("DIMS_STREAM_0"),
        bsStatic(""),
    };
    static struct tagbstring reference[] = {
        bsStatic("M"),
        bsStatic("N"),
        bsStatic("NAME"),
        bsStatic("NUM_STREAMS"),
        bsStatic("NUM_THREADS"),
        bsStatic("DIMS_STREAM_0"),
        bsStatic("SIZE_STREAM_0_DIM_0"),
        bsStatic(""),
    };
    l = bstrListCreate();
    while (blength(&inputs[idx]) > 0)
    {
        bstrListAdd(l, &inputs[idx]);
        idx++;
    }
    input_len = idx;
    err = bstrListSortFunc(l, bstrlist_sort_lexi_min_size, &s);
    if (err != BSTR_OK)
    {
        bstrListDestroy(l);
        return -EFAULT;
    }
    idx = 0;
    r = bstrListCreate();
    while (blength(&reference[idx]) > 0)
    {
        bstrListAdd(r, &reference[idx]);
        idx++;
    }
    ref_len = idx;
    if (input_len == ref_len)
    {
        for (int i = 0; i < r->qty; i++)
        {
            if (bstrncmp(r->entry[i], s->entry[i], blength(r->entry[i])) != BSTR_OK)
            {
                printf("Sorting error at idx %d: %s vs %s\n", idx, bdata(r->entry[i]), bdata(l->entry[i]));
                printf("List:\n");
                bstrListPrint(s);
                printf("Reference:\n");
                bstrListPrint(r);
                err = -EFAULT;
                break;
            }
        }
    }
    else
    {
        printf("Test internal error, input and reference lists do not have equal entries (%d vs %d). Needs fixing\n", input_len, ref_len);
        err = -EFAULT;
    }
    bstrListDestroy(s);
    bstrListDestroy(r);
    bstrListDestroy(l);
    return err;
}

static BstrListTest bstrList_tests[] = {
    {bsStatic("create_add_close"), bstrList_create_add_close, 0},
    {bsStatic("sort"), bstrList_sort, 0},
    {bsStatic(""), NULL, 0},
};

int run_bstrlist_tests()
{
    int idx = 0;
    int all = 0;
    int success = 0;
    int shouldfail = 0;
    printf("Testing bstrList helper functions\n");
    printf(SEPARATOR);
    while (blength(&bstrList_tests[idx].name) > 0 && bstrList_tests[idx].func != NULL)
    {
        
        int err = bstrList_tests[idx].func();
        if (err == bstrList_tests[idx].error)
        {
            if (err == 0)
            {
                success++;
                printf("%s: success\n", bdata(&bstrList_tests[idx].name));
            }
            else
            {
                shouldfail++;
                printf("%s: shouldfail success (err %d)\n", bdata(&bstrList_tests[idx].name), err);
            }
        }
        else
        {
            printf("%s: error %d\n", bdata(&bstrList_tests[idx].name), err);
        }
        all++;
        idx++;
        printf(SEPARATOR);
    }
    printf("Testing bstrList helper functions done\n");
    printf("Results: Total %d, ok %d, shouldfail %d, err %d\n", all, success, success, all - success - shouldfail);
    return -(all - success - shouldfail);
}


int main()
{
    int err = 0;
    int suite_err = 0;
    err = run_conversion_tests(tests, NUM_CONVERT_TESTS);
    if (err != 0)
    {
        suite_err++;
    }

    err = run_bstrlist_tests();
    if (err != 0)
    {
        suite_err++;
    }
    return -suite_err;
}
