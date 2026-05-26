#ifndef ARENA_H
#define ARENA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>

#define ARENA_DEF_PAGE_SIZE  1000

union max_alignment {
    long l;
    long long ll;
    void *p;
    double d;
    long double ld;
};
#ifndef MAX_ALIGN
/*use a named struct so offsetof is portable*/
struct max_align_helper { char c; union max_alignment a; };
#define MAX_ALIGN (offsetof(struct max_align_helper, a))
#endif

typedef struct m_arena{
    size_t size;
    size_t capacity;
    void* region;
    void* free_mem;
    struct m_arena* next_region;
    struct m_arena* prev_region;
} arena_t;

size_t _calculate_align_offset(void* raw, size_t align);

arena_t* create_arena();
arena_t* arena_give_child(arena_t* parent, arena_t* child);
arena_t* _inc_arena(arena_t* arena);
arena_t* _inc_arena_page(arena_t* page, size_t cap);
void _dec_arena(arena_t* arena);

void* _arena_aligned_alloc(arena_t* arena, size_t sz, size_t align);

void* arena_alloc(arena_t* arena, size_t sz);
void* arena_calloc(arena_t* arena, size_t pc_sz, size_t sz);
void arena_reset(arena_t* arena);
void arena_destroy(arena_t* arena);


#endif