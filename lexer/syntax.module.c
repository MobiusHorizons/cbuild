package "lex_syntax";

#include <ctype.h>
#include <string.h>

import lexer  from "./lex.module.c";
import stream from "../deps/stream/stream.module.c";

/* declaration for state functions */
static void * lex_c(lexer.t * lex);

static void * lex_comment(lexer.t * lex);
/*static void * lex_export(lexer.t * lex);*/

static void * lex_whitespace(lexer.t * lex);
static void * lex_id(lexer.t * lex);
static void * lex_number(lexer.t * lex);
static void * lex_quote(lexer.t * lex);
static void * lex_squote(lexer.t * lex);
static void * lex_preprocessor(lexer.t * lex);

export lexer.t * new(stream.t * input, const char * filename, char ** error) {
  return lexer.new(lex_c, input, filename);
}

static void * emit_c_code(lexer.t * lex, lexer.state_fn fn){
  lexer.backup(lex);

  if (lex->pos > lex->start) {
      lexer.emit(lex, item_c_code);
  }

  if (fn == NULL) lexer.next(lex);

  return fn;
}

static void * eof(lexer.t * lex) {
  lexer.backup(lex);

  if (lex->pos > lex->start) {
      lexer.emit(lex, item_c_code);
  }
  lexer.emit(lex, item_eof);

  return NULL;
}

/* lexes standard C code looking for things that might be modular c */
static void * lex_c(lexer.t * lex) {
  char next;

  while (true) {
    char c = lexer.next(lex);

    if (c == 0)      return eof(lex);
    if (isspace(c))  return emit_c_code(lex, lex_whitespace);
    if (isalpha(c))  return emit_c_code(lex, lex_id);
    if (isnumber(c)) return emit_c_code(lex, lex_number);

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
        lexer.emit(lex, item_symbol);
        break;

      case '-':
        if (lexer.peek(lex) == '>') {
          lexer.next(lex);
          lexer.emit(lex, item_arrow);
        }
        break;

      case '#':
        if (lex->pos > 2 && lex->input[lex->pos - 2] == '\n')
          return emit_c_code(lex, lex_preprocessor);
        break;

      case '_':
        return emit_c_code(lex, lex_id);

      case '/':
        next = lexer.peek(lex);
        if (next == '/' || next == '*')
          return emit_c_code(lex, lex_comment);
        break;

      case '(':
      case '[':
      case '{':
        emit_c_code(lex, NULL);
        lexer.emit(lex, item_open_symbol);
        break;

      case ')':
      case ']':
      case '}':
        emit_c_code(lex, NULL);
        lexer.emit(lex, item_close_symbol);
        break;

    }
  }
}


static void * lex_whitespace(lexer.t * lex) {
  char c;
  while ((c = lexer.next(lex)) != 0 && isspace(c));
  lexer.backup(lex);
  lexer.emit(lex, item_whitespace);

  if (c == 0) return eof(lex);
  return lex_c;
}

static void * lex_oneline_comment(lexer.t * lex) {
  char c;
  while ((c = lexer.next(lex)) != 0 && c != '\n');
  lexer.emit(lex, item_comment);
  if (c == 0) return eof(lex);
  return lex_c;
}

static void * lex_multiline_comment(lexer.t * lex) {
  char c;
  do {
    while ((c = lexer.next(lex)) != 0 && c != '*');

    if (c == 0) {
      lexer.errorf(lex, "Unterminated multiline comment\n %*s", lex->input + lex->start);
      return eof(lex);
    }

  } while (lexer.peek(lex) != '/');

  lexer.next(lex);
  lexer.emit(lex, item_comment);
  return lex_c;
}

static void * lex_comment(lexer.t * lex) {
  lex->pos++;
  char c = lexer.next(lex);

  if (c == '/') return lex_oneline_comment(lex);
  else          return lex_multiline_comment(lex);
}

static void * lex_number(lexer.t * lex) {
  char c;
  while ((c = lexer.next(lex)) != 0 && isnumber(c));
  lexer.backup(lex);
  lexer.emit(lex, item_number);

  if (c == 0) return eof(lex);
  return lex_c;
}

static void * lex_id(lexer.t * lex) {
  char c;
  while ((c = lexer.next(lex)) != 0 && (isalnum(c) || c == '_'));
  lexer.backup(lex);
  lexer.emit(lex, item_id);

  if (c == 0) return eof(lex);
  return lex_c;
}


static void * lex_quote(lexer.t * lex) {
  lex->pos ++;

  char c;
  do {
    c = lexer.next(lex);
    if (c == '\\' && lexer.next(lex) != 0) {
      c = lexer.next(lex);
      continue;
    }
  } while(c != 0 && c != '"' && c != '\n');

  if (c == 0 || c == '\n') {
    return lexer.errorf(lex, "Missing terminating '\"' character\n");
  }

  lexer.emit(lex, item_quoted_string);
  return lex_c;
}

static void * lex_squote(lexer.t * lex) {
  lex->pos ++;

  char c;
  do {
    c = lexer.next(lex);
    if (c == '\\' && lexer.next(lex) != 0) {
      c = lexer.next(lex);
      continue;
    }
  } while(c != 0 && c != '\'');

  if (c == 0) {
    size_t length = lex->pos - lex->start;
    if (length > 10) {
      return lexer.errorf(lex, "Missing terminating ' character\n%.*s...", 10, lex->input + lex->start);
    } else {
      return lexer.errorf(lex, "Missing terminating ' character\n%s", lex->input + lex->start);
    }
  }

  lexer.emit(lex, item_char_literal);
  return lex_c;
}

static void * lex_preprocessor(lexer.t * lex) {
  char c = lexer.next(lex);
  while (c != '\n' && c != 0) {
    c = lexer.next(lex);
  }
  lexer.backup(lex);
  lexer.emit(lex, item_preprocessor);
  return lex_c;
}
