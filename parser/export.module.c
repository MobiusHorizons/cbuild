#include <stdarg.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

build depends "../deps/hash/hash.c";
#include "../deps/hash/hash.h"

import parser     from "./parser.module.c";
import string     from "string.module.c";
import utils      from "../utils/utils.module.c";
import identifier from "./identifier.module.c";
import lex_item   from "../lexer/item.module.c";
import Package    from "../package/package.module.c";
import pkg_export from "../package/export.module.c";
import pkg_import from "../package/import.module.c";

const lex_item.t enum_i   = { .value = "enum",   .length = strlen("enum"),   .type = item_id };
const lex_item.t union_i  = { .value = "union",  .length = strlen("union"),  .type = item_id };
const lex_item.t struct_i = { .value = "struct", .length = strlen("struct"), .type = item_id };

typedef struct {
  lex_item.t * items;
  size_t       length;
  bool         error;
} decl_t;

static lex_item.t errorf(parser.t * p, lex_item.t item, decl_t * decl, const char * fmt, ...) {
  va_list args;
  va_start(args, fmt);

  parser.verrorf(p, item, "Invalid export syntax: ", fmt, args);
  decl->error = true;
  return lex_item.empty;
}

static void append(decl_t * decl, lex_item.t value) {
  decl->items = realloc(decl->items, sizeof(lex_item.t) * (decl->length + 1));
  decl->items[decl->length] = value;
  decl->length++;
}

static void rewind_until(parser.t * p, decl_t * decl, lex_item.t item) {
  ssize_t last = decl->length -1;

  while (last >= 0 && !lex_item.equals(item, decl->items[last])) {
    parser.backup(p, decl->items[last]);
    last--;
  }
  parser.backup(p, decl->items[last]);

  decl->length = last;
}

static void rewind_whitespace(parser.t * p, decl_t * decl, lex_item.t item) {
  parser.backup(p, item);

  ssize_t last = decl->length -1;
  while (last >= 0 && decl->items[last].type == item_whitespace) {
    parser.backup(p, decl->items[last]);
    last--;
  }

  decl->length = last + 1;
}

static lex_item.t symbol_rename(parser.t * p, decl_t * decl, lex_item.t name, lex_item.t alias) {
  char * export_name = alias.type == 0 ? name.value : alias.value;
  int i;
  for (i = 0; i < decl->length; i++) {
    lex_item.t item = decl->items[i];
    if (lex_item.equals(name, item)) {
      item.length = asprintf(&item.value, "%s_%s", p->pkg->name, export_name);
      decl->items[i] = item;
      return item;
    }
  }
  return lex_item.empty;
}

static char * emit(parser.t * p, decl_t * decl, bool is_function, bool is_extern) {
  size_t length = 0;
  int start = 0;
  int end = decl->length;
  int i;

  // trim leading/trailing whitespace
  while (start < end && decl->items[start  ].type == item_whitespace) start++;
  while (end > start && decl->items[end - 1].type == item_whitespace) end--;

  for (i = 0; i < decl->length; i++) {
    lex_item.t item = decl->items[i];
    if (!is_extern) Package.emit(p->pkg, item.value);
    if (i >= start && i < end) length += item.length;
  }

  if (is_function) length ++;

  char * output = malloc(length + 1);
  length = 0;
  for (i = start; i < end; i++) {
    lex_item.t item = decl->items[i];
    memcpy(output + length, item.value, item.length);
    length += item.length;
  }

  if (is_function && !is_extern) output[length++] = ';';

  output[length] = 0;
  global.free(decl->items);
  return output;
}

static lex_item.t collect_newlines(parser.t * p, decl_t * decl) {
  size_t line     = p->lexer->line;
  lex_item.t item = parser.next(p);

  while (item.type == item_whitespace || item.type == item_comment) {
    if (p->lexer->line != line) {
      append(decl, item);
    }

    line = p->lexer->line;
    item = parser.next(p);
  }

  return item;
}

static lex_item.t collect(parser.t * p, decl_t * decl) {
  lex_item.t item = parser.next(p);

  while (item.type == item_whitespace || item.type == item_comment) {
    append(decl, item);
    item = parser.next(p);
  }

  return item;
}

static lex_item.t parse_typedef       (parser.t * p, decl_t * decl);
static lex_item.t parse_struct        (parser.t * p, decl_t * decl);
static lex_item.t parse_enum          (parser.t * p, decl_t * decl);
static lex_item.t parse_union         (parser.t * p, decl_t * decl);
static lex_item.t parse_struct_block  (parser.t * p, decl_t * decl, lex_item.t start);
static lex_item.t parse_enum_block    (parser.t * p, decl_t * decl, lex_item.t start);
static lex_item.t parse_export_block  (parser.t * p, decl_t * decl);
static lex_item.t parse_as            (parser.t * p, decl_t * decl);
static lex_item.t parse_variable      (parser.t * p, decl_t * decl);
static lex_item.t parse_function      (parser.t * p, decl_t * decl);
static lex_item.t parse_function_args (parser.t * p, decl_t * decl);

static int parse_passthrough (parser.t * p);

static hash_t * export_types = NULL;
static void init_export_types(){
  export_types = hash_new();

  hash_set(export_types, "typedef", parse_typedef);
  hash_set(export_types, "struct",  parse_struct );
  hash_set(export_types, "enum",    parse_enum   );
  hash_set(export_types, "union",   parse_union  );
}

typedef lex_item.t (*export_fn)(parser.t * p, decl_t * decl);

void parse_semicolon(parser.t * p, decl_t * decl) {
  if (decl->error) return;

  lex_item.t item = collect_newlines(p, decl);

  if (item.type != item_symbol || item.value[0] != ';') {
    errorf(p, item, decl, "expecting ';' but got %s", lex_item.to_string(item));
    return;
  }
  append(decl, item);
}

export int parse(parser.t * p) {
  if (export_types == NULL) init_export_types();

  decl_t decl  = {0};
  export_fn fn = NULL;
  lex_item.t name  = {0};
  lex_item.t alias = {0};
  bool has_semicolon = false;
  bool is_extern     = false;
  int t = 0;

  lex_item.t type = collect_newlines(p, &decl);

  switch (type.type) {
    case item_id:
      if (strcmp("extern", type.value) == 0) {
        append(&decl, type);
        is_extern = true;
        type = collect(p, &decl);
      }
      fn = (export_fn) hash_get(export_types, type.value);
      has_semicolon = is_extern || fn != NULL;
      t = 1;
      break;
    case item_symbol:
      if (type.value[0] == '*') return parse_passthrough(p);
      break;
    case item_open_symbol:
      if (type.value[0] == '{') {
        fn = parse_export_block;
        type.value = "";
        type.length = 0;
        t = 0;
      }
      break;
    default:
      fn = NULL;
      break;
  }
  if (fn == NULL) {
    parser.backup(p, type);
    type.type = item_symbol;
    type.value = "variable";
    type.length = strlen("variable");
    name = parse_variable(p, &decl);
    t = 2;
  } else {
    if (t == 1) append(&decl, type);
    name = fn(p, &decl);
  }

  alias = parse_as(p, &decl);
  if (has_semicolon) parse_semicolon(p, &decl);
  if (decl.error) {
    global.free(decl.items);
    return -1;
  }

  if (name.type == 0) {
    errorf(p, type, &decl, "can't export anonymous symbols");
    global.free(decl.items);
    return -1;
  }

  lex_item.t symbol = symbol_rename(p, &decl, name, alias);

  if (symbol.type == 0 && fn != parse_export_block) {
    errorf(p, type, &decl, "unable to export symbol '%s'", name.value);
    global.free(decl.items);
    return -1;
  }

  pkg_export.add(name.value, alias.value, symbol.value, type.value, emit(p, &decl, t==2, is_extern), p->pkg);
  return 1;
}

static int parse_passthrough(parser.t * p) {
  decl_t decl = {0};
  lex_item.t from = collect(p, &decl);
  if (from.type != item_id || strcmp(from.value, "from") != 0) {
    parser.errorf(p, from, "Exporting passthrough: ", "expected 'from', but got %s", lex_item.to_string(from));
    global.free(decl.items);
    return -1;
  }

  lex_item.t filename = collect(p, &decl);
  if (filename.type != item_quoted_string) {
    parser.errorf(p, filename, "Exporting passthrough: ", "expected filename, but got %s", lex_item.to_string(filename));
    global.free(decl.items);
    return -1;
  }

  parse_semicolon(p, &decl);
  if (decl.error) {
    global.free(decl.items);
    return -1;
  }

  char * error = NULL;
  pkg_import.t * imp = pkg_import.passthrough(p->pkg, string.parse(filename.value), &error);
  if (imp == NULL) {
    parser.errorf(p, filename, "Exporting passthrough: ", "%s", error);
    global.free(decl.items);
    return -1;
  }

  char * header = NULL;
  asprintf(&header, "#include \"%s\"", utils.relative(p->pkg->source_abs, imp->pkg->header_abs));
  Package.emit(p->pkg, header);

  global.free(decl.items);
  global.free(header);
  return 1;
}

static lex_item.t parse_as(parser.t * p, decl_t * decl) {
  if (decl->error) return lex_item.empty;

  size_t start = decl->length;

  lex_item.t as = collect(p, decl);
  if (as.type != item_id || strcmp("as", as.value) != 0) {
    parser.backup(p, as);
    while(decl->length > start) {
      decl->length--;
      parser.backup(p, decl->items[decl->length]);
    }
    return lex_item.empty;
  }

  while(decl->length > start) {
    decl->length--;
    parser.backup(p, decl->items[decl->length]);
  }

  lex_item.t alias = collect_newlines(p, decl);

  if (alias.type != item_id) {
    errorf(p, alias, decl, "expecting identifier but got %s", lex_item.to_string(alias));
    return lex_item.empty;
  }

  return alias;
}

static lex_item.t parse_function_ptr(parser.t * p, decl_t * decl) {
  lex_item.t star = collect(p, decl);
  if (star.type != item_symbol || star.value[0] != '*') {
    return errorf(p, star, decl, "function pointer: expecting '*' but found '%s'", star.value);
  }
  append(decl, star);

  lex_item.t name = collect(p, decl);
  if (name.type != item_id) {
    return errorf(p, name, decl, "function pointer: expecting identifier but found '%s'", name.value);
  }
  append(decl, name);

  lex_item.t item;

  item = collect(p, decl);
  if (item.type != item_close_symbol || item.value[0] != ')') {
    return errorf(p, item, decl, "function pointer: expecting ')' but found '%s'", item.value);
  }
  append(decl, item);

  item = collect(p, decl);
  if (item.type != item_open_symbol || item.value[0] != '(') {
    return errorf(p, item, decl, "function pointer: expecting '(' but found '%s'", item.value);
  }
  append(decl, item);

  parse_function_args(p, decl);

  if (decl->error) return lex_item.empty;
  return name;
}
static lex_item.t parse_declaration(parser.t * p, decl_t * decl) {
  lex_item.t type = collect(p, decl);
  if (type.type != item_id) {
    if (type.type == item_close_symbol && type.value[0] == '}') return type;
    return errorf(p, type, decl, "in declaration: expecting identifier but got %s",
        lex_item.to_string(type));
  }
  if (strcmp("as", type.value) == 0) return type;
  type = identifier.parse(p, type, true);

  export_fn fn = (export_fn) hash_get(export_types, type.value);

  if (fn == parse_typedef) {
    return errorf(p, type, decl, "in declaration: unexpected identifier 'typedef'");
  }

  if (fn == NULL) {
    fn = parse_variable;
  }
  append(decl, type);

  lex_item.t item = fn(p, decl);
  if (decl->error) return lex_item.empty;

  if (fn == parse_enum || fn == parse_union || fn == parse_struct) {
    item = collect(p, decl);
    if (item.type != item_id) {
      parser.backup(p, item);
    } else {
      append(decl, item);
    }
  }
  parse_semicolon(p, decl);
  if (decl->error) return lex_item.empty;
  return item;
}


static lex_item.t parse_typedef (parser.t * p, decl_t * decl) {
  lex_item.t type = collect(p, decl);
  if (type.type != item_id) {
    return errorf(p, type, decl, "in typedef: expected identifier");
  }

  export_fn fn = (export_fn) hash_get(export_types, type.value);

  /*if (fn != parse_struct && fn != parse_enum && fn != parse_union) append(decl, type);*/
  append(decl, type);

  if (fn != NULL) {
    fn(p, decl);
    if (decl->error) return lex_item.empty;
  }

  lex_item.t item;
  lex_item.t name = {0};
  bool function_ptr = false;
  bool as           = false;
  do {
    item = collect(p, decl);

    switch(item.type) {
      case item_eof:
        return errorf(p, item, decl, "in typedef: expected identifier or '('");
      case item_id:
        if (strcmp("as", item.value) == 0) {
          rewind_whitespace(p, decl, item);
          as = true;
          continue;
        }
        if (!function_ptr) name = item;
        break;
      case item_symbol:
        if (item.value[0] == ';') continue;
      case item_open_symbol:
        if (!function_ptr && item.value[0] == '(') {
          function_ptr = true;
          append(decl, item);
          name = parse_function_ptr(p, decl);
          if (decl->error) return name;
          continue;
        }
      default:
        break;
    }

    append(decl, item);
  } while (
      item.type != item_error                             &&
      !(item.type == item_symbol && item.value[0] == ';') &&
      !as
  );

  if (item.type == item_error) {
    decl->error = true;
    return item;
  }

  if (!as) parser.backup(p, item);
  return name;
}

static lex_item.t parse_struct (parser.t * p, decl_t * decl) {
  lex_item.t name = {0};
  lex_item.t item = collect(p, decl);
  if (item.type == item_id) {
    name = identifier.parse_typed(p, struct_i, item, true);
    append(decl, name);
    item = collect(p, decl);
  }

  if (item.type != item_open_symbol || item.value[0] != '{') {
    parser.backup(p, item);
    return name;
  }

  append(decl, item);
  parse_struct_block(p, decl, item);

  return (decl->error) ? lex_item.empty : name;
}

static lex_item.t parse_union (parser.t * p, decl_t * decl) {
  lex_item.t name = {0};
  lex_item.t item = collect(p, decl);
  if (item.type == item_id) {
    name = identifier.parse_typed(p, union_i, item, true);
    append(decl, name);
    item = collect(p, decl);
  }

  if (item.type != item_open_symbol || item.value[0] != '{') {
    return errorf(p, item, decl, "in union: expecting '{' but got %s", lex_item.to_string(item));
  }

  append(decl, item);
  parse_struct_block(p, decl, item);
  return (decl->error) ? lex_item.empty : name;
}

static lex_item.t parse_enum (parser.t * p, decl_t * decl) {
  lex_item.t name = {0};
  lex_item.t item = collect(p, decl);
  if (item.type == item_id) {
    name = identifier.parse_typed(p, enum_i, item, true);
    append(decl, name);
    item = collect(p, decl);
  }

  if (item.type != item_open_symbol || item.value[0] != '{') {
    return errorf(p, item, decl, "in enum: expecting '{' but got %s", lex_item.to_string(item));
  }

  append(decl, item);
  parse_enum_block(p, decl, item);
  return (decl->error) ? lex_item.empty : name;
}

static lex_item.t parse_export_block(parser.t * p, decl_t * decl) {
  lex_item.t item;

  int escaped_id = 0;
  while(true) {
    int record = 0;
    item = collect(p, decl);

    switch(item.type) {
      case item_c_code:
      case item_quoted_string:
      case item_comment:
      case item_symbol:
      case item_preprocessor:
        record = 1;
        break;

      case item_arrow:
        escaped_id = 1;
        record     = 1;
        break;

      case item_id:
        if (escaped_id == 0) item = identifier.parse(p, item, true);
        record     = 1;
        escaped_id = 0;
        break;

      case item_close_symbol:
        if (item.value[0] == '}') {
          item.value  = "block";
          item.length = strlen("block");
          return item;
        };
        break;

      case item_eof:
        return errorf(p, item, decl, "in block: unmatched '{'");
      case item_error:
        parser.backup(p, item);
        decl->error = true;
        return lex_item.empty;
      default:
        return errorf(p, item, decl, "in block: unexpected input '%s'", lex_item.to_string(item));
    }
    if (record) append(decl, item);
  }

}

static lex_item.t parse_struct_block( parser.t * p, decl_t * decl, lex_item.t start) {
  lex_item.t item;
  do {
    item = parse_declaration(p, decl);
    if (item.type == item_eof) {
        return errorf(p, start, decl, "in struct block: missing matching '}' before end of file");
    }
    if (item.type == item_error) return item;
  } while (!(item.type == item_close_symbol && item.value[0] == '}'));
  append(decl, item);
  return item;
}

static lex_item.t parse_enum_declaration(parser.t * p, decl_t * decl) {
  lex_item.t item = collect(p, decl);
  if (item.type != item_id) {
    if (item.type == item_close_symbol && item.value[0] == '}') return item;
    return errorf(p, item, decl, "in enum: expecting identifier but got %s",
        lex_item.to_string(item));
  }
  if (strcmp("as", item.value) == 0) return item;
  append(decl, item);

  item = collect(p, decl);
  if (!(item.type == item_symbol && (item.value[0] != ',' || item.value[0] != '='))) {
    return item;
  }
  append(decl, item);

  if (item.value[0] == '=') {
    item = collect(p, decl);
    if (item.type != item_number) {
      return errorf(p, item, decl, "in enum: expecting a number but got %s",
          lex_item.to_string(item));
    }
    append(decl, item);

    item = collect(p, decl);
    if (item.type != item_symbol || item.value[0] != ',') {
      return item;
    }
    append(decl, item);
  }

  return item;
}

static lex_item.t parse_enum_block ( parser.t * p, decl_t * decl, lex_item.t start) {
  lex_item.t item;
  do {
    item = parse_enum_declaration(p, decl);
    if (item.type == item_eof) {
        return errorf(p, start, decl, "in struct block: missing matching '}' before end of file");
    }
    if (item.type == item_error) return item;
  } while (!(item.type == item_close_symbol && item.value[0] == '}'));
  append(decl, item);
  return item;
}

static lex_item.t parse_type (parser.t * p, decl_t * decl) {
  lex_item.t item;
  lex_item.t name = {0};
  int escaped_id = 0;
  do {
    item = collect(p, decl);
    switch(item.type) {
      case item_arrow:
        escaped_id = 1;
        append(decl, item);
        continue;
      case item_id:
        if (escaped_id == 0) item = identifier.parse(p, item, true);
        name = item;
        break;
      case item_symbol:
        if (item.value[0] == '*') break;
      case item_open_symbol:
        if (item.value[0] == '(') {
          append(decl, item);
          return name;
        }
      default:
        return errorf(p, item, decl, "in type declaration: expecting identifier or '('");
    }
    append(decl, item);
  } while (item.type != item_open_symbol || item.value[0] != '(');
  return lex_item.empty;
}

static lex_item.t parse_variable (parser.t * p, decl_t * decl) {
  lex_item.t item;
  lex_item.t name = {0};
  bool escaped_id = 0;
  bool escape_till_semicolon = false;
  do {
    item = collect(p, decl);
    if (escape_till_semicolon && (item.type != item_symbol || item.value[0] != ';')) {
      append(decl, item);
      continue;
    }
    switch(item.type) {
      case item_arrow:
        escaped_id = true;
        append(decl, item);
        continue;
      case item_id:
        if (strcmp(item.value, "as") == 0) {
          rewind_whitespace(p, decl, item);
          return name;
        }
        if (escaped_id == false) item = identifier.parse(p, item, true);
        name = item;
        break;
      case item_symbol:
        if (item.value[0] == '*') break;
        if (item.value[0] == '=') {
          escape_till_semicolon = true;
          break;
        }
        if (item.value[0] == ';') {
          parser.backup(p, item);
          return name;
        }
      case item_open_symbol:
        if (item.value[0] == '(') {
          lex_item.t star = parser.next(p);
          parser.backup(p, star);
          if (star.type == item_symbol && star.value[0] == '*') {
            append(decl, item);
            return parse_function_ptr(p, decl);
          }
          parser.backup(p, item);
          rewind_until(p, decl, name);
          return parse_function(p, decl);
        }
        if (item.value[0] == '[') {
          append(decl, item);
          item = parser.next(p);
          if (item.type == item_number) {
            append(decl, item);
            item = parser.next(p);
          }
          if (item.type != item_close_symbol || item.value[0] != ']') {
            return errorf(p, item, decl,
                "in type declaration: expecting terminating ']' but got %s",
                lex_item.to_string(item)
            );
          }
          break;
        }
      default:
        return errorf(p, item, decl, "in type declaration: expecting identifier or '(' but got %s", lex_item.to_string(item));
    }
    append(decl, item);
  } while (item.type != item_eof);
  return name;
}

static lex_item.t parse_function_args(parser.t * p, decl_t * decl) {
  int level = 1;
  int escaped_id = 0;
  lex_item.t item;
  do {
    item = collect(p, decl);
    switch(item.type) {
      case item_arrow:
        escaped_id = 1;
        append(decl, item);
        continue;
      case item_id:
        if (escaped_id == 0) {
          item = identifier.parse(p, item, true);
        }
        break;
      case item_open_symbol:
        if (item.value[0] == '(') level++;
        break;
      case item_close_symbol:
        if (item.value[0] == ')') level--;
        break;
      default:
        break;
    }

    escaped_id = 0;
    append(decl, item);
  } while (level > 0 && item.type != item_eof && item.type != item_error);
  return item;
}

static lex_item.t parse_function (parser.t * p, decl_t * decl) {
  lex_item.t name = parse_type(p, decl);

  if (decl->error) {
    emit(p, decl, true, false);
    decl->items = NULL;
    decl->length = 0;
    return lex_item.empty;
  }

  parse_function_args(p, decl);

  if (decl->error) return lex_item.empty;
  return name;
}
