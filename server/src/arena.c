#include "../arena.h"

size_t _calculate_align_offset(void* raw, size_t align){
    void* aligned_addr = (void*)(((uintptr_t)raw+align-1) & ~(align-1));
    return (uintptr_t)aligned_addr - (uintptr_t)raw;
}

arena_t* create_arena(){
    arena_t* new_arena = malloc(sizeof(arena_t));
    if(!new_arena) return NULL;

    new_arena->capacity = ARENA_DEF_PAGE_SIZE;
    new_arena->size = 0;
    new_arena->next_region = NULL;
    new_arena->prev_region = new_arena; //head is the tail
    new_arena->region = malloc(ARENA_DEF_PAGE_SIZE);
    if(!new_arena->region){
        free(new_arena);
        return NULL;
    }
    new_arena->free_mem = new_arena->region;
    return new_arena;
}
arena_t* arena_give_child(arena_t* parent, arena_t* child){
    assert(parent && child);
    arena_t* last_page = parent->prev_region;
    last_page->next_region = child;
    parent->prev_region = child;
    child->prev_region = last_page;
    return parent;
}
arena_t* _inc_arena(arena_t* arena){
    arena_t* new_arena = malloc(sizeof(arena_t));
    arena_t* last_node = arena->prev_region;
    if(!new_arena) return NULL;

    new_arena->capacity = ARENA_DEF_PAGE_SIZE;
    new_arena->size = 0;
    new_arena->next_region = NULL;
    new_arena->region = malloc(ARENA_DEF_PAGE_SIZE);
    if(!new_arena->region){
        free(new_arena);
        return NULL;
    }
    new_arena->free_mem = new_arena->region;

    new_arena->prev_region = last_node;
    last_node->next_region = new_arena;
    arena->prev_region = new_arena;

    return new_arena;
}
void _dec_arena(arena_t* arena){
    assert(arena);
    assert(arena->next_region);
    
    arena_t* last_node = arena->prev_region;
    arena_t* new_last = last_node->prev_region;

    new_last->next_region = NULL;
    arena->prev_region = new_last;

    free(last_node->region);
    free(last_node);
}

arena_t* _inc_arena_page(arena_t* page, size_t cap){
    assert(page);
    assert(cap > ARENA_DEF_PAGE_SIZE);
    assert(page->size == 0); //should only be used on empty pages as it resets them and unvalidates pointers

    void* region_cpy = page->region;
    page->region = realloc(page->region, cap);
    if(!page->region){
        page->region = region_cpy;
        return NULL;
    }
    page->free_mem = page->region;
    page->capacity = cap;
    page->size = 0;

    return page;
}

void* _arena_aligned_alloc(arena_t* arena, size_t sz, size_t align){
    assert(arena);
    assert(sz > 0);
    if(align == 0 || (align & (align-1)) != 0 ) return NULL; // align is a power of 2 

    arena_t* curr_page = arena;
    size_t max_offset = align - 1;
    size_t used_size;


    void* raw_addr = curr_page->free_mem;
    used_size = sz+_calculate_align_offset(raw_addr, align);
    
    while(curr_page->size + used_size > curr_page->capacity){
        if(curr_page->next_region) {
            curr_page = curr_page->next_region;
        }
        else {
            curr_page = _inc_arena(arena);
            if(!curr_page) return NULL;
            if(used_size > ARENA_DEF_PAGE_SIZE){
                if(!_inc_arena_page(curr_page, sz+max_offset)){ //to account for realloc possibly messing up alignment
                    _dec_arena(arena);
                    return NULL;
                }
            }
        }
        raw_addr = curr_page->free_mem;
        used_size = sz + _calculate_align_offset(raw_addr, align);;
    }

    curr_page->free_mem = (char*)curr_page->free_mem + used_size;
    curr_page->size += used_size;

    return (char*)raw_addr+_calculate_align_offset(raw_addr, align);
}

void* arena_alloc(arena_t* arena, size_t sz){
    assert(arena);
    assert(sz > 0);

    void* ptr = _arena_aligned_alloc(arena, sz, MAX_ALIGN);
    if(!ptr) return NULL;

    return ptr;
}
void* arena_calloc(arena_t* arena, size_t pc_sz, size_t sz){
    assert(arena);
    assert(pc_sz > 0 && sz > 0);

    void* ptr = arena_alloc(arena, pc_sz*sz);
    if(!ptr) return NULL;

    memset(ptr, 0, pc_sz*sz);
    return ptr;
}
void arena_reset(arena_t* arena){
    arena_t* page;
    arena_t* next_cpy;

    arena->free_mem = arena->region;
    arena->size = 0;
    arena->prev_region = arena;

    if(!arena->next_region) return;
    for(page = arena->next_region; page; page = next_cpy){
        next_cpy = page->next_region;
        free(page->region);
        free(page);
    }
    arena->next_region = NULL;
}
void arena_destroy(arena_t* arena){
    arena_reset(arena);
    free(arena->region);
    free(arena);
}