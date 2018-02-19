#include <stdlib.h>
#include <stdio.h>
export {
#include <stdbool.h>
}

build depends "deps/hash/hash.c";
#include "deps/hash/hash.h"

export typedef struct {
	const char * description;
	const char * long_name;
	const char * short_name;
	bool   required;
} flag_options;

enum flag_type {
	flag_type_bool = 0,
	flag_type_int,
	flag_type_string,
};

typedef struct {
	void           * dest;
	enum flag_type   type;
	const char     * description;
	char           * long_name;
	char           * short_name;
	char             short_name_buf[3];
	bool             required;
	bool             has_value;
} flag_t;

export typedef struct {
	void       * cb;
	void       * ctx;
	char       * name;
	const char * description;
} cmd_t;

export typedef struct {
	hash_t      * flags;
	hash_t      * commands;
	cmd_t       * default_command;

	bool          has_commands;

	const char  * usage;
	const char  * name;

	int           argc;
	const char ** argv;
} cli_t as t;

export typedef int (*cmd_cb)(cli_t * cli, char * cmd, void * ctx);

export cli_t * new(const char * usage) {
	cli_t * cli = calloc(1, sizeof(cli_t));
	cli->flags    = hash_new();
	cli->commands = hash_new();
	cli->usage    = usage;

	return cli;
}

static void flag(cli_t * cli, void * dest, enum flag_type type, flag_options options) {
	flag_t * f = calloc(1, sizeof(flag_t));

	f->type        = type;
	f->dest        = dest;
	f->required    = options.required;
	f->description = options.description;
	f->long_name   = malloc(strlen(options.long_name) + 3);

	strcpy(f->long_name, "--");
	strcat(f->long_name, options.long_name);

	hash_set(cli->flags, f->long_name, f);

	if (options.short_name == NULL) {
		f->short_name = NULL;
	} else {
		f->short_name_buf[0] = '-';
		f->short_name_buf[1] = options.short_name[0];
		f->short_name_buf[2] = 0;
		f->short_name        = f->short_name_buf;

		hash_set(cli->flags, f->short_name, f);
	}
}

export void flag_bool(cli_t * cli, bool * out, flag_options options) {
	options.required = false;
	flag(cli, out, flag_type_bool, options);
}

export void flag_int(cli_t * cli, long * out, flag_options options) {
	flag(cli, out, flag_type_string, options);
}

export void flag_string(cli_t * cli, const char ** out, flag_options options) {
	flag(cli, out, flag_type_string, options);
}

export void command(cli_t * cli, const char * name, cmd_cb cb, const char * description, bool is_default, void * ctx) {
	cli->has_commands = true;
	cmd_t * cmd = calloc(1, sizeof(cmd_t));

	cmd->name        = strdup(name);
	cmd->cb          = cb;
	cmd->ctx         = ctx;
	cmd->description = description;

	if (is_default) cli->default_command = cmd;
	hash_set(cli->commands, cmd->name, cmd);
}

void parse_flag(flag_t * flag, const char * arg) {
	switch(flag->type) {
		case flag_type_bool:
			if (flag->dest) *((bool *) flag->dest) = true;
			break;
		case flag_type_int:
			if (flag->dest) *((long *) flag->dest) = strtol(arg, NULL, 0);
			break;
		case flag_type_string:
			if (flag->dest) *(const char**)flag->dest = arg;
	}
	flag->has_value = true;
}

export int usage(cli_t * cli) {
	fprintf(stderr, "usage %s: [options]%s %s\n\n", cli->name, cli->has_commands ? " command" : "", cli->usage);

	{ // flags
		fprintf(stderr, "FLAGS: \n\n");
		int width = strlen("--help");
		hash_each_key(cli->flags, {
				width = strlen(key) > width ? strlen(key) : width;
		});

		int w = width - strlen("--help");
		fprintf(stderr, "          %s%*.*s %s\t%s\n", "--help", w, w, " ", "-h", "prints this message");

		hash_each(cli->flags, {
				if (key[1] != '-') continue;
				flag_t * flag = (flag_t *) val;
				int w = width - strlen(key);
				fprintf(stderr, "          %s%*.*s %s\t%s\n", key, w, w, " ",
						flag->short_name ? flag->short_name : "  ", flag->description);
		})
		fprintf(stderr, "\n");
	}

	if (cli->has_commands) {
		fprintf(stderr, "COMMANDS: \n\n");
		int width = 0;
		hash_each_key(cli->commands, {
				width = strlen(key) > width ? strlen(key) : width;
		});

		hash_each(cli->commands, {
				cmd_t * cmd = (cmd_t *) val;
				int w  = width - strlen(key);
				fprintf(stderr, "%s%s%*.*s  %s\n",
						cmd==cli->default_command ? "(default) " : "          ",
						cmd->name, w, w, " ", cmd->description);
		})
		fprintf(stderr, "\n");

	}
	return -1;
}

export int parse(cli_t * cli, int argc, const char ** argv) {
	cli->argv = malloc(sizeof(char*) * (argc - 1));
	cli->name = argv[0];

	cmd_t * cmd = NULL;

	int i;
	// collect args, commands, flags
	for ( i = 1; i < argc; i++ ) {
		const char * arg = argv[i];
		if (strcmp("--", arg) == 0) break;
		if (strcmp("-h", arg) == 0 || strcmp("--help", arg) == 0) return usage(cli);

		char * arg_allocated = strdup(arg);

		flag_t * flag = (flag_t *) hash_get(cli->flags, arg_allocated);
		if (flag != NULL) {
			parse_flag(flag, arg);
			free(arg_allocated);
			continue;
		}

		if (arg[0] == '-') {
			fprintf(stderr, "Unrecognized flag '%s'\n\n", arg);
			free(arg_allocated);
			return usage(cli);
		}

		if (cmd == NULL) {
			cmd = (cmd_t *) hash_get(cli->commands, arg_allocated);
			if (cmd) {
				free(arg_allocated);
				continue;
			}
		}

		cli->argv[cli->argc] = arg;
		cli->argc++;
		free(arg_allocated);
	}

	// collect args after a --
	for (; i < argc; i++ ) {
		cli->argv[cli->argc] = argv[i];
		cli->argc++;
	}

	if (cmd == NULL) cmd = cli->default_command;
	if (cmd == NULL) return cli->has_commands ? usage(cli) : 0;

	cmd_cb cb = cmd->cb;
	int result = cb(cli, cmd->name, cmd->ctx);
	if (result == 0) return result;
	return usage(cli);
}
