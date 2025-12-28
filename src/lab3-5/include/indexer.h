#ifndef INDEXER_H
#define INDEXER_H

#include "types.h"
#include <cstdint>
#include <cstddef>

struct DocEntry {
    uint32_t doc_id;
    char* title;
    char* url;

    DocEntry() : doc_id(0), title(nullptr), url(nullptr) {}
};

class CustomIndexer {
private:
    static const size_t HASH_SIZE = 16384;
    DictNode* table[HASH_SIZE];
    
    DocEntry* forward_index;
    size_t forward_size;
    size_t forward_capacity;

    uint32_t total_unique_terms;      

    size_t hash_func(const char* str);

public:
    CustomIndexer();
    ~CustomIndexer();

    void add_document(uint32_t id, const char* title, const char* url, const char** tokens, size_t token_count);    
    void add_term(const char* term, uint32_t doc_id);
    void save_to_file(const char* dict_path, const char* post_path);
    void save_forward_index(const char* path, uint32_t doc_id, const char* title, const char* url);
};

#endif