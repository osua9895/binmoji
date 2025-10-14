# Binmoji: A Compact 64-bit Emoji Encoding Library

[Specification](./SPEC.md)

[](https://opensource.org/licenses/MIT)

**Binmoji** is a C library and command-line tool that encodes any standard Unicode emoji into a single, fixed-size 64-bit integer (`uint64_t`). This provides a highly efficient, compact, and indexable alternative to storing emojis as variable-length UTF-8 strings.

-----

## Key Features ✨

  * **Compact Storage**: Represents any emoji, no matter how complex, as a single `uint64_t`.
  * **High Performance**: Blazing-fast encoding and decoding with minimal overhead.
  * **Lossless Conversion**: Guarantees perfect round-trip conversion from emoji to ID and back.
  * **Unicode Compliant**: The lookup table is generated from the official Unicode emoji data files, ensuring compatibility.
  * **Self-Contained**: Includes a test suite to verify correctness against the Unicode standard.

-----

## How It Works ⚙️

An emoji sequence is deconstructed into its fundamental parts, which are then packed into a 64-bit integer.

| Bits (63-0) | Field Name | Size (bits) | Description |
| :--- | :--- | :--- | :--- |
| `63-42` | **Primary Codepoint** | 22 | The first base emoji in the sequence (e.g., '👩'). |
| `41-10` | **Component Hash** | 32 | A CRC-32 hash of all subsequent emojis (e.g., '‍👩‍👧‍👦'). |
| `9-7` | **Skin Tone 1** | 3 | The first skin tone modifier. |
| `6-4` | **Skin Tone 2** | 3 | The second skin tone modifier (for couple/family emojis). |
| `3-0` | **Flags** | 4 | Reserved for future use. |

Because hashing is a one-way process, a pre-computed lookup table (`emoji_hash_table.h`) is used to map a **Component Hash** back to its original list of component codepoints during decoding.

-----

## Building the Project 🛠️

The project requires a one-time setup step to generate the emoji hash table.

1.  **Generate the Hash Table**: Run the Python script to download the latest Unicode emoji data and create the C header file.

    ```bash
    python3 generate_hash_table.py
    ```

    This will create `emoji_hash_table.h` and download the necessary `emoji-*.txt` files.

2.  **Compile the C Code**: Use a C compiler like GCC to build the command-line tool.

    ```bash
    gcc -o binmoji binmoji.c -O2 -Wall
    ```

-----

## Usage 🚀

The compiled `binmoji` executable can be used for encoding, decoding, and testing.

### Encode an Emoji to a 64-bit ID

To convert an emoji string to its Binmoji ID, pass the emoji as an argument.

```bash
./binmoji "❤️"
# Emoji: "❤️"
# ID:    0x0000000000000000

./binmoji "👩‍👩‍👧‍👦"
# Emoji: "👩‍👩‍👧‍👦"
# ID:    0x1271D50C48000000

./binmoji "👍🏾"
# Emoji: "👍🏾"
# ID:    0x1250400000000200
```

### Decode a 64-bit ID to an Emoji

To convert a Binmoji ID back to its emoji string, pass the hex ID (prefixed with `0x`) as an argument.

```bash
./binmoji 0x1271D50C48000000
# ID:    0x1271d50c48000000
# Emoji: "👩‍👩‍👧‍👦"

./binmoji 0x1250400000000200
# ID:    0x1250400000000200
# Emoji: "👍🏾"
```

### Run the Test Suite

To verify the implementation, run the built-in test suite. It performs a round-trip conversion test on thousands of emojis from the official Unicode data files.

```bash
./binmoji --test
```

A successful run will report zero failures.

-----

## Library API Overview

You can integrate the core logic into your own C/C++ projects by including `binmoji.c` and `emoji_hash_table.h`.

The main functions are:

  * `void parse_emoji_string(const char *emoji_str, EmojiComponents *components);`
      * Parses a UTF-8 string into the `EmojiComponents` struct.
  * `uint64_t generate_emoji_id(const EmojiComponents *components);`
      * Generates a 64-bit ID from a populated `EmojiComponents` struct.
  * `void decode_emoji_id(uint64_t id, EmojiComponents *components);`
      * Decodes a 64-bit ID into an `EmojiComponents` struct, using the hash table to look up components.
  * `void reconstruct_emoji_string(const EmojiComponents *components, char *out_str, size_t out_str_size);`
      * Builds the final UTF-8 emoji string from a decoded `EmojiComponents` struct.

-----

## License

This project is licensed under the MIT License. See the LICENSE file for details.
