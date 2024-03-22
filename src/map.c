/*
 * =======================================================================================
 *
 *      Filename:  map.c
 *
 *      Description:  Implementation a hashmap in C using ghash as backend
 *
 *      Version:   1.0
 *      Released:  21.04.2021
 *
 *      Author:   Thomas Gruber (tg), thomas.gruber@googlemail.com
 *      Project:  c-map
 *
 *      Copyright (C) 2019 RRZE, University Erlangen-Nuremberg
 *
 *      This program is free software: you can redistribute it and/or modify it under
 *      the terms of the GNU General Public License as published by the Free Software
 *      Foundation, either version 2 of the License, or (at your option) any later
 *      version.
 *
 *      This program is distributed in the hope that it will be useful, but WITHOUT ANY
 *      WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 *      PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along with
 *      this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =======================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>

#include <map.h>

#ifdef WITH_BSTRING
#include <bstrlib.h>
#endif

static int* int_dup(int val)
{
    int* valptr = malloc(sizeof(int));
    if (valptr)
    {
        *valptr = val;
    }
    return valptr;
}

#ifdef WITH_BSTRING
gboolean
g_bstr_equal (gconstpointer v1,
              gconstpointer v2)
{
  const_bstring string1 = v1;
  const_bstring string2 = v2;

  return bstrcmp (string1, string2) == BSTR_OK;
}

guint
g_bstr_hash (gconstpointer v)
{
    uint32_t h = 5381;
    const signed char *p;
    bstring b = (bstring)v;
    int i = 0;
    for (; i < blength(b); ++i)
    {
        p = bdataofs(b, i);
        h = (h << 5) + h + *p;
    }
    return h;
}

void g_bstr_destroy(void* v)
{
    bstring b = (bstring)v;
    bdestroy(b);
}
#endif



static int _init_map(Map_t* map, MapKeyType type, int max_size, map_value_destroy_func value_func)
{
    int err = 0;
    Map* m = malloc(sizeof(Map));
    if (m)
    {
        switch(type)
        {
            case MAP_KEY_TYPE_STR:
                m->ghash = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
                if (m->ghash)
                {
                    err = 0;
                }
                break;
            case MAP_KEY_TYPE_INT:
                m->ghash = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, free);
                if (m->ghash)
                {
                    err = 0;
                }
                break;
            case MAP_KEY_TYPE_UINT:
                m->ghash = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, free);
                if (m->ghash)
                {
                    err = 0;
                }
                break;
#ifdef WITH_BSTRING
            case MAP_KEY_TYPE_BSTR:
                m->ghash = g_hash_table_new_full(g_bstr_hash, g_bstr_equal, g_bstr_destroy, free);
                if (m->ghash)
                {
                    err = 0;
                }
                break;
#endif
            default:
                printf("Unknown hash type\n");
                free(m);
                err = -ENODEV;
                break;
        }
    }
    else
    {
        err = -ENOMEM;
    }
    if (!err && m)
    {
        m->num_values = 0;
        m->size = 0;
        m->max_size = max_size;
        m->values = NULL;
        m->key_type = type;
        m->value_func = value_func;
        *map = m;
    }
    return err;
}

#define init_map_func(name, keytype) \
int init_##name##map(Map_t* map, map_value_destroy_func func) \
{ \
    int ret = _init_map(map, keytype, -1, (func)); \
    return ret; \
} \

init_map_func(s, MAP_KEY_TYPE_STR)
init_map_func(i, MAP_KEY_TYPE_INT)
init_map_func(u, MAP_KEY_TYPE_UINT)
#ifdef WITH_BSTRING
init_map_func(b, MAP_KEY_TYPE_BSTR)
#endif

#define initsize_map_func(name, keytype) \
int init_##name##map_size(Map_t* map, map_value_destroy_func func, int max_size) \
{ \
    int ret = _init_map(map, (keytype), (max_size), (func)); \
    return ret; \
} \

initsize_map_func(s, MAP_KEY_TYPE_STR)
initsize_map_func(i, MAP_KEY_TYPE_INT)
initsize_map_func(u, MAP_KEY_TYPE_UINT)
#ifdef WITH_BSTRING
initsize_map_func(b, MAP_KEY_TYPE_BSTR)
#endif

#define add_map_func(name, type, keyaccess, valueassign) \
int add_##name##map(Map_t map, type key, void* val) \
{ \
    MapValue *mval = NULL; \
    gpointer gval = g_hash_table_lookup(map->ghash, (keyaccess)); \
    if (gval) \
    { \
        printf("Key exists\n"); \
        return -EEXIST; \
    } \
    if (map->num_values == map->size) \
    { \
        if (map->max_size > 0 && map->size == map->max_size) \
        { \
            return -ENOSPC; \
        } \
        MapValue *vals = realloc(map->values, (map->size+1)*sizeof(MapValue)); \
        if (!vals) \
        { \
            return -ENOMEM; \
        } \
        map->values = vals; \
        map->values[map->size].key = NULL; \
        map->values[map->size].value = NULL; \
        map->values[map->size].iptr = NULL; \
        map->size++; \
    } \
    if (map->num_values < map->size) \
    { \
        int idx = map->size-1; \
        while (idx >= 0 && map->values[idx].value != NULL) \
        { \
            idx--; \
        } \
        map->values[idx].key = (valueassign); \
        map->values[idx].value = val;\
        map->values[idx].iptr = int_dup(idx);\
        g_hash_table_insert(map->ghash, map->values[idx].key, map->values[idx].iptr);\
        map->num_values++;\
        return idx;\
    }\
    return -1;\
}
add_map_func(i, int64_t, (gpointer)key, (gpointer)key)
add_map_func(u, uint64_t, (gpointer)key, (gpointer)key)
add_map_func(s, char*, key, g_strdup(key))
#ifdef WITH_BSTRING
add_map_func(b, bstring, key, bstrcpy(key))
#endif


#define get_map_func(name, type, keyaccess) \
int get_##name##map_by_key(Map_t map, type key, void** val) \
{ \
    gpointer gval = g_hash_table_lookup(map->ghash, keyaccess); \
    if (gval) \
    { \
        if (val) \
        { \
            int* ival = (int*)gval; \
            MapValue *mval = &(map->values[*ival]); \
            *val = (void*)mval->value; \
        } \
        return 0; \
    } \
    return -ENOENT; \
}

get_map_func(i, int64_t, (gpointer)key)
get_map_func(u, uint64_t, (gpointer)key)
get_map_func(s, char*, key)
#ifdef WITH_BSTRING
get_map_func(b, bstring, key)
#endif

#define getidx_map_func(name) \
int get_##name##map_by_idx(Map_t map, int idx, void** val) \
{ \
    if (idx >= 0 && idx < map->size) \
    { \
        MapValue *mval = &(map->values[idx]); \
        *val = (void*)mval->value; \
        return 0; \
    } \
    return -ENOENT; \
}

getidx_map_func(i)
getidx_map_func(u)
getidx_map_func(s)
#ifdef WITH_BSTRING
getidx_map_func(b)
#endif

#define get_idx_for_key(name, type, keyaccess) \
int get_##name##map_idx_for_key(Map_t map, type key, int* idx) \
{ \
    gpointer gval = g_hash_table_lookup(map->ghash, keyaccess); \
    if (gval) \
    { \
        if (idx) \
        { \
            int* ival = (int*)gval; \
            *idx = *ival; \
        } \
        return 0; \
    } \
    return -ENOENT; \
}

get_idx_for_key(i, int64_t, (gpointer)key)
get_idx_for_key(u, uint64_t, (gpointer)key)
get_idx_for_key(s, char*, key)
#ifdef WITH_BSTRING
get_idx_for_key(b, bstring, key)
#endif

#define update_map_func(name, type, keyaccess) \
int update_##name##map(Map_t map, type key, void* val, void** old) \
{ \
    MapValue *mval = NULL; \
    gpointer gval = g_hash_table_lookup(map->ghash, (keyaccess)); \
    if (map->num_values < map->size) \
    { \
        int idx = map->size-1; \
        while (idx >= 0 && map->values[idx].value != NULL) \
        { \
            idx--; \
        } \
        if (old) \
        { \
            *old = map->values[idx].value; \
        } \
        map->values[idx].value = val; \
        return idx; \
    } \
    return -1; \
}

update_map_func(s, char*, key)
update_map_func(i, int64_t, (gpointer)key)
update_map_func(u, uint64_t, (gpointer)key)
#ifdef WITH_BSTRING
update_map_func(b, bstring, key)
#endif


#define delete_map_func(name, type, keyaccess, valueaccess) \
int del_##name##map(Map_t map, type key) \
{ \
    gpointer gval = g_hash_table_lookup(map->ghash, (keyaccess)); \
    if (gval) \
    { \
        int* ival = (int*)gval; \
        map->values[*ival].key = NULL; \
        if (map->value_func != NULL) \
        { \
            map->value_func(map->values[*ival].value); \
        } \
        map->values[*ival].value = NULL; \
        map->values[*ival].iptr = NULL; \
        g_hash_table_remove(map->ghash, (keyaccess)); \
        map->num_values--; \
        return 0; \
    } \
    return -ENOENT; \
}

delete_map_func(i, int64_t, (gpointer)key, (gpointer)&key)
delete_map_func(u, uint64_t, (gpointer)key, (gpointer)&key)
delete_map_func(s, char*, key, key)
#ifdef WITH_BSTRING
delete_map_func(b, bstring, key, key)
#endif

#define destroy_map_func(name) \
void destroy_##name##map(Map_t map) \
{ \
    if (map) \
    { \
        g_hash_table_destroy(map->ghash); \
        map->ghash = NULL; \
        if (map->values) \
        { \
            if (map->value_func) \
            { \
                for (int i = 0; i < map->size; i++) \
                { \
                    if (map->values[i].value != NULL) \
                    { \
                        map->value_func(map->values[i].value); \
                    } \
                } \
            } \
            free(map->values); \
            map->values = NULL; \
        } \
        map->num_values = 0; \
        map->size = 0; \
        free(map); \
    } \
}

destroy_map_func(s)
destroy_map_func(i)
destroy_map_func(u)
#ifdef WITH_BSTRING
destroy_map_func(b)
#endif

struct cmap_foreach_data {
    Map_t map;
    map_foreach_func func;
    mpointer user_data;
} ;

void cmap_foreach(mpointer key, mpointer value, mpointer user_data)
{
    int* ival = (int*) value;
    struct cmap_foreach_data* data = (struct cmap_foreach_data*)user_data;
    if (ival)
    {
        data->func(key, data->map->values[*ival].value, data->user_data);
    }
}

#define foreach_map_func(name) \
void foreach_in_##name##map(Map_t map, map_foreach_func func, mpointer user_data) \
{ \
    if ((!map) || (!func)) return; \
    struct cmap_foreach_data data = {.map = map, .func = func, .user_data = user_data}; \
    if (map && func) \
    { \
        g_hash_table_foreach(map->ghash, cmap_foreach, &data); \
    } \
}

foreach_map_func(i)
foreach_map_func(u)
foreach_map_func(s)
#ifdef WITH_BSTRING
foreach_map_func(b)
#endif

#define size_map_func(name) \
int get_##name##map_size(Map_t map) \
{ \
    if (map) \
    { \
        return map->num_values; \
    } \
    return -1; \
}

size_map_func(i)
size_map_func(u)
size_map_func(s)
#ifdef WITH_BSTRING
size_map_func(b)
#endif
