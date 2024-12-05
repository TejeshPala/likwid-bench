#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "error.h"
#include "table.h"

int global_verbosity = DEBUGLEV_DEVELOP;

static struct tagbstring tname = bsStatic("LIKWID-BENCH");
static struct tagbstring tempty = bsStatic("");
static struct tagbstring sno = bsStatic("#No");
static struct tagbstring name = bsStatic("Name");
static struct tagbstring one = bsStatic("1");
static struct tagbstring two = bsStatic("2");
static struct tagbstring three = bsStatic("3");

static const char* fname = "test-sample.csv";

int main(int argc, char* argv)
{
    struct bstrList* headers = bstrListCreate();
    bstrListAdd(headers, &tname);
    bstrListAdd(headers, &sno);

    Table* table = NULL;
    table_create(headers, &table);

    struct bstrList* row1 = bstrListCreate();
    bstrListAdd(row1, &name);
    bstrListAdd(row1, &one);
    table_addrow(table, row1);

    struct bstrList* row2 = bstrListCreate();
    bstrListAdd(row2, &name);
    bstrListAdd(row2, &two);
    table_addrow(table, row2);

    table_print(table);

    struct bstrList* row3 = bstrListCreate();
    bstrListAdd(row3, &name);
    bstrListAdd(row3, &three);
    table_addrow(table, row3);

    table_print(table);

    struct bstrList* row4 = bstrListCreate();
    bstrListAdd(row4, &tempty);
    bstrListAdd(row4, &tempty);
    table_addrow(table, row4);

    table_to_csv(stdout, table, fname, 2);
    table_print_csv(stdout, fname);

/*
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "rm %s", fname);
    if (system(cmd) == 0)
    {
        printf("File '%s' removed\n", fname);
    }
    else
    {
        printf("Unable to remove file '%s'\n", fname);
    }
*/

    if (access(fname, F_OK) == 0)
    {
        if (unlink(fname) == 0)
        {
            printf("File '%s' removed\n", fname);
        }
        else
        {
            printf("Unable to remove file '%s'\n", fname);
        }
    }
    else
    {
        printf("File '%s' does not exist\n", fname);
    }
    table_destroy(table);

    bstrListDestroy(headers);
    bstrListDestroy(row1);
    bstrListDestroy(row2);
    bstrListDestroy(row3);
    bstrListDestroy(row4);

    return 0;
}
