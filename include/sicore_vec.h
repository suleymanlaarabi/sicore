#ifndef SICORE_VEC_H
#define SICORE_VEC_H
#include "sicore.h"

#if SICORE_HAS_VEC
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    void *data;
    uint32_t size;
    uint32_t capacity;
} sicore_vec_t;

void sicore_vec_init(sicore_vec_t *vec, const uint32_t element_size);
void sicore_vec_init_w_size(sicore_vec_t *vec, const uint32_t element_size, uint32_t size);
void sicore_vec_fini(sicore_vec_t *vec);
void sicore_vec_grow(sicore_vec_t *vec, const uint32_t element_size);

// Ensure vec has at least `count` elements. New slots are zero-initialized.
void sicore_vec_ensure(sicore_vec_t *vec, uint32_t count, const uint32_t element_size);

// Copy element into the vec (memcpy). The pointer is not retained.
// Safe to call repeatedly — any grow only invalidates the internal buffer, not
// the source pointer.
void sicore_vec_push(sicore_vec_t *vec, const void *element, const uint32_t element_size);

// Reserve one slot and return a pointer to it (uninitialized).
// WARNING: the returned pointer is invalidated by any subsequent push or grow
// on the same vec. Finish all writes through this pointer before pushing again.
static inline void *sicore_vec_push_empty(sicore_vec_t *vec, const uint32_t element_size) {
    if (SICORE_UNLIKELY(vec->size >= vec->capacity)) {
        sicore_vec_grow(vec, element_size);
    }
    void *ptr = (uint8_t *)vec->data + (vec->size * element_size);
    vec->size++;
    return ptr;
}

bool sicore_vec_contains_u16(const sicore_vec_t *vec, uint16_t value);
void sicore_vec_remove_u16(sicore_vec_t *vec, uint16_t value);
void sicore_vec_remove_u64(sicore_vec_t *vec, uint64_t value);

// Specialized push for 2-byte types
static inline void sicore_vec_push_u16(sicore_vec_t *vec, const uint16_t value) {
    if (SICORE_UNLIKELY(vec->size >= vec->capacity)) {
        sicore_vec_grow(vec, sizeof(uint16_t));
    }
    ((uint16_t *)vec->data)[vec->size++] = value;
}

// Specialized push for 8-byte types
static inline void sicore_vec_push_u64(sicore_vec_t *vec, const uint64_t value) {
    if (SICORE_UNLIKELY(vec->size >= vec->capacity)) {
        sicore_vec_grow(vec, sizeof(uint64_t));
    }
    ((uint64_t *)vec->data)[vec->size++] = value;
}

void sicore_vec_remove_fast(sicore_vec_t *vec, uint32_t index, const uint32_t element_size);

// Direct pointer access for fast iteration
#define sicore_vec_get(vec, index, type) (&((const type *)(vec)->data)[index])
#define sicore_vec_get_mut(vec, index, type) (&((type *)(vec)->data)[index])
#define sicore_vec_remove_last(vec) ((vec)->size--)
#define sicore_vec_clear(vec) ((vec)->size = 0)
#define sicore_vec_data(vec, type) ((type *)(vec)->data)

#define sicore_vec_iter(vec, type, value, ...)                                                     \
    const type *__values = (vec)->data;                                                            \
    const uint32_t __count = (vec)->size;                                                          \
    for (uint32_t i = 0; i < __count; i++) {                                                       \
        const type *value = &__values[i];                                                          \
        __VA_ARGS__                                                                                \
    }

#endif
#endif
