#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "obj_loader.h"

#define MAX_TRIPLES_PER_LINE 64

typedef struct TriangleInfoIndex
{
    int vertex;
    int texture;
    int normal;
}TriangleInfoIndex;


typedef struct MeshData
{
    float *vertices;
    u32 vertices_count;
    
    float *textures;
    u32 textures_count;
    
    float *normals;
    u32 normals_count;
    
    TriangleInfoIndex *index;
    u32 indices_count;
    
    u32 faces_count;
}MeshData;

local_persist FileContent
file_read(char *filename)
{
    FileContent result = {0};
    
    FILE *file = fopen(filename, "rb");
    if(file)
    {
        fseek(file, 0,SEEK_END);
        result.size = ftell(file);
        fseek(file, 0,SEEK_SET);
        
        result.contents = malloc(result.size);
        
        if(fread(result.contents,1,result.size,file) == result.size)
        {
            printf("Success");
        }
        else
        {
            printf("Failure");
        }
    }
    else
    {
        
    }
    
    return result;
}


local_persist bool
is_digit(char *string)
{
    bool result = false;
    
    if((string[0] >= '0') && (string[0] <= '9'))
    {
        result = true;
    }
    
    return result;
}

local_persist bool
is_whitespace(char *str)
{
    bool result = ((str[0] == ' ') || (str[0] == '\t'));
    
    return result;
}


local_persist void
skip_whitespace(char **str)
{
    // TODO(shvayko): check that expression. Can I do str++ shen I pass
    while(is_whitespace(*str))
    {
        *str = *str + 1;
    }
}


char *
ASCII_to_float(char **str, f32 *out)
{
    char *c = *str;
    s32 skip = 0;
    for(;
        is_whitespace(c);
        c++)
    {
        skip++;
    }
    
    for(;
        (!is_whitespace(c)) && (is_digit(c) || (c[0] == '.'));
        c++)
    {
        skip++;
    }
    *out = (f32)atof(*str);
    return *str + skip;
}

local_persist bool 
is_line_end(u8 *buf)
{
    bool result = false;
    if((buf[0] == '\n') || (buf[0] == '\r'))
    {
        result = true;
    }
    return result;
}

MeshData*
load_mesh(MemoryArena *arena,char *filename)
{
    MeshData *mesh = push_size(arena, MeshData);
    // TODO(shvayko): doesn't allocate 2GB memory
    void *memory_for_vertices = VirtualAlloc(0 ,GB(1), MEM_RESERVE,PAGE_READWRITE);
    ASSERT(memory_for_vertices);
    void *memory_for_textures = VirtualAlloc(0, GB(1), MEM_RESERVE,PAGE_READWRITE);
    ASSERT(memory_for_textures);
    void *memory_for_normals  = VirtualAlloc(0, GB(1), MEM_RESERVE,PAGE_READWRITE);
    ASSERT(memory_for_normals);
    void *memory_for_faces    = VirtualAlloc(0, GB(1), MEM_RESERVE,PAGE_READWRITE);
    ASSERT(memory_for_faces);
    
    MemoryArena arena_for_vertices = {0};
    MemoryArena arena_for_textures = {0};
    MemoryArena arena_for_normals  = {0};
    MemoryArena arena_for_faces    = {0};
    
    init_arena(&arena_for_vertices, GB(1), memory_for_vertices);
    init_arena(&arena_for_textures, GB(1), memory_for_textures);
    init_arena(&arena_for_normals,  GB(1), memory_for_normals);
    init_arena(&arena_for_faces,  GB(1), memory_for_faces);
    
    
    
    FileContent test = file_read(filename);
    
    
    s32 lines_total = 0;
    
    char *end = (char*)test.contents + test.size;
    char *last_found_line = 0;
    
    // TODO(shvayko): Need I todo this pass?
    for(u8 *current = test.contents;
        current != end;
        )
    {
        if(is_line_end(current++))
        {
            lines_total++;
            last_found_line = current;
        }
    }
    // But what if we have one more line that does not ending with "new line escape
    // sequences"?
    // TODO(shvayko): May be just null terminate entire file?
    if((end - last_found_line) > 0)
    {
        lines_total++;
    }
    
    LineInfo *lines_info = (LineInfo*)malloc(sizeof(LineInfo)*lines_total);
    
    u32 line_index = 0;
    u8 *prev = test.contents;
    for(u8 *current = test.contents;
        current != end;)
    {
        if(is_line_end(current++))
        {
            lines_info[line_index].p = prev;
            lines_info[line_index++].length = current - prev;
            prev = current;
        }
    }
    
    
    for(u32 lines_index = 0;
        lines_index < lines_total;
        lines_index++)
    {
        LineInfo *line = lines_info + lines_index;
        u8 *token = line->p;
        u32 line_length = line->length;
        u8 *end = token + line_length;
        while(token != end)
        {
            if((token[0] == '#') && (token[1] == ' '))
            {
                break;
            }
            else if(token[0] == 'o')
            {
                // TODO(shvayko): implement this
                token += line_length;
            }
            else if((token[0] == 'v') && (token[1] == ' '))
            {
                token++;
                f32 x = 0;
                f32 y = 0;
                f32 z = 0;
                token = ASCII_to_float(&token, &x);
                token = ASCII_to_float(&token, &y);
                token = ASCII_to_float(&token, &z);
                
                float *vertices = push_array(&arena_for_vertices,float,3);
                vertices[0] = x;
                vertices[1] = y;
                vertices[2] = z;
                
                mesh->vertices_count++;
                
            }
            else if((token[0] == 'v') && (token[1] == 't'))
            {
                token += 2;
                float u = 0;
                float v = 0;
                token = ASCII_to_float(&token, &u);
                token = ASCII_to_float(&token, &v);
                
                float *textures = push_array(&arena_for_textures,float,2);
                textures[0] = u;
                textures[1] = v;
                
                mesh->textures_count++;
            }
            else if((token[0] == 'v') && (token[1] == 'n'))
            {
                token += 2;
                float xn = 0;
                float yn = 0;
                float zn = 0;
                token = ASCII_to_float(&token, &xn);
                token = ASCII_to_float(&token, &yn);
                token = ASCII_to_float(&token, &zn);
                
                
                float *normals = push_array(&arena_for_normals,float,3);
                normals[0] = xn;
                normals[1] = yn;
                normals[2] = zn;
                
                mesh->normals_count++;
            }
            else if((token[0] == 'f') && (token[1] == ' '))
            {
                // NOTE(shvayko): f v/vt/vn v/vt/vn  v/vt/vn 
                /* NOTE(shvayko): Face elements use surface normals to indicate their orientation. If vertices are ordered counterclockwise around the face, both the face and the normal will point toward the viewer. If the vertex
                   ordering is clockwise, both will point away from the viewer. If
                   vertex normals are assigned, they should point in the general
                   direction of the surface normal, otherwise unpredictable results
                   may occur.*/
                token+=2;
                
                skip_whitespace(&token);
                TriangleInfoIndex this_face_indices[MAX_TRIPLES_PER_LINE] = {0};
                u32 line_vertices_count = 0;
                while((token[0] != is_line_end(token)) && (!is_line_end(token)))
                {
                    f32 v = 0;
                    f32 t = 0;
                    f32 n = 0;
                    
                    token = ASCII_to_float(&token, &v);
                    if((token[0] != '/' ) && (token[1] != '/'))
                    {
                        // NOTE(shvayko): f v v v
                        this_face_indices[line_vertices_count].vertex = v;
                        mesh->indices_count++;
                    }
                    else if((token[0] == '/') && (is_digit(&token[1])))
                    {
                        // NOTE(shvayko): f v/t/?
                        token++;
                        token = ASCII_to_float(&token, &t);
                        
                        // NOTE(shvayko): f v/t/n v/t/n v/t/n
                        if(token[0] == '/' && (is_digit(&token[1])))
                        {
                            token++;
                            token = ASCII_to_float(&token, &n);
                            
                            this_face_indices[line_vertices_count].vertex  = v;
                            this_face_indices[line_vertices_count].texture = t;
                            this_face_indices[line_vertices_count].normal  = n;
                            
                            this_face_indices[line_vertices_count];
                        }
                        else if(token[0] != '/')
                        {
                            // NOTE(shvayko): f v/t v/t v/t
                            this_face_indices[line_vertices_count].vertex  = v;
                            this_face_indices[line_vertices_count].texture = t;
                            mesh->indices_count++;
                        }
                        else
                        {
                            ASSERT(!"Undefined triple");
                        }
                        line_vertices_count++;
                    }
                    else if((token[0] == '/') && (token[1] == '/'))
                    {
                        // NOTE(shvayko): f v//t
                        token += 2;
                        token = ASCII_to_float(&token, &n);
                        this_face_indices[mesh->indices_count].vertex  = v;
                        this_face_indices[mesh->indices_count++].texture  = t;
                        line_vertices_count++;
                    }
                }
                
                // NOTE(shvayko): simple triangulation
                u32 triangles_count = 0;
                u32 face_vertices = 0;
                if(line_vertices_count > 3)
                {
                    for(s32 index = 1;
                        index < line_vertices_count - 1;
                        index++)
                    {
                        TriangleInfoIndex i0 = this_face_indices[0];
                        TriangleInfoIndex i1 = this_face_indices[index];
                        TriangleInfoIndex i2 = this_face_indices[index+1];
                        
                        TriangleInfoIndex *i = push_array(&arena_for_faces, TriangleInfoIndex,3);
                        
                        i[0] = i0;
                        i[1] = i1;
                        i[2] = i2;
                        mesh->indices_count+=3;
                    }
                }
                
                
                mesh->faces_count++;
            }
            else if((token[0] == 'm') && (token[1] == 't') &&
                    (token[2] == 'l') && (token[3] == 'l') && 
                    (token[4] == 'i') && (token[5] == 'b') && 
                    (token[6] == ' '))
            {
                // TODO(shvayko): implement
                token += line_length;
            }
            else if((token[0] == 'u') && (token[1] == 's') &&
                    (token[2] == 'e') && (token[3] == 'm') && 
                    (token[4] == 't') && (token[5] == 'l') && 
                    (token[6] == ' '))
            {
                // TODO(shvayko): implement
                token += line_length;
            }
            else if(token[0] == 's')
            {
                // TODO(shvayko): implement
                token += line_length;
            }
            else
            {
                ASSERT(!"Undefined header!");
            }
            while(token != end)
            {
                token++;
            }
        }
    }
    
    // pushing into "permanent" storage and freeing "temps arenas"
    mesh->vertices = push_array(arena,float, mesh->vertices_count*3);
    mesh->textures = push_array(arena,float, mesh->textures_count*2);
    mesh->normals  = push_array(arena,float, mesh->normals_count*3);
    mesh->index    = push_array(arena,TriangleInfoIndex, mesh->indices_count*sizeof(TriangleInfoIndex));
    
    memcpy(mesh->vertices, arena_for_vertices.base, mesh->vertices_count*3*sizeof(float));
    arena_release(&arena_for_vertices);
    
    memcpy(mesh->textures, arena_for_textures.base, mesh->textures_count*2*sizeof(float));
    arena_release(&arena_for_textures);
    
    memcpy(mesh->normals, arena_for_normals.base, mesh->normals_count*3*sizeof(float));
    arena_release(&arena_for_normals);
    
    memcpy(mesh->index, arena_for_faces.base, mesh->indices_count*sizeof(TriangleInfoIndex));
    arena_release(&arena_for_faces);
    
    return mesh;
}

int 
main(int argc, char *argv[])
{
    void *general_memory = VirtualAlloc(0, MB(512), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    ASSERT(general_memory);
    MemoryArena arena = {0};
    
    
    init_arena(&arena, MB(512), general_memory);
    
    
    MeshData *mesh = load_mesh(&arena,"../data/mario-sculpture.obj");
    
    int stop = 6;
    
    return 0;
}