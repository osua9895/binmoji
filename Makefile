

binmoji: binmoji.c test.c binmoji_table.h
	$(CC) -std=c89 -Wall -Werror -pedantic binmoji.c test.c -o $@

binmoji_table.h: emoji-sequences.txt emoji-test.txt emoji-zwj-sequences.txt generate_hash_table.py
	python generate_hash_table.py emoji-sequences.txt emoji-test.txt emoji-zwj-sequences.txt > $@

check: binmoji
	./binmoji --test

format: binmoji.c binmoji.h
	clang-format -style="{BasedOnStyle: LLVM, IndentWidth: 8, UseTab: Always, BreakBeforeBraces: Linux}" -i $^

clean:
	rm binmoji

tags:
	ctags *.c

.PHONY: tags
