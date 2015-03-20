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

	/**
	 * TODO:
	 * The first word contains the "cd" string, the second one contains
	 * the path.
	 * Check possible errors:
	 * - The words pointer could be NULL, the first string or the second
	 *   string could be NULL, or the first string is not a cd command
	 * - If so, return an EXIT_FAILURE status to indicate something is
	 *   wrong.
	 */

   if (words == NULL || words[0] == NULL || words[1] == NULL ||
       strcmp(words[0], "cd") != 0 || words[2] != NULL) {
     return EXIT_FAILURE;

   } else {
     return chdir(words[1]);
   }


	/**
	 * TODO:
	 * The safest way would be to first determine if the path is relative
	 * or absolute (see is_relative function provided).
	 * - If it's not relative, then simply change the directory to the path
	 * specified in the second word in the array.
	 * - If it's relative, then make sure to get the current working
	 * directory, append the path in the second word to the current working
	 * directory and change the directory to this path.
	 * Hints: see chdir and getcwd man pages.
	 * Return the success/error code obtained when changing the directory.
	 */
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

	/**
	 * TODO: execute a program, based on the tokens provided.
	 * The first token is the command name, the rest are the arguments
	 * for the command.
	 * Hint: see execlp/execvp man pages.
	 *
	 * - In case of error, make sure to use "perror" to indicate the name
	 *   of the command that failed.
	 *   You do NOT have to print an identical error message to what would
	 *   happen in bash.
	 *   If you use perror, an output like:
	 *      my_silly_command: No such file of directory
	 *   would suffice.
	 * Function returns only in case of a failure (EXIT_FAILURE).
	 */

	int error_code = execvp(tokens[0], tokens);

	if (error_code == -1){
		fprintf(stderr, "%s: %s: failed\n", shellname, tokens[0]);
		return EXIT_FAILURE;
	}
	printf("execvp returned %d\n", error_code);
	return 0;
}


/**
 * Executes a non-builtin command.
 */
int execute_nonbuiltin(simple_command *s) {
	/**
	 * TODO: Check if the in, out, and err fields are set (not NULL),
	 * and, IN EACH CASE:
	 * - Open a new file descriptor (make sure you have the correct flags,
	 *   and permissions);
	 * - redirect stdin/stdout/stderr to the corresponding file.
	 *   (hint: see dup2 man pages).
	 * - close the newly opened file descriptor in the parent as well.
	 *   (Avoid leaving the file descriptor open across an exec!)
	 * - finally, execute the command using the tokens (see execute_command
	 *   function above).
	 * This function returns only if the execution of the program fails.
	 */

	if (s->in) {
		int fd_in = open(s->in, O_RDONLY, S_IRUSR | S_IWUSR);
		dup2(fd_in, fileno(stdin));
	}
	if (s->out && s->err) {
		int fd_out_err = open(s->out, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		dup2(fd_out_err, fileno(stdout));
		dup2(fd_out_err, fileno(stderr));
	} else if (s->out) {
		int fd_out = open(s->out, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		dup2(fd_out, fileno(stdout));
	} else if (s->err) {
		int fd_err = open(s->err, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR);
		dup2(fd_err, fileno(stderr));
	}

	execute_command(s->tokens);

	exit(0);
}


/**
 * Executes a simple command (no pipes).
 */
int execute_simple_command(simple_command *cmd) {

	/**
	 * TODO:
	 * Check if the command is builtin.
	 * 1. If it is, then handle BUILTIN_CD (see execute_cd function provided)
	 *    and BUILTIN_EXIT (simply exit with an appropriate exit status).
	 * 2. If it isn't, then you must execute the non-builtin command.
	 * - Fork a process to execute the nonbuiltin command
	 *   (see execute_nonbuiltin function above).
	 * - The parent should wait for the child.
	 *   (see wait man pages).
	 */
	int builtin = cmd->builtin;
 	int exit_code;

 	if (builtin == BUILTIN_CD) {
		exit_code = execute_cd(cmd->tokens);

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
		exit(0);
	} else {
 		// non-builtin command

 		// process id and status
   	pid_t pid;
   	pid_t status;
   	// fork
   	pid = fork();

   	if (pid < 0) {
     	perror("fork()");
		} else if (pid > 0) {
     	// parent

     	// child has exited
     	pid_t exit_code;
     	if (wait(&status) != -1) {
       	if (WIFEXITED(status)) {
			  	exit_code = WEXITSTATUS(status);
					if (exit_code != 0) {
						printf("got exit code as: %d\n", exit_code);
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

	/**
	 * TODO:
	 * Check if this is a simple command, using the scmd field.
	 * Remember that this will be called recursively, so when you encounter
	 * a simple command you should act accordingly.
	 * Execute nonbuiltin commands only. If it's exit or cd, you should not
	 * execute these in a piped context, so simply ignore builtin commands.
	 */

	if (c->scmd) {
		execute_nonbuiltin(c->scmd);
	}



	/**
	 * Optional: if you wish to handle more than just the
	 * pipe operator '|' (the '&&', ';' etc. operators), then
	 * you can add more options here.
	 */

	if (!strcmp(c->oper, "|")) {

		/**
		 * TODO: Create a pipe "pfd" that generates a pair of file
		 * descriptors, to be used for communication between the
		 * parent and the child. Make sure to check any errors in
		 * creating the pipe.
		 */

		int pfd[2];
		pipe(pfd);
		pid_t pid[2];
		//pid_t status;

	  pid[0] = fork();

		if (pid[0] < 0) {
			perror("fork()");
		} else if (pid[0] > 0) {
			// parent

			pid[1] = fork();

			if (pid[1] < 0) {
				perror("fork()");
			} else if (pid[1] > 0) {
				// parent

				// close both ends of the pipe
				close(pfd[0]);
				close(pfd[1]);

				wait(NULL);
				wait(NULL);

			} else if (pid[1] == 0) {
				// child 2

				// close writing
				close(pfd[1]);

				dup2(pfd[0], fileno(stdin));

				execute_complex_command(c->cmd2);
				exit(1);

			}
		} else if (pid[0] == 0) {
			// child
			// close reading
			close(pfd[0]);

			dup2(pfd[1], fileno(stdout));

			execute_complex_command(c->cmd1);
			exit(1);
		}

		/**
		 * TODO: Fork a new process.
		 * In the child:
		 *  - close one end of the pipe pfd and close the stdout
		 * file descriptor.
		 *  - connect the stdout to the other end of the pipe (the
		 * one you didn't close).
		 *  - execute complex command cmd1 recursively.
		 * In the parent:
		 *  - fork a new process to execute cmd2 recursively.
		 *  - In child 2:
		 *     - close one end of the pipe pfd (the other one than
		 *       the first child), and close the standard input file
		 *       descriptor.
		 *     - connect the stdin to the other end of the pipe (the
		 *       one you didn't close).
		 *     - execute complex command cmd2 recursively.
		 *  - In the parent:
		 *     - close both ends of the pipe.
		 *     - wait for both children to finish.
		 */

	}
	return 0;
}
