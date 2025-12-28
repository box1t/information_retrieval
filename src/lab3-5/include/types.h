#ifndef TYPES_H
#define TYPES_H

#include <cstring>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <cstring>

class IntArray {
    uint32_t* data;
    size_t size;
    size_t capacity;

public:
    IntArray() : data(nullptr), size(0), capacity(0) {}
    ~IntArray() { delete[] data; }

    void push_back(uint32_t value) {
        if (size == capacity) {
            capacity = (capacity == 0) ? 4 : capacity * 2;
            uint32_t* new_data = new uint32_t[capacity];
            if (data) {
                memcpy(new_data, data, size * sizeof(uint32_t));
                delete[] data;
            }
            data = new_data;
        }
        data[size++] = value;
    }

    uint32_t* get_data() const { return data; }
    size_t get_size() const { return size; }
};

struct DictNode {
    char* term;
    IntArray* doc_ids;
    DictNode* next;
    uint64_t file_offset = 0; // Смещение в postings.bin
    uint32_t doc_freq = 0;
    
    DictNode(const char* t) {
        term = new char[std::strlen(t) + 1];
        std::strcpy(term, t);
        
        doc_ids = new IntArray();
        next = nullptr;
    }

    ~DictNode() {
        delete[] term;
        delete doc_ids;
    }
};


#endif