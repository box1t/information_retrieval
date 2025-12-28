cmake_minimum_required(VERSION 3.10)
project(InformationRetrieval)

set(CMAKE_CXX_STANDARD 17)

# Поиск библиотек MongoDB
find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)

include_directories(include)
include_directories(${MONGOCXX_INCLUDE_DIRS} ${BSONCXX_INCLUDE_DIRS})

# 1. Основная программа индексации
add_executable(tokenizer_lab 
    src/main.cpp
    src/indexer.cpp
)

target_link_libraries(tokenizer_lab 
    PRIVATE 
    mongo::mongocxx_shared 
    mongo::bsoncxx_shared
    stemmer  # Убедитесь, что библиотека stemmer видна в системе
)

# 2. Утилита поиска (CLI)
add_executable(search_cli 
    src/cli.cpp 
    src/search.cpp
    src/indexer.cpp # Добавляем indexer.cpp, так как поиску нужны методы загрузки индекса
)

target_link_libraries(search_cli 
    PRIVATE 
    mongo::mongocxx_shared 
    mongo::bsoncxx_shared
    stemmer
)

# Указываем папки с хедерами для обеих целей
target_include_directories(tokenizer_lab PRIVATE include)
target_include_directories(search_cli PRIVATE include)