#define PY_SSIZE_T_CLEAN
#include "_bpe.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_bytes(text_chunk_node_t* node)
{
    for(int i = 0; i < (node->array_size/2)+1;i++)
    {
        printf("%hu ", node->bytes[i]);
        if (node->bytes[i] == 0){
            break;
        }
    }
    printf("\n");
}

unsigned long hash_text(unsigned short unigram1, unsigned short unigram2)
{
    unsigned long hash = 5381;
    hash = ((hash << 5) + hash) + unigram1; /* hash * 33 + c */
    hash = ((hash << 5) + hash) + unigram2; /* hash * 33 + c */

    return hash % BIGRAM_TABLE_SIZE;
}

void dfs(unsigned short* token, unsigned short index, token_node_t** token_table, int* result_count, unsigned short token_idx_end, size_t max_size) 
{
    if (index > token_idx_end || token_table[index] == NULL || *result_count >= max_size) 
    {
        fprintf(stderr, "ERROR IN DFS");
        return;
    }

    token_node_t* node = token_table[index];

    for (int i = 0; i < 2; i++) 
    {
        if (node->token[i] < 256) 
        {
           token[(*result_count)++] = node->token[i];
        } 
        else 
        {
            dfs(token, node->token[i], token_table, result_count, token_idx_end, max_size);
        }
    }
}

token_node_t* create_token(bigram_node_t** max_node)
{
    token_node_t* new_node = malloc(sizeof(token_node_t));
    if (new_node == NULL)
    {
        fprintf(stderr, "Memory allocation failed for token node\n");
    }    
    new_node->token[0] = (*max_node)->bigram[0];
    new_node->token[1] = (*max_node)->bigram[1];
    
    return new_node;
}

text_chunk_node_t* create_text_chunk_node(unsigned short* word, size_t size, unsigned short count)
{   
    unsigned short i;
    unsigned short loc = 0;
    text_chunk_node_t* node = malloc(sizeof(text_chunk_node_t) + size);
    if (node == NULL)
    {
        fprintf(stderr, "Memory allocation failed for text chunk node\n");
    }    
    memset(node, 0, sizeof(text_chunk_node_t) + size);
    while ((i = *word++)!=0)
    { 
        node->bytes[loc] = i;
        loc++;
    }
    node -> array_size =  size;
    node -> num_elements = size / sizeof(unsigned short);
    node -> count = count;
    return node;
}

bigram_node_t* create_bigram_node(unsigned short unigram1, unsigned short unigram2, int freq)
{
    bigram_node_t* node = malloc(sizeof(bigram_node_t));
    if (node == NULL)
    {
        fprintf(stderr, "Memory allocation failed for bigram node\n");
    } 
    node->bigram[0] = unigram1;
    node->bigram[1] = unigram2;
    node -> next = NULL;
    node -> freq = freq;

    return node;
}

void update_text_table (text_chunk_node_t** text_table, unsigned short* word, size_t size, unsigned short count, unsigned int* text_table_idx)
{
    text_chunk_node_t* new_node = NULL;
    new_node = create_text_chunk_node(word, size, count);
    text_table[*text_table_idx] = new_node;
    (*text_table_idx)++;
}

void update_bigram_table (unsigned short unigram1, unsigned short unigram2, int count, bigram_node_t **bigram_table)
{
    int hash = hash_text(unigram1, unigram2);
    if (bigram_table[hash] != NULL){
    }
    bigram_node_t* check;
    bigram_node_t* new_node = NULL;
    int comparison;
    if ((check = bigram_table[hash]))
    {
        comparison = (check -> bigram[0] == unigram1 && check->bigram[1] == unigram2);
        while ((comparison != 1) && (check -> next))
        {
            check = check -> next;
            comparison = (check -> bigram[0] == unigram1 && check->bigram[1] == unigram2);

        }
        if (comparison == 1)
        {
            check -> freq += count;
        }
        else
        {
            new_node = create_bigram_node(unigram1, unigram2, count);
            check -> next = new_node;
        }
    }
    else
    {
        new_node = create_bigram_node(unigram1, unigram2, count);
        bigram_table[hash] = new_node;
    }
}

unsigned short* word_to_ints(char* word)
{
    unsigned short* int_word = (unsigned short*)malloc((strlen(word)+1) * sizeof(unsigned short));
    unsigned short current_loc = 0;
    unsigned char* current_char = (unsigned char*)word;  

    for (int i = 0;i < strlen(word); i++)
    {
        int_word[i] = (unsigned short)*current_char;
        current_char++;
        current_loc++;
    }
    int_word[current_loc] = 0;
    return int_word;
}

void init_stats(text_chunk_node_t* text_node, bigram_node_t **bigram_table)
{
    unsigned short* unigram_1 = text_node -> bytes;
    unsigned short* unigram_2 = unigram_1 + 1;
    unsigned short element = 0;
    while (element < text_node->num_elements)
    {   
        if (*unigram_1 != 0 && *unigram_2 != 0)
        {
            update_bigram_table(*unigram_1, *unigram_2, text_node->count, bigram_table);
        }
        unigram_1++;
        unigram_2++;
        element++;
    }
}

bigram_node_t** build_bigram_table(text_chunk_node_t** text_table, unsigned int text_table_len)
{
    bigram_node_t** bigram_table = (bigram_node_t**)malloc(BIGRAM_TABLE_SIZE * sizeof(bigram_node_t*));
    memset(bigram_table, 0, BIGRAM_TABLE_SIZE * sizeof(bigram_node_t*));
    for (int i=0; i < text_table_len; i++)
    {
        init_stats(text_table[i], bigram_table);
    }
    return bigram_table;
}

void word_retokenize(text_chunk_node_t* text_chunk_node, bigram_node_t **max_node, bigram_node_t** bigram_table, unsigned short token_idx)
{
    unsigned short* unigram_L = text_chunk_node -> bytes;
    unsigned short* unigram_R = NULL;
    
    unsigned short* bytes_start = text_chunk_node -> bytes;

    unsigned short* unigram_1 = text_chunk_node -> bytes;
    unsigned short* unigram_2 = unigram_1 + 1;
    unsigned short element = 1;
    size_t remaining_bytes;

    while (element < text_chunk_node->num_elements)
    {   
        if (*unigram_1 == (*max_node)->bigram[0] && *unigram_2 == (*max_node)->bigram[1])
        {
            if (unigram_L != unigram_1)
            {
                unigram_L = unigram_1 - 1;
                update_bigram_table(*unigram_L,*unigram_1, -1 * text_chunk_node ->count, bigram_table);
                update_bigram_table(*unigram_L, token_idx, text_chunk_node ->count, bigram_table);
            }
            unigram_R = unigram_2 + 1;
            
            if (*unigram_R != 0)
            {
                update_bigram_table(*unigram_2,*unigram_R, -1 * text_chunk_node ->count, bigram_table);
                update_bigram_table(token_idx, *unigram_R, text_chunk_node ->count, bigram_table);
            }
            *unigram_1 = token_idx;
            remaining_bytes = (text_chunk_node->num_elements - element - 2) * sizeof(unsigned short);

            memmove(unigram_2, unigram_R, remaining_bytes);
            *(unigram_2 + (remaining_bytes / sizeof(unsigned short))) = 0;
            text_chunk_node->array_size -= sizeof(unsigned short);
            text_chunk_node->num_elements--;

        }
        if (unigram_1 != bytes_start)
        {
            unigram_L++;
        }
        element++;
        unigram_1++;
        unigram_2++;
    }
}

void retokenize(int text_table_len, text_chunk_node_t** text_table, bigram_node_t** max_node,  bigram_node_t** bigram_table, unsigned short token_idx)
{
    for (int i = 0; i < text_table_len; i++)
    {
        word_retokenize(text_table[i], max_node, bigram_table, token_idx);
    }
}


void update_max_node(bigram_node_t** max_node, bigram_node_t** bigram_table, token_node_t** token_table, unsigned short token_idx)
{
    (*max_node) -> freq = 0;
    bigram_node_t* check;
    for (int i = 0; i < BIGRAM_TABLE_SIZE; i++)
    {
        check = bigram_table[i];
        while (check != NULL)
        {
            if (check -> freq > 
            (*max_node) -> freq)
            {
                (*max_node) = check;
            }
            check = check -> next;
        }
    }
    token_node_t* new_node = create_token(max_node);
    token_table[token_idx] = new_node;
}

void free_bigram_table(bigram_node_t** bigram_table)
{
    bigram_node_t* current;
    bigram_node_t* next;
    for (int i = 0; i < BIGRAM_TABLE_SIZE; i++)
    {        
        current = bigram_table[i];
        while (current != NULL)
        {
            next = current -> next;
            free(current);
            current = next;
        }
    }
}

size_t get_array_size(unsigned short* array)
{
    size_t size = 0;
    while (array[size] != 0) 
    {
        size++;
    }
    return (1 + size) * sizeof(unsigned short);
}

void cleanup_train_resources(text_chunk_node_t **text_table, int text_table_len, 
                       bigram_node_t **bigram_table, token_node_t **token_table, 
                       int token_idx_start, int token_idx_end, unsigned short *token) 
{
    if (text_table) {
        for (int i = 0; i < text_table_len; i++) {
            free(text_table[i]);
        }
        free(text_table);
    }
    if (bigram_table) {
        free_bigram_table(bigram_table);
        free(bigram_table);
    }
    if (token_table) {
        for (int i = token_idx_start; i < token_idx_end; i++) {
            free(token_table[i]);
        }
        free(token_table);
    }
    free(token);
}

trie_node* create_node() 
{
    trie_node* node = (trie_node*)calloc(1, sizeof(trie_node));
    node->token_id = -1;
    
    return node;
}

Trie* create_trie() 
{
    Trie* trie = (Trie*)malloc(sizeof(Trie));
    trie->root = create_node();
    
    return trie;
}

void insert_trie(Trie* trie, unsigned char* token, int token_length, int token_id) {
    trie_node* current = trie->root;
    for (int i = 0; i < token_length; i++) {
        if (!current->children[token[i]]) {
            current->children[token[i]] = create_node();
            if (!current->children[token[i]]) {
                printf("ERROR LINE 335");
                return;
            }
        }
        current = current->children[token[i]];
    }
    current->token_id = token_id;
}

int search_trie(Trie* trie, unsigned char* text, int text_length, int* match_length) 
{
    trie_node* current = trie->root;
    int last_valid_match = -1;
    *match_length = 0;

    int last_valid_match_length = 0;

    for (int i = 0; i < text_length; i++) 
    {
        if (!current->children[text[i]]) 
        {
            break;
        }
        current = current->children[text[i]];
        (*match_length)++;
        if (current->token_id != -1) 
        {
            last_valid_match = current->token_id;
            last_valid_match_length = *match_length;
        }
    }
    *match_length = last_valid_match_length;

    return last_valid_match;
}

void free_trie_node(trie_node* node) 
{
    if (node == NULL) return;
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (node->children[i]) {
            free_trie_node(node->children[i]);
        }
    }
    free(node);
}

void free_trie(Trie* trie) 
{
    if (trie == NULL) return;
    free_trie_node(trie->root);
    free(trie);
} 

static void trie_capsule_destructor(PyObject *capsule) 
{
    Trie *trie = PyCapsule_GetPointer(capsule, "bpe_trie");
    if (trie) {
        free_trie(trie);
    }
}


static PyObject* train(PyObject* self, PyObject* args) 
{
    PyObject* dict;
    int text_table_len;
    int num_merges;
    PyObject* key;
    PyObject* value;
    Py_ssize_t pos = 0;

    // Parse the input dictionary and integer pass from python call
    if (!PyArg_ParseTuple(args, "O!ii", &PyDict_Type, &dict, &text_table_len, &num_merges)) 
    {
        return NULL;
    }

    text_chunk_node_t **text_table = malloc(sizeof(text_chunk_node_t*) * text_table_len);
    if (text_table == NULL) 
    {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate text table");
        goto error;
    }
    memset(text_table, 0, sizeof(text_chunk_node_t*) * text_table_len);

    token_node_t** token_table = malloc(sizeof(token_node_t*) * (num_merges + 257));
    if (token_table == NULL)
    {
        PyErr_SetString(PyExc_MemoryError,"token table malloc failed");
        goto error;    
    }
    memset(token_table, 0, sizeof(token_node_t*) * (num_merges + 257));
    unsigned short token_idx_start = 256;
    unsigned short token_idx = 256;

    // Iterate through the input dictionary
    unsigned int text_table_idx = 0;
    unsigned int* idx = &text_table_idx;
    unsigned short* byte_word;
    unsigned short size;
    size_t max_size = 0;

    while (PyDict_Next(dict, &pos, &key, &value)) 
    {
        if (PyUnicode_Check(key) && PyLong_Check(value)) 
        {
            const char* key_str = PyUnicode_AsUTF8(key);
            long count = PyLong_AsLong(value);

            byte_word = word_to_ints(key_str);
            size = get_array_size(byte_word);
            if (size > max_size)
            {
                max_size = size;
            }

            update_text_table(text_table, byte_word, size, count, idx);
            free(byte_word);
        }
    }

    bigram_node_t** bigram_table;
    bigram_table = build_bigram_table(text_table, text_table_len);

    bigram_node_t* init_max_node = malloc(sizeof(bigram_node_t));
    init_max_node->freq = 0;
    bigram_node_t* max_node = init_max_node;

    for (int i = 0; i < num_merges; i++)
    {
        update_max_node(&max_node, bigram_table, token_table, token_idx);
        retokenize(text_table_len, text_table,&max_node, bigram_table, token_idx);

        token_idx++;
    }
    free(init_max_node);

    PyObject* token_output = PyList_New(0); 
    unsigned short* token = malloc(max_size);
    int token_idx_end = token_idx_start + num_merges;
    
    for (int i = token_idx_start; i < token_idx_end; i++) 
    {
        if (!token_table[i]) continue;
        memset(token, 0, max_size);
        int result_count = 0;
        dfs(token, i, token_table, &result_count, token_idx_end, max_size);
        PyObject* token_list = PyList_New(0);

        if (!token_list) 
        {
            PyErr_SetString(PyExc_MemoryError, "Failed to create Python list for dfs token\n");
            goto error;
        }

        // Append each unsigned short value from token array to token_list
        for (int j = 0; j < result_count; j++) 
        {
            PyObject* py_value = PyLong_FromUnsignedLong(token[j]);
            if (!py_value) 
            {
                PyErr_SetString(PyExc_MemoryError, "Failed to convert unsigned short to Python object\n");
                Py_DECREF(token_list);
                goto error;
            }
            if (PyList_Append(token_list, py_value) == -1) 
            {
                PyErr_SetString(PyExc_MemoryError, "Failed to append value to result list\n");
                Py_DECREF(token_list);
                Py_DECREF(py_value);
                goto error;
            }
            Py_DECREF(py_value);
        }

        if (PyList_Append(token_output, token_list) == -1) 
        {
            PyErr_SetString(PyExc_MemoryError, "Failed to append result list to token list\n");
            Py_DECREF(token_list);
            goto error;
        }

        Py_DECREF(token_list);
    }

    cleanup_train_resources(text_table, text_table_len, bigram_table, token_table, token_idx_start, token_idx_end, token);
    return token_output;

error:
    cleanup_train_resources(text_table, text_table_len, bigram_table, token_table, token_idx_start, token_idx_end, token);
    Py_XDECREF(token_output);
    return NULL;
}

static PyObject* build_trie(PyObject* self, PyObject* args) {
    PyObject* decode_dict;

    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &decode_dict)) {
        return NULL;
    }

    Trie* trie = create_trie();

    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(decode_dict, &pos, &key, &value)) {
        if (!PyBytes_Check(value) || !PyLong_Check(key)) {
            PyErr_SetString(PyExc_TypeError, "Dictionary must contain integer keys and byte values");
            free_trie(trie);
            return NULL;
        }
    
        char* token;
        Py_ssize_t token_length;
        PyBytes_AsStringAndSize(value, &token, &token_length);

        int token_id = PyLong_AsLong(key);

        insert_trie(trie, (unsigned char*)token, token_length, token_id);
    }

    PyObject* trie_capsule = PyCapsule_New(trie, "bpe_trie",NULL);// (PyCapsule_Destructor)free_trie);
    if (trie_capsule == NULL) {
        free_trie(trie);
        return NULL;
    }

    return trie_capsule;
}

static PyObject* manual_free_trie(PyObject* self, PyObject* args) {
    PyObject* trie_capsule;
    if (!PyArg_ParseTuple(args, "O", &trie_capsule)) {
        return NULL;
    }
    
    if (!PyCapsule_IsValid(trie_capsule, "bpe_trie")) {
        PyErr_SetString(PyExc_ValueError, "Invalid or already freed trie capsule");
        return NULL;
    }

    Trie* trie = PyCapsule_GetPointer(trie_capsule, "bpe_trie");
    if (trie) {
        free_trie(trie);
        // Instead of setting pointer to NULL, clear the destructor
        // This prevents the destructor from being called again
        if (PyCapsule_SetDestructor(trie_capsule, NULL) != 0) {
            PyErr_SetString(PyExc_RuntimeError, "Failed to clear trie capsule destructor");
            return NULL;
        }
    }

    Py_RETURN_NONE;
}

static PyObject* encode_train(PyObject* self, PyObject* args) {
    PyObject* text_iterator;
    PyObject* trie_capsule;
    
    if (!PyArg_ParseTuple(args, "OO", &text_iterator, &trie_capsule)) {
        return NULL;
    }

    if (trie_capsule == Py_None) {
        PyErr_SetString(PyExc_ValueError, "Trie is None. Tokenizer may not have been trained or a encode dict was not loaded.");
        return NULL;
    }

    Trie* trie = (Trie*)PyCapsule_GetPointer(trie_capsule, "bpe_trie");
    if (!trie) {
        PyErr_SetString(PyExc_ValueError, "Invalid trie object");
        return NULL;
    }

    PyObject* iter = PyObject_GetIter(text_iterator);
    if (!iter) {
        PyErr_SetString(PyExc_TypeError, "First argument must be iterable");
        return NULL;
    }

    PyObject* encoded_list = PyList_New(0);
    if (!encoded_list) return NULL;

    PyObject* chunk;
    while ((chunk = PyIter_Next(iter))) {
        PyObject* match_str = PyObject_CallMethod(chunk, "group", NULL);
        Py_DECREF(chunk);
        if (!PyUnicode_Check(match_str)) {
            PyErr_SetString(PyExc_TypeError, "Each chunk must be a string");
            Py_DECREF(match_str);
            Py_DECREF(encoded_list);
            Py_DECREF(iter);
            return NULL;
        }

        Py_ssize_t text_length;
        const char* text = PyUnicode_AsUTF8AndSize(match_str, &text_length);
        if (!text) {
            Py_DECREF(match_str);
            Py_DECREF(iter);
            Py_DECREF(encoded_list);
            return NULL;
        }

        Py_ssize_t i = 0;
        while (i < text_length) {
            int match_length;
            int token_id = search_trie(trie, (unsigned char*)text + i, text_length - i, &match_length);
            PyObject* token_obj;

            if (token_id != -1) {
                token_obj = PyLong_FromLong(token_id);
                i += match_length;
            } else {
                token_obj = PyLong_FromLong((unsigned char)text[i]);
                i++;
            }

            if (!token_obj) {
                Py_DECREF(match_str);
                Py_DECREF(encoded_list);
                Py_DECREF(iter);
                return NULL;
            }

            if (PyList_Append(encoded_list, token_obj) == -1) {
                Py_DECREF(token_obj);
                Py_DECREF(match_str);
                Py_DECREF(encoded_list);
                Py_DECREF(iter);
                return NULL;
            }
            Py_DECREF(token_obj);
        }

        Py_DECREF(match_str);
    }
    Py_DECREF(iter);
    if (PyErr_Occurred()) {
        Py_DECREF(encoded_list);
        return NULL;
    }

    return encoded_list;
}

static PyObject* encode_inference(PyObject* self, PyObject* args) {
    PyObject* input_chunks;
    PyObject* trie_capsule;
    
    if (!PyArg_ParseTuple(args, "OO", &input_chunks, &trie_capsule)) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to parse function args. Must be a list of strings and pointer to trie.");

        return NULL;
    }
    if (trie_capsule == Py_None) {
        PyErr_SetString(PyExc_ValueError, "Trie is None. Tokenizer may not have been trained or a encode dict was not loaded.");
        return NULL;
    }
    Trie* trie = (Trie*)PyCapsule_GetPointer(trie_capsule, "bpe_trie");
    if (!trie) {
        PyErr_SetString(PyExc_ValueError, "Invalid trie object");
        return NULL;
    }
    if (!PyList_Check(input_chunks)) {
        PyErr_SetString(PyExc_TypeError, "Input must be a list of strings");
        return NULL;
    }
    Py_ssize_t num_chunks = PyList_Size(input_chunks);
    PyObject* encoded_list = PyList_New(0);
    for (Py_ssize_t chunk_idx = 0; chunk_idx < num_chunks; chunk_idx++) {
        PyObject* chunk = PyList_GetItem(input_chunks, chunk_idx);
        if (!PyUnicode_Check(chunk)) {
            PyErr_SetString(PyExc_TypeError, "Each chunk must be a string");
            Py_DECREF(encoded_list);
            return NULL;
        }
        const char* text = PyUnicode_AsUTF8(chunk);
        if (!text) {
            Py_DECREF(encoded_list);
            return NULL;
        }
        int text_length = strlen(text);
        int i = 0;
        while (i < text_length) {
            int match_length;
            int token_id = search_trie(trie, (unsigned char*)text + i, text_length - i, &match_length);
            if (token_id != -1) {
                PyList_Append(encoded_list, PyLong_FromLong(token_id));
                i += match_length;
            } 
            else {
                PyList_Append(encoded_list, PyLong_FromLong((unsigned char)text[i]));
                i++;
            }
        }
    }
    return encoded_list;
}

// Method definitions
static PyMethodDef _BpeMethods[] = {
    {"train", train, METH_VARARGS, "Train a text tokenizer using byte-pair encoding."},
    {"build_trie", build_trie, METH_VARARGS, "Build a trie from an encoding dictionary."},
    {"manual_free_trie", manual_free_trie, METH_VARARGS, "Manually free the trie structure."},
    {"encode_train", encode_train, METH_VARARGS, "Encode text using the trained BPE model. Uses less memory but slower."},
    {"encode_inference", encode_inference, METH_VARARGS, "Encode text using the trained BPE model. Faster but uses more memory."},

    {NULL, NULL, 0, NULL}
};

// Module definition
static struct PyModuleDef _bpe_module = {
    PyModuleDef_HEAD_INIT,
    "_bpe",        // Name of the module
    NULL,          // Module documentation
    -1,            // Size of per-interpreter state of the module
    _BpeMethods,   // Corrected method table name
    NULL,          // m_reload
    NULL,          // m_traverse
    NULL,          // m_clear
    NULL           // m_free 
};

// Module initialization function
PyMODINIT_FUNC PyInit__bpe(void) {
    return PyModule_Create(&_bpe_module);
}