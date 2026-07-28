#include "sicore.h"

#if SICORE_HAS_VEC
#include "sicore_vec.h"
#include <stdlib.h>
#include <string.h>

void sicore_vec_init(sicore_vec_t *vec, uint32_t element_size) {
    vec->data = malloc(element_size);
    vec->size = 0;
    vec->capacity = 1;
}

void sicore_vec_init_w_size(sicore_vec_t *vec, uint32_t element_size, uint32_t size) {
    vec->data = malloc(element_size * size);
    vec->size = 0;
    vec->capacity = size;
}

void sicore_vec_fini(sicore_vec_t *vec) { free(vec->data); }

void sicore_vec_grow(sicore_vec_t *vec, uint32_t element_size) {
    vec->capacity *= 2;
    vec->data = realloc(vec->data, element_size * vec->capacity);
}

void sicore_vec_push(sicore_vec_t *vec, const void *element, const uint32_t element_size) {
    if (SICORE_UNLIKELY(vec->size >= vec->capacity)) {
        sicore_vec_grow(vec, element_size);
    }
    memcpy((uint8_t *)vec->data + (vec->size * element_size), element, element_size);
    vec->size++;
}

void sicore_vec_ensure(sicore_vec_t *vec, uint32_t count, const uint32_t element_size) {
    if (count <= vec->size)
        return;
    while (vec->capacity < count)
        sicore_vec_grow(vec, element_size);
    memset((uint8_t *)vec->data + vec->size * element_size, 0, (count - vec->size) * element_size);
    vec->size = count;
}

void sicore_vec_remove_fast(sicore_vec_t *vec, uint32_t index, const uint32_t element_size) {
    if (index < vec->size - 1) {
        void *dst = (uint8_t *)vec->data + (index * element_size);
        const void *src = (uint8_t *)vec->data + ((vec->size - 1) * element_size);
        memcpy(dst, src, element_size);
    }
    vec->size--;
}

bool sicore_vec_contains_u16(const sicore_vec_t *vec, const uint16_t value) {
    sicore_vec_iter(vec, uint16_t, current, {
        if (*current == value) {
            return true;
        }
    });
    return false;
}

static inline void sicore_vec_remove_fast_u16(sicore_vec_t *vec, uint32_t index) {
    if (index < vec->size - 1) {
        uint16_t *data = vec->data;
        data[index] = data[vec->size - 1];
    }
    vec->size--;
}

void sicore_vec_remove_u16(sicore_vec_t *vec, const uint16_t value) {
    sicore_vec_iter(vec, uint16_t, current, {
        if (*current == value) {
            sicore_vec_remove_fast_u16(vec, i);
            return;
        }
    });
}

static inline void sicore_vec_remove_fast_u64(sicore_vec_t *vec, uint32_t index) {
    if (index < vec->size - 1) {
        uint64_t *data = vec->data;
        data[index] = data[vec->size - 1];
    }
    vec->size--;
}

void sicore_vec_remove_u64(sicore_vec_t *vec, uint64_t value) {
    sicore_vec_iter(vec, uint64_t, current, {
        if (*current == value) {
            sicore_vec_remove_fast_u64(vec, i);
            return;
        }
    });
}
#endif
