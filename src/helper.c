#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "bstrlib.h"
#include "bstrlib_helper.h"

double convertToSeconds(const_bstring input)
{
    double result = -1.0;
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
        result = value * 0.001;
    }
    else if (biseq(unit, &bs))
    {
        result = value;
    }
    else if (biseq(unit, &bm))
    {
        result = value * 60;
    }
    else if (biseq(unit, &bh))
    {
        result = value * 3600;
    }
    else
    {
        fprintf(stdout, "No unit mentioned for runtime. It uses %.15lfs\n", value);
        result = value; // default to use 's' when no unit is mentioned for --runtime/-r
    }
    bdestroy(unit);
    return result;
}

size_t convertToBytes(const_bstring input)
{
    char* endptr = NULL;
    size_t result = 0;
    size_t value = (size_t)strtoumax((char *)input->data, &endptr, 10);
    if (endptr == (char *)input->data || value > SIZE_MAX)
    {
        fprintf(stderr, "Invalid input. No valid number found in %s\n", (char *)input->data);
        return 0;
    }

    bstring unit = bfromcstr(endptr);
    btoupper(unit);

    struct tagbstring bb = bsStatic("B");
    struct tagbstring bkb = bsStatic("KB");
    struct tagbstring bmb = bsStatic("MB");
    struct tagbstring bgb = bsStatic("GB");
    struct tagbstring btb = bsStatic("TB");
    struct tagbstring bkib = bsStatic("KIB");
    struct tagbstring bmib = bsStatic("MIB");
    struct tagbstring bgib = bsStatic("GIB");
    struct tagbstring btib = bsStatic("TIB");

    if (biseq(unit, &bb))
    {
	    result = (size_t)value;
    }
    else if (biseq(unit, &bkb))
    {
	    result = (size_t)value * (size_t)1000;
    }
    else if (biseq(unit, &bmb))
    {
        result = (size_t)value * (size_t)1000 * (size_t)1000;
    }
    else if (biseq(unit, &bgb))
    {
        result = (size_t)value * (size_t)1000 * (size_t)1000 * (size_t)1000;
    }
    else if (biseq(unit, &btb))
    {
        result = (size_t)value * (size_t)1000 * (size_t)1000 * (size_t)1000 * (size_t)1000;
    }
    else if (biseq(unit, &bkib))
    {
	    result = (size_t)value * (size_t)1024;
    }
    else if (biseq(unit, &bmib))
    {
        result = (size_t)value * (size_t)1024 * (size_t)1024;
    }
    else if (biseq(unit, &bgib))
    {
        result = (size_t)value * (size_t)1024 * (size_t)1024 * (size_t)1024;
    }
    else if (biseq(unit, &btib))
    {
        result = (size_t)value * (size_t)1024 * (size_t)1024 * (size_t)1024 * (size_t)1024;
    }
    else
    {
	    result = (size_t)value;
        fprintf(stdout, "No unit mentioned for array size. It uses %zu B\n", result);
    }

    bdestroy(unit);

    if (result < value && result > SIZE_MAX)
    {
        fprintf(stderr, "Size converted is too large for size_t\n");
        return 0ULL;
    }

    return result;
}
