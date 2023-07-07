/*
 * dpa_Logger.c
 *
 *  Created on: Aug 19, 2012
 *      Author: Nicolò Rivetti
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "dpa_Runtime.h"
#include "dpa_Logger.h"

static char log_file_path[256];
static FILE *file_pointer = NULL;
static char *timestamp = NULL;
static char *log_path = "/home/nicolas/cloud/log/";

#define LOGGGER_OFF 0

static char *time_stamp() {

	time_t ltime;
	ltime = time(NULL );
	struct tm *tm;
	tm = localtime(&ltime);

	sprintf(timestamp, "%04d%02d%02d%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min,
			tm->tm_sec);
	return timestamp;
}

void print_data(char* label, char* data_name, int* array, int size, int level) {
	if (level < MIN_LOGLEVEL)
		return;

	char string[250];
	int i = 0;
	for (i = 0; i < size; i++) {

		sprintf(string, "%s %s[%d]:%d ", label, data_name, i, array[i]);
		fprintf(file_pointer, "%s Rank %d: %s\n", time_stamp(), my_rank, string);

		if (num_procs == 1)
			printf("%s\n", string);
	}
	fflush(file_pointer);
}

int runtime_log(int level) {
	if (level < MIN_LOGLEVEL)
		return 0;

	fprintf(file_pointer, "%s Rank %d: %s\n", time_stamp(), my_rank, log_buffer);
	fflush(file_pointer);
	return 0;
}

int logger_init() {

	if (LOGGGER_OFF)
		return 0;

	char hostname[50]; /* hostname string */
	char command[256];

	/* get hostname */
	gethostname(hostname, sizeof(hostname));

	timestamp = (char *) malloc(sizeof(char) * 16);

	sprintf(log_file_path, "%sPROCESS_%d_%s_on_%s_Runtime", log_path, my_rank, time_stamp(), hostname);

	file_pointer = fopen(log_file_path, "w");

	if (my_rank == 0) {
		int pid = fork();
		if (pid == 0) {
			sprintf(command, "gnome-terminal -e \"tail -f %s\" -t \"Log of Process %d\"", log_file_path, my_rank);
			int ret = system(command);
			// Removing Warning
			if (ret) {

			}
			exit(0);
		}
	}
	sleep(1);

	fprintf(file_pointer, "#### PROCESS Rank %d on host %s Runtime starting ####\n", my_rank, hostname);
	fflush(file_pointer);

	return 0;
}

int logger_clean() {
	if (file_pointer != NULL )
		fclose(file_pointer);

	if (timestamp != NULL )
		free(timestamp);

	return 0;
}
