#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bstrlib.h"
#include "bstrlib_helper.h"
#include "error.h"
#include "table.h"
#include "test_types.h"


int table_destroy(Table* table)
{
    if (!table)
    {
        ERROR_PRINT(Invalid Table);
        return -EINVAL;
    }

    bstrListDestroy(table->headers);
    bstrListDestroy(table->rows);
    free(table->col_widths);
    free(table);
    return 0;
}

int table_create(struct bstrList* headers, Table** table)
{
    Table* itable = (Table*)malloc(sizeof(Table));
    if (!itable)
    {
        ERROR_PRINT(Failed to allocate memory for Table);
        return -ENOMEM;
    }
    itable->headers = bstrListCopy(headers);
    itable->rows = bstrListCreate();
    itable->num_cols = headers->qty;
    itable->col_widths = (int*)malloc(itable->num_cols * sizeof(int));
    if (!itable->col_widths)
    {
        ERROR_PRINT(Failed to allocate memory for Table coloumn widths);
        return -ENOMEM;
    }
    memset(itable->col_widths, 0, itable->num_cols * sizeof(int));
    for (int c = 0; c < itable->num_cols; c++)
    {
        itable->col_widths[c] = blength(headers->entry[c]);
    }
    *table = itable;
    return 0;
}

int table_addrow(Table* table, struct bstrList* row)
{
    if (!table)
    {
        ERROR_PRINT(Inavlid Table);
        return -EINVAL;
    }

    if (row->qty != table->num_cols)
    {
        ERROR_PRINT(Row quantity %d does not match with coloumn count %d, row->qty, table->num_cols);
        return -EINVAL;
    }

    bstring brow = bfromcstr("");
    for (int c = 0; c < table->num_cols; c++)
    {
        if (c > 0) bconchar(brow, '|');
        bstring btmp = bstrcpy(row->entry[c]);

        if (bisinteger(btmp))
        {
            int ivalue;
            if (batoi(btmp, &ivalue) == BSTR_OK && ivalue >= 0)
            {
                bstring btmpd = bformat("%d", ivalue);
                bconcat(brow, btmpd);
                int cell = blength(btmpd);
                if (cell > table->col_widths[c])
                {
                    table->col_widths[c] = cell;
                }
                bdestroy(btmpd);
            }
        }
        else if (bisnumber(btmp))
        {
            int ivalue;
            double dvalue;
            if (batod(btmp, &dvalue) == BSTR_OK && dvalue >= 0.0)
            {
                bstring btmpd = (dvalue < 1.0) ? bformat("%.6lf", dvalue): bformat("%.7g", dvalue);
                bconcat(brow, btmpd);
                int cell = blength(btmpd);
                if (cell > table->col_widths[c])
                {
                    table->col_widths[c] = cell;
                }
                bdestroy(btmpd);
            }
        }

        bdestroy(btmp);
    }
    bstrListAdd(table->rows, brow);
    bdestroy(brow);
    return 0;
}

#define PRINT_BORDER(table, file) \
    for (int c = 0; c < table->num_cols; c++) \
    { \
        fprintf(file, "+"); \
        for (int w = 0; w < table->col_widths[c] + 2; w++) fprintf(file, "-"); \
    } \
    fprintf(file, "+\n");

#define PRINT_ROW(table, data, file) \
    for (int c = 0; c < table->num_cols; c++) \
    { \
        fprintf(file, "| %*s ", table->col_widths[c], bdata(data->entry[c])); \
    } \
    fprintf(file, "|\n");

int table_print(FILE* output, Table* table, int transpose)
{
    if (!table)
    {
        ERROR_PRINT(Invalid Table);
        return -EINVAL;
    }

    FILE* file = output;

    if (!file)
    {
        ERROR_PRINT(Failed to use given output);
        return -errno;
    }

    if (transpose)
    {
        int max_data_width = 0;
        for (int c = 0; c < table->num_cols; c++)
        {
            if (table->col_widths[c] > max_data_width) max_data_width = table->col_widths[c];
        }
        max_data_width = max_data_width;

        fprintf(file, "+"); // TOP BORDER
        for (int w = 0; w < max_data_width + 2; w++) fprintf(file, "-");

        for (int r = 0; r < table->rows->qty; r++)
        {
            fprintf(file, "+");
            for (int w = 0; w < max_data_width + 2; w++) fprintf(file, "-");
        }
        fprintf(file, "+\n");

        for (int c = 0; c < table->num_cols; c++)
        {
            fprintf(file, "| %-*s ", max_data_width, bdata(table->headers->entry[c]));
            for (int r = 0; r < table->rows->qty; r++)
            {
                struct bstrList* cells = bsplit(table->rows->entry[r], '|');
                if (cells && c < cells->qty)
                {
                    fprintf(file, "| %*s ", max_data_width, bdata(cells->entry[c]));
                }
                else
                {
                    fprintf(file, "| %-*s ", max_data_width, "");
                }
                if (cells) bstrListDestroy(cells);
            }
            fprintf(file, "|\n");
            // Each row seperator for results
            /*
            if (c < table->num_cols - 1)
            {
                fprintf(file, "+");
                for (int w = 0; w < max_data_width + 2; w++) fprintf(file, "-");
                for (int r = 0; r < table->rows->qty; r++)
                {
                    fprintf(file, "+");
                    for (int w = 0; w < max_data_width + 2; w++) fprintf(file, "-");
                }
                fprintf(file, "+\n");
            }
            */
        }

        fprintf(file, "+");
        for (int w = 0; w < max_data_width + 2; w++) fprintf(file, "-");
        for (int r = 0; r < table->rows->qty; r++)
        {
            fprintf(file, "+");
            for (int w = 0; w < max_data_width + 2; w++) fprintf(file, "-");
        }
        fprintf(file, "+\n");
    }
    else
    {
        // Top border
        PRINT_BORDER(table, file);

        // Headers
        PRINT_ROW(table, table->headers, file);

        // Header body seperator
        PRINT_BORDER(table, file);

        // Rows
        for (int r = 0; r < table->rows->qty; r++)
        {
            struct bstrList* cells = bsplit(table->rows->entry[r], '|');
            PRINT_ROW(table, cells, file);
            bstrListDestroy(cells);
        }

        // Bottom border
        PRINT_BORDER(table, file);
    }
    return 0;
}

#define SEPARATOR(cols) \
    for (int c = 0; c < cols; c++) \
    { \
        fprintf(file, ","); \
    }

int table_to_csv(FILE* output, Table* table, const char* fname, int max_cols, int transpose)
{
    int err = 0;
    if (!table)
    {
        ERROR_PRINT(Invalid Table);
        return -EINVAL;
    }

    FILE* file = output;
    if (file == NULL)
    {
        err = errno;
        ERROR_PRINT(Unable to write to file %s, fname);
        return err;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    if (size > 0)
    {
        fseek(file, -1, SEEK_END);
        int last_char = fgetc(file);
        if (last_char == EOF)
        {
            SEPARATOR(max_cols);
            fprintf(file, "\n");
        }
    }
    
    if (transpose)
    {
        for (int c = 0; c < table->num_cols; c++)
        {
            fprintf(file, "%s", bdata(table->headers->entry[c]));
            for (int r = 0; r < table->rows->qty; r++)
            {
                struct bstrList* cells = bsplit(table->rows->entry[r], '|');
                if (cells && c < cells->qty)
                {
                    fprintf(file, ",%s", bdata(cells->entry[c]));
                }
                if (table->rows->qty < max_cols - 1)
                {
                    SEPARATOR(max_cols - table->rows->qty);
                }
                if (cells) bstrListDestroy(cells);
            }
            fprintf(file, "\n");
        }
    }
    else
    {
        for (int c = 0; c < table->num_cols; c++)
        {
            fprintf(file, "%s", bdata(table->headers->entry[c]));
            if (c < table->num_cols - 1)
            {
                fprintf(file, ",");
            }
        }
        if (max_cols > table->num_cols)
        {
            SEPARATOR(max_cols - table->num_cols);
        }
        fprintf(file, "\n");

        for (int r = 0; r < table->rows->qty; r++)
        {
            struct bstrList* cells = bsplit(table->rows->entry[r], '|');
            for (int c = 0; c < table->num_cols; c++)
            {
                fprintf(file, "%s", bdata(cells->entry[c]));
                if (c < table->num_cols - 1)
                {
                    fprintf(file, ",");
                }
            }
            if (max_cols > table->num_cols)
            {
                SEPARATOR(max_cols - table->num_cols);
            }
            fprintf(file, "\n");
            bstrListDestroy(cells);
        }
    }
    DEBUG_PRINT(DEBUGLEV_DEVELOP, Results are saved to file '%s', fname);
    return err;
}

int table_to_json(FILE* output, Table* table, const char* fname, const char* tname)
{
    int err = 0;
    int place_holder = (strcmp(tname, "global_results") == 0) ? 1 : 0; // check for last table name
    int indent = 0;
    if (!table)
    {
        ERROR_PRINT(Inavlid Table);
        return -EINVAL;
    }

    FILE* file = output;
    if (file == NULL)
    {
        err = errno;
        ERROR_PRINT(Unable to write to file %s, fname);
        return err;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    if (size == 0)
    {
        fprintf(file, "{\n");
        indent = 1;
    }

    indent = 1;
    write_indent(file, indent);
    fprintf(file, "\"%s\": [\n", tname);
    indent++;

    for (int r = 0; r < table->rows->qty; r++)
    {
        write_indent(file, indent);
        fprintf(file, "{\n");
        indent++;
        struct bstrList* cells = bsplit(table->rows->entry[r], '|');
        for (int c = 0; c < table->num_cols; c++)
        {
            write_indent(file, indent);
            fprintf(file, "\"%s\": %s", bdata(table->headers->entry[c]), bdata(cells->entry[c]));
            if (c < table->num_cols - 1)
            {
                fprintf(file, ",");
            }
            fprintf(file, "\n");
        }
        indent--;
        write_indent(file, indent);
        fprintf(file, "}%s\n", r < table->rows->qty - 1 ? "," : "");
        bstrListDestroy(cells);
    }

    indent--;
    write_indent(file, indent);

    fprintf(file, "]%s\n", (place_holder == 0) ? "," : "");

    if (place_holder == 1)
    {
        indent--;
        write_indent(file, indent);
        fprintf(file, "}\n");
    }

    DEBUG_PRINT(DEBUGLEV_DEVELOP, Results are saved to file '%s', fname);
    return err;
}

int table_print_csv(const char* fname)
{
    int err = 0;
    FILE* file = fopen(fname, "r");
    if (!file)
    {
        err = errno;
        ERROR_PRINT(Failed to open file %s, fname);
        return err;
    }

    Table* table = NULL;
    char line[1024];
    int row_count = 0;

    while (fgets(line, sizeof(line), file))
    {
        bstring bline = bfromcstr(line);
        btrimws(bline);
        struct bstrList* fields = bsplit(bline, ',');
        if (row_count == 0)
        {
            err = table_create(fields, &table);
            if (!table)
            {
                ERROR_PRINT(Failed to create table from '%s' headers, fname);
                bstrListDestroy(fields);
                fclose(file);
                err = errno;
                return err;
            }
        }
        else
        {
            table_addrow(table, fields);
        }
        bstrListDestroy(fields);
        row_count++;
        bdestroy(bline);
    }

    fclose(file);

    if (table)
    {
        DEBUG_PRINT(DEBUGLEV_DEVELOP, Printing CSV '%s' file, fname);
        table_print(stdout, table, 1);
        table_destroy(table);
    }
    else
    {
        ERROR_PRINT(No data read from CSV '%s' file, fname);
        return -EINVAL;
    }
    return err;
}
