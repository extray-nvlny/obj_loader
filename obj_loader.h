/* date = September 23rd 2021 7:45 pm */
#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#define local_persist static 

#define KB(x) (x * 1024)
#define MB(x) (KB(x) * 1024)
#define GB(x) (MB(x) * 1024)

#define ASSERT(condition) if(!(condition)) {*(int*)0 = 0;}
#define MAX(x,y) ((x > y) ? x : y)
#define MIN(x,y) ((x < y) ? x : y)

typedef uint8_t  u8;
typedef uint32_t u16;
typedef uint32_t u32;
typedef uint32_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float f32;
typedef double f64;

typedef struct MemoryArena
{
    char *base;
    size_t size;
    size_t used;
    size_t commited;
    
    s32 temp_count;
}MemoryArena;

typedef struct FileContent
{
    u32 size;
    void *contents;
}FileContent;


typedef struct LineInfo
{
    u8 *p;
    u32 length;
}LineInfo;

void
init_arena(MemoryArena *arena, size_t size, void *base)
{
    arena->base = (char*)base;
    arena->size = size;
    arena->commited = 0;
    arena->used = 0;
}

#define push_size(arena, type) (type*)push_size_(arena, sizeof(type))
#define push_array(arena, type, count) (type*)push_size_(arena, sizeof(type)*count)

#define ALIGN_POW_2(x,p) (((x) + (p) - 1)&~((p) - 1))
#define DEFAULT_COMMIT_SIZE (MB(1))

void*
push_size_(MemoryArena *arena, size_t size)
{
    void *result = 0;
    ASSERT((arena->used+size) <= arena->size);
    
    result = arena->base + arena->used;
    arena->used += size;
    size_t commited = arena->commited;
    
    if(arena->used > commited)
    {
        size_t new_commited_size = ALIGN_POW_2(arena->used, MAX(size,DEFAULT_COMMIT_SIZE));;
        VirtualAlloc(arena->base + commited,new_commited_size, MEM_COMMIT, PAGE_READWRITE);
        arena->commited += new_commited_size;
    }
    
    arena->size -= size;
    
    return result;
}

void
arena_release(MemoryArena *arena)
{
    ASSERT(arena);
    if(arena->base)
    {
        VirtualFree(arena->base,0,MEM_RELEASE);
    }
}

typedef struct TempMemory
{
    MemoryArena *arena;
    size_t used;
    
}TempMemory;

TempMemory
begin_temp_memory(MemoryArena *arena)
{
    TempMemory result;
    
    arena->temp_count++;
    result.arena = arena;
    result.used = arena->used;
    
    return result;
}

void
end_temp_memory(TempMemory temp_memory)
{
    MemoryArena *arena = temp_memory.arena;
    arena->used = temp_memory.used;
    arena->temp_count--;
}

#endif //OBJ_LOADER_H
