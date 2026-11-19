/*
 * test.c - test suite for the dynamic array (vector.h)
 *
 * Checks every function against the requirements doc:
 *   - "0 / -1": 0 on success, -1 on error
 *   - on any error the vector is left exactly as it was
 *   - capacity starts at 0, first growth goes to 8, then doubles
 *
 * Written against your vector.h:
 *   - vector() is the constructor, vector_destroy() frees it
 *   - vector_reserve(v) takes no size: these tests expect it to make capacity
 *     strictly larger while leaving size and contents untouched
 *   - every test frees its vectors, so `make asan` / `make valgrind` also
 *     report any memory your vector leaks
 *
 * Build and run with:  make test     (or  make asan  to also catch memory bugs)
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "vector.h"


/* ------------------------------------------------------------------ */
/* Tiny test framework                                                 */
/* ------------------------------------------------------------------ */

static int checks   = 0;
static int failures = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        checks++;                                                       \
        if (!(cond)) {                                                  \
            failures++;                                                 \
            printf("    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
        }                                                               \
    } while (0)

#define RUN(test)                                                       \
    do {                                                                \
        int before = failures;                                          \
        test();                                                         \
        printf("%-6s %s\n", failures == before ? "ok" : "FAILED", #test); \
    } while (0)

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

/* Vector holding 0, 1, 2, ..., n-1. */
static Vector *make_vec(size_t n)
{
    Vector *v = vector();
    if (!v) return NULL;
    for (size_t i = 0; i < n; i++) {
        if (vector_push_back(v, (int)i) != 0) { vector_destroy(v); return NULL; }
    }
    return v;
}

/* True if v holds exactly expected[0..n-1], in order. */
static bool matches(const Vector *v, const int *expected, size_t n)
{
    if (vector_size(v) != n) return false;
    for (size_t i = 0; i < n; i++) {
        int got = 0;
        if (vector_get(v, i, &got) != 0) return false;
        if (got != expected[i]) return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* vector() / vector_destroy()                                         */
/* ------------------------------------------------------------------ */

static void test_create_is_empty(void)
{
    Vector *v = vector();
    CHECK(v != NULL);
    CHECK(vector_size(v) == 0);
    CHECK(vector_capacity(v) == 0);
    vector_destroy(v);
}

static void test_destroy_null_is_safe(void)
{
    vector_destroy(NULL);   /* must not crash */
    CHECK(true);
}

static void test_destroy_empty_vector(void)
{
    /* Never pushed anything, so data may still be NULL. */
    Vector *v = vector();
    CHECK(v != NULL);
    vector_destroy(v);
}

static void test_destroy_after_use(void)
{
    /* `make asan` reports a leak here if destroy misses the data buffer. */
    Vector *v = make_vec(1000);
    CHECK(v != NULL);
    vector_destroy(v);
}

static void test_create_gives_independent_vectors(void)
{
    Vector *a = vector();
    Vector *b = vector();
    CHECK(a != b);
    vector_push_back(a, 1);
    CHECK(vector_size(a) == 1);
    CHECK(vector_size(b) == 0);
    vector_destroy(a);
    vector_destroy(b);
}

/* ------------------------------------------------------------------ */
/* vector_push_back()                                                  */
/* ------------------------------------------------------------------ */

static void test_push_back_single(void)
{
    Vector *v = vector();
    CHECK(vector_push_back(v, 42) == 0);
    CHECK(vector_size(v) == 1);
    int out = 0;
    CHECK(vector_get(v, 0, &out) == 0);
    CHECK(out == 42);
    vector_destroy(v);
}

static void test_push_back_keeps_order(void)
{
    Vector *v = vector();
    int expected[] = {5, -3, 0, 99, 7};
    for (size_t i = 0; i < 5; i++) CHECK(vector_push_back(v, expected[i]) == 0);
    CHECK(matches(v, expected, 5));
    vector_destroy(v);
}

static void test_push_back_growth_policy(void)
{
    /* Doc: capacity 0 -> 8 on first growth, then doubles. */
    Vector *v = vector();
    CHECK(vector_push_back(v, 1) == 0);
    CHECK(vector_capacity(v) == 8);

    for (int i = 2; i <= 8; i++) vector_push_back(v, i);
    CHECK(vector_size(v) == 8);
    CHECK(vector_capacity(v) == 8);      /* full, but no growth yet */

    vector_push_back(v, 9);
    CHECK(vector_capacity(v) == 16);     /* doubled */

    for (int i = 10; i <= 17; i++) vector_push_back(v, i);
    CHECK(vector_capacity(v) == 32);     /* doubled again */
    vector_destroy(v);
}

static void test_push_back_many(void)
{
    const size_t n = 100000;
    Vector *v = make_vec(n);
    CHECK(v != NULL);
    CHECK(vector_size(v) == n);
    CHECK(vector_capacity(v) >= n);

    bool all_ok = true;
    for (size_t i = 0; i < n; i++) {
        int out = -1;
        if (vector_get(v, i, &out) != 0 || out != (int)i) { all_ok = false; break; }
    }
    CHECK(all_ok);
    vector_destroy(v);
}

/* ------------------------------------------------------------------ */
/* vector_pop_back()                                                   */
/* ------------------------------------------------------------------ */

static void test_pop_back_returns_last(void)
{
    Vector *v = make_vec(3);                 /* 0 1 2 */
    int out = -1;
    CHECK(vector_pop_back(v, &out) == 0);
    CHECK(out == 2);
    CHECK(vector_size(v) == 2);
    CHECK(vector_pop_back(v, &out) == 0);
    CHECK(out == 1);
    int expected[] = {0};
    CHECK(matches(v, expected, 1));
    vector_destroy(v);
}

static void test_pop_back_null_out(void)
{
    Vector *v = make_vec(3);
    CHECK(vector_pop_back(v, NULL) == 0);    /* allowed: just discards */
    CHECK(vector_size(v) == 2);
    vector_destroy(v);
}

static void test_pop_back_empty_fails(void)
{
    Vector *v = vector();
    int out = 123;
    CHECK(vector_pop_back(v, &out) == -1);
    CHECK(vector_size(v) == 0);
    vector_destroy(v);
}

static void test_pop_back_until_empty(void)
{
    Vector *v = make_vec(10);
    for (int i = 9; i >= 0; i--) {
        int out = -1;
        CHECK(vector_pop_back(v, &out) == 0);
        CHECK(out == i);
    }
    CHECK(vector_size(v) == 0);
    CHECK(vector_pop_back(v, NULL) == -1);
    vector_destroy(v);
}

static void test_pop_back_keeps_capacity(void)
{
    /* 8 -> 7 elements is well above the optional 1/4 shrink threshold. */
    Vector *v = make_vec(8);
    size_t cap = vector_capacity(v);
    vector_pop_back(v, NULL);
    CHECK(vector_capacity(v) == cap);
    vector_destroy(v);
}

/* ------------------------------------------------------------------ */
/* vector_get()                                                        */
/* ------------------------------------------------------------------ */

static void test_get_valid_indexes(void)
{
    Vector *v = make_vec(5);
    for (size_t i = 0; i < 5; i++) {
        int out = -1;
        CHECK(vector_get(v, i, &out) == 0);
        CHECK(out == (int)i);
    }
    vector_destroy(v);
}

static void test_get_out_of_range(void)
{
    Vector *v = make_vec(5);
    int out = 0;
    CHECK(vector_get(v, 5, &out) == -1);         /* index == size */
    CHECK(vector_get(v, 1000, &out) == -1);
    CHECK(vector_get(v, (size_t)-1, &out) == -1); /* huge index */
    CHECK(vector_size(v) == 5);
    vector_destroy(v);
}

static void test_get_on_empty(void)
{
    Vector *v = vector();
    int out = 0;
    CHECK(vector_get(v, 0, &out) == -1);
    vector_destroy(v);
}

/* ------------------------------------------------------------------ */
/* vector_set()                                                        */
/* ------------------------------------------------------------------ */

static void test_set_overwrites(void)
{
    Vector *v = make_vec(5);                     /* 0 1 2 3 4 */
    CHECK(vector_set(v, 0, 100) == 0);
    CHECK(vector_set(v, 2, 200) == 0);
    CHECK(vector_set(v, 4, 400) == 0);
    int expected[] = {100, 1, 200, 3, 400};
    CHECK(matches(v, expected, 5));
    vector_destroy(v);
}

static void test_set_keeps_size_and_capacity(void)
{
    Vector *v = make_vec(5);
    size_t cap = vector_capacity(v);
    vector_set(v, 3, 999);
    CHECK(vector_size(v) == 5);
    CHECK(vector_capacity(v) == cap);
    vector_destroy(v);
}

static void test_set_cannot_append(void)
{
    Vector *v = make_vec(5);
    CHECK(vector_set(v, 5, 999) == -1);          /* index == size */
    CHECK(vector_set(v, 100, 999) == -1);
    int expected[] = {0, 1, 2, 3, 4};
    CHECK(matches(v, expected, 5));
    vector_destroy(v);
}

static void test_set_on_empty(void)
{
    Vector *v = vector();
    CHECK(vector_set(v, 0, 1) == -1);
    CHECK(vector_size(v) == 0);
    vector_destroy(v);
}

/* ------------------------------------------------------------------ */
/* vector_insert()                                                     */
/* ------------------------------------------------------------------ */

static void test_insert_front(void)
{
    Vector *v = make_vec(3);                     /* 0 1 2 */
    CHECK(vector_insert(v, 0, 9) == 0);
    int expected[] = {9, 0, 1, 2};
    CHECK(matches(v, expected, 4));
    vector_destroy(v);
}

static void test_insert_middle(void)
{
    Vector *v = make_vec(4);                     /* 0 1 2 3 */
    CHECK(vector_insert(v, 2, 9) == 0);
    int expected[] = {0, 1, 9, 2, 3};
    CHECK(matches(v, expected, 5));
    vector_destroy(v);
}

static void test_insert_at_size_appends(void)
{
    Vector *v = make_vec(3);
    CHECK(vector_insert(v, 3, 9) == 0);          /* index == size is allowed */
    int expected[] = {0, 1, 2, 9};
    CHECK(matches(v, expected, 4));
    vector_destroy(v);
}

static void test_insert_into_empty(void)
{
    Vector *v = vector();
    CHECK(vector_insert(v, 0, 7) == 0);
    int expected[] = {7};
    CHECK(matches(v, expected, 1));
    vector_destroy(v);
}

static void test_insert_past_size_fails(void)
{
    Vector *v = make_vec(3);
    CHECK(vector_insert(v, 4, 9) == -1);
    CHECK(vector_insert(v, 1000, 9) == -1);
    int expected[] = {0, 1, 2};
    CHECK(matches(v, expected, 3));
    vector_destroy(v);
}

static void test_insert_when_full_grows(void)
{
    Vector *v = make_vec(8);                     /* size 8, capacity 8 */
    CHECK(vector_capacity(v) == 8);
    CHECK(vector_insert(v, 4, 99) == 0);
    CHECK(vector_capacity(v) == 16);
    int expected[] = {0, 1, 2, 3, 99, 4, 5, 6, 7};
    CHECK(matches(v, expected, 9));
    vector_destroy(v);
}

/* ------------------------------------------------------------------ */
/* vector_remove()                                                     */
/* ------------------------------------------------------------------ */

static void test_remove_middle(void)
{
    Vector *v = make_vec(5);                     /* 0 1 2 3 4 */
    int out = -1;
    CHECK(vector_remove(v, 2, &out) == 0);
    CHECK(out == 2);
    int expected[] = {0, 1, 3, 4};
    CHECK(matches(v, expected, 4));
    vector_destroy(v);
}

static void test_remove_front_and_back(void)
{
    Vector *v = make_vec(5);
    int out = -1;
    CHECK(vector_remove(v, 0, &out) == 0);
    CHECK(out == 0);
    CHECK(vector_remove(v, 3, &out) == 0);       /* last element now */
    CHECK(out == 4);
    int expected[] = {1, 2, 3};
    CHECK(matches(v, expected, 3));
    vector_destroy(v);
}

static void test_remove_null_out(void)
{
    Vector *v = make_vec(3);
    CHECK(vector_remove(v, 1, NULL) == 0);
    int expected[] = {0, 2};
    CHECK(matches(v, expected, 2));
    vector_destroy(v);
}

static void test_remove_out_of_range(void)
{
    Vector *v = make_vec(3);
    CHECK(vector_remove(v, 3, NULL) == -1);      /* index == size */
    CHECK(vector_remove(v, 100, NULL) == -1);
    int expected[] = {0, 1, 2};
    CHECK(matches(v, expected, 3));
    vector_destroy(v);
}

static void test_remove_on_empty(void)
{
    Vector *v = vector();
    CHECK(vector_remove(v, 0, NULL) == -1);
    CHECK(vector_size(v) == 0);
    vector_destroy(v);
}

static void test_remove_only_element(void)
{
    Vector *v = make_vec(1);
    int out = -1;
    CHECK(vector_remove(v, 0, &out) == 0);
    CHECK(out == 0);
    CHECK(vector_size(v) == 0);
    vector_destroy(v);
}

/* ------------------------------------------------------------------ */
/* vector_size() / vector_capacity()                                   */
/* ------------------------------------------------------------------ */

static void test_size_tracks_every_operation(void)
{
    Vector *v = vector();
    vector_push_back(v, 1);      CHECK(vector_size(v) == 1);
    vector_push_back(v, 2);      CHECK(vector_size(v) == 2);
    vector_insert(v, 0, 3);      CHECK(vector_size(v) == 3);
    vector_set(v, 0, 4);         CHECK(vector_size(v) == 3);
    vector_remove(v, 0, NULL);   CHECK(vector_size(v) == 2);
    vector_pop_back(v, NULL);    CHECK(vector_size(v) == 1);
    vector_destroy(v);
}

static void test_capacity_never_below_size(void)
{
    Vector *v = vector();
    bool ok = true;
    for (int i = 0; i < 1000; i++) {
        vector_push_back(v, i);
        if (vector_capacity(v) < vector_size(v)) ok = false;
    }
    CHECK(ok);
    vector_destroy(v);
}

/* ------------------------------------------------------------------ */
/* vector_reserve()                                                    */
/* ------------------------------------------------------------------ */

static void test_reserve_on_empty(void)
{
    Vector *v = vector();                        /* capacity 0 */
    CHECK(vector_reserve(v) == 0);
    CHECK(vector_capacity(v) > 0);
    CHECK(vector_size(v) == 0);                  /* never changes size */
    vector_destroy(v);
}

static void test_reserve_grows_full_vector(void)
{
    Vector *v = make_vec(8);                     /* size 8, capacity 8 */
    size_t cap = vector_capacity(v);
    CHECK(vector_reserve(v) == 0);
    CHECK(vector_capacity(v) > cap);
    CHECK(vector_size(v) == 8);
    vector_destroy(v);
}

static void test_reserve_grows_partly_full_vector(void)
{
    Vector *v = make_vec(3);                     /* room left, still grows */
    size_t cap = vector_capacity(v);
    CHECK(vector_reserve(v) == 0);
    CHECK(vector_capacity(v) > cap);
    CHECK(vector_size(v) == 3);
    vector_destroy(v);
}

static void test_reserve_keeps_contents(void)
{
    Vector *v = make_vec(20);
    CHECK(vector_reserve(v) == 0);
    CHECK(vector_reserve(v) == 0);               /* twice in a row */
    int expected[20];
    for (int i = 0; i < 20; i++) expected[i] = i;
    CHECK(matches(v, expected, 20));
    vector_destroy(v);
}

static void test_reserve_then_push_uses_room(void)
{
    Vector *v = make_vec(8);
    vector_reserve(v);
    size_t cap = vector_capacity(v);
    while (vector_size(v) < cap) vector_push_back(v, 0);
    CHECK(vector_capacity(v) == cap);            /* no regrowth until full */
    vector_destroy(v);
}

/* ------------------------------------------------------------------ */
/* vector_clear()                                                      */
/* ------------------------------------------------------------------ */

static void test_clear_empties_but_keeps_capacity(void)
{
    Vector *v = make_vec(20);
    size_t cap = vector_capacity(v);
    vector_clear(v);
    CHECK(vector_size(v) == 0);
    CHECK(vector_capacity(v) == cap);
    CHECK(vector_get(v, 0, &(int){0}) == -1);
    vector_destroy(v);
}

static void test_clear_then_reuse(void)
{
    Vector *v = make_vec(5);
    vector_clear(v);
    vector_push_back(v, 42);
    int expected[] = {42};
    CHECK(matches(v, expected, 1));
    vector_destroy(v);
}

static void test_clear_on_empty(void)
{
    Vector *v = vector();
    vector_clear(v);
    CHECK(vector_size(v) == 0);
    vector_destroy(v);
}

/* ------------------------------------------------------------------ */
/* vector_print()                                                      */
/* ------------------------------------------------------------------ */

static void test_print_does_not_crash(void)
{
    /* Output format isn't specified, so this only checks it runs. */
    Vector *v = make_vec(3);
    Vector *empty = vector();
    printf("    print [0 1 2]: ");  fflush(stdout); vector_print(v);
    printf("    print []:      ");  fflush(stdout); vector_print(empty);
    CHECK(vector_size(v) == 3);
    vector_destroy(v);
    vector_destroy(empty);
}

/* ------------------------------------------------------------------ */
/* Randomized stress test against a plain-array model                  */
/* ------------------------------------------------------------------ */

static unsigned long rng_state = 12345;
static unsigned long rng(void)
{
    rng_state = rng_state * 1103515245UL + 12345UL;
    return (rng_state >> 16) & 0x7fff;
}

static void test_random_ops_match_model(void)
{
    enum { MAX = 5000, OPS = 20000 };
    static int model[MAX];
    size_t n = 0;
    Vector *v = vector();
    bool ok = true;

    for (int op = 0; op < OPS && ok; op++) {
        unsigned long r = rng() % 100;
        int value = (int)(rng() % 1000);

        if (r < 35 && n < MAX) {                          /* push_back */
            ok = vector_push_back(v, value) == 0;
            model[n++] = value;
        } else if (r < 50) {                              /* pop_back */
            int out = 0;
            int rc = vector_pop_back(v, &out);
            if (n == 0) ok = rc == -1;
            else        ok = rc == 0 && out == model[--n];
        } else if (r < 65 && n < MAX) {                   /* insert */
            size_t idx = n ? rng() % (n + 1) : 0;
            ok = vector_insert(v, idx, value) == 0;
            for (size_t i = n; i > idx; i--) model[i] = model[i - 1];
            model[idx] = value;
            n++;
        } else if (r < 80) {                              /* remove */
            if (n == 0) { ok = vector_remove(v, 0, NULL) == -1; continue; }
            size_t idx = rng() % n;
            int out = 0;
            ok = vector_remove(v, idx, &out) == 0 && out == model[idx];
            for (size_t i = idx; i + 1 < n; i++) model[i] = model[i + 1];
            n--;
        } else if (r < 90) {                              /* set */
            if (n == 0) { ok = vector_set(v, 0, value) == -1; continue; }
            size_t idx = rng() % n;
            ok = vector_set(v, idx, value) == 0;
            model[idx] = value;
        } else {                                          /* out-of-range get */
            ok = vector_get(v, n, &(int){0}) == -1;
        }

        if (ok && op % 500 == 0) ok = matches(v, model, n);
        if (ok) ok = vector_capacity(v) >= vector_size(v);
    }

    CHECK(ok);
    CHECK(matches(v, model, n));
    vector_destroy(v);
}

/* ------------------------------------------------------------------ */

int main(void)
{
    puts("vector() / vector_destroy()");
    RUN(test_create_is_empty);
    RUN(test_create_gives_independent_vectors);
    RUN(test_destroy_null_is_safe);
    RUN(test_destroy_empty_vector);
    RUN(test_destroy_after_use);

    puts("\nvector_push_back()");
    RUN(test_push_back_single);
    RUN(test_push_back_keeps_order);
    RUN(test_push_back_growth_policy);
    RUN(test_push_back_many);

    puts("\nvector_pop_back()");
    RUN(test_pop_back_returns_last);
    RUN(test_pop_back_null_out);
    RUN(test_pop_back_empty_fails);
    RUN(test_pop_back_until_empty);
    RUN(test_pop_back_keeps_capacity);

    puts("\nvector_get()");
    RUN(test_get_valid_indexes);
    RUN(test_get_out_of_range);
    RUN(test_get_on_empty);

    puts("\nvector_set()");
    RUN(test_set_overwrites);
    RUN(test_set_keeps_size_and_capacity);
    RUN(test_set_cannot_append);
    RUN(test_set_on_empty);

    puts("\nvector_insert()");
    RUN(test_insert_front);
    RUN(test_insert_middle);
    RUN(test_insert_at_size_appends);
    RUN(test_insert_into_empty);
    RUN(test_insert_past_size_fails);
    RUN(test_insert_when_full_grows);

    puts("\nvector_remove()");
    RUN(test_remove_middle);
    RUN(test_remove_front_and_back);
    RUN(test_remove_null_out);
    RUN(test_remove_out_of_range);
    RUN(test_remove_on_empty);
    RUN(test_remove_only_element);

    puts("\nvector_size() / vector_capacity()");
    RUN(test_size_tracks_every_operation);
    RUN(test_capacity_never_below_size);

    puts("\nvector_reserve()");
    RUN(test_reserve_on_empty);
    RUN(test_reserve_grows_full_vector);
    RUN(test_reserve_grows_partly_full_vector);
    RUN(test_reserve_keeps_contents);
    RUN(test_reserve_then_push_uses_room);

    puts("\nvector_clear()");
    RUN(test_clear_empties_but_keeps_capacity);
    RUN(test_clear_then_reuse);
    RUN(test_clear_on_empty);

    puts("\nvector_print()");
    RUN(test_print_does_not_crash);

    puts("\nstress");
    RUN(test_random_ops_match_model);

    printf("\n%d checks, %d failed\n", checks, failures);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
