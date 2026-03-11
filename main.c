#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

FILE* out;

typedef enum {
	T_F,
	T_IDENT,
	T_RPARAN, T_LPARAN,
	T_RBRACE, T_LBRACE,
	T_STRING, T_NUMBER,
	T_SEMI,
	T_TYPE,
	T_EOF
} TokenType;

typedef struct {
	char value[256];
	TokenType type;
} Token;

int pos;
char *src;

Token tok(TokenType type, char *value) {
	Token t;
	t.type = type;
	strcpy(t.value, value);
	return t;
}

Token nexttoken() {
	while (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\n')
		pos++;
	switch (src[pos]) {
		case '(': pos++; return tok(T_RPARAN, "(");
		case ')': pos++; return tok(T_LPARAN, ")");
		case '{': pos++; return tok(T_RBRACE, "{");
		case '}': pos++; return tok(T_LBRACE, "}");
		case ';': pos++; return tok(T_SEMI,   ";");
		case '\0': return tok(T_EOF, "");
	}
	if(isalpha(src[pos])) {
		int i = 0;
		char buf[256];
		while (isalpha(src[pos]) || isdigit(src[pos]) || src[pos] == '_') buf[i++] = src[pos++];
		buf[i] = '\0';
		if (strcmp(buf, "fn") == 0 || strcmp(buf, "func") == 0) return tok(T_F, buf);
		if (strcmp(buf, "int") == 0) return tok(T_TYPE, buf);
		return tok(T_IDENT, buf);
	}
	if (isdigit(src[pos])) {
		int i = 0;
		char buf[256];
		while (isdigit(src[pos]))
			buf[i++] = src[pos++];
		buf[i] = '\0';
		return tok(T_NUMBER, buf);
	}
	if (src[pos] == '\'') {
		int i = 0;
		char buf[256];
		pos++;
		while (src[pos] != '\'' && src[pos] != '\0') {
			if (src[pos] == '\\') {
				buf[i++] = src[pos++];
				buf[i++] = src[pos++];
			} else {
				buf[i++] = src[pos++];
			}
		}
		buf[i] = '\0';
		pos++;
		return tok(T_STRING, buf);
	}
	if (src[pos] == '/' && src[pos+1] == '/') {
		while (src[pos] != '\n' && src[pos] != '\0')
			pos++;
		return nexttoken();
	}

	return tok(T_EOF, "");
}

Token current;

void advance() {
	current = nexttoken();
	fprintf(stderr, "token: %d '%s'\n", current.type, current.value);
}

int check(TokenType type) {
	return current.type == type;
}

Token expect(TokenType type) {
	if (current.type != type) {
		fprintf(stderr, "Expected token type %d, got %d.\n", type, current.type);
		exit(1);
	}
	advance();
	return current;
}

void parsevar() {
	char type[256];
	strcpy(type, current.value);
	advance();
	char name[256];
	strcpy(name, expect(T_IDENT).value);
	expect(T_SEMI);
	fprintf(out, "%s %s;\n", type, name);
}

void parsecall() {
	char name[256];
	strcpy(name, current.value);
	advance();
	expect(T_RPARAN);
	char arg[256] = "";
	if (!check(T_LPARAN)) {
		strcpy(arg, current.value);
		advance();
	}
	expect(T_LPARAN);
	expect(T_SEMI);
	fprintf(out, "\t%s(\"%s\");\n", name, arg);
}

void parsestatement() {
	if (check(T_TYPE))
		parsevar();
	else if (check(T_IDENT))
		parsecall();
	else {
		fprintf(stderr, "Unexpected token: %s\n", current.value);
		exit(1);
	}
}

int parsefunc() {
	advance();
	char name[256];
	strcpy(name, current.value);
	advance();
	expect(T_RPARAN);
	expect(T_LPARAN);
	expect(T_RBRACE);
	fprintf(out, "int %s() {\n", name);
	while (!check(T_LBRACE) && !check(T_EOF)) {
		parsestatement();
	}
	expect(T_LBRACE);
	fprintf(out, "\treturn 0;\n}\n");
	return 0;
}

void parse() {
	pos = 0; advance();
	while (!check(T_EOF)) {
		if (check(T_F)) {
			parsefunc();
		}else if (check(T_TYPE)) {
			parsevar();
		}else if (check(T_IDENT)) {
			parsecall();
		} else {
			fprintf(stderr, "Unexpected token: %s\n", current.value);
			exit(1);
		}
	}
}

int main(int argc, char** argv) {
	FILE *f = fopen(argv[1], "r");
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	rewind(f);
	src = malloc(len + 1);
	fread(src, 1, len, f);
	src[len] = '\0';
	fclose(f);

	out = fopen(argv[2], "w");
	fprintf(out, "#include <stdio.h>\n");
	parse();
	fclose(out);
	printf("Compilation completed with 0 error(s). At least, i think.\n");
}
