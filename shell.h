#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <glob.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 256
#define MAX_HISTORY 1000
#define MAX_DIR_STACK 50
#define MAX_ALIASES 100
#define DEFAULT_PROMPT "mysh> "

typedef struct {
    char *command;
} History;

typedef struct {
    char *alias;
    char *command;
} Alias;

typedef struct {
    char *dirs[MAX_DIR_STACK];
    int top;
} DirStack;

typedef struct {
    History history[MAX_HISTORY];
    int history_count;
    int history_index;
    
    DirStack dir_stack;
    
    Alias aliases[MAX_ALIASES];
    int alias_count;
    
    char prompt[256];
    char cwd[PATH_MAX];
    
    int exit_flag;
} ShellState;

void init_shell(ShellState *shell);
void cleanup_shell(ShellState *shell);
char *read_command(ShellState *shell);
char **parse_command(char *command);
void execute_command(ShellState *shell, char **args);
void execute_pipeline(ShellState *shell, char *command);
void add_to_history(ShellState *shell, const char *command);
void expand_wildcards(char **args);
char *substitute_alias(ShellState *shell, const char *command);
void execute_script(ShellState *shell, const char *filename);

void builtin_cd(ShellState *shell, char **args);
void builtin_pushd(ShellState *shell, char **args);
void builtin_popd(ShellState *shell, char **args);
void builtin_dirs(ShellState *shell, char **args);
void builtin_history(ShellState *shell, char **args);
void builtin_alias(ShellState *shell, char **args);
void builtin_unalias(ShellState *shell, char **args);
void builtin_prompt(ShellState *shell, char **args);
void builtin_exit(ShellState *shell, char **args);

char *get_history_command(ShellState *shell, const char *pattern);
void free_args(char **args);

#endif