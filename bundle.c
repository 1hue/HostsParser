// Single-executable wrapper: embeds Makefile and links hosts.c (as hosts_main)
#define _GNU_SOURCE
#include <err.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

int hosts_main(int argc, char **argv);

static const char MAKEFILE[] = {
#embed "Makefile"
};

static_assert(sizeof(MAKEFILE) <= 4096, "Makefile too large for pipe buffer");

// Absolute path of the running executable
static void exe_path(char *buf) {
#ifdef __APPLE__
	char tmp[PATH_MAX];
	uint32_t size = sizeof(tmp);
	if (_NSGetExecutablePath(tmp, &size) != 0 || !realpath(tmp, buf))
		err(1, "executable path");
#else
	ssize_t len = readlink("/proc/self/exe", buf, PATH_MAX - 1);
	if (len < 0)
		err(1, "executable path");
	buf[len] = 0;
#endif
}

// Replace this process with make, reading the embedded Makefile from a pipe
int run_make(int argc, char **argv) {
	char self[PATH_MAX];
	exe_path(self);

	// Makefile fits in the pipe buffer, so no writer process is needed
	int fd[2];
	if (pipe(fd) < 0 ||
		write(fd[1], MAKEFILE, sizeof(MAKEFILE)) != sizeof(MAKEFILE))
		err(1, "pipe");
	close(fd[1]);

	char mk[32], bin[PATH_MAX + 4], dir[PATH_MAX];
	snprintf(mk, sizeof(mk), "/dev/fd/%d", fd[0]);
	snprintf(bin, sizeof(bin), "BIN=%s", self);
	snprintf(dir, sizeof(dir), "%s", self);

	char *args[argc + 8];
	int n = 0;
	args[n++] = "make";
	args[n++] = "--no-print-directory";
	args[n++] = "-f";
	args[n++] = mk;
	args[n++] = "-C";
	args[n++] = dirname(dir);
	args[n++] = bin;
	args[n++] = "BUNDLED=1";
	for (int i = 1; i < argc; i++)
		args[n++] = argv[i];
	args[n] = NULL;

	// Prefer current GNU make (Homebrew installs it as gmake on macOS)
	execvp("gmake", args);
	execvp("make", args);
	err(127, "make");
}

int main(int argc, char **argv) {
	if (argc > 1 && argv[1][0] == '-')
		return hosts_main(argc, argv);
	return run_make(argc, argv);
}
