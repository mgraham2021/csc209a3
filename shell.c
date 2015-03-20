#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <mcheck.h>

#include "parser.h"
#include "shell.h"

/**
 * Program that simulates a simple shell.
 * The shell covers basic commands, including builtin commands
 * (cd and exit only), standard I/O redirection and piping (|).
 */

#define MAX_DIRNAME 100
#define MAX_COMMAND 1024
#define MAX_TOKEN 128

// Functions to implement, see below after main
int execute_cd(char** words);
int execute_nonbuiltin(simple_command *s);
int execute_simple_command(simple_command *cmd);
int execute_complex_command(command *cmd);

// cause you only yolo once, right?
char *shellname = "swagshell";


int main(int argc, char** argv) {
  // Current working directory
	char cwd[MAX_DIRNAME];
  // The command
	char command_line[MAX_COMMAND];
  // Command tokens (program name, * parameters, pipe, etc.)
	char *tokens[MAX_TOKEN];

	while (1) {
		// Display prompt
		getcwd(cwd, MAX_DIRNAME-1);
		printf("%s> ", cwd);

		// Read the command line
		fgets(command_line, MAX_COMMAND, stdin);
		// Strip the new line character
		if (command_line[strlen(command_line) - 1] == '\n') {
			command_line[strlen(command_line) - 1] = '\0';
		}

		// Parse the command into tokens
		parse_line(command_line, tokens);

		// Check for empty command
		if (!(*tokens)) {
			continue;
		}

		// Construct chain of commands, if multiple commands
		command *cmd = construct_command(tokens);
		//print_command(cmd, 0);

		int exitcode = 0;
		if (cmd->scmd) {
			exitcode = execute_simple_command(cmd->scmd);
			if (exitcode == -1) {
				break;
			}
		}
		else {
			exitcode = execute_complex_command(cmd);
			if (exitcode == -1) {
				break;
			}
		}
		release_command(cmd);
	}

	return 0;
}


/**
 * Changes directory to a path specified in the words argument;
 * For example: words[0] = "cd"
 *              words[1] = "csc209/assignment3/"
 * Your command should handle both relative paths to the current
 * working directory, and absolute paths relative to root,
 * e.g., relative path:  cd csc209/assignment3/
 *       absolute path:  cd /u/bogdan/csc209/assignment3/
 */
int execute_cd(char** words) {
	// check possible errors
	// null pointers, check if cd command, too many args
 	if (words == NULL || words[0] == NULL || words[1] == NULL ||
     	strcmp(words[0], "cd") != 0 || words[2] != NULL) {
   	return EXIT_FAILURE;
 	} else {
		// no errors, change directory
		// chdir handles absolute/relative itself
   	return chdir(words[1]);
 	}
}


/**
 * Executes a program, based on the tokens provided as
 * an argument.
 * For example, "ls -l" is represented in the tokens array by
 * 2 strings "ls" and "-l", followed by a NULL token.
 * The command "ls -l | wc -l" will contain 5 tokens,
 * followed by a NULL token.
 */
int execute_command(char **tokens) {
	// Execute an external program
	// pass in command name,  vector of arguments
	execvp(tokens[0], tokens);

	// This code only reachable if execvp returned -1 (failed)
	// Build an error message
	char *inter = ": ";
	int len = strlen(shellname) + strlen(tokens[0]) + strlen(inter) + 4;
	char message[len];

	strncat(message, shellname, strlen(shellname));
	strncat(message, inter, strlen(inter));
	strncat(message, tokens[0], strlen(tokens[0]));

	perror(message);
	return EXIT_FAILURE;
}


/**
 * Executes a non-builtin command.
 */
int execute_nonbuiltin(simple_command *s) {
	// The following if blocks follow this template:
	// 1. create a file descriptor with proper flags and modes
	// 2. redirect file descriptor
	// 3. close old file descriptor
	if (s->in) {
		int fd_in = open(s->in, O_RDONLY, S_IRUSR | S_IWUSR);

		if(dup2(fd_in, fileno(stdin)) == -1) {
			perror("dup2");
		}

		if (close(fd_in) == -1) {
			perror("close");
		}
	}
	if (s->out && s->err) {
		int fd_out_err = open(s->out, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

		if (dup2(fd_out_err, fileno(stdout)) == -1) {
			perror("dup2");
		}
		if (dup2(fd_out_err, fileno(stderr)) == -1) {
			perror("dup2");
		}

		if (close(fd_out_err) == -1) {
			perror("close");
		}
	} else if (s->out) {
		int fd_out = open(s->out, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

		if (dup2(fd_out, fileno(stdout)) == -1) {
			perror("dup2");
		}

		if (close(fd_out) == -1) {
			perror("close");
		}
	} else if (s->err) {
		int fd_err = open(s->err, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR);

		if (dup2(fd_err, fileno(stderr)) == -1) {
			perror("dup2");
		}

		if (close(fd_err) == -1) {
			perror("close");
		}
	}

	// Execute the command
	int error_code = execute_command(s->tokens);

	// This code only reachable if execute_command returned EXIT_FAILURE
	return error_code;
}


/**
 * Executes a simple command (no pipes).
 */
int execute_simple_command(simple_command *cmd) {
	int builtin = cmd->builtin;
 	int exit_code;

	// check if command is builtin
 	if (builtin == BUILTIN_CD) {
		exit_code = execute_cd(cmd->tokens);

		// messages for cd failure
 		if (exit_code != 0) {
 			if (exit_code == EXIT_FAILURE) {
   			printf("cd: usage: cd [dir]\n");
   		} else if (exit_code == -1) {
   		printf("%s: cd: %s: No such file or directory\n",shellname, cmd->tokens[1]);
   		} else {
   			printf("cd failed with exit code: %d\n", exit_code);
   		}
		}
	} else if (builtin == BUILTIN_EXIT) {
		// user typed 'exit'
		exit(0);
	} else {
 		// non-builtin command

 		// process id and status
   	pid_t pid;
   	pid_t status;
   	// fork
   	pid = fork();

   	if (pid < 0) {
     	perror("fork");
		} else if (pid > 0) {
     	// parent

     	// child has exited
     	pid_t exit_code;
     	if (wait(&status) != -1) {
       	if (WIFEXITED(status)) {
			  	exit_code = WEXITSTATUS(status);
					if (exit_code != 0) {
						//printf("got exit code as: %d\n", exit_code);
					}
       	}
     	}

   	} else if (pid == 0) {
			// child
		 	execute_nonbuiltin(cmd);
	 	}
 	}

 	return 0;
}


/**
 * Executes a complex command.  A complex command is two commands chained
 * together with a pipe operator.
 */
int execute_complex_command(command *c) {
	// execute simple command
	if (c->scmd) {
		execute_nonbuiltin(c->scmd);
	}

	// encountered a complex command
	if (!strcmp(c->oper, "|")) {
		// file descriptors
		int pfd[2];
		// pip
		if (pipe(pfd) == -1) {
			perror("pipe");
		}
		// will fork again so two process ids
		pid_t pid[2];

		// fork
	  pid[0] = fork();

		if (pid[0] < 0) {
			perror("fork");
		} else if (pid[0] > 0) {
			// parent

			// fork again
			pid[1] = fork();

			if (pid[1] < 0) {
				perror("fork");
			} else if (pid[1] > 0) {
				// parent

				// close both ends of the pipe
				close(pfd[0]);
				close(pfd[1]);

				// wait for _both_ children to exit
				wait(NULL);
				wait(NULL);

			} else if (pid[1] == 0) {
				// child 2

				// close writing
				close(pfd[1]);

				// redirect input file descriptor to stdin
				if (dup2(pfd[0], fileno(stdin)) == -1) {
					perror("dup2");
				}

				if (close(pfd[0]) == -1) {
					perror("close");
				}

				execute_complex_command(c->cmd2);
				exit(1);
			}
		} else if (pid[0] == 0) {
			// child

			// close reading
			close(pfd[0]);

			// redirect output file descriptor to stdout
			if (dup2(pfd[1], fileno(stdout)) == -1) {
				perror("dup2");
			}

			if (close(pfd[1]) == -1) {
				perror("close");
			}

			execute_complex_command(c->cmd1);
			exit(1);
		}
	}
	return 0;
}
