// table.h
#ifndef TABLE_H
#define TABLE_H

#include <stdio.h>
#include <stdlib.h>

#include "bstrlib.h"
#include "bstrlib_helper.h"

typedef struct {
    struct bstrList* headers;
    struct bstrList* rows;
    int* col_widths;
    int num_cols;
} Table;

#define INDENT_SIZE 4

static void write_indent(FILE* file, int level)
{
    for (int i = 0; i < level * INDENT_SIZE; i++)
    {
        fputc(' ', file);
    }
}

int table_destroy(Table* table);
int table_create(struct bstrList* headers, Table** table);
int table_addrow(Table* table, struct bstrList* row);
int table_print(FILE* output, Table* table);
int table_to_csv(FILE* output, Table* table, const char* fname, int max_cols);
int table_to_json(FILE* output, Table* table, const char* fname, const char* tname);
int table_print_csv(const char* fname);


#endif /* TABLE_H */
