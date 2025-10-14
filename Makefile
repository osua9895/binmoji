

binmoji: binmoji.c test.c emoji_hash_table.h
	$(CC) -std=c89 -Wall -pedantic binmoji.c test.c -o $@

emoji_hash_table.h: emoji-sequences.txt emoji-test.txt emoji-zwj-sequences.txt generate_hash_table.py
	python generate_hash_table.py emoji-sequences.txt emoji-test.txt emoji-zwj-sequences.txt > $@

check: binmoji
	./binmoji --test

clean:
	rm binmoji

tags:
	ctags *.c

.PHONY: tags
