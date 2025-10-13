#include "scanner.h"
#include "common.h"
#include <stdio.h>
#include <string.h>

typedef struct {
	const char *start;
	const char *current;
	int line;
} Scanner;

Scanner scanner;

void initScanner(const char *source) {
	scanner.start = source;
	scanner.current = source;
	scanner.line = 1;
};

static bool isAtEnd() {
	// We just check if its a null terminated string ending.
	return *scanner.current == '\0';
}

static Token makeToken(TokenType type) {
	// We want to create a token.
	Token token;
	token.type = type;
	token.start = scanner.start;
	token.length = (int)(scanner.current - scanner.start);
	token.line = scanner.line;
	return token;
}

static Token errorToken(const char *message) {
	Token token;
	token.type = TOKEN_ERROR;
	token.start = message;
	token.length = (int)strlen(message);
	token.line = scanner.line;
	return token;
}
Token scanToken() {
	scanner.start = scanner.current;
	if (isAtEnd())
		return makeToken(TOKEN_EOF);
	return errorToken("Unexpected Character.");
}
