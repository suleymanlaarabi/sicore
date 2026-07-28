#if SICORE_HAS_MAP
#ifndef SICORE_MAP_H
#define SICORE_MAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Hash map spécialisée: chaîne C immuable -> uint32_t.
 *
 * La map ne copie pas les clés. Le pointeur et le contenu de chaque clé doivent
 * rester valides et inchangés jusqu'à sicore_map_fini().
 *
 * La structure est volontairement exposée afin de permettre une allocation
 * directe (pile, composant, arena, etc.) sans indirection vers un objet opaque.
 * Ses champs restent des détails d'implémentation.
 */
typedef struct {
    uint8_t *ctrl;
    void *entries;
    uint32_t size;
    uint32_t capacity;
    uint32_t growth_left;
    uint32_t group_mask;
} sicore_map_t;

void sicore_map_init(sicore_map_t *map);
void sicore_map_fini(sicore_map_t *map);

/* Retourne UINT32_MAX si la clé est absente. */
uint32_t sicore_map_get(const sicore_map_t *map, const char *key);

bool sicore_map_has(const sicore_map_t *map, const char *key);
void sicore_map_set(sicore_map_t *map, const char *key, uint32_t value);

#endif
#endif
