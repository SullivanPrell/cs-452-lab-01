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
#include <errno.h>
#include <signal.h>
#include <string.h>

extern char **parseline();
int piping(char**args);

int ampersand(char **args);
int internal_command(char **args);
void do_command(char **args, int block, int input, char *input_filename, int output, char *output_filename);
void sig_handler(int signal);
int redirect_input(char **args, char **input_filename);
int redirect_output(char **args, char **output_filename);
void do_pipe_command(char ***args, int block, int input, char *input_filename, int output, char *output_filename);
void parsePipe(char** args, char*** commands);
void test_pipes(char***args);
void do_multi_pipe_command(char ***args, int block, int input, char *input_filename, int output, char *output_filename);

/*
 * Handle exit signals from child processes
 */
void sig_handler(int signal) {
  int status;
  int result = wait(&status);

  printf("Wait returned %d\n", result);
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
 

  int pipes;

  // Set up the signal handler
  sigset(SIGCHLD, sig_handler);

  // Loop forever
  while(1) {

    // Print out the prompt and get the input
    printf("->");
    args = parseline();

    // No input, continue
    if(args[0] == NULL)
      continue;
	  

    // Check for internal shell commands, such as exit
    if(internal_command(args))
      continue;

    // Check for an ampersand
    block = (ampersand(args) == 0);
	
	pipes= piping(args);
	char **filler[pipes];
	//test_args(args);

	if(pipes>1){
		int i;
		for(i=0;i<pipes;i++){
			filler[i]=malloc(sizeof(char**));
		}
		parsePipe(args,filler);
		test_pipes(filler);
	}

    // Check for redirected input
    input = redirect_input(args, &input_filename);

    switch(input) {
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
    output = redirect_output(args, &output_filename);

    switch(output) {
    case -1:
      printf("Syntax error!\n");
      continue;
      break;
    case 0:
      break;
    case 1:
      printf("Redirecting output to: %s\n", output_filename);
      break;
    }
    // Do the command
	if(pipes>1){
		do_multi_pipe_command(filler, block,input, input_filename, output, output_filename);
		int i;
		for(int i=0;i<pipes;i++){
			free(filler[i]);
		}	
	}
	else{
	    do_command(args, block,
		       input, input_filename,
			   output, output_filename);
		}

  }
}

/*
 * Check for ampersand as the last argument
 */
int ampersand(char **args) {
  int i;

  for(i = 1; args[i] != NULL; i++) ;

  if(args[i-1][0] == '&') {
    free(args[i-1]);
    args[i-1] = NULL;
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
  if(strcmp(args[0], "exit") == 0) {
    exit(0);
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
  switch(child_id) {
  case EAGAIN:
    perror("Error EAGAIN: ");
    return;
  case ENOMEM:
    perror("Error ENOMEM: ");
    return;
  }

  if(child_id == 0) {

    // Set up redirection in the child process
    if(input)
      freopen(input_filename, "r", stdin);

    if(output)
      freopen(output_filename, "w+", stdout);

    // Execute the command
    result = execvp(args[0], args);

    exit(-1);
  }

  // Wait for the child process to complete, if necessary
  if(block) {
    printf("Waiting for child, pid = %d\n", child_id);
    result = waitpid(child_id, &status, 0);
  }
}



/*
 * Check for input redirection
 */
int redirect_input(char **args, char **input_filename) {
  int i;
  int j;

  for(i = 0; args[i] != NULL; i++) {

    // Look for the <
    if(args[i][0] == '<') {
      free(args[i]);

      // Read the filename
      if(args[i+1] != NULL) {
	*input_filename = args[i+1];
      } else {
	return -1;
      }

      // Adjust the rest of the arguments in the array
      for(j = i; args[j-1] != NULL; j++) {
	args[j] = args[j+2];
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

  for(i = 0; args[i] != NULL; i++) {

    // Look for the >
    if(args[i][0] == '>') {
      free(args[i]);

      // Get the filename
      if(args[i+1] != NULL) {
	*output_filename = args[i+1];
      } else {
	return -1;
      }

      // Adjust the rest of the arguments in the array
      for(j = i; args[j-1] != NULL; j++) {
	args[j] = args[j+2];
      }

      return 1;
    }
  }

  return 0;
}


//Piping things_____________________________

//Determines if there is a pipe in the command
int piping(char **args){
	int i;
	int a;
	int out=1;

	for(i=0;args[i]!=NULL;i++);

	for(a=0;a<i;a++){
		printf("%s",args[a]);
		if(strchr(args[a],'|')!=NULL){
			out++;
		}
	}
			printf("\n");

	return out;
}

//takes in an array of args and an empty 3d array. The array will have 2 slots, in which you can put in sequences of commands
void parsePipe(char** args, char*** commands){
	int p=0;
	int a=0;
	int b=0;

	while(args[p]!=NULL){
		if(strchr(args[p],'|')!=NULL){
		//	commands[a][b]='\0';
			a++;
			p++;
			b=0;

		}
		commands[a][b]=args[p];
		p++;
		b++;
	}
	commands[a+1]='\0';
	
}

void test_pipes(char ***args){
/*
	int b=0;
	printf("First Arr: ");
	while(args[0][b-1]!=NULL){
		printf("%s, ",args[0][b]);
		b++;
	}
	b=0;
	printf("\nSecond Arr: ");
	while(args[1][b-1]!=NULL){
		printf("%s, ",args[1][b]);
		b++;
	}
	printf("\n");
	*/

	int a=0;
	int b=0;
	while(args[a]!=NULL){
		printf("The number %d array contains: ",a);
		while(args[a][b]!=NULL){
			printf("%s, ",args[a][b]);
			b++;
		}
		a++;
		b=0;
		printf("\n");
	}

}

void test_args(char **args){
	int b=0;

	while(args[b]!=NULL){
		printf("%s, ",args[b]);
		b++;
	}printf("\n%d Args Tested \n",b);
}

void do_pipe_command(char ***args, int block, int input, char *input_filename, int output, char *output_filename) {
	int result1,result2;
	int status1,status2;
	
	int directors[2];
	pid_t p1, p2;
	pid_t parent=getpid();
	printf("%d\n",parent);
	char **proc1=args[0];
	char **proc2=args[1];

	if(pipe(directors)<0){
		printf("Pipes were unable to start\n");
	}
	p1=fork();

	switch(p1) {
	case EAGAIN:
		perror("Error EAGAIN: ");
		return;
	case ENOMEM:
		perror("Error ENOMEM: ");
		return;
	}

	if(p1==0){
		close(directors[0]);
		dup2(directors[1],STDOUT_FILENO);
		close(directors[1]);
		if(execvp(proc1[0],proc1)<0){
			printf("1 Failed\n");
			exit(-1);
		}
		//result1=execvp(proc1[0],proc1);
		//exit(0);
		
	}
	else{
		p2=fork();
		switch(p2) {
			case EAGAIN:
				perror("Error EAGAIN: ");
				return;
			case ENOMEM:
				perror("Error ENOMEM: ");
				return;
		}	
		if(p2==0){
			close(directors[1]);
			dup2(directors[0],STDIN_FILENO);
			close(directors[0]);

			if(execvp(proc2[0],proc2)<0){
				printf("2 Failed\n");
				exit(-1);
			}

			//result2=execvp(proc2[0],proc2);
			//exit(-1);
		}
		else{
			if(block) {
				close(directors[0]);
				close(directors[1]);
			//	printf("Waiting for child 1, pid = %d\n", p1);
				result2 = waitpid(p1, &status1, 0);	
				//printf("One has finished\n");		
				//printf("Waiting for child 2, pid = %d\n", p2);
				result2 = waitpid(p2, &status2, 0);	
				//printf("Two has finished\n");		
			}
		}
	}
}


void do_multi_pipe_command(char ***args, int block, int input, char *input_filename, int output, char *output_filename) {
	int i;
	for(i=0;args[i]!=NULL;i++);
	
	
	int result;
	int status1,status2;
	
	int directors[2*i];
	pid_t processes[i];
	int a=0;
	
	for(a=0;a<i;a++){
		if(pipe(directors)<0){
			printf("Pipes were unable to start\n");
		}
		//processes[0]=fork();

		switch(processes[0]) {
		case EAGAIN:
			perror("Error EAGAIN: ");
			return;
		case ENOMEM:
			perror("Error ENOMEM: ");
			return;
		}

		if(fork()==0){
			close(directors[a]);
			dup2(directors[a+1],STDOUT_FILENO);
			close(directors[a+1]);
			if(execvp(args[a][0],args[a])<0){
				printf("1 Failed\n");
				exit(-1);
			}
			//result1=execvp(proc1[0],proc1);
			//exit(0);
		
		}
		else{
			//[a+1]=fork();
			switch(processes[a+1]) {
				case EAGAIN:
					perror("Error EAGAIN: ");
					return;
				case ENOMEM:
					perror("Error ENOMEM: ");
					return;
			}	
			if(fork()==0){
				close(directors[a+1]);
				dup2(directors[a],STDIN_FILENO);
				close(directors[a]);

				if(execvp(args[a+1][0],args[a+1])<0){
					printf("2 Failed\n");
					exit(-1);
				}

				//result2=execvp(proc2[0],proc2);
				//exit(-1);
			}
			else{
				if(block) {
				//	close(directors[a]);
					close(directors[a+1]);
				//	printf("Waiting for child 1, pid = %d\n", p1);
					result = waitpid(processes[a], &status1, 0);	
					//printf("One has finished\n");		
					//printf("Waiting for child 2, pid = %d\n", p2);
					result = waitpid(processes[a+1], &status2, 0);	
					//printf("Two has finished\n");		
				}
			}
		}
	}
}

//___________________________________________________


