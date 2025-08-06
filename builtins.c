#include "shell.h"

void builtin_cd(ShellState *shell, char **args) {
    const char *dir;
    
    if (args[1] == NULL) {
        dir = getenv("HOME");
        if (dir == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return;
        }
    } else {
        dir = args[1];
    }
    
    if (chdir(dir) != 0) {
        perror("cd");
        return;
    }
    
    if (getcwd(shell->cwd, sizeof(shell->cwd)) == NULL) {
        perror("getcwd");
    }
}

void builtin_pushd(ShellState *shell, char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "pushd: no directory specified\n");
        return;
    }
    
    if (shell->dir_stack.top >= MAX_DIR_STACK - 1) {
        fprintf(stderr, "pushd: directory stack full\n");
        return;
    }
    
    shell->dir_stack.top++;
    shell->dir_stack.dirs[shell->dir_stack.top] = strdup(shell->cwd);
    
    builtin_cd(shell, args);
}

void builtin_popd(ShellState *shell, char **args) {
    if (shell->dir_stack.top < 0) {
        fprintf(stderr, "popd: directory stack empty\n");
        return;
    }
    
    char *dir = shell->dir_stack.dirs[shell->dir_stack.top];
    shell->dir_stack.top--;
    
    if (chdir(dir) != 0) {
        perror("popd");
    } else {
        if (getcwd(shell->cwd, sizeof(shell->cwd)) == NULL) {
            perror("getcwd");
        }
    }
    
    free(dir);
}

void builtin_dirs(ShellState *shell, char **args) {
    printf("%s", shell->cwd);
    
    for (int i = shell->dir_stack.top; i >= 0; i--) {
        printf(" %s", shell->dir_stack.dirs[i]);
    }
    printf("\n");
}

void builtin_history(ShellState *shell, char **args) {
    int start = 0;
    int count = shell->history_count;
    
    if (args[1] != NULL) {
        count = atoi(args[1]);
        if (count <= 0) {
            count = shell->history_count;
        }
        if (count > shell->history_count) {
            count = shell->history_count;
        }
        start = shell->history_count - count;
    }
    
    for (int i = start; i < shell->history_count; i++) {
        printf("%4d  %s\n", i + 1, shell->history[i].command);
    }
}

void builtin_alias(ShellState *shell, char **args) {
    if (args[1] == NULL) {
        for (int i = 0; i < shell->alias_count; i++) {
            printf("alias %s='%s'\n", shell->aliases[i].alias, shell->aliases[i].command);
        }
        return;
    }
    
    char *equals = strchr(args[1], '=');
    if (equals == NULL) {
        for (int i = 0; i < shell->alias_count; i++) {
            if (strcmp(shell->aliases[i].alias, args[1]) == 0) {
                printf("alias %s='%s'\n", shell->aliases[i].alias, shell->aliases[i].command);
                return;
            }
        }
        fprintf(stderr, "alias: %s: not found\n", args[1]);
        return;
    }
    
    *equals = '\0';
    char *alias_name = args[1];
    char *alias_value = equals + 1;
    
    if (*alias_value == '\'' || *alias_value == '"') {
        alias_value++;
        char *end = strrchr(alias_value, *equals + 1);
        if (end != NULL) {
            *end = '\0';
        }
    }
    
    for (int i = 0; i < shell->alias_count; i++) {
        if (strcmp(shell->aliases[i].alias, alias_name) == 0) {
            free(shell->aliases[i].command);
            shell->aliases[i].command = strdup(alias_value);
            return;
        }
    }
    
    if (shell->alias_count >= MAX_ALIASES) {
        fprintf(stderr, "alias: too many aliases\n");
        return;
    }
    
    shell->aliases[shell->alias_count].alias = strdup(alias_name);
    shell->aliases[shell->alias_count].command = strdup(alias_value);
    shell->alias_count++;
}

void builtin_unalias(ShellState *shell, char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "unalias: usage: unalias name\n");
        return;
    }
    
    for (int i = 0; i < shell->alias_count; i++) {
        if (strcmp(shell->aliases[i].alias, args[1]) == 0) {
            free(shell->aliases[i].alias);
            free(shell->aliases[i].command);
            
            for (int j = i; j < shell->alias_count - 1; j++) {
                shell->aliases[j] = shell->aliases[j + 1];
            }
            shell->alias_count--;
            return;
        }
    }
    
    fprintf(stderr, "unalias: %s: not found\n", args[1]);
}

void builtin_prompt(ShellState *shell, char **args) {
    if (args[1] == NULL) {
        printf("Current prompt: %s\n", shell->prompt);
        return;
    }
    
    strncpy(shell->prompt, args[1], sizeof(shell->prompt) - 1);
    shell->prompt[sizeof(shell->prompt) - 1] = '\0';
}

void builtin_exit(ShellState *shell, char **args) {
    shell->exit_flag = 1;
}