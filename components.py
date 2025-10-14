import urllib.request
import os
import sys

# -----------------------------------------------------------------------------
# 1. REPLICATION OF THE C LOGIC
# -----------------------------------------------------------------------------

def crc32(data: list[int]) -> int:
    """
    Calculates a 32-bit CRC hash on a list of 32-bit integers (codepoints).

    This is a direct Python port of the bitwise CRC32 implementation
    from the provided C code, using the same polynomial (0x04C11DB7).
    """
    crc = 0xFFFFFFFF
    poly = 0x04C11DB7

    if not data:
        return 0

    for item in data:
        # Process each bit of the 32-bit integer codepoint
        for j in range(32):
            bit = (item >> (31 - j)) & 1
            if ((crc >> 31) & 1) != bit:
                # XOR with polynomial if the leading bits differ
                crc = ((crc << 1) ^ poly)
            else:
                crc = (crc << 1)
            # Ensure crc remains a 32-bit unsigned integer
            crc &= 0xFFFFFFFF
            
    return crc

def is_base_emoji(cp: int) -> bool:
    """
    Checks if a codepoint is a base emoji character, mirroring the C helper.
    Returns False for modifiers like skin tones, ZWJ, and gender signs.
    """
    if 0x1F3FB <= cp <= 0x1F3FF:  # Skin Tones
        return False
    if cp in [0x200D, 0xFE0F]:    # ZWJ, Variation Selector
        return False
    if cp in [0x2640, 0x2642]:    # Gender signs (Female, Male)
        return False
    return True

def parse_emoji_string(emoji_str: str) -> dict:
    """
    Parses a UTF-8 emoji string into its components, just like the C code.
    It identifies the primary codepoint, components, skin tones, and flags.
    """
    components = {
        'primary_cp': 0,
        'component_list': [],
        'skin_tone1': 0,
        'skin_tone2': 0,
        'flags': 0,
        'hash': 0
    }
    
    codepoints = [ord(c) for c in emoji_str]

    for cp in codepoints:
        if 0x1F3FB <= cp <= 0x1F3FF:
            # Skin tones are 1-5, not the raw codepoint
            tone_val = (cp - 0x1F3FB) + 1
            if components['skin_tone1'] == 0:
                components['skin_tone1'] = tone_val
            elif components['skin_tone2'] == 0:
                components['skin_tone2'] = tone_val
        elif cp == 0x2642:  # Male sign
            components['flags'] |= 1
        elif cp == 0x2640:  # Female sign
            components['flags'] |= 2
        elif is_base_emoji(cp):
            if components['primary_cp'] == 0:
                components['primary_cp'] = cp
            elif len(components['component_list']) < 16:
                components['component_list'].append(cp)

    # Calculate hash from the collected component list
    if components['component_list']:
        components['hash'] = crc32(components['component_list'])
        
    return components

# -----------------------------------------------------------------------------
# 2. EMOJI DATA PROCESSING
# -----------------------------------------------------------------------------

def get_emoji_test_file(url: str, filename: str) -> None:
    """Downloads the emoji-test.txt file if it doesn't exist."""
    if not os.path.exists(filename):
        print(f"Downloading {filename} from Unicode Consortium...")
        try:
            urllib.request.urlretrieve(url, filename)
            print("Download complete.")
        except Exception as e:
            print(f"Error downloading file: {e}")
            exit(1)

def main():
    """
    Main function to process the emoji data file and generate hashes.
    """
    UNICODE_EMOJI_URL = "https://unicode.org/Public/emoji/15.1/emoji-test.txt"
    FILE_NAME = "emoji-test.txt"
    
    get_emoji_test_file(UNICODE_EMOJI_URL, FILE_NAME)
    
    sys.stderr.write("\n--- Generating Hashes for ZWJ Emoji Sequences ---\n")
    sys.stderr.write("This will process the emoji-test.txt file and print the hash for any emoji with components.\n")
    sys.stderr.write(f"{'Emoji':<20}\t{'Primary CP':<12}\t{'Components':<35}\t{'Hash (Hex)'}\n")
    sys.stderr.write("-" * 90)
    sys.stderr.write("\n")

    count = 0
    with open(FILE_NAME, 'r', encoding='utf-8') as f:
        for line in f:
            # Skip comments and lines that are not fully-qualified emojis
            if line.startswith('#') or 'fully-qualified' not in line:
                continue
            
            # Extract the emoji sequence from the line
            parts = line.split('#')
            if len(parts) < 2:
                continue
            
            emoji_char = parts[1].strip().split(' ')[0]
            
            # We only care about ZWJ sequences, as they are the only ones
            # that will have a non-empty component list and thus a hash.
            if '\u200D' in emoji_char:
                parsed = parse_emoji_string(emoji_char)
                
                # Only print if a hash was actually generated
                if parsed['hash'] != 0:
                    count += 1
                    primary_hex = f"U+{parsed['primary_cp']:X}"
                    components_hex = ", ".join([f"U+{cp:X}" for cp in parsed['component_list']])
                    hash_hex = f"0x{parsed['hash']:08X}"
                    
                    print(f"{emoji_char:<20}\t{primary_hex:<12}\t{components_hex:<35}\t{hash_hex}")

    sys.stderr.write("-" * 90)
    sys.stderr.write("\n")
    sys.stderr.write(f"Found and processed {count} component-based emoji sequences.\n")


if __name__ == "__main__":
    main()
