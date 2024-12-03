// table.h
#ifndef TABLE_H
#define TABLE_H

#include "bstrlib.h"
#include "bstrlib_helper.h"

typedef struct {
    struct bstrList* headers;
    struct bstrList* rows;
    int* col_widths;
    int num_cols;
} Table;


int table_destroy(Table* table);
int table_create(struct bstrList* headers, Table** table);
int table_addrow(Table* table, struct bstrList* row);
int table_print(Table* table);
int table_to_csv(Table* table, const char* fname, int max_cols);
int table_print_csv(const char* fname);


#endif /* TABLE_H */
