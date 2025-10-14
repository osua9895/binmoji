
#include "binmoji.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

static int append_utf8(char *buf, size_t buf_size, size_t *offset, uint32_t cp)
{
	char *p;
	int bytes_to_write = 0;

	if (!buf)
		return 0;
	if (cp < 0x80)
		bytes_to_write = 1;
	else if (cp < 0x800)
		bytes_to_write = 2;
	else if (cp < 0x10000)
		bytes_to_write = 3;
	else if (cp < 0x110000)
		bytes_to_write = 4;
	else
		return 0;
	if (*offset + bytes_to_write >= buf_size)
		return 0;

	p = buf + *offset;
	if (bytes_to_write == 1) {
		*p = (char)cp;
	} else if (bytes_to_write == 2) {
		p[0] = 0xC0 | (cp >> 6);
		p[1] = 0x80 | (cp & 0x3F);
	} else if (bytes_to_write == 3) {
		p[0] = 0xE0 | (cp >> 12);
		p[1] = 0x80 | ((cp >> 6) & 0x3F);
		p[2] = 0x80 | (cp & 0x3F);
	} else {
		p[0] = 0xF0 | (cp >> 18);
		p[1] = 0x80 | ((cp >> 12) & 0x3F);
		p[2] = 0x80 | ((cp >> 6) & 0x3F);
		p[3] = 0x80 | (cp & 0x3F);
	}
	*offset += bytes_to_write;
	return bytes_to_write;
}

static int run_test_suite(const char *filename)
{
	char line[512], *emoji_end, *hash_char, *emoji_start,
	    original_char_at_end;
	const char *orig_emoji;
	int tests_passed = 0;
	int tests_failed = 0;
	int line_num = 0;
	struct binmoji binmoji, original_binmoji, decoded_binmoji;
	uint32_t start_cp, end_cp, cp;
	uint64_t id;
	size_t offset;
	char original_emoji[8];
	char reconstructed_emoji[64];
	FILE *f = fopen(filename, "r");

	if (!f) {
		perror(filename);
		return 1;
	}

	printf("\n--- Running Test Suite from: %s ---\n", filename);

	while (fgets(line, sizeof(line), f)) {
		line_num++;
		if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
			continue;
		}

		if (strstr(line, "..")) {
			if (sscanf(line, "%X..%X", &start_cp, &end_cp) == 2) {
				for (cp = start_cp; cp <= end_cp; ++cp) {
					memset(original_emoji, 0,
					       sizeof(original_emoji));
					offset = 0;
					append_utf8(original_emoji,
						    sizeof(original_emoji),
						    &offset, cp);

					/* memset(binmoji, 0,
					 * sizeof(binmoji));*/
					memset(reconstructed_emoji, 0,
					       sizeof(reconstructed_emoji));

					binmoji_parse(original_emoji, &binmoji);
					id = binmoji_encode(&binmoji);
					binmoji_decode(id, &binmoji);
					binmoji_to_string(
					    &binmoji, reconstructed_emoji,
					    sizeof(reconstructed_emoji));

					if (strcmp(original_emoji,
						   reconstructed_emoji) != 0) {
						printf(
						    "FAIL (Line %d, CP: %X): "
						    "Original: \"%s\" -> "
						    "Reconstructed: \"%s\" "
						    "(ID: 0x%016" PRIX64 ")\n",
						    line_num, cp,
						    original_emoji,
						    reconstructed_emoji, id);
						tests_failed++;
					} else {
						tests_passed++;
					}
				}
			}
		} else {
			hash_char = strchr(line, '#');
			if (!hash_char)
				continue;

			emoji_start = hash_char + 1;
			while (*emoji_start != '\0' &&
			       (unsigned char)*emoji_start < 0x80) {
				emoji_start++;
			}
			if (*emoji_start == '\0' || *emoji_start == '\n' ||
			    *emoji_start == '\r')
				continue;

			emoji_end = emoji_start;
			while (*emoji_end != '\0' && *emoji_end != ' ' &&
			       *emoji_end != '\t' && *emoji_end != ')' &&
			       *emoji_end != '\n' && *emoji_end != '\r') {
				emoji_end++;
			}

			original_char_at_end = *emoji_end;
			*emoji_end = '\0';
			orig_emoji = emoji_start;

			memset(reconstructed_emoji, 0,
			       sizeof(reconstructed_emoji));

			binmoji_parse(orig_emoji, &original_binmoji);
			id = binmoji_encode(&original_binmoji);
			binmoji_decode(id, &decoded_binmoji);
			binmoji_to_string(&decoded_binmoji, reconstructed_emoji,
					  sizeof(reconstructed_emoji));

			if (strcmp(orig_emoji, reconstructed_emoji) != 0) {
				printf("FAIL (Line %d): Original: \"%s\" -> "
				       "Reconstructed: \"%s\" (ID: 0x%016" PRIX64 ")\n",
				       line_num, orig_emoji,
				       reconstructed_emoji, id);
				tests_failed++;
			} else {
				tests_passed++;
			}
			*emoji_end = original_char_at_end;
		}
	}

	fclose(f);
	printf("--- Results for %s: %d Passed, %d Failed ---\n", filename,
	       tests_passed, tests_failed);
	return tests_failed;
}

static int run_all_tests(void)
{
	size_t i;
	const char *test_files[] = {
	    "emoji-sequences.txt",
	    "emoji-test.txt",
	    "emoji-zwj-sequences.txt",
	};
	int total_failures = 0;

	printf("🚀 Starting comprehensive emoji round-trip tests...\n");

	for (i = 0; i < sizeof(test_files) / sizeof(test_files[0]); i++) {
		total_failures += run_test_suite(test_files[i]);
	}

	if (total_failures == 0) {
		printf("\n✅✅✅ All test suites passed! ✅✅✅\n");
		return EXIT_SUCCESS;
	} else {
		printf(
		    "\n❌❌❌ Found %d failures in the test suites. ❌❌❌\n",
		    total_failures);
		return EXIT_FAILURE;
	}
}

int main(int argc, char *argv[])
{
	const char *input;
	char *endptr;
	uint64_t id;
	struct binmoji binmoji;
	char emoji_buffer[64];

	/* Check for --test flag to run the test suite */
	if (argc == 2 && strcmp(argv[1], "--test") == 0) {
		return run_all_tests();
	}

	/* For conversions, exactly one argument is required */
	if (argc != 2) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr,
			"  %s \"<emoji>\"       (to convert emoji to hex ID)\n",
			argv[0]);
		fprintf(stderr,
			"  %s <0x_hex_id>     (to convert hex ID to emoji)\n",
			argv[0]);
		fprintf(stderr,
			"  %s --test          (to run the test suite)\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	input = argv[1];

	/* Check if the input is a hex ID (must start with "0x") */
	if (strncmp(input, "0x", 2) == 0) {
		/* --- DECODE from Hex ID to Emoji --- */
		id = strtoul(input, &endptr, 16);

		if (*endptr != '\0') {
			fprintf(stderr,
				"Error: Invalid hexadecimal ID format: %s\n",
				input);
			return EXIT_FAILURE;
		}

		memset(emoji_buffer, 0, sizeof(emoji_buffer));

		binmoji_decode(id, &binmoji);
		binmoji_to_string(&binmoji, emoji_buffer, sizeof(emoji_buffer));

		printf("%s\n", emoji_buffer);

	} else {
		/* --- ENCODE from Emoji to Hex ID --- */
		binmoji_parse(input, &binmoji);
		id = binmoji_encode(&binmoji);

		printf("0x%016" PRIX64 "\n", id);
	}

	return EXIT_SUCCESS;
}
