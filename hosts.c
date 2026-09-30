#include <getopt.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE_LEN 300
#define CUSTOM_FILE "custom.txt"
#define WHITELIST_FILE "whitelist.txt"

const char *CUSTOM_LINES[] = {
	// Custom lines prepended to output (supplemented by CUSTOM_FILE)
	"127.0.0.1   localhost localhost.localdomain local",
	"::1         localhost ip6-localhost ip6-loopback",
	NULL};

const char *ALLOW_PATTERNS[] = {
	// Allowed input patterns, output to hostsfile; anything else is invalid
	"^0\\.0\\.0\\.0",
	NULL};

const char *WHITELIST_PATTERNS[] = {
	// Patterns excluded from hostsfile (supplemented by WHITELIST_FILE)
	"::", // ipv6
	"^255\\.255\\.255\\.255",
	"^127\\.0\\.0\\.1",
	"\\.localdomain",
	NULL};

typedef struct {
	int valid_output;
	int valid_skipped;
	int invalid;
} Stats;

typedef struct {
	char **items;
	int n;
} List;

List custom_list = {0};
List allow_list = {0};
List whitelist_list = {0};

regex_t *allow_regex = NULL;
regex_t *whitelist_regex = NULL;

void list_add(List *list, const char *s) {
	list->items = realloc(list->items, (list->n + 1) * sizeof(char *));
	list->items[list->n++] = strdup(s);
}

void list_free(List *list) {
	for (int i = 0; i < list->n; i++)
		free(list->items[i]);
	free(list->items);
	list->items = NULL;
	list->n = 0;
}

// Hardcoded entries first, then non-empty, non-comment lines from path
int load_list(List *list, const char **hardcoded, const char *path) {
	for (int i = 0; hardcoded[i]; i++)
		list_add(list, hardcoded[i]);

	if (!path)
		return 0;

	FILE *f = fopen(path, "r");
	if (!f)
		return -1;

	char *line = NULL;
	size_t cap = 0;
	ssize_t len;
	while ((len = getline(&line, &cap, f)) != -1) {
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
			line[--len] = 0;
		const char *l = line;
		while (*l == ' ' || *l == '\t')
			l++;
		if (*l == '#' || *l == 0)
			continue;
		list_add(list, line);
	}

	free(line);
	fclose(f);
	return 0;
}

regex_t *compile_list(const List *list) {
	regex_t *regex = malloc(list->n * sizeof(regex_t));
	for (int i = 0; i < list->n; i++) {
		if (regcomp(&regex[i], list->items[i], REG_EXTENDED | REG_NOSUB)) {
			fprintf(stderr, "Invalid pattern: %s\n", list->items[i]);
			for (int j = 0; j < i; j++)
				regfree(&regex[j]);
			free(regex);
			return NULL;
		}
	}
	return regex;
}

void free_regex(regex_t *regex, const List *list) {
	if (!regex)
		return;
	for (int i = 0; i < list->n; i++)
		regfree(&regex[i]);
	free(regex);
}

void free_all() {
	free_regex(allow_regex, &allow_list);
	free_regex(whitelist_regex, &whitelist_list);
	list_free(&custom_list);
	list_free(&allow_list);
	list_free(&whitelist_list);
}

int load_all() {
	if (load_list(&custom_list, CUSTOM_LINES, CUSTOM_FILE) < 0) {
		fprintf(stderr, "Could not load %s\n", CUSTOM_FILE);
		return -1;
	}
	if (load_list(&whitelist_list, WHITELIST_PATTERNS, WHITELIST_FILE) < 0) {
		fprintf(stderr, "Could not load %s\n", WHITELIST_FILE);
		return -1;
	}
	load_list(&allow_list, ALLOW_PATTERNS, NULL);

	allow_regex = compile_list(&allow_list);
	whitelist_regex = compile_list(&whitelist_list);
	if (!allow_regex || !whitelist_regex)
		return -1;
	return 0;
}

int classify_line(const char *line) {
	const char *l = line;
	while (*l == ' ' || *l == '\t')
		l++;

	if (*l == '#' || *l == '\n' || *l == 0)
		return 2;

	for (int i = 0; i < whitelist_list.n; i++) {
		if (regexec(&whitelist_regex[i], l, 0, NULL, 0) == 0)
			return 2;
	}

	for (int i = 0; i < allow_list.n; i++) {
		if (regexec(&allow_regex[i], l, 0, NULL, 0) == 0)
			return 1;
	}

	return 3;
}

int count_lines(const char *path) {
	FILE *f = fopen(path, "r");
	if (!f)
		return -1;
	int count = 0;
	char buf[MAX_LINE_LEN];
	while (fgets(buf, MAX_LINE_LEN, f))
		count++;
	fclose(f);
	return count;
}

int load_hosts(const char *path, char (*lines)[MAX_LINE_LEN], int max) {
	FILE *f = fopen(path, "r");
	if (!f)
		return -1;
	int n = 0;
	while (fgets(lines[n], MAX_LINE_LEN, f) && n < max)
		n++;
	fclose(f);
	return n;
}

void process_results(
	char (*lines)[MAX_LINE_LEN], int n, const char *outfile, Stats *stats) {
	FILE *out = fopen(outfile, "w");
	if (!out)
		return;

	// Prepend custom lines
	for (int i = 0; i < custom_list.n; i++) {
		fputs(custom_list.items[i], out);
		fputc('\n', out);
	}

	stats->valid_output = 0;
	stats->valid_skipped = 0;
	stats->invalid = 0;

	for (int i = 0; i < n; i++) {
		int type = classify_line(lines[i]);
		if (type == 1) {
			fputs(lines[i], out);
			stats->valid_output++;
		} else if (type == 2) {
			stats->valid_skipped++;
		} else {
			fprintf(stderr,
				"\033[1;33mWARNING: Invalid line %d:\033[0m %s",
				i + 1,
				lines[i]);
			stats->invalid++;
		}
	}

	fclose(out);
}

int main(int argc, char **argv) {
	char *input = NULL;
	char *output = NULL;
	int opt;

	while ((opt = getopt(argc, argv, "i:o:")) != -1) {
		switch (opt) {
		case 'i':
			input = optarg;
			break;
		case 'o':
			output = optarg;
			break;
		default:
			fprintf(stderr, "Usage: %s -i <input> -o <output>\n", argv[0]);
			return 1;
		}
	}

	if (!input || !output) {
		fprintf(stderr, "Usage: %s -i <input> -o <output>\n", argv[0]);
		return 1;
	}

	if (load_all() < 0) {
		free_all();
		return 1;
	}

	int line_count = count_lines(input);
	if (line_count < 0) {
		fprintf(stderr, "Invalid line count in %s", input);
		free_all();
		return 1;
	}

	char (*lines)[MAX_LINE_LEN] = malloc(line_count * sizeof(*lines));

	int n = load_hosts(input, lines, line_count);
	if (n < 0) {
		fprintf(stderr,
			"Could not load input file: %s (%d expected lines)\n",
			input,
			line_count);
		free(lines);
		free_all();
		return 1;
	}

	Stats stats;
	process_results(lines, n, output, &stats);

	int tty = isatty(STDOUT_FILENO);
	const char *bold = tty ? "\033[1m" : "";
	const char *green = tty ? "\033[32m" : "";
	const char *dim = tty ? "\033[2m" : "";
	const char *yellow = tty ? "\033[1;33m" : "";
	const char *reset = tty ? "\033[0m" : "";

	printf("\n%sOutput:%s %s%d%s\n",
		bold,
		reset,
		green,
		stats.valid_output,
		reset);
	printf("%sSkipped:%s %s%d%s\n",
		bold,
		reset,
		dim,
		stats.valid_skipped,
		reset);
	printf("%sInvalid:%s %s%d%s\n\n",
		bold,
		reset,
		stats.invalid ? yellow : green,
		stats.invalid,
		reset);

	free_all();
	free(lines);
	return stats.invalid > 0 ? 1 : 0;
}