## **Binmoji: A 64-bit Compact Emoji Representation Format**

### 1. Overview

The **Binmoji** format provides a method for encoding any standard Unicode emoji sequence into a single, fixed-size 64-bit unsigned integer (`uint64_t`). This offers a compact, memory-efficient, and easily indexable alternative to variable-length UTF-8 strings.

The core principle is to deconstruct an emoji sequence into its essential parts:
* A single **primary codepoint**.
* Up to two **skin tone modifiers**.
* A variable-length list of **component codepoints**.
* A set of **flags** for metadata.

To fit the variable-length component list into the fixed-size integer, the list is hashed. The original component sequence is then retrieved during decoding using a pre-computed lookup table that maps the hash back to its corresponding codepoint list.

---

### 2. The 64-bit Binmoji ID Structure

The `uint64_t` ID is a bitfield composed of five distinct sections, arranged from most-significant bit (MSB) to least-significant bit (LSB).

| Bits (63-0) | Field Name | Size (bits) | Description |
| :--- | :--- | :--- | :--- |
| `63-42` | **Primary Codepoint** | 22 | The first base emoji codepoint in the sequence. |
| `41-10` | **Component Hash** | 32 | A CRC-32 hash of the subsequent component codepoints. |
| `9-7` | **Skin Tone 1** | 3 | The first skin tone modifier applied to the primary codepoint. |
| `6-4` | **Skin Tone 2** | 3 | The second skin tone modifier, typically for multi-person emojis. |
| `3-0` | **Flags** | 4 | Reserved for future metadata (e.g., gender, variation selectors). |



---

### 3. Component Definitions

#### 3.1. Primary Codepoint
The **Primary Codepoint** is defined as the first non-modifier Unicode codepoint encountered when parsing the emoji string from left to right. Modifier codepoints include skin tones (`0x1F3FB`-`0x1F3FF`) and the Zero-Width Joiner (`0x200D`).

* **Example:** In "👩‍👩‍👧‍👦" (`1F469 200D 1F469 200D 1F467 200D 1F466`), the primary codepoint is `0x1F469` (👩 Woman).

#### 3.2. Component List & Hash
The **Component List** contains all subsequent non-modifier codepoints that follow the primary codepoint. This list can contain up to 16 codepoints. The Zero-Width Joiner (`0x200D`) is not stored in this list but is implied during reconstruction between components.

To encode this list into the 64-bit ID, a **Component Hash** is calculated using the following algorithm:

* **Algorithm:** CRC-32 (Cyclic Redundancy Check)
* **Polynomial:** $0x04C11DB7$
* **Initial Value:** $0xFFFFFFFF$
* **Input Data:** The array of `uint32_t` codepoints in the component list.
* **Output:** A 32-bit hash value.

If an emoji has no components (e.g., "❤️"), the component list is empty and the hash is `0`.

#### 3.3. Skin Tones
The format supports up to two skin tone modifiers. The codepoints `0x1F3FB` through `0x1F3FF` are mapped to integer values 1 through 5, respectively. A value of `0` indicates no skin tone.

* `skin_tone1`: The first skin tone encountered. It typically modifies the primary codepoint.
* `skin_tone2`: The second skin tone encountered. It typically modifies the last component in a multi-person sequence.

| Value | Unicode Codepoint | Description |
| :--- | :--- | :--- |
| `1` | `U+1F3FB` | Light Skin Tone |
| `2` | `U+1F3FC` | Medium-Light Skin Tone |
| `3` | `U+1F3FD` | Medium Skin Tone |
| `4` | `U+1F3FE` | Medium-Dark Skin Tone |
| `5` | `U+1F3FF` | Dark Skin Tone |

#### 3.4. Flags
The 4-bit `flags` field is reserved for future use or for metadata that cannot be represented by the component list. The current implementation does not actively use this field for encoding.

---

### 4. Encoding Process (Emoji String → 64-bit ID)

1.  **Parse String:** The input UTF-8 emoji string is parsed into a sequence of Unicode codepoints.
2.  **Deconstruct:** The codepoint sequence is iterated through to populate an `EmojiComponents` structure:
    * The first valid base emoji is stored as `primary_cp`.
    * Subsequent base emojis are added to the `component_list`.
    * The first two skin tone modifiers found are converted to integers (1-5) and stored in `skin_tone1` and `skin_tone2`.
    * The Zero-Width Joiner (`0x200D`) acts as a delimiter and is not stored.
3.  **Hash Components:** If the `component_list` is not empty, the CRC-32 hash is calculated on it and stored as `component_hash`.
4.  **Assemble ID:** The final 64-bit ID is assembled by bit-shifting each field into its correct position and combining them with a bitwise OR operation.

---

### 5. Decoding Process (64-bit ID → Emoji String)

1.  **Extract Fields:** The `primary_cp`, `component_hash`, `skin_tone1`, `skin_tone2`, and `flags` are extracted from the 64-bit ID using bitmasks and shifts.
2.  **Lookup Components:** If `component_hash` is not zero, it is used as a key to look up the original `component_list` in the pre-computed `emoji_hash_table.h`. If the hash is not found, the emoji cannot be fully reconstructed.
3.  **Reconstruct String:** The final UTF-8 emoji string is built in sequence:
    a. Append the `primary_cp`.
    b. Append the skin tone modifier corresponding to `skin_tone1` (if not 0).
    c. For each codepoint in the retrieved `component_list`:
        i. Prepend a Zero-Width Joiner (`0x200D`), with exceptions for sequences that do not use it (e.g., country and subdivision flags).
        ii. Append the component codepoint.
    d. If `skin_tone2` is set and the emoji is a multi-person sequence, append its corresponding skin tone modifier at the end.
    e. The resulting buffer is null-terminated.

---

### 6. Toolchain and Dependencies

The Binmoji format relies on a pre-computed lookup table to reverse the component hash.

#### `generate_hash_table.py`
This Python script is responsible for creating the `emoji_hash_table.h` C header file. Its process is:
1.  Downloads the official emoji data files (`emoji-sequences.txt`, `emoji-zwj-sequences.txt`, etc.) from the Unicode Consortium for a specific version (e.g., 15.1).
2.  Parses every valid emoji sequence from these files.
3.  For each sequence with components, it calculates the CRC-32 hash using a Python implementation identical to the C version.
4.  It stores each unique hash and its corresponding component list.
5.  Finally, it writes all unique hash-component pairs into a static C array (`EmojiHashEntry emoji_hash_table[]`) in the header file, sorted by hash value.

#### `emoji_hash_table.h`
This auto-generated file contains the static lookup table. This table is essential for the `decode_emoji_id` function to operate correctly. Any application using the Binmoji decoder must be compiled with this header.
