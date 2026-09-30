// Single-executable wrapper: embeds Makefile and links hosts.c (as hosts_main)
#define _GNU_SOURCE
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int hosts_main(int argc, char **argv);

static const char MAKEFILE[] = {
#embed "Makefile"
};

// Replace this process with make, using the embedded Makefile via an in-memory file
int run_make(int argc, char **argv) {
	char self[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", self, sizeof(self) - 1);
	int fd = memfd_create("Makefile", 0);
	if (len < 0 || fd < 0 || write(fd, MAKEFILE, sizeof(MAKEFILE)) < 0) {
		perror("hosts-parser");
		return 1;
	}
	self[len] = 0;

	char mk[32], bin[PATH_MAX + 4], dir[PATH_MAX];
	snprintf(mk, sizeof(mk), "/proc/self/fd/%d", fd);
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

	execvp("make", args);
	perror("make");
	return 127;
}

int main(int argc, char **argv) {
	if (argc > 1 && argv[1][0] == '-')
		return hosts_main(argc, argv);
	return run_make(argc, argv);
}
