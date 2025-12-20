#include <getopt.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LEN 300

const char *PREPEND_LINES[] = {
    // Custom hosts to prepend
    "127.0.0.1   localhost localhost.localdomain local\n",
    "::1         localhost ip6-localhost ip6-loopback\n",
    "0.0.0.0 0.0.0.0\n",
    NULL};

const char *VALID_PATTERNS[] = {
    // Valid patterns to output to hostsfile
    "^0\\.0\\.0\\.0",
    NULL};

const char *SKIP_PATTERNS[] = {
    // Valid patterns to exclude from hostsfile
    "::", // ipv6
    "^255\\.255\\.255\\.255",
    "^127\\.0\\.0\\.1",
    "\\.localdomain",
    NULL};

typedef struct
{
  int valid_output;
  int valid_skipped;
  int invalid;
} Stats;

regex_t *valid_regex = NULL;
regex_t *skip_regex = NULL;

void compile_patterns()
{
  int i = 0;
  while (VALID_PATTERNS[i])
    i++;
  valid_regex = malloc(i * sizeof(regex_t));
  for (int j = 0; j < i; j++)
  {
    regcomp(&valid_regex[j], VALID_PATTERNS[j], REG_EXTENDED | REG_NOSUB);
  }

  i = 0;
  while (SKIP_PATTERNS[i])
    i++;
  skip_regex = malloc(i * sizeof(regex_t));
  for (int j = 0; j < i; j++)
  {
    regcomp(&skip_regex[j], SKIP_PATTERNS[j], REG_EXTENDED | REG_NOSUB);
  }
}

void free_patterns()
{
  int i = 0;
  while (VALID_PATTERNS[i])
  {
    regfree(&valid_regex[i]);
    i++;
  }
  free(valid_regex);

  i = 0;
  while (SKIP_PATTERNS[i])
  {
    regfree(&skip_regex[i]);
    i++;
  }
  free(skip_regex);
}

int classify_line(const char *line)
{
  const char *l = line;
  while (*l == ' ' || *l == '\t')
    l++;

  if (*l == '#' || *l == '\n' || *l == 0)
    return 2;

  int i = 0;
  while (SKIP_PATTERNS[i])
  {
    if (regexec(&skip_regex[i], l, 0, NULL, 0) == 0)
      return 2;
    i++;
  }

  i = 0;
  while (VALID_PATTERNS[i])
  {
    if (regexec(&valid_regex[i], l, 0, NULL, 0) == 0)
      return 1;
    i++;
  }

  return 3;
}

int count_lines(const char *path)
{
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

int load_hosts(const char *path, char (*lines)[MAX_LINE_LEN], int max)
{
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
    char (*lines)[MAX_LINE_LEN], int n, const char *outfile, Stats *stats)
{
  FILE *out = fopen(outfile, "w");
  if (!out)
    return;

  // Prepend localhost lines
  int i = 0;
  while (PREPEND_LINES[i])
  {
    fputs(PREPEND_LINES[i], out);
    i++;
  }

  stats->valid_output = 0;
  stats->valid_skipped = 0;
  stats->invalid = 0;

  for (int i = 0; i < n; i++)
  {
    int type = classify_line(lines[i]);
    if (type == 1)
    {
      fputs(lines[i], out);
      stats->valid_output++;
    }
    else if (type == 2)
    {
      stats->valid_skipped++;
    }
    else
    {
      fprintf(stderr,
              "\033[1;33mWARNING: Invalid line %d: %s\033[0m",
              i + 1,
              lines[i]);
      stats->invalid++;
    }
  }

  fclose(out);
}

int main(int argc, char **argv)
{
  char *input = NULL;
  char *output = NULL;
  int opt;

  while ((opt = getopt(argc, argv, "i:o:")) != -1)
  {
    switch (opt)
    {
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

  if (!input || !output)
  {
    fprintf(stderr, "Usage: %s -i <input> -o <output>\n", argv[0]);
    return 1;
  }

  int line_count = count_lines(input);
  if (line_count < 0)
  {
    fprintf(stderr, "Invalid line count in %s", input);
    return 1;
  }

  char (*lines)[MAX_LINE_LEN] = malloc(line_count * sizeof(*lines));

  int n = load_hosts(input, lines, line_count);
  if (n < 0)
  {
    fprintf(stderr,
            "Could not load input file: %s (%d expected lines)\n",
            input,
            line_count);
    free(lines);
    return 1;
  }

  compile_patterns();

  Stats stats;
  process_results(lines, n, output, &stats);

  printf("Valid (output): %d\n", stats.valid_output);
  printf("Valid (skipped): %d\n", stats.valid_skipped);
  printf("Invalid (unexpected): %d\n", stats.invalid);

  free_patterns();
  free(lines);
  return stats.invalid > 0 ? 1 : 0;
}
