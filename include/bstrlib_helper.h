#ifndef BSTRLIB_HELPER_INCLUDE
#define BSTRLIB_HELPER_INCLUDE

#include "bstrlib.h"

#ifdef __cplusplus
extern "C" {
#endif


int bstrListAdd(struct bstrList * sl, bstring str);
int bstrListAddChar(struct bstrList * sl, char* str);

int bstrListDel(struct bstrList * sl, int idx);

bstring bstrListGet(struct bstrList * sl, int idx);

struct bstrList* bstrListCopy(struct bstrList * sl);
int bstrListRemove(struct bstrList * sl, bstring str);

void bstrListPrint(struct bstrList * sl);
int bstrListSort(struct bstrList* in, struct bstrList** out);
typedef int (*bstrListSortFunction)(const struct tagbstring * left, const struct tagbstring * right);
int bstrListSortFunc(struct bstrList* in, bstrListSortFunction cmp, struct bstrList** out);


int btrimbrackets (bstring b);
int bisnumber(bstring b);
int bisinteger(bstring b);

int batoi(bstring b, int* value);
int batoi64(bstring b, int64_t* value);
int batof(bstring b, float* value);
int batod(bstring b, double* value);

bstring read_file(char *filename);
int write_bstrList_to_file(struct bstrList* list, char* filename);

#ifdef __cplusplus
}
#endif

#endif /* BSTRLIB_HELPER_INCLUDE */
