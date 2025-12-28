#include "search.h"
#include <cstring>
#include <cctype>

IntArray* SearchEngine::execute_query(const char* query, CustomIndexer& indexer, uint32_t total_docs) {
    QueryToken* rpn = new QueryToken[256]; 
    int rpn_count = 0;
    memset(rpn, 0, sizeof(QueryToken) * 256);
    shunting_yard(query, rpn, rpn_count);
    IntArray** eval_stack = new IntArray*[256];
    int top = -1;

    for (int i = 0; i < rpn_count; ++i) {
        if (rpn[i].type == TOKEN) {
            IntArray* p = indexer.get_postings(rpn[i].word);
            std::cout << "  DEBUG: слово '" << rpn[i].word << "' найдено в " 
                  << (p ? p->get_size() : 0) << " док." << std::endl;
            eval_stack[++top] = p ? p : new IntArray();
        } 
        else if (rpn[i].type == AND || rpn[i].type == OR) {
            if (top < 1) continue;
            IntArray* b = eval_stack[top--];
            IntArray* a = eval_stack[top--];
            eval_stack[++top] = (rpn[i].type == AND) ? intersect(a, b) : merge_union(a, b);
            delete a; delete b;
        } 
        else if (rpn[i].type == NOT) {
            if (top < 0) continue;
            IntArray* a = eval_stack[top--];
            eval_stack[++top] = invert(a, total_docs);
            delete a;
        }
    }

    IntArray* final_result = (top >= 0) ? eval_stack[top--] : new IntArray();
    while (top >= 0) delete eval_stack[top--];
    
    delete[] rpn;
    delete[] eval_stack;

    return final_result;
}

IntArray* SearchEngine::intersect(IntArray* a, IntArray* b) {
    IntArray* res = new IntArray();
    uint32_t* data_a = a->get_data();
    uint32_t* data_b = b->get_data();
    size_t i = 0, j = 0;

    while (i < a->get_size() && j < b->get_size()) {
        if (data_a[i] == data_b[j]) {
            res->push_back(data_a[i]);
            i++; j++;
        } else if (data_a[i] < data_b[j]) {
            i++;
        } else {
            j++;
        }
    }
    return res;
}

IntArray* SearchEngine::merge_union(IntArray* a, IntArray* b) {
    IntArray* res = new IntArray();
    uint32_t* data_a = a->get_data();
    uint32_t* data_b = b->get_data();
    size_t i = 0, j = 0;

    while (i < a->get_size() || j < b->get_size()) {
        if (i < a->get_size() && (j >= b->get_size() || data_a[i] < data_b[j])) {
            res->push_back(data_a[i++]);
        } else if (j < b->get_size() && (i >= a->get_size() || data_b[j] < data_a[i])) {
            res->push_back(data_b[j++]);
        } else {
            res->push_back(data_a[i++]);
            j++;
        }
    }
    return res;
}

IntArray* SearchEngine::invert(IntArray* a, uint32_t total_docs) {
    IntArray* res = new IntArray();
    uint32_t* data_a = a->get_data();
    size_t a_idx = 0;
    size_t a_size = a->get_size();

    for (uint32_t i = 0; i < total_docs; ++i) {
        if (a_idx < a_size && data_a[a_idx] == i) {
            a_idx++;
        } else {
            res->push_back(i);
        }
    }
    return res;
}


void SearchEngine::shunting_yard(const char* query, QueryToken* output, int& count) {
    QueryToken op_stack[64]; 
    int top = -1;
    count = 0;

    int n = strlen(query);
    for (int i = 0; i < n; i++) {
        if (isspace(query[i])) continue;

        if (count >= 60) break; 

        if (query[i] == '(') {
            if (top < 60) {
                op_stack[++top].type = L_BRACKET;
            }
        } else if (query[i] == ')') {
            while (top >= 0 && op_stack[top].type != L_BRACKET) {
                output[count++] = op_stack[top--];
            }
            if (top >= 0) top--; 
        } else if (query[i] == '!' || (query[i] == '&' && query[i+1] == '&') || (query[i] == '|' && query[i+1] == '|')) {
            OpType current_op;
            if (query[i] == '!') current_op = NOT;
            else if (query[i] == '&') { current_op = AND; i++; }
            else { current_op = OR; i++; }

            while (top >= 0 && op_stack[top].type != L_BRACKET && 
                   get_priority(op_stack[top].type) >= get_priority(current_op)) {
                output[count++] = op_stack[top--];
            }
            if (top < 60) {
                op_stack[++top].type = current_op;
            }
        } else {
            QueryToken t;
            t.type = TOKEN;
            int k = 0;
            while (i < n && !isspace(query[i]) && query[i] != '(' && query[i] != ')' && 
                   query[i] != '&' && query[i] != '|' && query[i] != '!') {
                if (k < 63) { // Защита от Stack Smashing (размер word[64])
                    t.word[k++] = tolower(query[i]);
                }
                i++;
            }
            t.word[k] = '\0';
            i--; 
            
            if (k > 0) {
                output[count++] = t;
            }
        }
    }

    while (top >= 0) {
        if (op_stack[top].type != L_BRACKET) {
            output[count++] = op_stack[top--];
        } else {
            top--;
        }
    }
}

int SearchEngine::get_priority(OpType type) {
    if (type == NOT) return 3;
    if (type == AND) return 2;
    if (type == OR) return 1;
    return 0;
}