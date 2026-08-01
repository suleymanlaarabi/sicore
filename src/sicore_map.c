#include "sicore.h"

#if SICORE_HAS_MAP
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#if defined(__SSE2__) || defined(_M_X64) || defined(_M_AMD64)
#include <emmintrin.h>
#define SICORE_MAP_SSE2 1
#elif defined(__aarch64__) && defined(__ARM_NEON)
#include <arm_neon.h>
#define SICORE_MAP_NEON 1
#endif

#define SICORE_GROUP_WIDTH 16u
#define SICORE_INITIAL_CAPACITY 16u
#define SICORE_CTRL_EMPTY UINT8_C(0x80)
#define SICORE_CTRL_DELETED UINT8_C(0xfe)

/* 16 octets sur ABI 64 bits: 1/4 de ligne de cache de 64 octets. */
typedef struct {
    const char *key;
    uint32_t value;
    uint32_t key_length;
} sicore_map_entry_t;

/*
 * Hash de chaîne basé sur wyhash final v4 (domaine public / Unlicense), adapté
 * et préfixé pour rester entièrement interne à cette unité de compilation.
 */
static const uint64_t sicore_hash_secret[5] = { UINT64_C(0xa0761d6478bd642f),
                                                UINT64_C(0xe7037ed1a0b428db),
                                                UINT64_C(0x8ebc6af09c88c6e3),
                                                UINT64_C(0x589965cc75374cc3),
                                                UINT64_C(0x1d8e4e27c47d124f) };

static inline void sicore_mul128(uint64_t *a, uint64_t *b) {
#if defined(__SIZEOF_INT128__)
    __uint128_t r = (__uint128_t)(*a) * (*b);
    *a = (uint64_t)r;
    *b = (uint64_t)(r >> 64);
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_AMD64))
    *a = _umul128(*a, *b, b);
#else
    const uint64_t ah = *a >> 32;
    const uint64_t al = (uint32_t)*a;
    const uint64_t bh = *b >> 32;
    const uint64_t bl = (uint32_t)*b;
    const uint64_t rh = ah * bh;
    const uint64_t rm0 = ah * bl;
    const uint64_t rm1 = bh * al;
    const uint64_t rl = al * bl;
    const uint64_t t = rl + (rm0 << 32);
    uint64_t carry = t < rl;
    const uint64_t lo = t + (rm1 << 32);
    carry += lo < t;
    *a = lo;
    *b = rh + (rm0 >> 32) + (rm1 >> 32) + carry;
#endif
}

static inline uint64_t sicore_mix(uint64_t a, uint64_t b) {
    sicore_mul128(&a, &b);
    return a ^ b;
}

static inline uint64_t sicore_read64(const uint8_t *p) {
    uint64_t v;
    memcpy(&v, p, sizeof(v));
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#if defined(_MSC_VER)
    v = _byteswap_uint64(v);
#else
    v = __builtin_bswap64(v);
#endif
#endif
    return v;
}

static inline uint64_t sicore_read32(const uint8_t *p) {
    uint32_t v;
    memcpy(&v, p, sizeof(v));
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#if defined(_MSC_VER)
    v = _byteswap_ulong((unsigned long)v);
#else
    v = __builtin_bswap32(v);
#endif
#endif
    return v;
}

static inline uint64_t sicore_read3(const uint8_t *p, size_t len) {
    return ((uint64_t)p[0] << 16) | ((uint64_t)p[len >> 1] << 8) | (uint64_t)p[len - 1];
}

static inline uint64_t
sicore_hash_finish16(const uint8_t *p, uint64_t len, uint64_t seed, size_t remaining) {
    uint64_t a;
    uint64_t b;

    if (remaining <= 8) {
        if (remaining >= 4) {
            a = sicore_read32(p);
            b = sicore_read32(p + remaining - 4);
        } else if (remaining != 0) {
            a = sicore_read3(p, remaining);
            b = 0;
        } else {
            a = 0;
            b = 0;
        }
    } else {
        a = sicore_read64(p);
        b = sicore_read64(p + remaining - 8);
    }

    return sicore_mix(sicore_hash_secret[1] ^ len, sicore_mix(a ^ sicore_hash_secret[1], b ^ seed));
}

static inline uint64_t sicore_hash_bytes(const uint8_t *p, size_t len) {
    size_t remaining = len;
    uint64_t seed = sicore_hash_secret[0];

    if (SICORE_UNLIKELY(remaining > 64)) {
        uint64_t seed2 = seed;
        do {
            seed =
                sicore_mix(sicore_read64(p) ^ sicore_hash_secret[1], sicore_read64(p + 8) ^ seed) ^
                sicore_mix(
                    sicore_read64(p + 16) ^ sicore_hash_secret[2],
                    sicore_read64(p + 24) ^ seed
                );
            seed2 = sicore_mix(
                        sicore_read64(p + 32) ^ sicore_hash_secret[3],
                        sicore_read64(p + 40) ^ seed2
                    ) ^
                    sicore_mix(
                        sicore_read64(p + 48) ^ sicore_hash_secret[4],
                        sicore_read64(p + 56) ^ seed2
                    );
            p += 64;
            remaining -= 64;
        } while (remaining > 64);
        seed ^= seed2;
    }

    while (remaining > 16) {
        seed = sicore_mix(sicore_read64(p) ^ sicore_hash_secret[1], sicore_read64(p + 8) ^ seed);
        p += 16;
        remaining -= 16;
    }

    return sicore_hash_finish16(p, (uint64_t)len, seed, remaining);
}

static inline uint64_t sicore_hash_string(const char *key, uint32_t *length) {
    const uint32_t len = (uint32_t)strlen(key);
    *length = len;
    return sicore_hash_bytes((const uint8_t *)key, len);
}

static inline uint32_t sicore_ctz32(uint32_t x) {
#if defined(_MSC_VER)
    unsigned long bit;
    _BitScanForward(&bit, x);
    return (uint32_t)bit;
#else
    return (uint32_t)__builtin_ctz(x);
#endif
}

#if defined(SICORE_MAP_SSE2)
static inline uint32_t sicore_match_byte(const uint8_t *ctrl, uint8_t byte) {
    const __m128i group = _mm_loadu_si128((const __m128i *)(const void *)ctrl);
    const __m128i wanted = _mm_set1_epi8((char)byte);
    return (uint32_t)_mm_movemask_epi8(_mm_cmpeq_epi8(group, wanted));
}
#elif defined(SICORE_MAP_NEON)
static inline uint32_t sicore_match_byte(const uint8_t *ctrl, uint8_t byte) {
    static const uint8_t weights_data[16] = { 1, 2, 4, 8, 16, 32, 64, 128,
                                              1, 2, 4, 8, 16, 32, 64, 128 };
    const uint8x16_t group = vld1q_u8(ctrl);
    const uint8x16_t equal = vceqq_u8(group, vdupq_n_u8(byte));
    const uint8x16_t bits = vandq_u8(equal, vld1q_u8(weights_data));
    const uint32_t low = vaddv_u8(vget_low_u8(bits));
    const uint32_t high = vaddv_u8(vget_high_u8(bits));
    return low | (high << 8);
}
#else
static inline uint32_t sicore_match_byte(const uint8_t *ctrl, uint8_t byte) {
    uint32_t mask = 0;
    for (uint32_t i = 0; i < SICORE_GROUP_WIDTH; ++i) {
        mask |= (uint32_t)(ctrl[i] == byte) << i;
    }
    return mask;
}
#endif

static inline uint32_t sicore_max_load(uint32_t capacity) {
    return capacity - (capacity >> 3); /* 87,5 % */
}

static inline uint8_t sicore_hash_h2(uint64_t hash) { return (uint8_t)(hash & UINT64_C(0x7f)); }

static inline uint32_t sicore_hash_group(uint64_t hash, uint32_t group_mask) {
    return (uint32_t)(hash >> 7) & group_mask;
}

static inline void sicore_allocate(sicore_map_t *map, uint32_t capacity) {
    const size_t ctrl_bytes = capacity;
    const size_t entries_bytes = (size_t)capacity * sizeof(sicore_map_entry_t);
    uint8_t *const block = (uint8_t *)malloc(ctrl_bytes + entries_bytes);

    memset(block, SICORE_CTRL_EMPTY, ctrl_bytes);

    map->ctrl = block;
    map->entries = block + ctrl_bytes;
    map->size = 0;
    map->capacity = capacity;
    map->growth_left = sicore_max_load(capacity);
    map->group_mask = (capacity / SICORE_GROUP_WIDTH) - 1u;
}

static inline void sicore_insert_absent_hashed(
    sicore_map_t *map,
    const char *key,
    uint32_t value,
    uint32_t key_length,
    uint64_t hash
) {
    sicore_map_entry_t *const entries = (sicore_map_entry_t *)map->entries;
    const uint8_t h2 = sicore_hash_h2(hash);
    uint32_t group = sicore_hash_group(hash, map->group_mask);
    uint32_t probe = 0;

    for (;;) {
        const uint32_t base = group * SICORE_GROUP_WIDTH;
        const uint32_t empties = sicore_match_byte(map->ctrl + base, SICORE_CTRL_EMPTY);

        if (empties != 0) {
            const uint32_t index = base + sicore_ctz32(empties);
            entries[index].key = key;
            entries[index].value = value;
            entries[index].key_length = key_length;
            map->ctrl[index] = h2;
            ++map->size;
            --map->growth_left;
            return;
        }

        ++probe;
        group = (group + probe) & map->group_mask;
    }
}

static inline uint32_t
sicore_find_index(const sicore_map_t *map, const char *key, uint32_t key_length, uint64_t hash) {
    const sicore_map_entry_t *const entries = (const sicore_map_entry_t *)map->entries;
    const uint8_t h2 = sicore_hash_h2(hash);
    uint32_t group = sicore_hash_group(hash, map->group_mask);
    uint32_t probe = 0;

    for (;;) {
        const uint32_t base = group * SICORE_GROUP_WIDTH;
        uint32_t candidates = sicore_match_byte(map->ctrl + base, h2);

        while (candidates != 0) {
            const uint32_t bit = sicore_ctz32(candidates);
            const uint32_t index = base + bit;
            const char *const candidate_key = entries[index].key;

            if (candidate_key == key || (entries[index].key_length == key_length &&
                                         memcmp(candidate_key, key, key_length) == 0)) {
                return index;
            }
            candidates &= candidates - 1u;
        }

        if (sicore_match_byte(map->ctrl + base, SICORE_CTRL_EMPTY) != 0) {
            return UINT32_MAX;
        }

        ++probe;
        group = (group + probe) & map->group_mask;
    }
}

void sicore_map_init(sicore_map_t *map) { sicore_allocate(map, SICORE_INITIAL_CAPACITY); }

void sicore_map_fini(sicore_map_t *map) { free(map->ctrl); }

SICORE_HOT uint32_t sicore_map_get(const sicore_map_t *map, const char *key) {
    uint32_t key_length;
    const uint64_t hash = sicore_hash_string(key, &key_length);
    const uint32_t index = sicore_find_index(map, key, key_length, hash);
    return index == UINT32_MAX ? UINT32_MAX
                               : ((const sicore_map_entry_t *)map->entries)[index].value;
}

SICORE_HOT bool sicore_map_has(const sicore_map_t *map, const char *key) {
    uint32_t key_length;
    const uint64_t hash = sicore_hash_string(key, &key_length);
    return sicore_find_index(map, key, key_length, hash) != UINT32_MAX;
}

static void sicore_rehash(sicore_map_t *map, uint32_t new_capacity) {
    sicore_map_t rebuilt;
    const uint32_t old_capacity = map->capacity;
    uint8_t *const old_ctrl = map->ctrl;
    sicore_map_entry_t *const old_entries = (sicore_map_entry_t *)map->entries;

    sicore_allocate(&rebuilt, new_capacity);

    for (uint32_t i = 0; i < old_capacity; ++i) {
        if (old_ctrl[i] < SICORE_CTRL_EMPTY) {
            const char *const key = old_entries[i].key;

            sicore_insert_absent_hashed(
                &rebuilt,
                key,
                old_entries[i].value,
                old_entries[i].key_length,
                sicore_hash_bytes((const uint8_t *)key, old_entries[i].key_length)
            );
        }
    }

    free(old_ctrl);
    *map = rebuilt;
}

SICORE_HOT void sicore_map_set(sicore_map_t *map, const char *key, uint32_t value) {
    sicore_map_entry_t *entries = (sicore_map_entry_t *)map->entries;

    uint32_t key_length;
    const uint64_t hash = sicore_hash_string(key, &key_length);
    const uint8_t h2 = sicore_hash_h2(hash);

    uint32_t group = sicore_hash_group(hash, map->group_mask);
    uint32_t probe = 0;
    uint32_t first_deleted = UINT32_MAX;

    for (;;) {
        const uint32_t base = group * SICORE_GROUP_WIDTH;
        uint32_t candidates = sicore_match_byte(map->ctrl + base, h2);

        while (candidates != 0) {
            const uint32_t bit = sicore_ctz32(candidates);
            const uint32_t index = base + bit;
            const char *const candidate_key = entries[index].key;

            if (candidate_key == key || (entries[index].key_length == key_length &&
                                         memcmp(candidate_key, key, key_length) == 0)) {
                entries[index].value = value;
                return;
            }

            candidates &= candidates - 1u;
        }

        if (first_deleted == UINT32_MAX) {
            const uint32_t deleted = sicore_match_byte(map->ctrl + base, SICORE_CTRL_DELETED);

            if (deleted != 0) {
                first_deleted = base + sicore_ctz32(deleted);
            }
        }

        const uint32_t empties = sicore_match_byte(map->ctrl + base, SICORE_CTRL_EMPTY);

        if (empties != 0) {
            if (first_deleted != UINT32_MAX) {
                entries[first_deleted].key = key;
                entries[first_deleted].value = value;
                entries[first_deleted].key_length = key_length;

                map->ctrl[first_deleted] = h2;
                ++map->size;
                return;
            }

            if (SICORE_UNLIKELY(map->growth_left == 0)) {
                const uint32_t max_load = sicore_max_load(map->capacity);

                sicore_rehash(map, map->size < max_load ? map->capacity : map->capacity << 1);

                sicore_insert_absent_hashed(map, key, value, key_length, hash);

                return;
            }

            const uint32_t index = base + sicore_ctz32(empties);

            entries[index].key = key;
            entries[index].value = value;
            entries[index].key_length = key_length;

            map->ctrl[index] = h2;
            ++map->size;
            --map->growth_left;
            return;
        }

        ++probe;
        group = (group + probe) & map->group_mask;
    }
}

SICORE_HOT bool sicore_map_unset(sicore_map_t *map, const char *key) {
    uint32_t key_length;
    const uint64_t hash = sicore_hash_string(key, &key_length);

    const uint32_t index = sicore_find_index(map, key, key_length, hash);

    if (index == UINT32_MAX) {
        return false;
    }

    const uint32_t base = index & ~(SICORE_GROUP_WIDTH - 1u);

    --map->size;

    if (sicore_match_byte(map->ctrl + base, SICORE_CTRL_EMPTY) != 0) {
        map->ctrl[index] = SICORE_CTRL_EMPTY;
        ++map->growth_left;
    } else {
        map->ctrl[index] = SICORE_CTRL_DELETED;
    }

    return true;
}

#endif
