#ifndef SEARCH_H
#define SEARCH_H

#include "indexer.h"
#include <cstring>

enum OpType { TOKEN, AND, OR, NOT, L_BRACKET, R_BRACKET };

struct QueryToken {
    OpType type;
    char word[64];
};

class SearchEngine {
public:
    // Только объявление
    IntArray* execute_query(const char* query, CustomIndexer& indexer, uint32_t total_docs);

private:
    // Обязательно объявляем все методы, которые используем в .cpp
    int get_priority(OpType type);
    void shunting_yard(const char* query, QueryToken* output, int& count);
    IntArray* intersect(IntArray* a, IntArray* b);
    IntArray* merge_union(IntArray* a, IntArray* b);
    IntArray* invert(IntArray* a, uint32_t total_docs);
};

#endif