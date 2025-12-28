#include "indexer.h"
#include <fstream>
#include <cstring>

CustomIndexer::CustomIndexer() : forward_size(0), forward_capacity(10), total_unique_terms(0) {
    for (size_t i = 0; i < HASH_SIZE; ++i) table[i] = nullptr;
    forward_index = new DocEntry[forward_capacity];
}

CustomIndexer::~CustomIndexer() {
    // Очистка хеш-таблицы
    for (size_t i = 0; i < HASH_SIZE; ++i) {
        DictNode* current = table[i];
        while (current != nullptr) {
            DictNode* next = current->next;
            delete current; // Вызовется ~DictNode(), который удалит term и IntArray
            current = next;
        }
    }
}

size_t CustomIndexer::hash_func(const char* str) {
    size_t hash = 5381;
    int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c;
    return hash % HASH_SIZE;
}

void CustomIndexer::add_term(const char* term, uint32_t doc_id) {
    size_t idx = hash_func(term);
    DictNode* current = table[idx];

    while (current) {
        if (strcmp(current->term, term) == 0) {
            size_t sz = current->doc_ids->get_size();
            if (sz == 0 || current->doc_ids->get_data()[sz - 1] != doc_id) {
                current->doc_ids->push_back(doc_id);
            }
            return;
        }
        current = current->next;
    }

    DictNode* new_node = new DictNode(term);
    new_node->next = table[idx];
    table[idx] = new_node;
    new_node->doc_ids->push_back(doc_id);
    total_unique_terms++;
}

void CustomIndexer::save_to_file(const char* dict_path, const char* post_path) {
    std::ofstream d_out(dict_path, std::ios::binary);
    std::ofstream p_out(post_path, std::ios::binary);

    uint32_t magic = 0x49445831;
    d_out.write((char*)&magic, sizeof(uint32_t));
    d_out.write((char*)&total_unique_terms, sizeof(uint32_t));

    uint64_t current_offset = 0;

    for (size_t i = 0; i < HASH_SIZE; ++i) {
        DictNode* node = table[i];
        while (node) {
            uint16_t len = (uint16_t)strlen(node->term);
            uint32_t df = (uint32_t)node->doc_ids->get_size();

            d_out.write((char*)&len, sizeof(uint16_t));
            d_out.write(node->term, len);
            d_out.write((char*)&current_offset, sizeof(uint64_t));
            d_out.write((char*)&df, sizeof(uint32_t));

            p_out.write((char*)node->doc_ids->get_data(), df * sizeof(uint32_t));

            current_offset += (df * sizeof(uint32_t));
            node = node->next;
        }
    }
}

void CustomIndexer::save_forward_index(const char* path, uint32_t id, const char* title, const char* url) {
    std::ofstream f_out(path, std::ios::binary | std::ios::app);
    if (!f_out.is_open()) return;

    uint32_t t_len = static_cast<uint32_t>(strlen(title));
    uint32_t u_len = static_cast<uint32_t>(strlen(url));

    f_out.write(reinterpret_cast<const char*>(&id), sizeof(uint32_t));
    f_out.write(reinterpret_cast<const char*>(&t_len), sizeof(uint32_t));
    f_out.write(title, t_len);
    f_out.write(reinterpret_cast<const char*>(&u_len), sizeof(uint32_t));
    f_out.write(url, u_len);
}