/*
 * =======================================================================================
 *      Filename:  stack.h
 *
 *      Description:  The header for the stack used by the calculator
 *
 *      Version:   <VERSION>
 *      Released:  <DATE>
 *
 *      Author:  Thomas Gruber (tg), thomas.roehl@gmail.com
 *      Project:  likwid-bench
 *
 *      Copyright (C) 2019 RRZE, University Erlangen-Nuremberg
 *
 *      This program is free software: you can redistribute it and/or modify it under
 *      the terms of the GNU General Public License as published by the Free Software
 *      Foundation, either version 3 of the License, or (at your option) any later
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

typedef struct
{
	void **content;
	int size;
	int top;
} Stack;

void stackInit(Stack *s, int size);
void stackPush(Stack *s, void* val);
void* stackTop(Stack *s);
void* stackPop(Stack *s);
int stackSize(Stack *s);
void stackFree(Stack *s);
