# koml2struct
krisvers' (not-so) Obvious Markup Language C struct code-gen tool written in C99.

### Dependencies
A functional libc and compiler that at least supports C99

### Usage (in CLI)
`koml2struct ./test.koml`

or

`koml2struct ./test.koml > test.h`

### KOML example
[test.koml](./test.koml)
```md
# comment
# `i` specifies `int` type
i integer = 2
# `f` specifies `float` type
f floating = 6.2
# `s` specifies `char *` type
s string = "hello, world"
# `b` specifies a bool, `char` type
b boolean = t

# a header is directly proportional to a struct
# brackets (`[`, `]`) start and end a "header" name
# if the "header" is empty, the struct is anonymous
[structure]
  i integer = 2
  f floating = 6.2
  s string = "hello, world"
  b boolean = t
  # `-` specifies the struct instance name if any
-structure_instance
```
#### outputted C code
```c
struct test_kml { /* # comment */ /* # `i` specifies `int` type */ int integer; /* # `f` specifies `float` type */ float floating; /* # `s` specifies `char *` type */ char * string; /* # `b` specifies a bool, `char` type */ char boolean; /* # a header is directly proportional to a struct */ /* # brackets (`[`, `]`) start and end a "header" name */ /* # if the "header" is empty, the struct is anonymous */ struct structure { int integer; float floating; char * string; char boolean; /* # `-` specifies the struct instance name if any */ } structure_instance; };
```
#### outputted C code (now with formatting for readabilitiy
```c
struct test_kml {
  /* # comment */
  /* # `i` specifies `int` type */
  int integer;

  /* # `f` specifies `float` type */
  float floating;

  /* # `s` specifies `char *` type */
  char * string;

  /* # `b` specifies a bool, `char` type */
  char boolean;

  /* # a header is directly proportional to a struct */
  /* # brackets (`[`, `]`) start and end a "header" name */
  /* # if the "header" is empty, the struct is anonymous */
  struct structure {
    int integer;
    float floating;
    char * string;
    char boolean;

    /* # `-` specifies the struct instance name if any */
  } structure_instance;
};
```
