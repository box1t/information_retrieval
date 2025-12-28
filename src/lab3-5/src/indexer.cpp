#include "indexer.h"
#include <fstream>
#include <cstring>

CustomIndexer::CustomIndexer() : forward_size(0), forward_capacity(10), total_unique_terms(0) {
    for (size_t i = 0; i < HASH_SIZE; ++i) table[i] = nullptr;
    forward_index = new DocEntry[forward_capacity];
}

CustomIndexer::~CustomIndexer() {
    for (size_t i = 0; i < HASH_SIZE; ++i) {
        DictNode* current = table[i];
        while (current != nullptr) {
            DictNode* next = current->next;
            delete current;
            current = next;
        }
    }
}

void CustomIndexer::add_term_with_offset(const char* term, uint64_t offset, uint32_t df) {
    size_t idx = hash_func(term);
    
    DictNode* newNode = new DictNode(term);
    
    newNode->file_offset = offset;
    newNode->doc_freq = df;
    
    newNode->next = table[idx];
    table[idx] = newNode;
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

void CustomIndexer::load_from_file(const char* d_path, const char* p_path) {
    std::ifstream d_in(d_path, std::ios::binary);
    if (!d_in) {
        std::cerr << "Ошибка: Не удалось открыть " << d_path << std::endl;
        return;
    }

    strncpy(this->postings_file_path, p_path, 255);
    this->postings_file_path[255] = '\0';

    uint32_t magic = 0, count = 0;
    d_in.read((char*)&magic, sizeof(uint32_t));
    d_in.read((char*)&count, sizeof(uint32_t));

    if (count > 1000000) { 
        return;
    }

    for (uint32_t i = 0; i < count; ++i) {
        uint16_t t_len = 0;
        if (!d_in.read((char*)&t_len, sizeof(uint16_t))) break;

        char term_buf[512]; 
        if (t_len >= 512) {
            d_in.seekg(t_len, std::ios::cur);
            d_in.seekg(sizeof(uint64_t) + sizeof(uint32_t), std::ios::cur);
            continue;
        }

        d_in.read(term_buf, t_len);
        term_buf[t_len] = '\0';

        uint64_t offset;
        uint32_t df;
        d_in.read((char*)&offset, sizeof(uint64_t));
        d_in.read((char*)&df, sizeof(uint32_t));

        add_term_with_offset(term_buf, offset, df);
    }
}

DictNode* CustomIndexer::find_node(const char* term) {
    size_t idx = hash_func(term);
    DictNode* curr = table[idx];
    while (curr) {
        if (strcmp(curr->term, term) == 0) return curr;
        curr = curr->next;
    }
    return nullptr;
}

IntArray* CustomIndexer::get_postings(const char* term) {
    DictNode* node = find_node(term);
    if (!node) return new IntArray(); 

    IntArray* res = new IntArray();
    std::ifstream p_in(this->postings_file_path, std::ios::binary);
    if (!p_in) return res;

    p_in.seekg(node->file_offset);
    
    for (uint32_t i = 0; i < node->doc_freq; ++i) {
        uint32_t id;
        p_in.read((char*)&id, sizeof(uint32_t));
        res->push_back(id);
    }
    return res;
}