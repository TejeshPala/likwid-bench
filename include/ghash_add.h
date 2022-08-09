/*
 * =======================================================================================
 *
 *      Filename:  ghash_add.h
 *
 *      Description:  Header File for ghash's g_hash_table_new_full function
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

#ifndef __G_HASH_ADD_H__
#define __G_HASH_ADD_H__

GHashTable * g_hash_table_new_full (GHashFunc      hash_func,
				    GEqualFunc     key_equal_func,
				    GDestroyNotify key_destroy_func,
				    GDestroyNotify value_destroy_func);
#endif
