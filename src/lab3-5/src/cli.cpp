#include <iostream>
#include <fstream>
#include <cstring>
#include "indexer.h"
#include "search.h"

void print_results(IntArray* results, const char* forward_path) {
    if (!results || results->get_size() == 0) {
        std::cout << "Ничего не найдено." << std::endl;
        return;
    }

    std::cout << "Найдено документов: " << results->get_size() << std::endl;
    
    uint32_t* data = results->get_data();
    for (size_t i = 0; i < results->get_size() && i < 50; ++i) {
        std::cout << " [DocID: " << data[i] << "]" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Использование: ./search_cli <input_queries.txt>" << std::endl;
        return 1;
    }

    CustomIndexer indexer;
    indexer.load_from_file("dict.bin", "postings.bin"); 
        uint32_t total_docs = 1000; 

    SearchEngine engine;
    std::ifstream infile(argv[1]);
    char line[1024];

    while (infile.getline(line, sizeof(line))) {
        if (strlen(line) < 1) continue;

        std::cout << "\nЗапрос: [" << line << "]" << std::endl;
        
        IntArray* results = engine.execute_query(line, indexer, total_docs);
        print_results(results, "forward.bin");
        
        delete results;
    }

    return 0;
}