#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <results.h>
#include <error.h>

int cpus[8] = {1,2,3,4,5,6,7,0};

int global_verbosity = DEBUGLEV_DEVELOP;

#define CHECK_ERROR(func) if (func != 0) { printf("Error in line %d\n", __LINE__); destroy_results(); return -1; }

int main(int argc, char* argv)
{
    int test_cpus = 2;
    struct tagbstring bconst = bsStatic("CONST");
    struct tagbstring bkey = bsStatic("Test");
    struct tagbstring bformula = bsStatic("(Nas*Nas)+CONST");
    struct tagbstring btest1 = bsStatic("Test1");
    struct tagbstring bnas = bsStatic("Nas");
    int err = init_results(test_cpus, cpus);
    
    CHECK_ERROR(set_result(0, &btest1, 1234));
/*    CHECK_ERROR(set_result(0, &bnas, 4321));*/
/*    CHECK_ERROR(set_result(1, &bnas, 4321));*/
    CHECK_ERROR(set_result_for_all(&bnas, 4321));
    CHECK_ERROR(set_result(0, &btest1, 3.14));
    CHECK_ERROR(add_const(&bconst, 3.14));
    CHECK_ERROR(add_thread_const(0, &bconst, 1.0));
    CHECK_ERROR(add_formula(&bkey, &bformula));

    for (int j = 0; j < test_cpus; j++)
    {
        double value = 0;
        int i = 1;
        while (i < 10000000)
        {
            CHECK_ERROR(set_result(j, &bnas, i));
            err = get_formula(j, &bkey, &value);
            if (err != 0)
            {
                value = -1;
                i = 0;
                break;
            }
            if (value > 100)
            {
                break;
            }
            i++;
        }
        printf("Final N=%d for thread %d\n", i-1, j);
    }
    destroy_results();
    
    return 0;
}
