#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <results.h>

int cpus[8] = {1,2,3,4,5,6,7,0};

int main(int argc, char* argv)
{

    int err = init_results(1, cpus);
    
    err = set_result(0, "Test1", 1234);
    
    err = set_result(0, "Nas", 4321);
    
    err = set_result(0, "Test1", 3.14);
    err = add_const("CONST",3.14);
    err = add_formula("Test", "(Nas*Nas)+Nas");
    
    double value;
    int i = 1;
    while (i < 10000000)
    {
        err = set_result(0, "Nas", i);
        err = get_formula(0, "Test", &value);
        if (value > 100000)
        {
            break;
        }
        i++;
    }
    printf("Final N=%d\n", i-1);
    destroy_results();
    
    return 0;
}
