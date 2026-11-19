#ifndef VECTOR_H
#define VECTOR_H

#include <stdlib.h>
#include <stdio.h>

typedef struct Vector Vector;

Vector*     vector(void);
int         vector_push_back(Vector *v, int value);
int         vector_pop_back(Vector *v, int *out);
int         vector_get(const Vector *v, size_t index, int *out);
int         vector_set(Vector *v, size_t index, int value);
int         vector_insert(Vector *v, size_t index, int value);
int         vector_remove(Vector *v, size_t index, int *out);
size_t      vector_size(const Vector *v);
size_t      vector_capacity(const Vector *v);
int         vector_reserve(Vector *v);
void        vector_clear(Vector *v);
void        vector_destroy(Vector *v);
void        vector_print(Vector *v);

#endif