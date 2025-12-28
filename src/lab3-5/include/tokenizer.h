#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <cstring>
#include <ctime>
#include <cctype>
#include <libstemmer.h>
#include <string>
#include <vector>

class CustomIndexer;

struct TokenStats {
    size_t total_tokens;
    size_t total_chars;
    double duration;
};

bool is_separator(const char* str, size_t i, size_t len) {
    unsigned char c = (unsigned char)str[i];
    if (c <= 32) return true;
    if (strchr(",!?()\":;[]{}<>\\/|*+=%^$#@~`", c)) return true;
    if (c == '.') {
        if (i > 0 && i < len - 1 && isdigit((unsigned char)str[i-1]) && isdigit((unsigned char)str[i+1])) return false;
        if (i > 0 && isupper((unsigned char)str[i-1])) return false;
        return true;
    }
    if (c == '-') {
        if (i > 0 && i < len - 1) {
            unsigned char prev = (unsigned char)str[i-1];
            unsigned char next = (unsigned char)str[i+1];
            if (isalnum(prev) && isalnum(next)) return false;
        }
        return true;
    }
    if (c == '\'') {
        if (i > 0 && i < len - 1 && isalpha((unsigned char)str[i-1]) && isalpha((unsigned char)str[i+1])) return false;
        return true;
    }
    return false;
}

TokenStats tokenize_and_analyze(const char* html_content, CustomIndexer& indexer, uint32_t doc_id) {
    size_t src_len = strlen(html_content);
    char* buffer = new char[src_len + 1];
    size_t b_idx = 0;
    bool in_tag = false;
    
    TokenStats stats = {0, 0, 0.0};
    clock_t start = clock();

    sb_stemmer* stemmer_ru = sb_stemmer_new("russian", "UTF_8");
    sb_stemmer* stemmer_en = sb_stemmer_new("english", "UTF_8");

    for (size_t i = 0; i < src_len; ++i) {
        char c = html_content[i];

        if (c == '<') { in_tag = true; continue; }
        if (c == '>') { in_tag = false; continue; }
        if (in_tag) continue;

        if (is_separator(html_content, i, src_len)) {
            if (b_idx > 0) {
                buffer[b_idx] = '\0';
                
                const sb_symbol* stemmed;
                bool has_cyrillic = false;
                for(size_t j=0; j<b_idx; ++j) {
                    if((unsigned char)buffer[j] > 127) { has_cyrillic = true; break; }
                }

                if (has_cyrillic) {
                    stemmed = sb_stemmer_stem(stemmer_ru, (unsigned char*)buffer, b_idx);
                } else {
                    stemmed = sb_stemmer_stem(stemmer_en, (unsigned char*)buffer, b_idx);
                }

                stats.total_tokens++;
                stats.total_chars += strlen((const char*)stemmed);
                
                b_idx = 0;
            }
        } else {
            buffer[b_idx++] = (char)tolower((unsigned char)c);
        }
    }

    if (b_idx > 0) {
        buffer[b_idx] = '\0';
        const sb_symbol* stemmed = sb_stemmer_stem(stemmer_en, (unsigned char*)buffer, b_idx);
        stats.total_tokens++;
        stats.total_chars += strlen((const char*)stemmed);
    }

    clock_t end = clock();
    stats.duration = (double)(end - start) / CLOCKS_PER_SEC;

    sb_stemmer_delete(stemmer_ru);
    sb_stemmer_delete(stemmer_en);
    delete[] buffer;
    return stats;
}

#endif