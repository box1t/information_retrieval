
cmake_minimum_required(VERSION 3.10)
project(InformationRetrieval)

set(CMAKE_CXX_STANDARD 17)

# Поиск библиотек mongocxx
find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)

include_directories(include)
include_directories(${MONGOCXX_INCLUDE_DIRS} ${BSONCXX_INCLUDE_DIRS})

add_executable(tokenizer_lab 
    src/main.cpp
    src/indexer.cpp
)

target_link_libraries(tokenizer_lab 
    PRIVATE 
    mongo::mongocxx_shared 
    mongo::bsoncxx_shared
    stemmer
)