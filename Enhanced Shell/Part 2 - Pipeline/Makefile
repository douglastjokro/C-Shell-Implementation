CC = clang

CLANG = -g -Wall

TARGETS = clean token-shell

.PHONY: token-shell

default: token-shell

all: $(TARGETS)

token-shell: tokenizer.c token-shell.c pipe.c
	clang -Wall -g $^ -o $@

clean: 
	$(RM) token-shell
