

#include <ctype.h>
#include <string.h>

#include "lex.h"
#include "../deps/stream/stream.h"

/* declaration for state functions */
static void * lex_c(lex_t * lex);

static void * lex_comment(lex_t * lex);
/*static void * lex_export(lexer.t * lex);*/

static void * lex_whitespace(lex_t * lex);
static void * lex_id(lex_t * lex);
static void * lex_number(lex_t * lex);
static void * lex_quote(lex_t * lex);
static void * lex_squote(lex_t * lex);
static void * lex_preprocessor(lex_t * lex);

lex_t * lex_syntax_new(stream_t * input, const char * filename, char ** error) {
  return lex_new(lex_c, input, filename);
}

static void * emit_c_code(lex_t * lex, lex_state_fn fn){
  lex_backup(lex);

  if (lex->pos > lex->start) {
      lex_emit(lex, item_c_code);
  }

  if (fn == NULL) lex_next(lex);

  return fn;
}

static void * eof(lex_t * lex) {
  lex_backup(lex);

  if (lex->pos > lex->start) {
      lex_emit(lex, item_c_code);
  }
  lex_emit(lex, item_eof);

  return NULL;
}

/* lexes standard C code looking for things that might be modular c */
static void * lex_c(lex_t * lex) {
  char next;

  while (true) {
    char c = lex_next(lex);

    if (c == 0)      return eof(lex);
    if (isspace(c))  return emit_c_code(lex, lex_whitespace);
    if (isalpha(c))  return emit_c_code(lex, lex_id);
    if (isdigit(c)) return emit_c_code(lex, lex_number);

    switch(c) {
      case '\'':
        return emit_c_code(lex, lex_squote);

      case '"':
        return emit_c_code(lex, lex_quote);

      case ';':
      case '.':
      case ',':
      case '=':
      case '*':
        emit_c_code(lex, NULL);
        lex_emit(lex, item_symbol);
        break;

      case '-':
        if (lex_peek(lex) == '>') {
          lex_next(lex);
          lex_emit(lex, item_arrow);
        }
        break;

      case '#':
        if (lex->pos > 2 && lex->input[lex->pos - 2] == '\n')
          return emit_c_code(lex, lex_preprocessor);
        break;

      case '_':
        return emit_c_code(lex, lex_id);

      case '/':
        next = lex_peek(lex);
        if (next == '/' || next == '*')
          return emit_c_code(lex, lex_comment);
        break;

      case '(':
      case '[':
      case '{':
        emit_c_code(lex, NULL);
        lex_emit(lex, item_open_symbol);
        break;

      case ')':
      case ']':
      case '}':
        emit_c_code(lex, NULL);
        lex_emit(lex, item_close_symbol);
        break;

    }
  }
}


static void * lex_whitespace(lex_t * lex) {
  char c;
  while ((c = lex_next(lex)) != 0 && isspace(c));
  lex_backup(lex);
  lex_emit(lex, item_whitespace);

  if (c == 0) return eof(lex);
  return lex_c;
}

static void * lex_oneline_comment(lex_t * lex) {
  char c;
  while ((c = lex_next(lex)) != 0 && c != '\n');
  lex_emit(lex, item_comment);
  if (c == 0) return eof(lex);
  return lex_c;
}

static void * lex_multiline_comment(lex_t * lex) {
  char c;
  do {
    while ((c = lex_next(lex)) != 0 && c != '*');

    if (c == 0) {
      lex_errorf(lex, "Unterminated multiline comment\n %*s", lex->input + lex->start);
      return eof(lex);
    }

  } while (lex_peek(lex) != '/');

  lex_next(lex);
  lex_emit(lex, item_comment);
  return lex_c;
}

static void * lex_comment(lex_t * lex) {
  lex->pos++;
  char c = lex_next(lex);

  if (c == '/') return lex_oneline_comment(lex);
  else          return lex_multiline_comment(lex);
}

static void * lex_number(lex_t * lex) {
  char c;
  while ((c = lex_next(lex)) != 0 && isdigit(c));
  lex_backup(lex);
  lex_emit(lex, item_number);

  if (c == 0) return eof(lex);
  return lex_c;
}

static void * lex_id(lex_t * lex) {
  char c;
  while ((c = lex_next(lex)) != 0 && (isalnum(c) || c == '_'));
  lex_backup(lex);
  lex_emit(lex, item_id);

  if (c == 0) return eof(lex);
  return lex_c;
}


static void * lex_quote(lex_t * lex) {
  lex->pos ++;

  char c;
  do {
    c = lex_next(lex);
    if (c == '\\' && lex_next(lex) != 0) {
      c = lex_next(lex);
      continue;
    }
  } while(c != 0 && c != '"' && c != '\n');

  if (c == 0 || c == '\n') {
    return lex_errorf(lex, "Missing terminating '\"' character\n");
  }

  lex_emit(lex, item_quoted_string);
  return lex_c;
}

static void * lex_squote(lex_t * lex) {
  lex->pos ++;

  char c;
  do {
    c = lex_next(lex);
    if (c == '\\' && lex_next(lex) != 0) {
      c = lex_next(lex);
      continue;
    }
  } while(c != 0 && c != '\'');

  if (c == 0) {
    size_t length = lex->pos - lex->start;
    if (length > 10) {
      return lex_errorf(lex, "Missing terminating ' character\n%.*s...", 10, lex->input + lex->start);
    } else {
      return lex_errorf(lex, "Missing terminating ' character\n%s", lex->input + lex->start);
    }
  }

  lex_emit(lex, item_char_literal);
  return lex_c;
}

static void * lex_preprocessor(lex_t * lex) {
  char c = lex_next(lex);
  while (c != '\n' && c != 0) {
    c = lex_next(lex);
  }
  lex_backup(lex);
  lex_emit(lex, item_preprocessor);
  return lex_c;
}