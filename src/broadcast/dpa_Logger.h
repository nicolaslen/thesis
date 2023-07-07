/*
 * dpa_Logger.h
 *
 *  Created on: Aug 20, 2012
 *      Author: Nicolò Rivetti
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#define MEMORY_LOG_LEVEL 12
#define MIN_LOGLEVEL 5
#define DEBUG_LOG_LEVEL 5
#define DEFAULT_LOG_LEVEL 6
#define INFO_LOG_LEVEL 8
#define ERROR_LOG_LEVEL 10

#define LOG(a,b,c) sprintf(log_buffer,a,b); runtime_log(c);

char log_buffer[256];

int logger_init();
int logger_clean();

void print_data(char* label, char* data_name, int* array, int size, int level);
int runtime_log(int level);

#endif /* LOGGER_H_ */
