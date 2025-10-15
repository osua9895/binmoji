
EMOJI_VERSION = 17.0.0
BASE_URL      = https://www.unicode.org/Public/$(EMOJI_VERSION)/emoji
EMOJI_FILES   = emoji-sequences.txt emoji-test.txt emoji-zwj-sequences.txt

binmoji: binmoji.c test.c binmoji_table.h
	$(CC) -std=c89 -Wall -Werror -pedantic binmoji.c test.c -o $@

binmoji_table.h: generate_hash_table.py $(EMOJI_FILES) 
	python $^ > $@

check: binmoji
	./binmoji --test

format: binmoji.c
	clang-format -style="{BasedOnStyle: LLVM, IndentWidth: 8, UseTab: Always, BreakBeforeBraces: Linux}" -i $^

emojis:
	for file in $(EMOJI_FILES); do curl -O $(BASE_URL)/$$file; done

clean:
	rm binmoji

tags:
	ctags *.c

.PHONY: tags emojis
