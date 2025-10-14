

binmoji: binmoji.c emoji_hash_table.h
	$(CC) $< -o $@

emoji_hash_table.h: emoji-sequences.txt emoji-test.txt emoji-zwj-sequences.txt generate_hash_table.py
	python generate_hash_table.py emoji-sequences.txt emoji-test.txt emoji-zwj-sequences.txt > $@

tags:
	ctags *.c
