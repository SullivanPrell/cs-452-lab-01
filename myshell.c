/*
 * This code implements a simple shell program
 * It supports the internal shell command "exit",
 * backgrounding processes with "&", input redirection
 * with "<" and output redirection with ">".
 * However, this is not complete.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

extern char **parseline();

extern int errno;

int piping(char **args);

int ampersand(char **args);

int internal_command(char **args);

void do_command(char **args, int block, int input, char *input_filename, int output, char *output_filename);

void sig_handler(int signal);

int redirect_input(char **args, char **input_filename);

int redirect_output(char **args, char **output_filename);

void parsePipe(char **args, char ***commands);

void piped_redirection(char ***args, int block, int input, char *input_filename, int output, char *output_filename);

int background_processing(int block, int *status, pid_t child_id);

/*
 * Handle exit signals from child processes
 */
void sig_handler(int signal) {
    int status;
    int result = wait(&status);

    //printf("Wait returned %d\n", result);
}

/*
 * The main shell function
 */
main() {
    int i;
    char **args;
    int result;
    int block;
    int output;
    int input;
    char *output_filename;
    char *input_filename;
    int inputIndex;
    int outputIndex;
    int inputNum;
    int outputNum;
    int pipes;

    // Loop forever
    while (1) {

        // Print out the prompt and get the input
        printf("452 Shell ~]$ ");
        args = parseline();

        // No input, continue
        if (args[0] == NULL)
            continue;


        // Check for internal shell commands, such as exit
        if (internal_command(args))
            continue;

        // Check for an ampersand
        block = (ampersand(args) == 0);

        if (block) {
            // Set up the signal handler - foreground
            signal(SIGCHLD, sig_handler);
        } else {
            // Set up the signal handler - background
            signal(SIGCHLD, SIG_IGN);
        }

        pipes = piping(args);
        char **filler[pipes + 1];


        if (pipes > 1) {
            int i;
            for (i = 0; i < pipes + 1; i++) {
                filler[i] = malloc(sizeof(char **));
            }
            parsePipe(args, filler);
        }
        // Check for redirected input
        if (pipes > 1) {
            input = redirect_input(filler[pipes - 1], &input_filename);
        } else {
            input = redirect_input(args, &input_filename);
        }
        switch (input) {
            case -1:
                printf("Syntax error!\n");
                continue;
                break;
            case 0:
                break;
            case 1:
                printf("Redirecting input from: %s\n", input_filename);
                break;
        }

        // Check for redirected output
        if (pipes > 1) {
            //outputNum=0;
            output = redirect_output(filler[pipes - 1], &output_filename);
        } else {
            output = redirect_output(args, &output_filename);
        }

        switch (output) {
            case -1:
                printf("Syntax error!\n");
                continue;
                break;
            case 0:
                break;
            case 1:
                printf("Redirecting output to: %s\n", output_filename);
                break;
            case 2:
                printf("Appending output to: %s\n", output_filename);
        }
        // Do the command
        if (pipes > 1) {
            piped_redirection(filler, block, input, input_filename, output, output_filename);
            int i;
            for (i = 0; i < pipes + 1; i++) {
                free(filler[i]);
            }
        } else {
            do_command(args, block, input, input_filename, output, output_filename);
        }
    }
}

/*
 * Check for ampersand as the last argument
 */
int ampersand(char **args) {
    int i;

    for (i = 1; args[i] != NULL; i++);

    if (args[i - 1][0] == '&') {
        free(args[i - 1]);
        args[i - 1] = NULL;
        return 1;
    } else {
        return 0;
    }

    return 0;
}

/*
 * Check for internal commands
 * Returns true if there is more to do, false otherwise
 */
int internal_command(char **args) {
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    } else if (strcmp(args[0], "cd") == 0) {
        chdir(args[1]);
    }

    return 0;
}

/*
 * Do the command
 */
void do_command(char **args, int block, int input, char *input_filename, int output, char *output_filename) {

    int result;
    pid_t child_id;
    int status;

    // Fork the child process
    child_id = fork();

    // Check for errors in fork()
    switch (child_id) {
        case EAGAIN:
            perror("Error EAGAIN: ");
            return;
        case ENOMEM:
            perror("Error ENOMEM: ");
            return;
    }

    if (child_id == 0) {

        // Set up redirection in the child process
        if (input) {
            freopen(input_filename, "r", stdin);
        }
        if (output == 1) {
            freopen(output_filename, "w+", stdout);
        } else if (output == 2) {
            freopen(output_filename, "a", stdout);
        }
        // Execute the command
        result = execvp(args[0], args);

        exit(-1);
    }

    result = background_processing(block, &status, child_id);

}


/*
 * Check for input redirection
 */
int redirect_input(char **args, char **input_filename) {
    int i;
    int j;

    for (i = 0; args[i] != NULL; i++) {

        // Look for the <
        if (args[i][0] == '<') {
            free(args[i]);

            // Read the filename
            if (args[i + 1] != NULL) {
                *input_filename = args[i + 1];
            } else {
                return -1;
            }

            // Adjust the rest of the arguments in the array
            for (j = i; args[j - 1] != NULL; j++) {
                args[j] = args[j + 2];
            }
            return 1;
        }
    }
    return 0;
}

/*
 * Check for output redirection
 */
int redirect_output(char **args, char **output_filename) {
    int i;
    int j;

    for (i = 0; args[i] != NULL; i++) {

        // Look for the >
        if (args[i][0] == '>') {
            if (args[i + 1][0] == '>') {
                printf("we found it\n");
                free(args[i]);
                free(args[i + 1]);
                // Get the filename
                if (args[i + 2] != NULL) {
                    *output_filename = args[i + 2];
                } else {
                    return -1;
                }
                // Adjust the rest of the arguments in the array
                for (j = i; args[j - 1] != NULL; j++) {
                    args[j] = args[j + 3];
                }
                return 2;
            }
                // Get the filename
            else {
                if (args[i + 1] != NULL) {
                    *output_filename = args[i + 1];
                } else {
                    return -1;
                }
                // Adjust the rest of the arguments in the array
                for (j = i; args[j - 1] != NULL; j++) {
                    args[j] = args[j + 2];
                }
                return 1;
            }
        }
    }
    return 0;
}



//Piping things_____________________________

//Determines if there is a pipe in the command
int piping(char **args) {
    int i;
    int a;
    int out = 1;

    for (i = 0; args[i] != NULL; i++);

    for (a = 0; a < i; a++) {
        if (strchr(args[a], '|') != NULL) {
            out++;
        }
    }

    return out;
}

//takes in an array of args and an empty 3d array. The array will have 2 slots, in which you can put in sequences of commands
void parsePipe(char **args, char ***commands) {
    int p = 0;
    int a = 0;
    int b = 0;

    while (args[p] != NULL) {
        if (strchr(args[p], '|') != NULL) {
            a++;
            p++;
            b = 0;

        }
        commands[a][b] = args[p];
        p++;
        b++;
    }
    commands[a + 1] = NULL;
    commands[a][b] = NULL;

}

void piped_redirection(char ***args, int block, int input, char *input_filename, int output, char *output_filename) {
    int i;
    for (i = 0; args[i] != NULL; i++);


    int result;

    int status;
    int directors[2 * (i - 1)];
    pid_t p[i];

    int a;


    int b;
    for (b = 0; b < i - 1; b++) {
        int check = pipe(directors + (2 * b));
        if (check < 0) {
            printf("Pipes were unable to start\n");
        }
    }

    for (a = 0; a < i; a++) {
        p[a] = fork();

        switch (p[a]) {
            case EAGAIN:
                perror("Error EAGAIN: ");
                return;
            case ENOMEM:
                perror("Error ENOMEM: ");
                return;
        }

        if (p[a] == 0) {
            if (a == 0) {
                int check = dup2(directors[2 * a + 1], STDOUT_FILENO);
                if (check < 0) {
                    printf("Start Case Dup Error\n");
                }

                for (b = 0; b < 2 * i - 2; b++) {
                    close(directors[b]);
                }

                if (input) {
                    freopen(input_filename, "r+", stdin);
                }
                check = execvp(args[a][0], args[a]);
                if (check < 0) {
                    printf("Execute Command %d Fail. Error = %d\n", a, errno);
                    exit(-1);
                }
            } else if (a + 1 == i) {
                int check = dup2(directors[2 * (a - 1)], STDIN_FILENO);
                if (check < 0) {
                    printf("End Case Dup Error\n", check, a, 2 * (a - 1));
                    exit(-1);
                }
                for (b = 0; b < 2 * i - 2; b++) {
                    close(directors[b]);
                }
                if (output == 1) {
                    freopen(output_filename, "w+", stdout);
                } else if (output == 2) {
                    freopen(output_filename, "a", stdout);
                }
                check = execvp(args[a][0], args[a]);
                if (check < 0) {
                    printf("Execute Command %d Fail. \nFailed Command was %s. \nExecvp returned %d\n", a, args[a][0],
                           errno);
                    exit(-1);
                }
            } else {
                if (dup2(directors[2 * a + 1], STDOUT_FILENO) < 0) {
                    printf("Middle Case Write Dup Error\n");
                    exit(-1);
                }
                if (dup2(directors[2 * (a - 1)], STDIN_FILENO) < 0) {
                    printf("Middle Case Read Dup Error\n");
                    exit(-1);
                }

                for (b = 0; b < 2 * i - 2; b++) {
                    close(directors[b]);
                }

                if (execvp(args[a][0], args[a]) < 0) {
                    printf("Execute Command %d Fail\n", a);
                    exit(-1);
                }
            }
        }
    }
    if (block) {
        for (b = 0; b < 2 * i - 2; b++) {
            close(directors[b]);
        }
        for (b = 0; b < i; b++) {
            wait(&status);
            //result = waitpid(p[b], &status, 0);
        }
    } else {
        for (b = 0; b < 2 * i - 2; b++) {
            close(directors[b]);
        }
        for (b = 0; b < i; b++) {
            //wait(&status);
            result = background_processing(block, &status, p[b]);
        }
    }
}

int background_processing(int block, int *status, pid_t child_id) {
    // Wait for the child process to complete, if necessary
    if (block) {
        printf("Waiting for child, pid = %d\n", child_id);
        return waitpid(child_id, &status, 0);
    } else {
        printf("[%d]\n", child_id);
        return waitpid(child_id, &status, WNOHANG);

    }
}


//___________________________________________________



