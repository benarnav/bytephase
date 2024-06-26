#ifndef _BPE_H
#define _BPE_H

#include <Python.h>

#define BIGRAM_TABLE_SIZE 1048576 // 2^20
#define MAX_CHILDREN 256

typedef struct text_chunk_node {
    unsigned short count;
    size_t array_size;
    size_t num_elements;
    unsigned short bytes[];
} text_chunk_node_t;

typedef struct bigram_node {
    unsigned short bigram[2];
    int freq;
    struct bigram_node* next;
} bigram_node_t;

typedef struct token_node {
    unsigned short token[2];
} token_node_t;

typedef struct trie_node {
    struct trie_node* children[MAX_CHILDREN];
    int token_id;
} trie_node;

typedef struct Trie {
    trie_node* root;
} Trie;

// Function prototypes
void print_bytes(text_chunk_node_t* node);
unsigned long hash_text(unsigned short unigram1, unsigned short unigram2);
void dfs(unsigned short* token, unsigned short index, token_node_t** token_table, int* result_count, unsigned short token_idx_end, size_t max_size);
token_node_t* create_token(bigram_node_t** max_node);
text_chunk_node_t* create_text_chunk_node(unsigned short* word, size_t size, unsigned short count);
bigram_node_t* create_bigram_node(unsigned short unigram1, unsigned short unigram2, int freq);
void update_text_table(text_chunk_node_t** text_table, unsigned short* word, size_t size, unsigned short count, unsigned int* text_table_idx);
void update_bigram_table(unsigned short unigram1, unsigned short unigram2, int count, bigram_node_t **bigram_table);
unsigned short* word_to_ints(char* word);
void init_stats(text_chunk_node_t* text_node, bigram_node_t **bigram_table);
bigram_node_t** build_bigram_table(text_chunk_node_t** text_table, unsigned int text_table_len);
void word_retokenize(text_chunk_node_t* text_chunk_node, bigram_node_t **max_node, bigram_node_t** bigram_table, unsigned short token_idx);
void retokenize(int text_table_len, text_chunk_node_t** text_table, bigram_node_t** max_node,  bigram_node_t** bigram_table, unsigned short token_idx);
void update_max_node(bigram_node_t** max_node, bigram_node_t** bigram_table, token_node_t** token_table, unsigned short token_idx);
void free_bigram_table(bigram_node_t** bigram_table);
size_t get_array_size(unsigned short* array);
void cleanup_train_resources(text_chunk_node_t **text_table, int text_table_len, bigram_node_t **bigram_table, token_node_t **token_table, int token_idx_start, int token_idx_end, unsigned short *token);

// Trie functions
trie_node* create_node();
Trie* create_trie();
void insert_trie(Trie* trie, unsigned char* token, int token_length, int token_id);
int search_trie(Trie* trie, unsigned char* text, int text_length, int* match_length);
void free_trie_node(trie_node* node);
void free_trie(Trie* trie);

// Python C API functions
static void trie_capsule_destructor(PyObject *capsule);
static PyObject* train(PyObject* self, PyObject* args);
static PyObject* build_trie(PyObject* self, PyObject* args);
static PyObject* encode_train(PyObject* self, PyObject* args);
static PyObject* encode_inference(PyObject* self, PyObject* args);


#endif