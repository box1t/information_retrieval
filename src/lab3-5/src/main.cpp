#include <iostream>
#include <cstdlib> 
#include <cstring>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/json.hpp>
#include "tokenizer.h"
#include "indexer.h"

int main() {
    CustomIndexer indexer; 
    try {
        mongocxx::instance instance{}; 

        const char* user = std::getenv("MONGO_USER");
        const char* password = std::getenv("MONGO_PASS");
        
        char uri_buf[256];
        if (user && password) {
            snprintf(uri_buf, sizeof(uri_buf), "mongodb://%s:%s@localhost:27017/", user, password);
        } else {
            strncpy(uri_buf, "mongodb://localhost:27017/", sizeof(uri_buf));
        }

        mongocxx::uri uri{uri_buf};
        mongocxx::client client{uri};

        const char* db_env = std::getenv("MONGO_DB");
        const char* db_name = db_env ? db_env : "admin"; 
        auto db = client[db_name]; 
        auto collection = db["documents"];

        size_t total_docs = 0;
        size_t total_bytes = 0; 
        TokenStats global_stats = {0, 0, 0.0};

        std::cout << "--- Обработка документов (C-STYLE) ---" << std::endl;

        auto cursor = collection.find({});
        
        for (auto&& doc : cursor) {
            auto html_elem = doc["html"];
            if (html_elem && html_elem.type() == bsoncxx::type::k_string) {
                
                const char* title = "No Title";
                const char* url = "No URL";
                
                if (doc["title"]) title = doc["title"].get_string().value.data();
                if (doc["url"])   url = doc["url"].get_string().value.data();

                indexer.save_forward_index("forward.bin", (uint32_t)total_docs, title, url);

                auto sv = html_elem.get_string().value;
                const char* content_ptr = sv.data();
                total_bytes += sv.size();

                TokenStats doc_stats = tokenize_and_analyze(content_ptr, indexer, (uint32_t)total_docs);
                
                char* text_copy = (char*)malloc(sv.size() + 1);
                memcpy(text_copy, content_ptr, sv.size());
                text_copy[sv.size()] = '\0';

                char* token = strtok(text_copy, " \n\t\r.,!?;:()[]\"'");
                while (token != nullptr) {
                    for(int i = 0; token[i]; i++) {
                        if(token[i] >= 'A' && token[i] <= 'Z') token[i] += 32;
                    }
                    
                    indexer.add_term(token, (uint32_t)total_docs);
                    token = strtok(nullptr, " \n\t\r.,!?;:()[]\"'");
                }
                free(text_copy);

                global_stats.total_tokens += doc_stats.total_tokens;
                global_stats.total_chars += doc_stats.total_chars;
                global_stats.duration += doc_stats.duration;
                total_docs++;
            }
        }
        
        indexer.save_to_file("dict.bin", "postings.bin");

        std::cout << "\n\n--- --- Индексация завершена. Файлы 'dict.bin', 'postings.bin', 'forward.bin' созданы в директории 'build'. --- ---" << std::endl;

        std::cout << "---------------------------------------" << std::endl;
        std::cout << "Документов обработано: " << total_docs << std::endl;
        std::cout << "Всего токенов (стем):  " << global_stats.total_tokens << std::endl;

        if (global_stats.total_tokens > 0) {
            std::cout << "Средняя длина стема:   " << (double)global_stats.total_chars / global_stats.total_tokens << std::endl;
        }
        std::cout << "Объем данных:          " << total_bytes / 1024.0 << " КБ" << std::endl;
        double total_kb = total_bytes / 1024.0;
        std::cout << "Время (чистое):        " << global_stats.duration << " сек." << std::endl;
        if (global_stats.duration > 0) {
            std::cout << "Скорость токенизации:  " << total_kb / global_stats.duration << " КБ/сек" << std::endl;
        }


    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }

    return 0;
}
