

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <inttypes.h>
#include <stdint.h>

//#include <unistd.h>
//#include <string.h>

#include "bstrlib.h"
#include <errno.h>


#define DELIMITERS ",*+[] "

int bstrListAdd(struct bstrList * sl, bstring str)
{
	if (sl->qty >= sl->mlen) {
        int mlen = sl->mlen * 2;
        bstring * tbl;
    

	    while (sl->qty >= mlen) {
	        if (mlen < sl->mlen) return BSTR_ERR;
	        mlen += mlen;
	    }

	    tbl = (bstring *) realloc (sl->entry, sizeof (bstring) * mlen);
	    if (tbl == NULL) return BSTR_ERR;

	    sl->entry = tbl;
	    sl->mlen = mlen;
	}
	sl->entry[sl->qty] = bstrcpy(str);
    sl->qty++;
    return BSTR_OK;
}

int bstrListAddChar(struct bstrList * sl, char* str)
{
	if (!sl || !str) return BSTR_ERR;
	bstring tmp = bformat("%s", str);
	int err = bstrListAdd(sl, tmp);
	bdestroy(tmp);
	return err;
}

void bstrListPrint(struct bstrList * sl)
{
	int i = 0;
	if (!sl) return;
	if (sl->qty > 0)
	{
		printf("[%s", bdata(sl->entry[0]));
		for (i = 1; i < sl->qty; i++)
		{
			printf(", %s", bdata(sl->entry[i]));
		}
		printf("]\n");
	}
	else if (sl->qty == 0)
	{
		printf("[]\n");
	}
}

int bstrListDel(struct bstrList * sl, int idx)
{
	int i;

	if (!sl || idx < 0 || idx >= sl->qty) return BSTR_ERR;

	bdestroy(sl->entry[idx]);

	for (i = idx+1; i < sl->qty; i++)
	{
		sl->entry[i-1] = sl->entry[i];
	}
	sl->qty--;

	return BSTR_OK;
}

int bstrListRemove(struct bstrList * sl, bstring str)
{
    int i = 0;
    if (!sl) return BSTR_ERR;
    while (i < sl->qty)
    {
        if (bstrcmp(str, sl->entry[i]) == BSTR_OK)
        {
            bstrListDel(sl, i);
            continue;
        }
        i++;
    }
    return BSTR_OK;
}

bstring bstrListGet(struct bstrList * sl, int idx)
{
    if (!sl || idx < 0 || idx >= sl->qty) return NULL;
    return sl->entry[idx];
}

int bstrListFindandCopy(struct bstrList* sl, bstring in, bstring* out)
{
    if (!sl || !in || !out) return BSTR_ERR;
    for (int i = 0; i < sl->qty; i++)
    {
        if (biseq(sl->entry[i], in))
        {
            bstring copy = bstrcpy(sl->entry[i]);
            btrimws(copy);
            *out = copy;
            return BSTR_OK;
        }
    }
    return BSTR_ERR;
}

int bstrListRemoveDup(struct bstrList * sl)
{
    int i = 0;
    if (!sl) return BSTR_ERR;
    for (i = 0; i < sl->qty; i++)
    {
        bstring curr = bstrListGet(sl, i);
        if (!curr) return BSTR_ERR;
        for (int j = i + 1; j < sl->qty; )
        {
            if (bstrncmp(curr, bstrListGet(sl, j), blength(curr)) == BSTR_OK)
            {
                if (bstrListDel(sl, j) != BSTR_OK)
                {
                    return BSTR_ERR;
                }
            }
            else
            {
                j++;
            }
        }
    }
    return BSTR_OK;
}

struct bstrList* bstrListCopy(struct bstrList * sl)
{
    int i = 0;
    if (!sl) return NULL;
    struct bstrList* ol = bstrListCreate();
    for (i = 0; i < sl->qty; i++)
    {
        bstrListAdd(ol, sl->entry[i]);
    }
    return ol;
}

int bstrListSortFunc(struct bstrList* in, int (*cmp)(const struct tagbstring * left, const struct tagbstring * right), struct bstrList** out)
{
    struct bstrList *tmp = bstrListCopy(in);
    struct bstrList *outList = bstrListCreate();

    while (tmp->qty > 0)
    {
        int mini = 0;
        for (int i = 0; i < tmp->qty; i++)
        {
            mini = (cmp(tmp->entry[mini], tmp->entry[i]) < 0 ? mini : i);
        }
        if (mini >= 0)
        {
            bstrListAdd(outList, tmp->entry[mini]);
            bstrListDel(tmp, mini);
        }
    }
    bstrListDestroy(tmp);
    *out = outList;
    return 0;
}

int bstrListSort(struct bstrList* in, struct bstrList** out)
{
    return bstrListSortFunc(in, bstrcmp, out);
}

/*
 * int btrimbrackets (bstring b)
 *
 * Delete opening and closing brackets contiguous from both ends of the string.
 */
 #define   bracket(c) ((((unsigned char) c) == '(' || ((unsigned char) c) == ')'))
int btrimbrackets (bstring b) {
int i, j;

    if (b == NULL || b->data == NULL || b->mlen < b->slen ||
        b->slen < 0 || b->mlen <= 0) return BSTR_ERR;

    for (i = b->slen - 1; i >= 0; i--) {
        if (!bracket (b->data[i])) {
            if (b->mlen > i) b->data[i+1] = (unsigned char) '\0';
            b->slen = i + 1;
            for (j = 0; bracket (b->data[j]); j++) {}
            return bdelete (b, 0, j);
        }
    }

    b->data[0] = (unsigned char) '\0';
    b->slen = 0;
    return BSTR_OK;
}

int bisinteger(bstring b)
{
    int count = 0;
    for (int i = 0; i < blength(b); i++)
    {
        if (!isdigit(bchar(b, i)))
            break;
        count++;
    }
    return count == blength(b);
}

int bisnumber(bstring b)
{
    int count = 0;
    int has_digits = 0;
    if (bchar(b, 0) == '-')
    {
        count++;
    }
    while (count < blength(b) && isdigit(bchar(b, count)))
    {
        count++;
        has_digits = 1;
    }
    if (count < blength(b) && bchar(b, count) == '.')
    {
        count++;
        while (count < blength(b) && isdigit(bchar(b, count)))
        {
            count++;
            has_digits = 1;
        }
    }

    if (!has_digits)
        return 0;

    if (count < blength(b) && ((bchar(b, count) == 'E' || bchar(b, count) == 'e')))
    {
        count++;
        if (count < blength(b) && (bchar(b, count) == '-' || bchar(b, count) == '+'))
        {
            count++;
        }

        int exp_digits = 0;
        while (count < blength(b) && isdigit(bchar(b, count)))
        {
            count++;
            exp_digits = 1;
        }
        if (!exp_digits)
            return 0;
    }
    return count == blength(b);
}

bstring read_file(char *filename)
{
    int ret = 0;
    FILE* fp = NULL;
    char buf[BUFSIZ];
    bstring content = bfromcstr("");
    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "fopen(%s): errno=%d\n", filename, errno);
        return content;
    }
    for (;;) {
        /* Read another chunk */
        ret = fread(buf, 1, sizeof(buf), fp);
        if (ret < 0) {
            fprintf(stderr, "fread(%p, 1, %lu, %p): %d, errno=%d\n", buf, sizeof(buf), fp, ret, errno);
            fclose(fp);
            return content;
        }
        else if (ret == 0) {
            break;
        }
        bcatblk(content, buf, ret);
    }
    fclose(fp);
    return content;
}

int write_bstrList_to_file(struct bstrList* list, char* filename)
{
    int ret = 0;
    char newline = '\n';
    FILE* fp = NULL;
    size_t (*myfwrite)(const void *ptr, size_t size, size_t nmemb, FILE *stream) = fwrite;
    fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "fopen(%s): errno=%d\n", filename, errno);
        return -1;
    }
    for (int i = 0; i < list->qty; i++)
    {
        ret = myfwrite(bdata(list->entry[i]), 1, blength(list->entry[i]) * sizeof(char), fp);
        if (ret < 0) {
            fprintf(stderr, "fwrite(%p, 1, %lu, %p): %d, errno=%d\n", bdata(list->entry[i]), blength(list->entry[i]) * sizeof(char), fp, ret, errno);
            fclose(fp);
            return -1;
        }
        ret = myfwrite(&newline, 1, sizeof(char), fp);
        if (ret < 0) {
            fprintf(stderr, "fwrite(%p, 1, %lu, %p): %d, errno=%d\n", &newline, sizeof(char), fp, ret, errno);
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    return 0;
}

int batoi(bstring b, int* value)
{
    if (b == NULL || value == NULL || b->data == NULL || blength(b) == 0) return BSTR_ERR;

    char* endptr = NULL;
    errno = 0;
    long (*mystrtol)(const char *nptr, char **endptr, int base) = strtol;
    long int result = mystrtol(bdata(b), &endptr, 10);

    if (endptr == bdata(b))
    {
        b = NULL;
        result = 0;
        *value = 0;
        return BSTR_ERR;
    }

    if ((errno == ERANGE && (result > ULLONG_MAX || result < LLONG_MIN)) || (errno != 0 && result == 0)) return BSTR_ERR;

    *value = (int)result;
    return BSTR_OK;
}

int batoi64(bstring b, int64_t* value)
{
    if (b == NULL || value == NULL || b->data == NULL || blength(b) == 0) return BSTR_ERR;

    char* endptr = NULL;
    errno = 0;
    long long int(*mystrtoll)(const char *nptr, char **endptr, int base) = strtoll;
    long long int result = mystrtoll(bdata(b), &endptr, 10);

    if (endptr == bdata(b))
    {
        b = NULL;
        result = 0;
        *value = 0;
        return BSTR_ERR;
    }

    if ((errno == ERANGE && (result > ULLONG_MAX || result < LLONG_MIN)) || (errno != 0 && result == 0)) return BSTR_ERR;

    *value = (int64_t)result;
    return BSTR_OK;
}

int batof(bstring b, float* value)
{
    if (b == NULL || value == NULL || bdata(b) == NULL || blength(b) == 0) return BSTR_ERR;

    char* endptr = NULL;
    errno = 0;
    float (*mystrtof)(const char *nptr, char **endptr) = strtof;
    float result = mystrtof(bdata(b), &endptr);

    if (endptr == bdata(b) || *endptr != '\0')
    {
        b = NULL;
        result = 0.0f;
        *value = 0.0f;
        return BSTR_ERR;
    }

    if ((errno == ERANGE && (result > HUGE_VALF || result < -HUGE_VALF)) || (errno != 0 && result == 0.0f)) return BSTR_ERR;

    *value = result;
    return BSTR_OK;
}

int batod(bstring b, double* value)
{
    if (b == NULL || value == NULL || bdata(b) == NULL || blength(b) == 0) return BSTR_ERR;

    char* endptr = NULL;
    errno = 0;
    double (*mystrtod)(const char *nptr, char **endptr) = strtod;
    double result = mystrtod(bdata(b), &endptr);

    if (endptr == bdata(b) || *endptr != '\0')
    {
        b = NULL;
        result = 0.0;
        *value = 0.0;
        return BSTR_ERR;
    }

    if ((errno == ERANGE && (result > HUGE_VAL || result < -HUGE_VAL)) || (errno != 0 && result == 0.0)) return BSTR_ERR;

    *value = result;
    return BSTR_OK;
}

void bstrtok_delimters(bstring input, struct bstrList* blist)
{
    const char* delimiters = DELIMITERS;
    char* str = bstr2cstr(input, ' ');
    int start = 0;
    int len = strlen(str);
    for (int i = 0; i <= len; i++)
    {
        if (strchr(delimiters, str[i]) || str[i] == '\0')
        {
            if (i > start)
            {
                bstring btok1 = blk2bstr(str + start, i - start);
                bstrListAdd(blist, btok1);
                bdestroy(btok1);
            }
            if (str[i] != '\0')
            {
                bstring btok2 = blk2bstr(str + i, 1);
                bstrListAdd(blist, btok2);
                bdestroy(btok2);
            }
            start = i + 1;
        }
    }
    bcstrfree(str);
}
