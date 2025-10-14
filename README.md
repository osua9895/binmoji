# Binmoji: A Compact 64-bit Emoji Encoding Library

[![Build Status](https://github.com/jb55/binmoji/actions/workflows/ci.yml/badge.svg)](https://github.com/jb55/binmoji/actions/workflows/ci.yml)

[Specification](./SPEC.md)

**Binmoji** is a C library and command-line tool that encodes any standard Unicode emoji into a single, fixed-size 64-bit integer (`uint64_t`). This provides a highly efficient, compact, and indexable alternative to storing emojis as variable-length UTF-8 strings.

-----

## Key Features ✨

  * **Compact Storage**: Represents any emoji, no matter how complex, as a single `uint64_t`.
  * **High Performance**: Blazing-fast encoding and decoding with minimal overhead.
  * **Lossless Conversion**: Guarantees perfect round-trip conversion from emoji to ID and back.
  * **Unicode Compliant**: The lookup table is generated from the official Unicode emoji data files, ensuring compatibility.
  * **Self-Contained**: Includes a test suite to verify correctness against the Unicode standard.
  * **Small lookup table**: Skin tone variations are flags, leading to a [small multi-component lookup table](./emoji_hash_table.h) (~158 entries)

-----

## How It Works ⚙️

An emoji sequence is deconstructed into its fundamental parts, which are then packed into a 64-bit integer.

| Bits (63-0) | Field Name        | Size (bits) | Description                                                  |
| :---------- | :---------------- | :---------- | :----------------------------------------------------------- |
| `63-42`     | **Primary Codepoint** | 22          | The first base emoji in the sequence (e.g., '👩').            |
| `41-10`     | **Component Hash** | 32          | A CRC-32 hash of all subsequent emojis (e.g., '‍👩‍👧‍👦'). |
| `9-7`       | **Skin Tone 1** | 3           | The first skin tone modifier.                                |
| `6-4`       | **Skin Tone 2** | 3           | The second skin tone modifier (for couple/family emojis).    |
| `3-0`       | **Flags** | 4           | Reserved for future use.                                     |

Because hashing is a one-way process, a pre-computed lookup table (`emoji_hash_table.h`) is used to map a **Component Hash** back to its original list of component codepoints during decoding.

-----

## Building the Project 🛠️

Just type `make` with a C compiler. You can regenerate the small ZWJ sequence hash table by getting the txt files from https://www.unicode.org/Public/17.0.0/emoji/. We also provide a snapshot of these in the repository for convenience.

-----

## Usage 🚀

The compiled `binmoji` executable can be used for encoding, decoding, and testing.

### Encode an Emoji to a 64-bit ID

Pass the emoji string as an argument to get its unique Binmoji ID. The tool handles everything from simple icons to complex ZWJ sequences with multiple skin tones.

#### **Example 1: A simple emoji**

The simplest emojis have no components or modifiers.

```bash
./binmoji ❤️
0x009D918FB6174C00
```

#### **Example 2: A ZWJ sequence**

The pirate flag is formed by joining 🏴 and ☠️ with a ZWJ.

```bash
./binmoji 🏴‍☠️
0x07CFD3F0E9125C00
```

#### **Example 3: An emoji with a single skin tone**

Here, a dark skin tone is applied to the astronaut.

```bash
./binmoji 🧑🏿‍🚀
0x07E746E4F8DD5680
```

#### **Example 4: An emoji with two different skin tones**

This complex sequence requires storing two separate skin tones.

```bash
./binmoji 👩🏻‍🤝‍👩🏿
0x07D1A7747240B0D0
```

### Decode a 64-bit ID to an Emoji

To convert a Binmoji ID back to its emoji string, pass the hex ID (prefixed with `0x`) as an argument.

```bash
# Decode the dual skin-tone emoji
./binmoji 0x07D1A7747240B0D0
👩🏻‍🤝‍👩🏿

# Decode the pirate flag
./binmoji 0x07CFD3F0E9125C00
🏴‍☠️
```

### Run the Test Suite

To verify the implementation, run the built-in test suite. It performs a round-trip conversion test on thousands of emojis from the official Unicode data files.

```bash
./binmoji --test
```

A successful run will report zero failures.

-----

## Library API Overview

You can integrate the core logic into your own C projects by including the header `binmoji.h` and compiling/linking `binmoji.c`. The main data structure is `struct binmoji`.

The main functions are:

* `void binmoji_parse(const char *emoji, struct binmoji *binmoji);`
    * Parses a UTF-8 emoji string into the `struct binmoji`.
* `uint64_t binmoji_encode(const struct binmoji *binmoji);`
    * Generates a 64-bit ID from a populated `struct binmoji`.
* `void binmoji_decode(uint64_t id, struct binmoji *binmoji);`
    * Decodes a 64-bit ID into a `struct binmoji`, using an internal hash table to look up components.
* `void binmoji_to_string(const struct binmoji *binmoji, char *out_str, size_t out_str_size);`
    * Builds the final UTF-8 emoji string from a `struct binmoji`.
