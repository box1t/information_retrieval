#include <iostream>
#include <cstdlib> // для getenv
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/json.hpp>
#include "tokenizer.h"

int main() {
    try {
        mongocxx::instance instance{}; 


        const char* user = std::getenv("MONGO_USER");
        const char* password = std::getenv("MONGO_PASS");
        
        // Если переменные окружения не подхватились (например, вы вне .venv),
        // можно временно вписать их строками:
        // std::string uri_str = "mongodb://admin:password123@localhost:27017/";
        
        std::string uri_str = "mongodb://";
        if (user && password) {
            uri_str += std::string(user) + ":" + std::string(password) + "@";
        }
        uri_str += "localhost:27017/"; // Хост и порт из конфига Лаб 2

        mongocxx::uri uri{uri_str};
        mongocxx::client client{uri};;

        // Укажите ваше имя БД из ЛР-2
        const char* db_env = std::getenv("MONGO_DB");
        std::string db_name = db_env ? db_env : "admin"; // если env пустой, пробуем admin
        auto db = client[db_name]; 
        auto collection = db["documents"];

        size_t total_docs = 0;
        TokenStats global_stats = {0, 0, 0.0, 0.0};

        std::cout << "--- Обработка документов из MongoDB ---" << std::endl;

        auto cursor = collection.find({});
        
        for (auto&& doc : cursor) {
            // Исправленное извлечение поля html
            auto html_elem = doc["html"];
            if (html_elem && html_elem.type() == bsoncxx::type::k_string) {

                auto sv = html_elem.get_string().value;
                std::string content(sv.data(), sv.size());

                
                TokenStats doc_stats = tokenize_and_analyze(content.c_str());

                global_stats.total_tokens += doc_stats.total_tokens;
                global_stats.total_chars += doc_stats.total_chars;
                global_stats.duration += doc_stats.duration;
                total_docs++;
            }
        }

        std::cout << "---------------------------------------" << std::endl;
        std::cout << "Документов обработано: " << total_docs << std::endl;
        std::cout << "Всего токенов:         " << global_stats.total_tokens << std::endl;
        if (global_stats.total_tokens > 0) {
            std::cout << "Средняя длина токена:  " << (double)global_stats.total_chars / global_stats.total_tokens << std::endl;
        }
        std::cout << "Время токенизации:     " << global_stats.duration << " сек." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }

    return 0;
}