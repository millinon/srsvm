#pragma once

#include "srsvm/impl.h"
#include "srsvm/opcode.h"

#define SRSVM_MODULE_MAX_COUNT (4 * WORD_SIZE)
#define SRSVM_MODULE_MAX_NAME_LEN 256

typedef struct
{
    char name[SRSVM_MODULE_MAX_NAME_LEN];

    srsvm_opcode_map *opcode_map;

    srsvm_native_module_handle handle;
    unsigned long ref_count;

    void *tag;

    srsvm_word id;
} srsvm_module;

srsvm_module *srsvm_module_alloc(const char* name, const char* filename, srsvm_word id);
void srsvm_module_free(srsvm_module *mod);

/*typedef struct srsvm_module_map_node srsvm_module_map_node;

struct srsvm_module_map_node
{
    srsvm_module_map_node *parent;

    srsvm_module *mod;

    srsvm_module_map_node *lchild;
    srsvm_module_map_node *rchild;

    unsigned height;
};

typedef struct
{
    srsvm_lock lock;

    srsvm_module_map_node *root;

    size_t count;
} srsvm_module_map;*/

char* srsvm_module_find(const char* module_name, const char* prog_cwd, char** search_path, bool search_multilib);

/*srsvm_module_map *srsvm_module_map_alloc(void);
void srsvm_module_map_free(srsvm_module_map* map);

bool srsvm_module_exists(const srsvm_string_map* map, const char* module_name);
srsvm_module *srsvm_module_lookup(const srsvam_string_map* map, const char* module_name);

bool srsvm_module_map_insert(srsvm_string_map *map, srsvm_module* mod);
bool srsvm_module_map_remove(srsvm_string_map *map, const char* module_name); */
