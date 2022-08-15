/*
 * =======================================================================================
 *
 *      Filename:  map.h
 *
 *      Description:  Header File for C Hashmap
 *
 *      Version:   <VERSION>
 *      Released:  <DATE>
 *
 *      Author:   Thomas Gruber (tg), thomas.gruber@googlemail.com
 *      Project:  likwid-bench
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

#ifndef MAP_H
#define MAP_H

#include "ghash.h"
#include "ghash_add.h"

#ifdef WITH_BSTRING
#include "bstrlib.h"
#endif /* WITH_BSTRING */

typedef void* mpointer;
typedef void (*map_value_destroy_func)(mpointer data);
typedef void (*map_foreach_func)(mpointer key, mpointer value, mpointer user_data);

typedef enum {
    MAP_KEY_TYPE_STR = 0,
    MAP_KEY_TYPE_INT,
    MAP_KEY_TYPE_UINT,
#ifdef WITH_BSTRING
    MAP_KEY_TYPE_BSTR,
#endif /* WITH_BSTRING */
    MAX_MAP_KEY_TYPE
} MapKeyType;

typedef struct {
    mpointer key;
    mpointer value;
    mpointer iptr;
} MapValue;

typedef struct {
    int num_values;
    int size;
    int max_size;
    int id;
    GHashTable *ghash;
    MapKeyType key_type;
    MapValue *values;
    map_value_destroy_func value_func;
} Map;

typedef Map* Map_t;

#define declare_map_type(name, type) \
int init_##name##map(Map_t* map, map_value_destroy_func value_func); \
int add_##name##map(Map_t map, type key, void* val); \
int get_##name##map_by_key(Map_t map, type key, void** val); \
int get_##name##map_by_idx(Map_t map, int idx, void** val); \
int update_##name##map(Map_t map, type key, void* val, void** old); \
void foreach_in_##name##map(Map_t map, map_foreach_func func, mpointer user_data); \
int del_##name##map(Map_t map, type key); \
int get_##name##map_size(Map_t map); \
void destroy_##name##map(Map_t map); \

declare_map_type(i, int64_t)
declare_map_type(u, uint64_t)
declare_map_type(s, char*)
#ifdef WITH_BSTRING
declare_map_type(b, bstring)
#endif /* WITH_BSTRING */

#endif /* MAP_H */
