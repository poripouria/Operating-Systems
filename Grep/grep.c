/*
Principles of Operating Systems, Dr. Bejani (Final Project)
Grep Command - Find a specific file or word in files - 2threads
Pouria Alimoradpor 9912035
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fnmatch.h>
#include <regex.h>

#define MAX_PATH_LENGTH 4096
#define MAX_FILE_LENGTH 256
#define MAX_LINE_LENGTH 1024
#define MAX_WORD_LENGTH 256
#define MAX_THREADS 2

typedef struct {
    char *pattern;      // The pattern to search for (regular expression)
    char *path;         // The path of the directory or file to search in
    int search_mode;    // The search mode to use
    int lineNumber;     // The flag to indicate whether to print line numbers or not
    int maxDepth;       // The maximum depth of subdirectories to search in
    int threadCount;    // The number of threads to use
    int invertMatch;    // The flag to indicate whether to invert the match or not
} GrepConfig;

void *search_file(void *arg);
void *search_dir(void *arg);
GrepConfig *parse_args(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    GrepConfig *config = parse_args(argc, argv);

    pthread_t threads[MAX_THREADS];
    for (int i = 0; i < config->threadCount; i++) {
        if (i == 0) {
            pthread_create(&threads[i], NULL, search_file, config);
        } else {
            pthread_create(&threads[i], NULL, search_dir, config);
        }
    }

    for (int i = 0; i < config->threadCount; i++) {
        pthread_join(threads[i], NULL);
    }

    free(config);
    return 0;
}

// Function to search for pattern in a file
void *search_file(void *arg) {
    GrepConfig *config = (GrepConfig *)arg;
    FILE *file = fopen(config->path, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", config->path);
    }

    char line[MAX_LINE_LENGTH];
    int line_number = 0;
    while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
        line_number++;
        bool match = false;
        if (config->search_mode == 0) {
            // Search for fixed string
            if (strstr(line, config->pattern) != NULL) {
                match = true;
            }
        } else if (config->search_mode == 1) {
            // Search for regular expression
            regex_t regex;
            int ret = regcomp(&regex, config->pattern, REG_EXTENDED);
            if (ret != 0) {
                fprintf(stderr, "Error: Cannot compile regular expression '%s'\n", config->pattern);
            }
            ret = regexec(&regex, line, 0, NULL, 0);
            if (ret == 0) {
                match = true;
            }
            regfree(&regex);
        }

        if (config->invertMatch) {
            match = !match;
        }

        if (match) {
            if (config->lineNumber) {
                printf("Path: \t\t%s\nLineNum: \t%d\nLine: \t\t%s \n", config->path, line_number, line);
            } else {
                printf("Path: \t\t%s\nLine: \t\t%s \n", config->path, line);
            }
        }
    }

    fclose(file);
    return NULL;
}

// Function to search for pattern in a directory and its subdirectories recursively
void *search_dir(void *arg) {
    GrepConfig *config = (GrepConfig *)arg;
    DIR *dir = opendir(config->path);
    if (dir == NULL) {
        fprintf(stderr, "Error: Cannot open directory '%s'\n", config->path);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip current and parent directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char path[MAX_PATH_LENGTH];
        snprintf(path, MAX_PATH_LENGTH, "%s/%s", config->path, entry->d_name);

        struct stat statbuf;
        if (lstat(path, &statbuf) == -1) {
            fprintf(stderr, "Error: Cannot stat '%s'\n", path);
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            // Search subdirectory recursively
            if (config->maxDepth == -1 || config->maxDepth > 0) {
                if (config->maxDepth > 0) {
                    config->maxDepth--;
                }
                search_dir(config);
            }
        } else if (S_ISREG(statbuf.st_mode)) {
            // Search file
            search_file(config);
        }
        regex_t regex;
        int ret;

        // Compile the regular expression
        ret = regcomp(&regex, config->pattern, REG_EXTENDED | REG_NOSUB);
        if (ret) {
            fprintf(stderr, "Invalid regular expression: %s\n", config->pattern);
            exit(1);
        }

        // Check for a match in the directory name
        ret = regexec(&regex, entry->d_name, 0, NULL, 0);
        if ((!ret && !config->invertMatch) || (ret && config->invertMatch)) {
            // Print the matching directory name
            printf("%s\n", path);
        }
        regfree(&regex);
    }

    closedir(dir);
    return NULL;
}

// Parse command line arguments
GrepConfig *parse_args(int argc, char *argv[]) {
    GrepConfig *config = malloc(sizeof(GrepConfig));
    config->pattern = NULL;
    config->path = NULL;
    config->search_mode = 0;
    config->lineNumber = 0;
    config->maxDepth = -1;
    config->threadCount = 1;
    config->invertMatch = 0;

    int opt;
    while ((opt = getopt(argc, argv, "E:R:nmd:it:vh")) != -1) {
        switch (opt) {
            case 'E':
                config->pattern = optarg;
                break;
            case 'R':
                config->pattern = optarg;
                config->search_mode = 1;
                break;
            case 'n':
                config->lineNumber = 1;
                break;
            case 'm':
                config->maxDepth = 0;
                break;
            case 'd':
                config->maxDepth = atoi(optarg);
                break;
            case 'i':
                config->invertMatch = 1;
                break;
            case 't':
                config->threadCount = atoi(optarg);
                break;
            case 'v':
                printf("Grep Command - Find a specific file or word in files - 2threads\nPouria Alimoradpor 9912035\n");
                exit(0);
            case 'h':
                printf("Usage: %s [OPTION]... [PATTERN] [PATH]\n", argv[0]);
                printf("Search for PATTERN in PATH.\n");
                printf("Example: %s -E \"^abc$\" /home/user/file.txt\n", argv[0]);
                printf("Options:\n");
                printf("  -E PATTERN\t\tsearch for PATTERN using extended regular expressions\n");
                printf("  -R PATTERN\t\tsearch for PATTERN recursively\n");
                printf("  -n\t\t\tprint line numbers with output lines\n");
                printf("  -m\t\t\tsearch only in the first level of subdirectories\n");
                printf("  -d MAXDEPTH\t\tsearch only in the first MAXDEPTH levels of subdirectories\n");
                printf("  -i\t\t\tinvert the sense of matching, to select non-matching lines\n");
                printf("  -t THREADCOUNT\tuse THREADCOUNT threads for searching\n");
                printf("  -v\t\t\tprint the version number and exit\n");
                printf("  -h\t\t\tprint this help text and exit\n");
                exit(0);
            default:
                fprintf(stderr, "Try '%s -h' for more information.\n", argv[0]);
                exit(1);
        }
    }
    
    if (config->pattern == NULL) {
        fprintf(stderr, "Error: Pattern not specified\n");
        exit(1);
    }

    if (optind < argc) {
        config->path = argv[optind];
    } else {
        fprintf(stderr, "Error: Path not specified\n");
        exit(1);
    }

    return config;
}
