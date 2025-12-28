#include <iostream>
#include <cstdlib> 
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
        
        std::string uri_str = "mongodb://";
        if (user && password) {
            uri_str += std::string(user) + ":" + std::string(password) + "@";
        }
        uri_str += "localhost:27017/"; 

        mongocxx::uri uri{uri_str};
        mongocxx::client client{uri};

        const char* db_env = std::getenv("MONGO_DB");
        std::string db_name = db_env ? db_env : "admin"; 
        auto db = client[db_name]; 
        auto collection = db["documents"];

        size_t total_docs = 0;
        size_t total_bytes = 0; 
        TokenStats global_stats = {0, 0, 0.0};

        std::cout << "--- Обработка документов со СТЕММИНГОМ ---" << std::endl;

        auto cursor = collection.find({});
        
        for (auto&& doc : cursor) {
            auto html_elem = doc["html"];
            if (html_elem && html_elem.type() == bsoncxx::type::k_string) {
                auto sv = html_elem.get_string().value;
                total_bytes += sv.size();
                
                std::string content(sv.data(), sv.size());

                TokenStats doc_stats = tokenize_and_analyze(content.c_str(), indexer, (uint32_t)total_docs);
                
                char* text = strdup(content.c_str()); // создаем копию для strtok
                char* token = strtok(text, " \n\t\r.,!?;:()[]\"'"); // разделители
                
                while (token != nullptr) {
                    for(int i = 0; token[i]; i++) token[i] = tolower(token[i]);
                    
                    // добавление в индекс. total_docs — это DocID
                    indexer.add_term(token, (uint32_t)total_docs);
                    
                    token = strtok(nullptr, " \n\t\r.,!?;:()[]\"'");
                }
                free(text);


                global_stats.total_tokens += doc_stats.total_tokens;
                global_stats.total_chars += doc_stats.total_chars;
                global_stats.duration += doc_stats.duration;
                total_docs++;
            }
        }
        indexer.save_to_file("dict.bin", "postings.bin");
        std::cout << "\n\n--- --- Индексация завершена. Файлы 'dict.bin', 'postings.bin' созданы в директории 'build'. --- ---" << std::endl;

        std::cout << "---------------------------------------" << std::endl;
        std::cout << "Документов обработано: " << total_docs << std::endl;
        std::cout << "Всего токенов (стем):  " << global_stats.total_tokens << std::endl;
        
        if (global_stats.total_tokens > 0) {
            std::cout << "Средняя длина стема:   " << (double)global_stats.total_chars / global_stats.total_tokens << std::endl;
        }

        double total_kb = total_bytes / 1024.0;
        std::cout << "Объем данных:          " << total_kb << " КБ" << std::endl;
        std::cout << "Время (чистое):        " << global_stats.duration << " сек." << std::endl;
        
        if (global_stats.duration > 0) {
            std::cout << "Скорость токенизации:  " << total_kb / global_stats.duration << " КБ/сек" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }

    return 0;
}