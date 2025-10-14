#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ## 1. DEFINITIONS (Unchanged)
#define PRIMARY_CP_SHIFT 42
#define HASH_SHIFT 10
#define TONE1_SHIFT 7
#define TONE2_SHIFT 4
#define FLAGS_SHIFT 0

#define PRIMARY_CP_MASK 0x3FFFFF
#define HASH_MASK 0xFFFFFFFF
#define TONE_MASK 0x7
#define FLAGS_MASK 0xF

// ## 2. STRUCTS AND HASHING (Unchanged)
typedef struct {
    uint32_t primary_cp;
    uint32_t component_list[16];
    size_t component_count;
    uint32_t component_hash;
    uint8_t skin_tone1;
    uint8_t skin_tone2;
    uint8_t flags;
} EmojiComponents;

uint32_t crc32(const uint32_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    if (data == NULL || length == 0) return 0;
    for (size_t i = 0; i < length; ++i) {
        uint32_t item = data[i];
        // Use a standard table-based CRC32 for production, this is slow but simple
        for (int j = 0; j < 32; ++j) {
            uint32_t bit = (item >> (31 - j)) & 1;
            if ((crc >> 31) ^ bit) {
                crc = (crc << 1) ^ 0x04C11DB7;
            } else {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

int is_base_emoji(uint32_t cp) {
    if (cp >= 0x1F3FB && cp <= 0x1F3FF) return 0;
    if (cp == 0x200D || cp == 0xFE0F) return 0;
    if (cp == 0x2640 || cp == 0x2642) return 0;
    return 1;
}

// ## 3. PARSER (Unchanged)
void parse_emoji_string(const char *emoji_str, EmojiComponents *components) {
    memset(components, 0, sizeof(EmojiComponents));
    const unsigned char *s = (const unsigned char *)emoji_str;
    while (*s) {
        uint32_t cp = 0;
        int len = 0;
        if (*s < 0x80) { len = 1; cp = s[0]; }
        else if ((*s & 0xE0) == 0xC0) { len = 2; cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F); }
        else if ((*s & 0xF0) == 0xE0) { len = 3; cp = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F); }
        else if ((*s & 0xF8) == 0xF0) { len = 4; cp = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F); }
        else { s++; continue; }
        s += len;

        if (cp >= 0x1F3FB && cp <= 0x1F3FF) {
            uint8_t tone_val = (cp - 0x1F3FB) + 1;
            if (components->skin_tone1 == 0) components->skin_tone1 = tone_val;
            else if (components->skin_tone2 == 0) components->skin_tone2 = tone_val;
        } else if (cp == 0x2642) {
            components->flags |= 1; // Male flag
        } else if (cp == 0x2640) {
            components->flags |= 2; // Female flag
        } else if (cp == 0xFE0F) {
            if (components->primary_cp != 0 && components->component_count == 0) {
                components->flags |= 4; // VS16 flag
            }
        } else if (is_base_emoji(cp)) {
            if (components->primary_cp == 0) {
                components->primary_cp = cp;
            } else if (components->component_count < 16) {
                components->component_list[components->component_count++] = cp;
            }
        }
    }
    components->component_hash = crc32(components->component_list, components->component_count);
}


// ## 4. ID GENERATOR AND LOOKUP TABLE (Updated to use generated header)
uint64_t generate_emoji_id(const EmojiComponents *components) {
    uint64_t id = 0;
    id |= ((uint64_t)(components->primary_cp & PRIMARY_CP_MASK) << PRIMARY_CP_SHIFT);
    id |= ((uint64_t)(components->component_hash & HASH_MASK) << HASH_SHIFT);
    id |= ((uint64_t)(components->skin_tone1 & TONE_MASK) << TONE1_SHIFT);
    id |= ((uint64_t)(components->skin_tone2 & TONE_MASK) << TONE2_SHIFT);
    id |= ((uint64_t)(components->flags & FLAGS_MASK) << FLAGS_SHIFT);
    return id;
}

typedef struct {
    uint32_t hash;
    size_t count;
    uint32_t components[16];
} EmojiHashEntry;

// Include the auto-generated hash table
#include "emoji_hash_table.h"

const size_t num_hash_entries = sizeof(emoji_hash_table) / sizeof(emoji_hash_table[0]);

int lookup_components_by_hash(uint32_t hash, uint32_t *out_components, size_t *out_count) {
    // This could be optimized with a binary search if the table is sorted by hash
    for (size_t i = 0; i < num_hash_entries; ++i) {
        if (emoji_hash_table[i].hash == hash) {
            *out_count = emoji_hash_table[i].count;
            memcpy(out_components, emoji_hash_table[i].components, (*out_count) * sizeof(uint32_t));
            return 1;
        }
    }
    *out_count = 0;
    return 0;
}

void decode_emoji_id(uint64_t id, EmojiComponents *components) {
    memset(components, 0, sizeof(EmojiComponents));
    components->primary_cp = (id >> PRIMARY_CP_SHIFT) & PRIMARY_CP_MASK;
    components->component_hash = (id >> HASH_SHIFT) & HASH_MASK;
    components->skin_tone1 = (id >> TONE1_SHIFT) & TONE_MASK;
    components->skin_tone2 = (id >> TONE2_SHIFT) & TONE_MASK;
    components->flags = (id >> FLAGS_SHIFT) & FLAGS_MASK;
    if (components->component_hash != 0) {
        lookup_components_by_hash(components->component_hash,
                                  components->component_list,
                                  &components->component_count);
    }
}

// ## 5. RECONSTRUCTOR (Unchanged)
int append_utf8(char *buf, size_t buf_size, size_t *offset, uint32_t cp) {
    if (!buf) return 0;
    int bytes_to_write = 0;
    if (cp < 0x80) bytes_to_write = 1;
    else if (cp < 0x800) bytes_to_write = 2;
    else if (cp < 0x10000) bytes_to_write = 3;
    else if (cp < 0x110000) bytes_to_write = 4;
    else return 0;
    if (*offset + bytes_to_write >= buf_size) return 0;

    char *p = buf + *offset;
    if (bytes_to_write == 1) { *p = (char)cp; }
    else if (bytes_to_write == 2) { p[0] = 0xC0 | (cp >> 6); p[1] = 0x80 | (cp & 0x3F); }
    else if (bytes_to_write == 3) { p[0] = 0xE0 | (cp >> 12); p[1] = 0x80 | ((cp >> 6) & 0x3F); p[2] = 0x80 | (cp & 0x3F); }
    else { p[0] = 0xF0 | (cp >> 18); p[1] = 0x80 | ((cp >> 12) & 0x3F); p[2] = 0x80 | ((cp >> 6) & 0x3F); p[3] = 0x80 | (cp & 0x3F); }
    *offset += bytes_to_write;
    return bytes_to_write;
}

void reconstruct_emoji_string(const EmojiComponents *components, char *out_str, size_t out_str_size) {
    if (!components || !out_str || out_str_size == 0) return;
    size_t offset = 0;
    out_str[0] = '\0';

    if (components->primary_cp > 0) {
        append_utf8(out_str, out_str_size, &offset, components->primary_cp);
    }
    if (components->flags & 4) { // VS16 flag
        append_utf8(out_str, out_str_size, &offset, 0xFE0F);
    }
    if (components->skin_tone1 > 0) {
        append_utf8(out_str, out_str_size, &offset, 0x1F3FB + components->skin_tone1 - 1);
    }
    for (size_t i = 0; i < components->component_count; i++) {
        append_utf8(out_str, out_str_size, &offset, 0x200D); // ZWJ
        append_utf8(out_str, out_str_size, &offset, components->component_list[i]);
        if (i == 0 && components->skin_tone2 > 0) {
            append_utf8(out_str, out_str_size, &offset, 0x1F3FB + components->skin_tone2 - 1);
        }
    }

    if (offset < out_str_size) out_str[offset] = '\0';
    else if (out_str_size > 0) out_str[out_str_size - 1] = '\0';
}


// Function to run tests from a single Unicode data file (FINAL, MORE ROBUST VERSION)
// Function to run tests from a single Unicode data file (FINAL, MORE ROBUST VERSION)
int run_test_suite(const char* filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror(filename);
        return 1;
    }

    printf("\n--- Running Test Suite from: %s ---\n", filename);
    char line[512];
    int tests_passed = 0;
    int tests_failed = 0;
    int line_num = 0;

    while (fgets(line, sizeof(line), f)) {
        line_num++;
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        // Find the '#' character which precedes the emoji information
        char* emoji_comment = strchr(line, '#');
        if (!emoji_comment) continue;

        // **FIX STARTS HERE: New robust parsing logic**
        // Scan forward from '#' to find the first multi-byte character (the emoji).
        // This correctly skips all ASCII annotations like "E17.0" and "[1]".
        char* emoji_start = emoji_comment + 1;
        while (*emoji_start != '\0' && (unsigned char)*emoji_start < 0x80) {
            // Some emojis are wrapped in parentheses, so if we find one,
            // we start looking from the character inside it.
            if (*emoji_start == '(') {
                emoji_start++;
                break;
            }
            emoji_start++;
        }

        // If we hit the end of the line, no emoji was found.
        if (*emoji_start == '\0' || *emoji_start == '\n' || *emoji_start == '\r') continue;

        // Find the end of the emoji string. It ends at the next space, tab,
        // or a closing parenthesis.
        char* emoji_end = emoji_start;
        while (*emoji_end != '\0' && *emoji_end != ' ' && *emoji_end != '\t' && *emoji_end != ')') {
            emoji_end++;
        }
        // **FIX ENDS HERE**
        
        // Temporarily null-terminate to isolate the emoji string
        char original_char_at_end = *emoji_end;
        *emoji_end = '\0';
        const char *original_emoji = emoji_start;

        // --- Perform the round-trip test ---
        EmojiComponents original_components, decoded_components;
        char reconstructed_emoji[64] = {0};

        parse_emoji_string(original_emoji, &original_components);
        uint64_t id = generate_emoji_id(&original_components);
        decode_emoji_id(id, &decoded_components);
        reconstruct_emoji_string(&decoded_components, reconstructed_emoji, sizeof(reconstructed_emoji));

        if (strcmp(original_emoji, reconstructed_emoji) != 0) {
            printf("FAIL (Line %d): Original: \"%s\" -> Reconstructed: \"%s\" (ID: 0x%016lX)\n",
                line_num, original_emoji, reconstructed_emoji, id);
            tests_failed++;
        } else {
            tests_passed++;
        }

        // Restore the line so we don't modify the buffer
        *emoji_end = original_char_at_end;
    }

    fclose(f);

    printf("--- Results for %s: %d Passed, %d Failed ---\n", filename, tests_passed, tests_failed);
    return tests_failed;
}

int main() {
    // List of official Unicode data files to test against.
    // The Python script will download these for you.
    const char* test_files[] = {
        "emoji-sequences.txt",
        "emoji-zwj-sequences.txt",
        // "emoji-test.txt" is very comprehensive and can also be added here.
    };
    int total_failures = 0;

    printf("🚀 Starting comprehensive emoji round-trip tests...\n");

    for (int i = 0; i < sizeof(test_files) / sizeof(test_files[0]); i++) {
        total_failures += run_test_suite(test_files[i]);
    }

    if (total_failures == 0) {
        printf("\n✅✅✅ All test suites passed! ✅✅✅\n");
        return EXIT_SUCCESS;
    } else {
        printf("\n❌❌❌ Found %d failures in the test suites. ❌❌❌\n", total_failures);
        return EXIT_FAILURE;
    }
}
