/*
 ============================================================================
 Name        : Main.c
 Author      : Nicolò Rivetti
 Created on  : Aug 23, 2012
 Version     : 1.0
 Copyright   : Copyright (c) 2012 Nicolò Rivetti
 Description : main for testing DPA internals
 ============================================================================
 */

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <wait.h>
#include <time.h>
#include <string.h>

#include "dpa.h"

static FILE *file_pointer;

struct timespec tim, tim2;
static int my_rank = -1;
static void test_tob4(int pid, int n);
static void test_tob3(int pid, int n);
static void test_tob2(int pid, int n);
static void test_tob(int pid, int n);
static void test_flooding_consensus(int pid, int n);
static void test_eager_reliable_broadcast(int pid, int n);
static void test_eager_reliable_broadcast2(int pid, int n);
static void test_best_effort_broadcast(int pid, int n);
static void test_best_effort_broadcast_2(int pid, int n);
static void test_perfect_failure_detector(int pid, int n);
static void main_logger_init();
static void configureExit();
static void my_handler(int s);
static void initSender(int n, unsigned int flags);
static char *log_path = "/home/nicolas/cloud/log/";

int main(int argc, char **argv) {

	/*printf("\nargc:%d\n", argc);
	for (int i = 0; i < argc; ++i)
	{
		printf("\nargv[%d]: %s\n", i, argv[i]);
	}*/

	tim.tv_sec = 0;
	tim.tv_nsec = 1000000L;

	int pid = getpid();
	int n = 0;
	
	/* Configuro el Control + C */
	configureExit();

	unsigned int flags = 0;
	if (!strcmp(argv[1], "tob")) {
		flags = USING_CONSENSUS_BASED_TOTAL_ORDER_BROADCAST;
	}
	else if (!strcmp(argv[1], "beb")) {
		flags = USING_BEST_EFFORT_BROADCAST;
	}
	else if (!strcmp(argv[1], "erb")) {
		flags = USING_EAGER_RELIABLE_BROADCAST;
	}
	else {
		printf("Error en argumentos, elija tob, beb o erb");
		exit(1);
		return 0;
	}

	DPA_Init(argc, argv, flags);

	//sleep(5);
	DPA_Get_Process_Number(&n);
	DPA_Get_Rank(&my_rank);

	main_logger_init();
	printf("%d: START with %d processes and rank %d\n", pid, n, my_rank);
		
	//printf("---------pid = %d - rank = %d - processes = %d\n", pid, my_rank, n);
	initSender(n, flags);
	
	//test_best_effort_broadcast(pid, n);
	//test_best_effort_broadcast_2(pid, n);
	//test_tob(pid,n);
	//test_tob4(pid, n);
	//test_perfect_failure_detector(pid, n);
	//test_eager_reliable_broadcast(pid, n);
	//test_eager_reliable_broadcast2(pid, n);
	//test_flooding_consensus(pid, n);
	DPA_Close();
	exit(1);
	return 0;
}

void initSender(int n, unsigned int flags) {
	int receive_data, receive_sender;
	int send_message = 0;
	int max = 10;
    FILE *fp_send;
    FILE *fp_receive;

	char send_filename[21];
	sprintf(send_filename, "send_%d.txt", my_rank);
	printf("send_filename = %s\n", send_filename);

	fp_send = fopen(send_filename, "w");
	fp_send = freopen(send_filename, "r", fp_send);
	if (fp_send == NULL) {
		printf("cannot open target file sender %s\n", send_filename);
		exit(1);
	}

	char receive_filename[24];
	sprintf(receive_filename, "receive_%d.txt", my_rank);
	printf("receive_filename = %s\n", receive_filename);
	fp_receive = fopen(receive_filename, "w");
	fp_receive = freopen(receive_filename, "a", fp_receive);
	if (fp_receive == NULL) {
		printf("cannot open target file receiver %s\n", receive_filename);
		exit(1);
	}
	
	while (1) {
		int isAnyMessageToSend = -1;
		int isAnyDeliveredMessage = -1;
		//printf("[%d] estoy esperando algun mensaje para enviar\n", my_rank);

		isAnyMessageToSend = fscanf(fp_send, "%d", &send_message);
		clearerr(fp_send);
		

		if (isAnyMessageToSend != -1) {
			printf("[%d] enviando %d\n", my_rank, send_message);
			switch (flags) {
				case USING_CONSENSUS_BASED_TOTAL_ORDER_BROADCAST:
					DPA_consensus_based_tob_send(send_message);
					break;
				case USING_BEST_EFFORT_BROADCAST:
					DPA_best_effort_broadcast_send(send_message);
					break;
				case USING_EAGER_RELIABLE_BROADCAST:
					DPA_eager_reliable_broadcast_send(send_message);
					break;
			}
		}
		switch (flags) {
			case USING_CONSENSUS_BASED_TOTAL_ORDER_BROADCAST:
				isAnyDeliveredMessage = DPA_consensus_based_tob_deliver(&receive_data, &receive_sender);
				break;
			case USING_BEST_EFFORT_BROADCAST:
				isAnyDeliveredMessage = DPA_best_effort_broadcast_deliver(&receive_data, &receive_sender);
				break;
			case USING_EAGER_RELIABLE_BROADCAST:
				isAnyDeliveredMessage = DPA_eager_reliable_broadcast_deliver(&receive_data, &receive_sender);
				break;
		}

		if (isAnyDeliveredMessage != -1) {
			printf("[%d] me llego %d de %d\n", my_rank, receive_data, receive_sender);
			fprintf(fp_receive, "%d:%d\n", receive_sender, receive_data);
			fflush(fp_receive);
		}

		/*if (isAnyMessageToSend == -1 && isAnyDeliveredMessage == -1) {
			sleep(1);
		}*/
	}

	fclose(fp_send);
	fclose(fp_receive);
}

void configureExit() {

	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = my_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGTERM, &sigIntHandler, NULL);
	sigaction(SIGINT, &sigIntHandler, NULL);
}

void my_handler(int s) {
	/*printf("Caught signal %d\n",s);*/
	DPA_Close();
	exit(1); 
}


void test_tob4(int pid, int n) {
	int times = 10;
	int res = -1;
	int data = -1;
	int sender = -1;
	int i = 0;
	int count = 0;
	int sleep_time = 0;
	for (i = 0; i < 10 * (pid + 1); i++)
		rand();

	for (i = 0; i < times; i++) {

		do {
			sleep_time = rand() % 10;
		} while (sleep_time == 0);
		sleep(sleep_time);

		printf("%d: A %02d Send TOB Cast %d (Slept for %d s)\n", pid, i, pid + i * 1000, sleep_time);
		fprintf(file_pointer, "%d: A %02d Send TOB Cast %d (Slept for %d s)\n", pid, i, pid + i * 1000, sleep_time);
		fflush(file_pointer);
		DPA_consensus_based_tob_send(pid + i * 1000);
		while (res == -1) {
			res = DPA_consensus_based_tob_deliver(&data, &sender);
			nanosleep(&tim, &tim2);
		}
		count++;
	}

	while (count < n * times) {
		res = -1;
		while (res == -1) {
			res = DPA_consensus_based_tob_deliver(&data, &sender);
			nanosleep(&tim, &tim2);
		}
		count++;
		fprintf(file_pointer, "%d: B %02d - TOB Deliver, data: %d, sender: %d \n", pid, i, data, sender);
		fflush(file_pointer);
	}
}

void test_tob3(int pid, int n) {
	int times = 10;
	int res = -1;
	int data = -1;
	int sender = -1;
	int i = 0;
	int sleep_time = 0;
	for (i = 0; i < 10 * (pid + 1); i++)
		rand();

	for (i = 0; i < times; i++) {

		do {
			sleep_time = 10;
		} while (sleep_time == 0);
		sleep(sleep_time);

		printf("%d: A %02d Send TOB Cast %d (Slept for %d s)\n", pid, i, pid + i * 1000, sleep_time);
		fprintf(file_pointer, "%d: A %02d Send TOB Cast %d (Slept for %d s)\n", pid, i, pid + i * 1000, sleep_time);
		fflush(file_pointer);

		printf("%d: ENVIO - INICIO\n", pid);
		DPA_consensus_based_tob_send(1234);
		printf("%d: ENVIO - FIN\n", pid);
	}

	for (i = 0; i < n * times; i++) {

		res = -1;
		while (res == -1) {
			res = DPA_consensus_based_tob_deliver(&data, &sender);
			nanosleep(&tim, &tim2);
		}

		printf("%d: B %02d - TOB Deliver, data: %d, sender: %d \n", pid, i, data, sender);
		fprintf(file_pointer, "%d: B %02d - TOB Deliver, data: %d, sender: %d \n", pid, i, data, sender);
		fflush(file_pointer);
	}
}

void test_tob2(int pid, int n) {
	int res = -1;
	int data = -1;
	int sender = -1;
	printf("%d: Send TOB Cast %d\n", pid, pid);
	DPA_consensus_based_tob_send(pid);
	DPA_consensus_based_tob_send(pid + 1000);

	int i = 0;
	for (i = 0; i < n * 2; i++) {
		res = -1;
		while (res == -1)
			res = DPA_consensus_based_tob_deliver(&data, &sender);

		printf("%d - %d: TOB Deliver, data: %d, sender: %d\n", pid, i, data, sender);
	}
}

void test_tob(int pid, int n) {
	int res = -1;
	int data = -1;
	int sender = -1;
	printf("%d: Main.c - test_tob - Send TOB Cast %d\n", pid, pid);
	DPA_consensus_based_tob_send(pid);

	int i = 0;
	for (i = 0; i < n; i++) {
		res = -1;
		while (res == -1)
			res = DPA_consensus_based_tob_deliver(&data, &sender);

		printf("%d - %d: Main.c - test_tob - TOB Deliver, data: %d, sender: %d\n", pid, i, data, sender);
	}
}

void test_flooding_consensus(int pid, int n) {
	int res = -1;
	int id = 5;
	int **data = (int**) malloc(sizeof(int*));
	int data_size;
	int times = 10;
	int i = 0;
	int sleep_time = 0;
	for (i = 0; i < 10 * (pid + 1); i++)
		rand();

	for (i = 0; i < times; i++) {
		int propose = pid + i * 1000;
		int id = 10 + 10 * i;
		do {
			sleep_time = rand() % 10;
		} while (sleep_time == 0);
		sleep(sleep_time);
		printf("%d: Propose %d id:%d\n", pid, propose, id);
		//fprintf(file_pointer, "%d: Propose %d id:%d\n", pid, propose, id);
		DPA_flooding_consensus_propose(&propose, 1, id);
	}

	for (i = 0; i < times; i++) {

		res = -1;
		while (res == -1) {
			res = DPA_flooding_consensus_decided(data, &data_size, &id);
			nanosleep(&tim, &tim2);
		}

		printf("%d: Decided id:%d\n", pid, id);
		fprintf(file_pointer, "%d: Decided id:%d\n", pid, id);
		int j = 0;
		for (j = 0; j < data_size; j++) {
			printf("%d: Decided set value %d: %d\n", pid, j, (*data)[j]);
			//fprintf(file_pointer, "%d: Decided set value %d: %d\n", pid, j, (*data)[j]);
		}
		//fflush(file_pointer);
		fflush(stdout);
	}
}

void test_eager_reliable_broadcast2(int pid, int n) {
	int i = 0;
	int count = 0;

	while (count < 5) {
		nanosleep(&tim, &tim2);
		DPA_eager_reliable_broadcast_send(i * (1 + count));

		if (i % 1000000 == 0)
			printf("%d: A %02d Send  Reliable Broadcast Cast %d\n", pid, i, i * (1 + count));

		if (i == 0)
			count++;

		i++;
	}
	//fprintf(file_pointer, "%d: A %02d Send TOB Cast %d (Slept for %d s)\n", pid, i, pid + i * 1000, sleep_time);
	//fflush(file_pointer);

}

void test_eager_reliable_broadcast(int pid, int n) {

	int times = 10;
	int res = -1;
	int data = -1;
	int sender = -1;
	int i = 0;
	int sleep_time = 0;
	for (i = 0; i < 10 * (pid + 1); i++)
		rand();

	for (i = 0; i < times; i++) {

		do {
			sleep_time = rand() % 10;
		} while (sleep_time == 0);
		sleep(sleep_time);

		printf("%d: A %02d Send  Reliable Broadcast Cast %d (Slept for %d s)\n", pid, i, pid + i * 1000, sleep_time);
		//fprintf(file_pointer, "%d: A %02d Send TOB Cast %d (Slept for %d s)\n", pid, i, pid + i * 1000, sleep_time);
		//fflush(file_pointer);
		DPA_eager_reliable_broadcast_send(pid + i * 1000);
	}

	for (i = 0; i < n * times; i++) {

		res = -1;
		while (res == -1) {
			res = DPA_eager_reliable_broadcast_deliver(&data, &sender);
			nanosleep(&tim, &tim2);
		}
		printf("%d: B %02d - Reliable Broadcast Deliver, data: %d, sender: %d \n", pid, i, data, sender);
		fprintf(file_pointer, "%d: B %02d - Reliable Broadcast Deliver, data: %d, sender: %d \n", pid, i, data, sender);
		fflush(file_pointer);
	}
}

void test_best_effort_broadcast_2(int pid, int n) {

	int times = 20;
	int res = -1;
	int data = -1;
	int sender = -1;
	int i = 0;
	int sleep_time = 0;
	for (i = 0; i < 10 * (pid + 1); i++)
		rand();

	for (i = 0; i < times; i++) {

		do {
			sleep_time = rand() % 10;
		} while (sleep_time == 0);
		sleep(sleep_time);

		printf("%d: A %02d Send BEB Cast %d (Slept for %d s)\n", pid, i, pid + i * 1000, sleep_time);
		//fprintf(file_pointer, "%d: A %02d Send TOB Cast %d (Slept for %d s)\n", pid, i, pid + i * 1000, sleep_time);
		DPA_best_effort_broadcast_send(pid + i * 1000);
		DPA_best_effort_broadcast_send(pid + i * 10000);
	}

	for (i = 0; i < 2 * n * times; i++) {

		res = -1;
		while (res == -1) {
			res = DPA_best_effort_broadcast_deliver(&data, &sender);
			nanosleep(&tim, &tim2);
		}

		printf("%d: B %02d - Best Effort Broadcast Deliver, data: %d, sender: %d \n", pid, i, data, sender);
		//fprintf(file_pointer, "%d: B %02d - Best Effort Broadcast Deliver, data: %d, sender: %d \n", pid, i, data,
		//		sender);
		//fflush(file_pointer);
		fflush(stdout);
	}
}

void test_best_effort_broadcast(int pid, int n) {

	int times = 10;
	int res = -1;
	int data = -1;
	int sender = -1;
	int i = 0;
	int sleep_time = 0;
	for (i = 0; i < 10 * (pid + 1); i++)
		rand();

	for (i = 0; i < times; i++) {

		do {
			sleep_time = rand() % 10;
		} while (sleep_time == 0);
		sleep(sleep_time);

		printf("%d: A %02d Send BEB Cast %d (Slept for %d s)\n", pid, i, pid + i * 1000, sleep_time);
		//fprintf(file_pointer, "%d: A %02d Send TOB Cast %d (Slept for %d s)\n", pid, i, pid + i * 1000, sleep_time);
		DPA_best_effort_broadcast_send(pid + i * 1000);
	}

	for (i = 0; i < n * times; i++) {

		res = -1;
		while (res == -1) {
			res = DPA_best_effort_broadcast_deliver(&data, &sender);
			nanosleep(&tim, &tim2);
		}

		printf("%d: B %02d - Best Effort Broadcast Deliver, data: %d, sender: %d \n", pid, i, data, sender);
		//fprintf(file_pointer, "%d: B %02d - Best Effort Broadcast Deliver, data: %d, sender: %d \n", pid, i, data,
		//		sender);
		//fflush(file_pointer);
		fflush(stdout);
	}
}

void test_perfect_failure_detector(int pid, int n) {
	int res = -1;
	int rank;
	int i = 0;
	for (i = 0; i < n - 1; i++) {

		while (res == -1) {
			res = DPA_perfect_failure_detector_detected(&rank);
			nanosleep(&tim, &tim2);
		}
		res = -1;
		printf("%d: Process %d crashed\n", pid, rank);
		fflush(stdout);
		fprintf(file_pointer, "%d: Process %d crashed\n", pid, rank);
		fflush(file_pointer);
	}
}

static char *time_stamp() {

	char *timestamp = (char *) malloc(sizeof(char) * 16);
	time_t ltime;
	ltime = time(NULL );
	struct tm *tm;
	tm = localtime(&ltime);

	sprintf(timestamp, "%04d%02d%02d%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min,
			tm->tm_sec);
	return timestamp;
}

void main_logger_init() {

	char hostname[50]; /* hostname string */
	char log_file_path[256];

	/* get hostname */
	gethostname(hostname, sizeof(hostname));

	sprintf(log_file_path, "%sPROCESS_%d_%s_on_%s_App", log_path, my_rank, time_stamp(), hostname);

	file_pointer = fopen(log_file_path, "w");

	fprintf(file_pointer, "#### PROCESS Rank %d on host %s Application starting ####\n", my_rank, hostname);
	fflush(file_pointer);

}