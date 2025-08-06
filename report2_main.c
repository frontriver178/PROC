#include "shell.h"

int main(int argc, char *argv[]) {
    ShellState shell;
    init_shell(&shell);
    
    if (argc > 1) {
        execute_script(&shell, argv[1]);
        cleanup_shell(&shell);
        return 0;
    }
    
    while (!shell.exit_flag) {
        char *input = read_command(&shell);
        if (input == NULL) {
            continue;
        }
        
        if (strlen(input) == 0) {
            continue;
        }
        
        char *command_to_execute = input;
        
        if (input[0] == '!') {
            char *hist_cmd = get_history_command(&shell, input);
            if (hist_cmd != NULL) {
                printf("%s\n", hist_cmd);
                command_to_execute = hist_cmd;
            } else {
                fprintf(stderr, "%s: event not found\n", input);
                continue;
            }
        }
        
        add_to_history(&shell, command_to_execute);
        
        if (strchr(command_to_execute, '|') != NULL) {
            execute_pipeline(&shell, command_to_execute);
        } else {
            char **args = parse_command(command_to_execute);
            if (args != NULL) {
                execute_command(&shell, args);
                free_args(args);
            }
        }
    }
    
    cleanup_shell(&shell);
    return 0;
}