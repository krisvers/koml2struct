#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef _MSC_VER
#include <malloc.h>
#define stalloc _malloca
#else
#include <alloca.h>
#define stalloc alloca
#endif

#define PRINT_ERR(s) \
	fprintf(stderr, "%s\n", s);							\
	fprintf(stdout, "\n#error \"kml2struct: %s\"\n", s);	\

int parse(char * buffer);

char * filename;

int main(int argc, char ** argv) {
	if (argc != 2) {
		PRINT_ERR("Provide a file to read from");
		return 1;
	}

	FILE * fp = fopen(argv[1], "rb");
	filename = argv[1];
	unsigned long long int len;
	fseek(fp, 0L, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	char * buf = stalloc(len + 1);
	if (buf == NULL) {
		PRINT_ERR("Failure to allocate buffer");
		return 1;
	}
	buf[len] = '\0';
	if (fread(buf, len, 1, fp) != 1) {
		PRINT_ERR("Failure whilst reading file");
		return 2;
	}
	fclose(fp);

	return parse(buf);
}

typedef struct Word {
	char * ptr;
	unsigned long long len;
} word_t;

typedef struct Header {
	struct Header * parent;
	word_t name;
} header_t;

typedef enum Which {
	WHICH_UNKNOWN = 0,
	WHICH_VAR,
	WHICH_HEADER,
	WHICH_HEADER_DELIMITER,
	WHICH_COMMENT,
} which_t;

const static which_t which_char_table[256] = {
	['['] = WHICH_HEADER,
	['b'] = WHICH_VAR,
	['f'] = WHICH_VAR,
	['i'] = WHICH_VAR,
	['s'] = WHICH_VAR,
	['-'] = WHICH_HEADER_DELIMITER,
	['#'] = WHICH_COMMENT,
};

typedef enum Type {
	TYPE_UNKNOWN = 0,
	TYPE_HEADER,
	TYPE_FLOAT,
	TYPE_INT,
	TYPE_STRING,
	TYPE_BOOL,
	TYPE_HEADER_DELIMITER,
	TYPE_COMMENT,
	TYPE_MAX,
} type_t;

const static type_t type_char_table[256] = {
	['['] = TYPE_HEADER,
	['f'] = TYPE_FLOAT,
	['i'] = TYPE_INT,
	['s'] = TYPE_STRING,
	['b'] = TYPE_BOOL,
	['-'] = TYPE_HEADER_DELIMITER,
	['#'] = TYPE_COMMENT,
};

const static char * type_string_table[TYPE_MAX] = {
	"unknown", "struct", "float", "int", "char *", "char", "}", "comment"
};

typedef struct LineState {
	which_t which;
	type_t type;
	word_t name;
	word_t value;
	header_t header;
} line_state_t;

char * strreplace(char * str, char chr, char rep) {
	char * current = strchr(str, chr);

	while (current != NULL) {
		*current = rep;
		current = strchr(current, chr);
	}

	return str;
}

int parse_line(char * line, line_state_t * out, unsigned long long int lnum, char ** outp) {
	unsigned long long int i = 0;
	char prevc = ' ';
	char c = -1;//line[i];
	char nextc = ' ';
	out->which = WHICH_UNKNOWN;
	out->type = TYPE_UNKNOWN;
	out->name.ptr = NULL;
	out->name.len = 0;
	out->value.ptr = NULL;
	out->value.len = 0;

	int value = 0;
	int name = 0;
	int string = 0;

	while (c != '\n' && c != '\0') {
		prevc = c;
		c = line[i];
		nextc = line[i + 1];
		if (c == '\r') {
			goto parse_line_next;
		}
		//putc(c, stdout);
		switch (out->which) {
			case WHICH_UNKNOWN:
				if (c == '\n') {
					return -2;
				}
				if (c == ' ' || c == '\t') {
					break;
				}
				if (c == '#') {
					out->which = WHICH_COMMENT;
					out->type = TYPE_COMMENT;
					out->name.ptr = &line[i];
					out->name.len = i;
					while (c != '\n' && c != '\0') {
						c = line[i];
						++i;
						if (line[i] == '\n') {
							break;
						}
					}
					out->name.len = i - out->name.len;
					*outp = &line[i];
					strreplace(out->name.ptr, '\r', ' ');
					return -1;
				}
				out->which = which_char_table[c];
				out->type = type_char_table[c];
				if (out->which == WHICH_HEADER_DELIMITER) {
					out->name.ptr = &line[i + 1];
					while (c != '\n' && c != '\0') {
						c = line[i];
						++i;
					}
					*outp = &line[i - 1];
					out->name.len = i - 2;
					return 0;
				}
				if (out->which == WHICH_UNKNOWN) {
					char * str = strreplace(line, '\"', '\'');
					fprintf(stderr, "Unknown token [%.*s] (line %llu, column %llu)\n", (unsigned int) (strchr(str, '\n') - str), str, lnum, i);
					fprintf(stdout, "\n#error \"Unknown token [%.*s] (line %llu, column %llu)\"\n", (unsigned int) (strchr(str, '\n') - str), str, lnum, i);
					return 1;
				}
				break;
			case WHICH_HEADER:
				if (out->name.ptr == NULL) {
					out->name.ptr = &line[i];
					out->name.len = 0;
				}
				if (c == ']') {
					/*if (out->name.len < 1) {
						char * str = strreplace(line, '\"', '\'');
						fprintf(stderr, "Expected header name [%.*s] (line %llu, column %llu)\n", (unsigned int) (strchr(str, '\n') - str), str, lnum, i + 1);
						fprintf(stdout, "\n#error \"Expected header name [%.*s] (line %llu, column %llu)\"\n", (unsigned int) (strchr(str, '\n') - str), str, lnum, i + 1);
						return 8;
					}*/
					out->header.name.ptr = out->name.ptr;
					out->header.name.len = out->name.len;
					while (c != '\n' && c != '\0') {
						c = line[i];
						++i;
					}
					*outp = &line[i - 1];
					return 0;
				}
				if (isalnum(c) || c == '_') {
					if (out->name.ptr == NULL) {
						out->name.ptr = &line[i];
						out->name.len = 0;
					}
					++out->name.len;
				}
				if (!(isalnum(c) || c == '_')) {
					char * str = strreplace(line, '\"', '\'');
					fprintf(stderr, "Character \'%c\' is disallowed in header names [%.*s] (line %llu, column %llu)\n", c, (unsigned int) (strchr(str, '\n') - str), str, lnum, i + 1);
					fprintf(stdout, "\n#error \"Character \'%c\' is disallowed in header names [%.*s] (line %llu, column %llu)\"\n", c, (unsigned int) (strchr(str, '\n') - str), str, lnum, i + 1);
					return 2;
				}
				break;
			case WHICH_VAR:
				if (value) {
					switch (out->type) {
						case TYPE_STRING:
							if (!string) {
								if (c == '\"') {
									string = 1;
								}
								break;
							}
							if (c == '\"' && (nextc == '\0' || nextc == '\n')) {
								break;
							}
							if (isprint(c)) {
								if (out->value.ptr == NULL) {
									out->value.ptr = &line[i];
									out->value.len = 0;
								}
								++out->value.len;
							} else if (c == ' ' || c == '\t' || c == '\n' || c == '\0') {
								break;
							} else if (c == '#') {
								while (c != '\n' && c != '\0') {
									c = line[i];
									++i;
								}
								*outp = &line[i - 1];
								return -1;
							} else {
								char * str = line;//strreplace(line, '\"', '\'');
								fprintf(stderr, "Character \'%c\' is disallowed in string values [%.*s] (line %llu, column %llu)\n", c, (unsigned int) (strchr(str, '\n') - str), str, lnum, i);
								fprintf(stdout, "\n#error \"Character \'%c\' is disallowed in string values [%.*s] (line %llu, column %llu)\"\n", c, (unsigned int) (strchr(str, '\n') - str), str, lnum, i);
								return 5;
							}
							break;
						case TYPE_INT:
							if (isdigit(c)) {
								if (out->value.ptr == NULL) {
									out->value.ptr = &line[i];
									out->value.len = 0;
								}
								++out->value.len;
							} else if (c == ' ' || c == '\t' || c == '\n' || c == '\0') {
								break;
							} else if (c == '#') {
								while (c != '\n' && c != '\0') {
									c = line[i];
									++i;
								}
								*outp = &line[i - 1];
								return -1;
							} else {
								char * str = strreplace(line, '\"', '\'');
								fprintf(stderr, "Character \'%c\' is disallowed in int values [%.*s] on line %llu\n", c, (unsigned int) (strchr(str, '\n') - str), str, lnum);
								fprintf(stdout, "\n#error \"Character \'%c\' is disallowed in int values [%.*s] on line %llu\"\n", c, (unsigned int) (strchr(str, '\n') - str), str, lnum);
								return 6;
							}
							break;
						case TYPE_FLOAT:
							if (isdigit(c) || c == '.') {
								if (out->value.ptr == NULL) {
									out->value.ptr = &line[i];
									out->value.len = 0;
								}
								++out->value.len;
							} else if (c == ' ' || c == '\t' || c == '\n' || c == '\0') {
								break;
							} else if (c == '#') {
								while (c != '\n' && c != '\0') {
									c = line[i];
									++i;
								}
								*outp = &line[i - 1];
								return -1;
							} else {
								char * str = strreplace(line, '\"', '\'');
								fprintf(stderr, "Character \'%c\' is disallowed in float values [%.*s] (line %llu, column %llu)\n", c, (unsigned int) (strchr(str, '\n') - str), str, lnum, i);
								fprintf(stdout, "\n#error \"Character \'%c\' is disallowed in float values [%.*s] (line %llu, column %llu)\"\n", c, (unsigned int) (strchr(str, '\n') - str), str, lnum, i);
								return 7;
							}
							break;
						case TYPE_BOOL:
							if (c == 'f' || c == 't' || c == '0' || c == '1') {
								if (out->value.ptr == NULL) {
									out->value.ptr = &line[i];
									out->value.len = 0;
								}
								++out->value.len;
							} else if (c == ' ' || c == '\t' || c == '\n' || c == '\0') {
								break;
							} else if (c == '#') {
								while (c != '\n' && c != '\0') {
									c = line[i];
									++i;
								}
								*outp = &line[i - 1];
								return -1;
							} else {
								char * str = strreplace(line, '\"', '\'');
								fprintf(stderr, "Character \'%c\' is disallowed in float values [%.*s] (line %llu, column %llu)\n", c, (unsigned int) (strchr(str, '\n') - str), str, lnum, i);
								fprintf(stdout, "\n#error \"Character \'%c\' is disallowed in float values [%.*s] (line %llu, column %llu)\"\n", c, (unsigned int) (strchr(str, '\n') - str), str, lnum, i);
								return 7;
							}
							break;
						default:
							return 1;
					}
				} else {
					if (isalnum(c) || c == '_') {
						if (!(isalnum(prevc) || prevc == '_') && name) {
							char * str = strreplace(line, '\"', '\'');
							fprintf(stderr, "Character \'%c\' is disallowed in variable names [%.*s] (line %llu, column %llu)\n", prevc, (unsigned int) (strchr(str, '\n') - str), str, lnum, i);
							fprintf(stdout, "\n#error \"Character \'%c\' is disallowed in variable names [%.*s] (line %llu, column %llu)\"\n", prevc, (unsigned int) (strchr(str, '\n') - str), str, lnum, i);
							return 2;
						}

						if (out->name.ptr == NULL) {
							out->name.ptr = &line[i];
							out->name.len = 0;
						}
						++out->name.len;
						name = 1;
					} else if (c == '#') {
						while (c != '\n' && c != '\0') {
							c = line[i];
							++i;
						}
						*outp = &line[i - 1];
						return -1;
					}
					if (c == '=') {
						if (out->name.ptr == NULL) {
							char * str = strreplace(line, '\"', '\'');
							fprintf(stderr, "Expected variable name before '=' [%.*s] (line %llu, column %llu)\n", (unsigned int) (strchr(str, '\n') - str), str, lnum, i);
							fprintf(stdout, "\n#error \"Expected variable name before '=' [%.*s] (line %llu, column %llu)\"\n", (unsigned int) (strchr(str, '\n') - str), str, lnum, i);
							return 3;
						}
						value = 1;
						break;
					}
					if ((prevc == '\t' || prevc == ' ') && !(isalnum(c) || c == '_')) {
						char * str = strreplace(line, '\"', '\'');
						fprintf(stderr, "Character \'%c\' is disallowed in variable names [%.*s] (line %llu, column %llu)\n", c, (unsigned int) (strchr(str, '\n') - str), str, lnum, i + 1);
						fprintf(stdout, "\n#error \"Character \'%c\' is disallowed in variable names [%.*s] (line %llu, column %llu)\"\n", c, (unsigned int) (strchr(str, '\n') - str), str, lnum, i + 1);
						return 2;
					}
				}
				break;
			default:
				return 1;
		}
	parse_line_next:
		++i;
	}
	*outp = &line[i - 1];

	return 0;
}

void line_state_print(line_state_t * lst) {
	fprintf(stdout, "[%.*s] = [%.*s]\n", (unsigned int) lst->name.len, lst->name.ptr, (unsigned int) lst->value.len, lst->value.ptr);
}

int parse(char * buf) {
	line_state_t lst;
	char * p = buf;
	unsigned long long int line = 1;
	while (!isalnum(filename[0])) {
		filename = filename + 1;
		if (filename[1] == '\0') {
			break;
		}
	}
	strreplace(filename, '.', '_');
	strreplace(filename, '/', '_');
	printf("struct %s { ", filename);
	while (*p != '\0') {
		char * prevp = p;
		int ret = parse_line(p, &lst, line, &p);
		if (ret == -2) {
			goto parse_next;
		}
		if (ret != 0 && ret != -1) {
			return ret;
		}
		if (lst.which == WHICH_UNKNOWN || lst.type == TYPE_UNKNOWN) {
			fprintf(stderr, "Failure\n");
			return -1;
		}
		//line_state_print(&lst);
		//printf("\n%.*s\n", (unsigned int) lst.name.len, lst.name.ptr);
		if (lst.type != TYPE_HEADER && lst.type != TYPE_HEADER_DELIMITER && lst.type != TYPE_COMMENT) {
			printf("%s %.*s; ", type_string_table[lst.type], (unsigned int) lst.name.len, lst.name.ptr);
		} else if (lst.type == TYPE_HEADER_DELIMITER) {
			printf("%s%s%.*s; ", type_string_table[lst.type], (lst.name.len >= 1) ? " " : "", (unsigned int) lst.name.len, lst.name.ptr);
		} else if (lst.type == TYPE_COMMENT) {
			printf("/* %.*s */ ", (unsigned int) lst.name.len, lst.name.ptr);
		} else {
			printf("%s %.*s%s{ ", type_string_table[lst.type], (unsigned int) lst.name.len, lst.name.ptr, (lst.name.len >= 1) ? " " : "");
		}
		if (*p != '\n') {
			break;
		}
		if (p == prevp) {
			break;
		}
parse_next:
		++p;
		++line;
	}
	printf("};\n");

	return 0;
}