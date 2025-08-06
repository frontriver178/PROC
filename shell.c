#include "shell.h"

void init_shell(ShellState *shell) {
    memset(shell, 0, sizeof(ShellState));
    strcpy(shell->prompt, DEFAULT_PROMPT);
    
    shell->dir_stack.top = -1;
    
    if (getcwd(shell->cwd, sizeof(shell->cwd)) == NULL) {
        perror("getcwd");
        exit(1);
    }
}

void cleanup_shell(ShellState *shell) {
    for (int i = 0; i < shell->history_count; i++) {
        free(shell->history[i].command);
    }
    
    for (int i = 0; i <= shell->dir_stack.top; i++) {
        free(shell->dir_stack.dirs[i]);
    }
    
    for (int i = 0; i < shell->alias_count; i++) {
        free(shell->aliases[i].alias);
        free(shell->aliases[i].command);
    }
}

char *read_command(ShellState *shell) {
    static char buffer[MAX_COMMAND_LENGTH];
    
    printf("%s", shell->prompt);
    fflush(stdout);
    
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        if (feof(stdin)) {
            printf("\n");
            shell->exit_flag = 1;
            return NULL;
        }
        return NULL;
    }
    
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    
    return buffer;
}

char **parse_command(char *command) {
    static char *args[MAX_ARGS];
    int argc = 0;
    char *token;
    char *saveptr;
    
    char *cmd_copy = strdup(command);
    if (!cmd_copy) {
        return NULL;
    }
    
    token = strtok_r(cmd_copy, " \t", &saveptr);
    while (token != NULL && argc < MAX_ARGS - 1) {
        args[argc++] = strdup(token);
        token = strtok_r(NULL, " \t", &saveptr);
    }
    args[argc] = NULL;
    
    free(cmd_copy);
    return args;
}

void add_to_history(ShellState *shell, const char *command) {
    if (strlen(command) == 0) {
        return;
    }
    
    if (shell->history_count > 0 && 
        strcmp(shell->history[shell->history_count - 1].command, command) == 0) {
        return;
    }
    
    if (shell->history_count >= MAX_HISTORY) {
        free(shell->history[0].command);
        memmove(&shell->history[0], &shell->history[1], 
                (MAX_HISTORY - 1) * sizeof(History));
        shell->history_count--;
    }
    
    shell->history[shell->history_count].command = strdup(command);
    shell->history_count++;
    shell->history_index = shell->history_count;
}

char *substitute_alias(ShellState *shell, const char *command) {
    for (int i = 0; i < shell->alias_count; i++) {
        if (strcmp(shell->aliases[i].alias, command) == 0) {
            return shell->aliases[i].command;
        }
    }
    return (char *)command;
}

void free_args(char **args) {
    if (args == NULL) return;
    
    for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
}

void expand_wildcards(char **args) {
    glob_t glob_result;
    int i = 0;
    
    while (args[i] != NULL) {
        if (strchr(args[i], '*') != NULL || strchr(args[i], '?') != NULL) {
            if (glob(args[i], GLOB_NOCHECK | GLOB_TILDE, NULL, &glob_result) == 0) {
                if (glob_result.gl_pathc > 0) {
                    free(args[i]);
                    args[i] = strdup(glob_result.gl_pathv[0]);
                    
                    int j;
                    for (j = 1; j < glob_result.gl_pathc && args[i + j] == NULL; j++) {
                        if (i + j < MAX_ARGS - 1) {
                            int k;
                            for (k = MAX_ARGS - 2; k > i + j; k--) {
                                args[k] = args[k - glob_result.gl_pathc + 1];
                            }
                            args[i + j] = strdup(glob_result.gl_pathv[j]);
                        }
                    }
                }
                globfree(&glob_result);
            }
        }
        i++;
    }
}

char *get_history_command(ShellState *shell, const char *pattern) {
    if (strcmp(pattern, "!!") == 0) {
        if (shell->history_count > 0) {
            return shell->history[shell->history_count - 1].command;
        }
    } else if (pattern[0] == '!') {
        const char *search = pattern + 1;
        for (int i = shell->history_count - 1; i >= 0; i--) {
            if (strncmp(shell->history[i].command, search, strlen(search)) == 0) {
                return shell->history[i].command;
            }
        }
    }
    return NULL;
}