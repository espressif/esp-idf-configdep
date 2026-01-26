#include <sys/wait.h>
#include <unistd.h>

#include "utils.h"

int exec_process(char **argv)
{
	pid_t pid;
	int status;

	pid = fork();
	if (pid == -1) {
		err_errno("fork");
		return EXIT_FAILURE;
	}

	if (pid == 0) {
		if (execvp(argv[1], &argv[1]) == -1) {
			err_errno("execvp: '%s'", argv[1]);
			exit(EXIT_FAILURE);
		}
	}

	if (waitpid(pid, &status, 0) == -1) {
		err_errno("waitpid: '%s'", argv[1]);
		return EXIT_FAILURE;
	}

	if (WIFEXITED(status))
		return WEXITSTATUS(status);

	if (WIFSIGNALED(status))
		return WTERMSIG(status);

	return 0;
}
