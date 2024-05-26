#include <config.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define CONFIGFILE "./cron.conf"

int main (void) {
    puts ("Hello World!");
    FILE* fp;

    fp = fopen(CONFIGFILE, "r");
    if (fp == NULL) {
        perror("Error opening config file");
        exit(errno);
    }

    char * line = NULL;
    size_t size = 0;
    ssize_t nread;

    while ((nread = getline(&line, &size, fp)) != -1) {
        printf("Retrieved line of length %zu:\n", nread);
        printf("%s", line);
    }

    fclose(fp);
    if (line) free(line);

    exit(EXIT_SUCCESS);
    return 0;
}
