/*    Copyright (C) 2024  Rein Fernhout

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <config.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "parse-datetime.h"

#define CONFIGFILE "./cron.conf"

struct job {
    char* command;
    char* timestring;
    struct timespec due;
};

struct timespec now;

// Time to sleep until a job
int job_sleep(struct job* job) {
    return job->due.tv_sec - now.tv_sec;
}

// The next job to be run
struct job* next_job(struct job* jobs, size_t size) {
    int sleeptime = job_sleep(jobs);
    struct job* next = jobs;
    for (int i=1; i < size; i++) {
        int nsleeptime = job_sleep(&jobs[i]);
        if (job_sleep(&jobs[i]) < sleeptime) {
            sleeptime = nsleeptime;
            next = &jobs[i];
        }
    }
    return next;
}

// parses the timestring to get a due time
struct timespec* job_setdue(struct job* job) {
    parse_datetime(&job->due, job->timestring, &now);
    return &job->due;
}

// execute this job
// this additionally calls setdue
void job_execute(struct job* job) {
    if (system(job->command) < 0)
        perror("Failed to execute child");
    job_setdue(job);
}

// Searches for the delimeter
// then allocates two new buffers
// containing the command and timestring
void parse_line(struct job* job, char* line, size_t len) {
    // rid the newline
    if (*(line + len - 1)  == '\n') {
        *(line + len - 1) = '\0';
        len -= 1;
    }

    char* delim = strchr(line, '|');
    if (delim == NULL) {
        printf("Failed to find delimeter on line '%s'\n", line);
        exit(EXIT_FAILURE);
    }

    // place nullbyte at delim
    *delim = '\0';

    int len_tstr = delim - line;
    int len_cstr = len - len_tstr;

    job->timestring = malloc(len_tstr + 1);
    job->command = malloc(len_cstr + 1);

    strncpy(job->timestring, line, len_tstr + 1);
    strncpy(job->command, delim + 1, len_cstr + 1);

    job_setdue(job);
}

int main (void) {
    FILE* fp;

    fp = fopen(CONFIGFILE, "r");
    if (fp == NULL) {
        perror("Error opening config file");
        exit(errno);
    }

    char* line = NULL;
    size_t size = 0;
    ssize_t nread;

    clock_gettime(CLOCK_REALTIME, &now);

    struct job* jobs = malloc(sizeof(struct job) * 32);
    size_t i = 0;

    while ((nread = getline(&line, &size, fp)) != -1) {
        parse_line(&jobs[i], line, nread);
        printf("%s in %d seconds\n", jobs[i].command, job_sleep(&jobs[i]));
        i++;
    }

    while (1) {
        clock_gettime(CLOCK_REALTIME, &now);
        struct job* next = next_job(jobs, i);
        int sleeptime = job_sleep(next);
        printf("Sleeping for %d seconds to '%s'\n", sleeptime, next->command);
        sleep(sleeptime);
        printf("Executing '%s'\n", next->command);
        job_execute(next);
    }

    fclose(fp);
    if (line) free(line);

    exit(EXIT_SUCCESS);
    return 0;
}
