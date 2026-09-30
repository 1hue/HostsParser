#include <ctype.h>
#include <err.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

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
	// Patterns excluded from hostsfile (plus domains in WHITELIST_FILE)
	"::", // ipv6
	"^255\\.255\\.255\\.255",
	"^127\\.0\\.0\\.1",
	"\\.localdomain",
	NULL};

const char *NO_LINES[] = {NULL};

typedef enum { LINE_OUTPUT, LINE_SKIP, LINE_INVALID } LineType;

typedef struct {
	int valid_output;
	int valid_skipped;
	int invalid;
} Stats;

typedef struct {
	char **items;
	int n;
	int cap;
} List;

List input_list = {};
List custom_list = {};
List allow_list = {};
List whitelist_list = {};
List domain_list = {};

regex_t *allow_regex = NULL;
regex_t *whitelist_regex = NULL;

void *xrealloc(void *ptr, size_t size) {
	ptr = realloc(ptr, size);
	if (!ptr)
		err(1, "realloc");
	return ptr;
}

void list_add(List *list, const char *s) {
	if (list->n == list->cap) {
		list->cap = list->cap ? list->cap * 2 : 64;
		list->items = xrealloc(list->items, list->cap * sizeof(char *));
	}
	list->items[list->n] = strdup(s);
	if (!list->items[list->n++])
		err(1, "strdup");
}

void list_free(List *list) {
	for (int i = 0; i < list->n; i++)
		free(list->items[i]);
	free(list->items);
	*list = (List){};
}

// Lines from path without line endings; optionally drop blank and # lines
void read_lines(List *list, const char *path, bool skip_comments) {
	FILE *f = fopen(path, "r");
	if (!f)
		err(1, "%s", path);

	char *line = NULL;
	size_t cap = 0;
	ssize_t len;
	while ((len = getline(&line, &cap, f)) != -1) {
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
			line[--len] = 0;
		const char *l = line + strspn(line, " \t");
		if (skip_comments && (*l == '#' || *l == 0))
			continue;
		list_add(list, line);
	}

	free(line);
	fclose(f);
}

// Hardcoded entries first, then non-empty, non-comment lines from path
void load_list(List *list, const char **hardcoded, const char *path) {
	for (int i = 0; hardcoded[i]; i++)
		list_add(list, hardcoded[i]);
	if (path)
		read_lines(list, path, true);
}

regex_t *compile_list(const List *list) {
	regex_t *regex = xrealloc(NULL, list->n * sizeof(regex_t));
	for (int i = 0; i < list->n; i++) {
		if (regcomp(&regex[i], list->items[i], REG_EXTENDED | REG_NOSUB))
			errx(1, "Invalid pattern: %s", list->items[i]);
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

void free_all(void) {
	free_regex(allow_regex, &allow_list);
	free_regex(whitelist_regex, &whitelist_list);
	list_free(&input_list);
	list_free(&custom_list);
	list_free(&allow_list);
	list_free(&whitelist_list);
	list_free(&domain_list);
}

// Trim whitespace and trailing comments, lowercase, and check each domain
void normalize_domains(List *list) {
	const char *valid = "abcdefghijklmnopqrstuvwxyz0123456789.-_";
	for (int i = 0; i < list->n; i++) {
		char *d = list->items[i];
		d[strcspn(d, "#")] = 0;
		char *start = d + strspn(d, " \t");
		size_t len = strcspn(start, " \t");
		if (start[len + strspn(start + len, " \t")] != 0)
			errx(1,
				"%s: invalid entry '%s' (use example.com or .example.com)",
				WHITELIST_FILE,
				start);

		memmove(d, start, len);
		d[len] = 0;
		for (char *c = d; *c; c++)
			*c = tolower((unsigned char)*c);

		const char *base = d + (d[0] == '.');
		if (!*base || strspn(base, valid) != strlen(base))
			errx(1,
				"%s: invalid entry '%s' (use example.com or .example.com)",
				WHITELIST_FILE,
				d);
	}
}

void load_all(void) {
	load_list(&custom_list, CUSTOM_LINES, CUSTOM_FILE);
	load_list(&whitelist_list, WHITELIST_PATTERNS, NULL);
	load_list(&domain_list, NO_LINES, WHITELIST_FILE);
	normalize_domains(&domain_list);
	load_list(&allow_list, ALLOW_PATTERNS, NULL);
	allow_regex = compile_list(&allow_list);
	whitelist_regex = compile_list(&whitelist_list);
}

// "example.com" matches exactly; ".example.com" also matches subdomains
bool domain_matches(const char *host, size_t len, const char *entry) {
	bool subdomains = entry[0] == '.';
	const char *base = entry + subdomains;
	size_t n = strlen(base);
	if (len == n)
		return strncasecmp(host, base, n) == 0;
	return subdomains && len > n && host[len - n - 1] == '.' &&
		strncasecmp(host + len - n, base, n) == 0;
}

// True if any hostname on the line (after the address) is whitelisted
bool is_whitelisted(const char *line) {
	const char *h = line + strcspn(line, " \t");
	while (true) {
		h += strspn(h, " \t");
		size_t len = strcspn(h, " \t#");
		if (len == 0)
			return false;
		for (int i = 0; i < domain_list.n; i++) {
			if (domain_matches(h, len, domain_list.items[i]))
				return true;
		}
		h += len;
	}
}

LineType classify_line(const char *line) {
	const char *l = line + strspn(line, " \t");
	if (*l == '#' || *l == 0)
		return LINE_SKIP;

	for (int i = 0; i < whitelist_list.n; i++) {
		if (regexec(&whitelist_regex[i], l, 0, NULL, 0) == 0)
			return LINE_SKIP;
	}

	if (is_whitelisted(l))
		return LINE_SKIP;

	for (int i = 0; i < allow_list.n; i++) {
		if (regexec(&allow_regex[i], l, 0, NULL, 0) == 0)
			return LINE_OUTPUT;
	}

	return LINE_INVALID;
}

void process_results(const List *lines, const char *outfile, Stats *stats) {
	FILE *out = fopen(outfile, "w");
	if (!out)
		err(1, "%s", outfile);

	// Prepend custom lines
	for (int i = 0; i < custom_list.n; i++)
		fprintf(out, "%s\n", custom_list.items[i]);

	for (int i = 0; i < lines->n; i++) {
		switch (classify_line(lines->items[i])) {
		case LINE_OUTPUT:
			fprintf(out, "%s\n", lines->items[i]);
			stats->valid_output++;
			break;
		case LINE_SKIP:
			stats->valid_skipped++;
			break;
		case LINE_INVALID:
			fprintf(stderr,
				"\033[1;33mWARNING: Invalid line %d:\033[0m %s\n",
				i + 1,
				lines->items[i]);
			stats->invalid++;
			break;
		}
	}

	if (fclose(out) != 0)
		err(1, "%s", outfile);
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

	atexit(free_all);
	load_all();
	read_lines(&input_list, input, false);

	// Write to a temp file; replace output only once it's complete
	char *tmp = xrealloc(NULL, strlen(output) + 5);
	sprintf(tmp, "%s.tmp", output);

	Stats stats = {};
	process_results(&input_list, tmp, &stats);

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

	if (stats.valid_output == 0) {
		unlink(tmp);
		errx(1, "No valid entries in %s; %s left unchanged", input, output);
	}
	if (rename(tmp, output) != 0)
		err(1, "%s", output);
	free(tmp);

	return stats.invalid > 0 ? 1 : 0;
}
