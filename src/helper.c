#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "bstrlib.h"
#include "bstrlib_helper.h"

double convertToSeconds(const_bstring input)
{
    double value = atof((char *)input->data);
    int len = blength(input);
    bstring unit;
    if (len >= 2 && input->data[len-2] == 'm' && input->data[len-1] == 's')
    {
        unit = bmidstr(input, blength(input) - 2, 2); // for 'ms'
    }
    else
    {
        unit = bmidstr(input, blength(input) - 1, 1); // for 's', 'm' and 'h'
    }
    struct tagbstring bms = bsStatic("ms");
    struct tagbstring bs = bsStatic("s");
    struct tagbstring bm = bsStatic("m");
    struct tagbstring bh = bsStatic("h");

    if (biseq(unit, &bms))
    {
        bdestroy(unit);
        return value * 0.001;
    }
    else if (biseq(unit, &bs))
    {
        bdestroy(unit);
        return value;
    }
    else if (biseq(unit, &bm))
    {
        bdestroy(unit);
        return value * 60;
    }
    else if (biseq(unit, &bh))
    {
        bdestroy(unit);
        return value * 3600;
    }
    else
    {
        bdestroy(unit);
        return value; // default to use 's' when no unit is mentioned for --runtime/-r 
    }
}

long long convertToBytes(const_bstring input)
{
    long long value = atoi((char *)input->data);
    bstring unit = bmidstr(input, blength(input) - 2, 2);

    btoupper(unit);

    struct tagbstring bkb = bsStatic("KB");
    struct tagbstring bmb = bsStatic("MB");
    struct tagbstring bgb = bsStatic("GB");
    struct tagbstring btb = bsStatic("TB");

    if (biseq(unit, &bkb))
    {
        bdestroy(unit);
	    return value * 1024LL;
    }
    else if (biseq(unit, &bmb))
    {
        bdestroy(unit);
        return value * 1024LL * 1024LL;
    }
    else if (biseq(unit, &bgb))
    {
        bdestroy(unit);
        return value * 1024LL * 1024LL * 1024LL;
    }
    else if (biseq(unit, &btb))
    {
        bdestroy(unit);
        return value * 1024LL * 1024LL * 1024LL * 1024LL;
    }
    else
    {
        bdestroy(unit);
        printf("Invalid unit. Valid array sizes are kB, MB, GB, TB. Retry again with valid input!\n");
        exit(EXIT_FAILURE);
    }
}

