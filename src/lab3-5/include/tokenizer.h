#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <cstring>
#include <ctime>
#include <cctype>

struct TokenStats {
    size_t total_tokens;
    size_t total_chars;
    double duration;
    double speed_kb_per_sec;
};

bool is_separator(const char* str, size_t i, size_t len) {
    unsigned char c = (unsigned char)str[i];

    // 1. Стандартные пробельные символы и управляющие символы ASCII
    if (c <= 32) return true;

    // 2. Расширенный список пунктуации (добавлены слеши, скобки, мат. знаки)
    // Эти символы ВСЕГДА разделяют слова
    if (strchr(",!?()\":;[]{}<>\\/|*+=%^$#@~`", c)) return true;

    // 3. Обработка точки (сложный случай)
    if (c == '.') {
        // Не разделитель, если это число (например, 3.14)
        if (i > 0 && i < len - 1 && isdigit((unsigned char)str[i-1]) && isdigit((unsigned char)str[i+1])) {
            return false; 
        }
        // Не разделитель, если это инициал (например, И. Иванов — работает для ASCII)
        if (i > 0 && isupper((unsigned char)str[i-1])) {
            return false;
        }
        return true;
    }

    // 4. Обработка дефиса
    if (c == '-') {
        // Не разделитель, если стоит внутри слова (сине-зеленый) или внутри числа
        if (i > 0 && i < len - 1) {
            unsigned char prev = (unsigned char)str[i-1];
            unsigned char next = (unsigned char)str[i+1];
            if (isalnum(prev) && isalnum(next)) return false;
        }
        return true;
    }

    // 5. Обработка апострофа (например, it's или don't)
    if (c == '\'') {
        if (i > 0 && i < len - 1 && isalpha((unsigned char)str[i-1]) && isalpha((unsigned char)str[i+1])) {
            return false;
        }
        return true;
    }

    return false;
}

TokenStats tokenize_and_analyze(const char* source) {
    size_t src_len = strlen(source);
    char* buffer = new char[src_len + 1];
    size_t b_idx = 0;
    bool in_tag = false;
    
    TokenStats stats = {0, 0, 0.0, 0.0};
    clock_t start = clock();

    for (size_t i = 0; i < src_len; ++i) {
        char c = source[i];

        // Пропуск HTML тегов
        if (c == '<') { in_tag = true; continue; }
        if (c == '>') { in_tag = false; continue; }
        if (in_tag) continue;

        if (is_separator(source, i, src_len)) {
            if (b_idx > 0) {
                buffer[b_idx] = '\0';
                stats.total_tokens++;
                stats.total_chars += b_idx;
                b_idx = 0;
            }
        } else {
            buffer[b_idx++] = (char)tolower((unsigned char)c);
        }
    }

    // Обработка последнего токена
    if (b_idx > 0) {
        stats.total_tokens++;
        stats.total_chars += b_idx;
    }

    clock_t end = clock();
    stats.duration = (double)(end - start) / CLOCKS_PER_SEC;

    delete[] buffer;
    return stats;
}

#endif