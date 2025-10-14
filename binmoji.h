
#ifndef BINMOJI_H
#define BINMOJI_H

#include <stdint.h>
#include <stdlib.h>

/* ## 2. STRUCTS AND HASHING (Unchanged) */
struct binmoji {
	uint32_t primary_cp;
	uint32_t component_list[16];
	size_t component_count;
	uint32_t component_hash;
	uint8_t skin_tone1;
	uint8_t skin_tone2;
	uint8_t flags;
};

void binmoji_to_string(const struct binmoji *binmoji, char *out_str, size_t out_str_size);
void binmoji_decode(uint64_t id, struct binmoji *binmoji);
void binmoji_parse(const char *emoji, struct binmoji *binmoji);
uint64_t binmoji_encode(const struct binmoji *binmoji);


#endif /* BINMOJI_H */
