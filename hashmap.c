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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "hashmaplib.h"


/* general hashing */

u_int64_t fnv1a_hash(const void* data, size_t len) {
    const unsigned char* bytes = data;
    u_int64_t hash = 14695981039346656037ULL;

    for (size_t i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }

    return hash;
}

/* str */

void* str_clone(const void* key) {
    const char* s = key;
    char* copy = (char*)malloc(strlen(s) + 1);
    if (!copy) return NULL; // OOM

    strcpy(copy, s);
    return copy;
}

u_int64_t str_hash(const void* key) {
    const char* s = key;

    return fnv1a_hash(s, strlen(s));
}

bool str_eq(const void* a, const void* b) {
    return strcmp((const char*)a, (const char*)b) == 0;
}

void str_free(void* x) {
    free((char*)x);
}

/* int */

void* int_clone(const void* key) {
    int* copy = (int*)malloc(sizeof(int));
    if (!copy) return NULL; // OOM

    *copy = *(int*)key;
    return copy;
}

u_int64_t int_hash(u_int64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;

    return x;
}

bool int_eq(const void* a, const void* b) {
    return *(int*)a == *(int*)b;
}

void int_free(void* x) {
    free((int*)x);
}

/* hashmaplib */

HashMap* map_new(
    size_t initial_capacity,
    HashFunction hash_function,
    EqualFunction equal_function,
    CloneFunction clone_key_function,
    FreeFunction free_key_function,
    CloneFunction clone_value_function,
    FreeFunction free_value_function
) {
    HashMap* new_hashmap = (HashMap*)malloc(sizeof(HashMap));
    if (!new_hashmap) return NULL;

    Entry** new_buckets = (Entry**)calloc(initial_capacity, sizeof(Entry*));
    if (!new_buckets) {
        free(new_hashmap);
        return NULL;
    }

    new_hashmap->buckets = new_buckets;
    new_hashmap->capacity = initial_capacity;
    new_hashmap->size = (size_t)0;
    new_hashmap->hash = hash_function;
    new_hashmap->equals = equal_function;
    new_hashmap->clone_key = clone_key_function;
    new_hashmap->free_key = free_key_function;
    new_hashmap->clone_value = clone_value_function;
    new_hashmap->free_value = free_value_function;

    return new_hashmap;
}

bool map_put(HashMap* map, void* key, void* value) {
    // resize if we are more than XX% full
    if ((map->size + 1) / (double)map->capacity > (double)RESIZE_THRESHOLD) {
        map_resize(map, map->capacity * 2);
    }

    // hash the key, clamp the index
    u_int64_t hash = map->hash(key);
    size_t index = hash & (map->capacity - 1);

    // check for existing keys in the Entry linked list
    Entry* entry = map->buckets[index];
    while (entry) {
        if (entry->hash == hash && map->equals(entry->key, key)) {
            // value already associated with this key, replace
            map->free_value(entry->value);
            entry->value = map->clone_value(value);
            return entry->value != NULL;
        }
        
        entry = entry->next;
    }

    // if none exist, add a new entry
    void* new_key = map->clone_key(key);
    if (!new_key) return false;

    void* new_value = map->clone_value(value);
    if (!new_value) {
        free(new_key);
        return false;
    }

    // new Entry
    Entry* new_entry = (Entry*)malloc(sizeof(Entry));
    if (!new_entry) {
        free(new_key);
        free(new_value);
        return false;
    }

    new_entry->key = new_key;
    new_entry->value = new_value;
    new_entry->hash = hash;
    new_entry->next = map->buckets[index];

    // register the bucket
    map->buckets[index] = new_entry;
    map->size++;

    return true;
}

bool map_delete(HashMap* map, const void* key) {
    u_int64_t hash = map->hash(key);
    size_t index = hash & (map->capacity - 1);
    Entry** current = &map->buckets[index];

    while (*current) {
        if ((*current)->hash == hash && map->equals((*current)->key, key)) {
            Entry* to_remove = *current;
            *current = to_remove->next;
            
            map->free_key(to_remove->key);
            map->free_value(to_remove->value);
            free(to_remove);
            map->size--;

            return true;
        }

        current = &(*current)->next;
    }

    return false;
}

void map_resize(HashMap* map, size_t new_capacity) {
    Entry** new_buckets = (Entry**)calloc(new_capacity, sizeof(Entry*));
    
    for (size_t i = 0; i < map->capacity; i++) {
        Entry* entry = map->buckets[i];

        while (entry) {
            Entry* next = entry->next; // save before relink
            size_t index = entry->hash & (new_capacity - 1);
            entry->next = new_buckets[index];
            new_buckets[index] = entry;
            entry = next;
        }
    }

    free(map->buckets);
    map->buckets = new_buckets;
    map->capacity = new_capacity;
}

void map_destroy(HashMap* map) {
    for (size_t i = 0; i < map->capacity; i++) {
        Entry* entry = map->buckets[i];

        while (entry) {
            Entry* next = entry->next;
            map->free_key(entry->key);
            map->free_value(entry->value);
            free(entry);
            entry = next;
        }
    }    

    free(map->buckets);
    free(map);
}

void* map_get(HashMap* map, const void* key) {
    u_int64_t hash = map->hash(key);
    size_t index = hash & (map->capacity - 1);

    Entry* entry = map->buckets[index];
    while (entry) {
        if (entry->hash == hash && map->equals(entry->key, key)) {
            return entry->value;
        }

        entry = entry->next;
    }

    return NULL; // not found
}
