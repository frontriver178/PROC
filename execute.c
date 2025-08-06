#include "shell.h"

void execute_pipeline(ShellState *shell, char *command) {
    char *commands[MAX_ARGS];
    int num_commands = 0;
    char *saveptr;
    char *cmd_copy = strdup(command);
    
    char *token = strtok_r(cmd_copy, "|", &saveptr);
    while (token != NULL && num_commands < MAX_ARGS - 1) {
        while (*token == ' ' || *token == '\t') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && (*end == ' ' || *end == '\t')) *end-- = '\0';
        
        commands[num_commands++] = strdup(token);
        token = strtok_r(NULL, "|", &saveptr);
    }
    
    free(cmd_copy);
    
    if (num_commands == 1) {
        char **args = parse_command(commands[0]);
        if (args != NULL) {
            execute_command(shell, args);
            free_args(args);
        }
        free(commands[0]);
        return;
    }
    
    int pipes[num_commands - 1][2];
    pid_t pids[num_commands];
    
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            for (int j = 0; j < num_commands; j++) {
                free(commands[j]);
            }
            return;
        }
    }
    
    for (int i = 0; i < num_commands; i++) {
        pids[i] = fork();
        
        if (pids[i] == 0) {
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            if (i < num_commands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            for (int j = 0; j < num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            char **args = parse_command(commands[i]);
            if (args != NULL) {
                expand_wildcards(args);
                char *substituted = substitute_alias(shell, args[0]);
                if (substituted != args[0]) {
                    free(args[0]);
                    args[0] = strdup(substituted);
                }
                
                execvp(args[0], args);
                fprintf(stderr, "%s: command not found\n", args[0]);
                exit(127);
            }
            exit(1);
        } else if (pids[i] < 0) {
            perror("fork");
        }
    }
    
    for (int i = 0; i < num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    for (int i = 0; i < num_commands; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }
    
    for (int i = 0; i < num_commands; i++) {
        free(commands[i]);
    }
}

static int is_builtin(const char *command) {
    const char *builtins[] = {
        "cd", "pushd", "popd", "dirs", "history", 
        "alias", "unalias", "prompt", "exit", NULL
    };
    
    for (int i = 0; builtins[i] != NULL; i++) {
        if (strcmp(command, builtins[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

static void execute_builtin(ShellState *shell, char **args) {
    if (strcmp(args[0], "cd") == 0) {
        builtin_cd(shell, args);
    } else if (strcmp(args[0], "pushd") == 0) {
        builtin_pushd(shell, args);
    } else if (strcmp(args[0], "popd") == 0) {
        builtin_popd(shell, args);
    } else if (strcmp(args[0], "dirs") == 0) {
        builtin_dirs(shell, args);
    } else if (strcmp(args[0], "history") == 0) {
        builtin_history(shell, args);
    } else if (strcmp(args[0], "alias") == 0) {
        builtin_alias(shell, args);
    } else if (strcmp(args[0], "unalias") == 0) {
        builtin_unalias(shell, args);
    } else if (strcmp(args[0], "prompt") == 0) {
        builtin_prompt(shell, args);
    } else if (strcmp(args[0], "exit") == 0) {
        builtin_exit(shell, args);
    }
}

static void execute_external(char **args) {
    pid_t pid = fork();
    
    if (pid == 0) {
        execvp(args[0], args);
        fprintf(stderr, "%s: command not found\n", args[0]);
        exit(127);
    } else if (pid < 0) {
        perror("fork");
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

void execute_command(ShellState *shell, char **args) {
    if (args[0] == NULL) {
        return;
    }
    
    expand_wildcards(args);
    
    char *substituted = substitute_alias(shell, args[0]);
    if (substituted != args[0]) {
        free(args[0]);
        args[0] = strdup(substituted);
    }
    
    if (is_builtin(args[0])) {
        execute_builtin(shell, args);
    } else {
        execute_external(args);
    }
}

void execute_script(ShellState *shell, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror(filename);
        return;
    }
    
    char line[MAX_COMMAND_LENGTH];
    while (fgets(line, sizeof(line), fp) != NULL) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }
        
        add_to_history(shell, line);
        
        char **args = parse_command(line);
        if (args != NULL) {
            execute_command(shell, args);
            free_args(args);
        }
        
        if (shell->exit_flag) {
            break;
        }
    }
    
    fclose(fp);
}