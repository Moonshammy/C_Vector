#include "vector.h"

typedef struct Vector{
    int *data;
    size_t size;
    size_t capacity;
}Vector;

Vector* vector(void){
    Vector *v = calloc(1, sizeof(Vector));
    v->data =calloc(1, sizeof(int));
    v->size = 0;
    v->capacity = 0;
    return v;
}

int vector_push_back(Vector *v, int value){
    if (!v){
        return -1;
    }
    if (v->size == v->capacity){
        if(vector_reserve(v) == -1){
            return -1;
        }
    }
    v->data[v->size] = value;
    v->size++;
    return 0;
}

int vector_pop_back(Vector *v, int *out){
    if(!v){
        return -1;
    }
    if(v->size == 0){
        return -1;
    }
    if (out){
        *out = v->data[v->size-1];
    }
    v->data[v->size-1] = 0;
    v->size--;
    return 0;
}

int vector_get(const Vector *v, size_t index, int *out){
    (void)out;
    if (!v || index >= v->size || (int)index < 0){
        return -1;
    }
    *out = v->data[index];
    return 0;
}

int vector_set(Vector *v, size_t index, int value){
    if (!v || index >= v->size || (int)index < 0){
        return -1;
    }
    v->data[index] = value;
    return 0;
}

int vector_insert(Vector *v, size_t index, int value){
    if (!v || index > v->size || (int)index < 0){
        return -1;
    }
    if (v->size == v->capacity){
        vector_reserve(v);
    }
    for (int i = (int)v->size; i > (int)index; i--){
        v->data[i] = v->data[i-1];
    }
    v->data[index] = value;
    v->size++;
    return 0;
}

int vector_remove(Vector *v, size_t index, int *out){
    if (!v){
        return -1;
    }
    if (index >= v->size || (int)index < 0){
        return -1;
    }
    if (out){
    *out = v->data[index];
    }
    for (int i = (int)index; i < (int)v->size-1; i++){
        v->data[i] = v->data[i+1];
    }
    v->data[v->size-1] = 0;
    v->size--;
    return 0;
}

size_t  vector_size(const Vector *v){
    return v->size;
}

size_t  vector_capacity(const Vector *v){
    return v->capacity;
}

int vector_reserve(Vector *v){
    v->capacity = v->capacity ? v->capacity * 2 : 8;
    int *tmp = realloc(v->data, v->capacity * sizeof(Vector));
    if (!tmp) return -1;
    v->data = tmp;
    return 0;
}

void vector_clear(Vector *v){
    for (int i = 0; i < (int)v->size; i++){
        v->data[i] = 0;
    }
    v->size = 0;
}

void vector_destroy(Vector *v){
    if (!v) return;
    free(v->data);
    free(v);
}

void vector_print(Vector *v){
    if (!v){
        printf("Function requires valid Vector\n");
        return;
    }
    printf("[");
    for (int i = 0; i < (int)v->size; i++){
        if ((int)v->size - i == 1){
            printf("%d", v->data[i]);
        }
        else {printf("%d,", v->data[i]);}
    }
    printf("]\n");
}