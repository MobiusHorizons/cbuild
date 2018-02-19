#ifndef _package_cli_
#define _package_cli_

#include <stdbool.h>

typedef struct {
	const char * description;
	const char * long_name;
	const char * short_name;
	bool   required;
} cli_flag_options;

typedef struct {
	void       * cb;
	void       * ctx;
	char       * name;
	const char * description;
} cli_cmd_t;

typedef struct {
	hash_t      * flags;
	hash_t      * commands;
	cli_cmd_t       * default_command;

	bool          has_commands;

	const char  * usage;
	const char  * name;

	int           argc;
	const char ** argv;
} cli_t;

typedef int (*cli_cmd_cb)(cli_t * cli, char * cmd, void * ctx);

cli_t * cli_new(const char * usage);
void cli_flag_bool(cli_t * cli, bool * out, cli_flag_options options);
void cli_flag_int(cli_t * cli, long * out, cli_flag_options options);
void cli_flag_string(cli_t * cli, const char ** out, cli_flag_options options);
void cli_command(cli_t * cli, const char * name, cli_cmd_cb cb, const char * description, bool is_default, void * ctx);
int cli_usage(cli_t * cli);
int cli_parse(cli_t * cli, int argc, const char ** argv);

#endif
