/**
 * Copyright (c) 2026 DustAtom
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#ifndef HASHMAP_LIB_H
#define HASHMAP_LIB_H 1

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

#define DEFAULT_BASE_CAPACITY 32
#define RESIZE_THRESHOLD 0.75L

typedef u_int64_t (*HashFunction)(const void* key);
typedef bool (*EqualFunction)(const void* a, const void* b);
typedef void* (*CloneFunction)(const void* item);
typedef void (*FreeFunction)(void* ptr);

typedef struct Entry {
    void* key;
    void* value;
    u_int64_t hash;
    struct Entry* next;
} Entry;

typedef struct HashMap {
    Entry** buckets;
    size_t capacity;
    size_t size;
    HashFunction hash;
    EqualFunction equals;
    CloneFunction clone_key;
    FreeFunction free_key;
    CloneFunction clone_value;
    FreeFunction free_value;
} HashMap;

u_int64_t fnv1a_hash(const void* data, size_t len);
void* str_clone(const void* key);
u_int64_t str_hash(const void* key);
bool str_eq(const void* a, const void* b);
void* int_clone(const void* key);
u_int64_t int_hash(u_int64_t x);
bool int_eq(const void* a, const void* b);
void int_free(void* x);

void map_resize(HashMap* map, size_t new_capacity);
HashMap* map_new(
    size_t initial_capacity,
    HashFunction hash_function,
    EqualFunction equal_function,
    CloneFunction clone_key_function,
    FreeFunction free_key_function,
    CloneFunction clone_value_function,
    FreeFunction free_value_function
);
bool map_put(HashMap* map, void* key, void* value);
bool map_delete(HashMap* map, const void* key);
void map_resize(HashMap* map, size_t new_capacity);
void map_destroy(HashMap* map);
void* map_get(HashMap* map, const void* key);

#endif // HASHMAP_LIB_H
