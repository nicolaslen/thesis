/*
 ============================================================================
 Name        : dpa_Runtime.c
 Author      : Nicolò Rivetti
 Created on  : Aug 10, 2012
 Version     : 1.0
 Copyright   : Copyright (c) 2012 Nicolò Rivetti
 Description :
 ============================================================================
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "dpa_Abstraction.h"
#include "dpa_PerfectFailureDetector.h"

// Logger Defines
#define DEBUG_RUNTIME  -1
#define INFO_RUNTIME  INFO_LOG_LEVEL
#define ERROR_RUNTIME ERROR_LOG_LEVEL

// Limits Defines
#define MAX_CALLBACK_NUM 10

// Application Layer Interaction Data
static struct {
	long type;
	int message;
} runtime_send;

static int application_layer_message_id = -1;
static int runtime_receive_handle = -1;

// Initialization Flag
static int initialized = 0;

// Callback Data
static int (**cleanup_callback)(void);
static int cleanup_callback_num = 0;
static int (**receive_callback)(int*, int, int);
static int receive_callback_num = 0;
static int (**request_callback)(void);
static int request_callback_num = 0;
static int (**timely_callback)(void);
static int timely_callback_num = 0;

// Runtime Logic Data
static int close_application = 0;

struct timespec tim, tim2;

// Application Layer Internal Interaction
int _runtime_send_command(int command, int * answer);

// Application Layer Internal Management
int _application_layer_runtime_init();
int _runtime_request_callback();

// Runtime Logic Implementation
int _runtime_init(int argc, char **argv, unsigned int flags);
int _runtime_cleanup();
int _runtime();

// Commodity and Utility Functions
int perform_request_callback();
int perform_receive_callback(int* message, int size, int sender, int tag);
int perform_timely_callbacks();
int _runtime_register_cleanup_callback(int (*callback_func)(void), int* handle);
int _runtime_register_receive_callback(int (*callback_func)(int*, int, int), int* handle);
int _runtime_register_request_callback(int (*callback_func)(void), int* handle);
int _runtime_register_timely_callback(int (*callback_func)(void), int* handle);

// APPLICATION LAYER INTERNAL INTERACTION

int _runtime_send_command(int command, int * answer) {
	if (application_layer_message_id == -1)
		return -1;

	*answer = -1;

	runtime_send.type = application_layer_message_id;
	runtime_send.message = command;
	_send_to_runtime(&runtime_send, sizeof(runtime_send.message));

	switch (command) {
	case SHUTDOWN:
		break;
	case GET_PROC_NUM:
	case GET_RANK:
		_receive_from_runtime_blck(&runtime_send, sizeof(runtime_send.message), application_layer_message_id);
		*answer = runtime_send.message;
		break;
	default:
		return -1;
		break;
	}

	return 0;
}

// APPLICATION LAYER INTERNAL MANAGEMENT

int _application_layer_runtime_init() {
	_receive_id_from_runtime(&application_layer_message_id);
	return 0;
}

int _runtime_request_callback() {
	if (application_layer_message_id == -1)
		return -1;

	if (_receive_from_application(&runtime_send, sizeof(runtime_send.message), application_layer_message_id) == -1)
		return -1;
	
	switch (runtime_send.message) {
	case SHUTDOWN:
		close_application = 1;
		break;
	case GET_PROC_NUM:
		runtime_send.message = num_procs;
		_sent_to_application(&runtime_send, sizeof(runtime_send.message));
		break;
	case GET_RANK:
		runtime_send.message = my_rank;
		_sent_to_application(&runtime_send, sizeof(runtime_send.message));
		break;
	default:
		return -1;
		break;
	}

	return 0;
}

// RUNTIME LOGIC IMPLEMENTATION

int _runtime_init(int argc, char **argv, unsigned int flags) {
	if (initialized)
		return -1;
	int i = 0;

	tim.tv_sec = 0;
	tim.tv_nsec = 1000000L;

	/* start up MPI if necessary */
	MPI_Initialized(&initialized);
	if (!initialized) {
		MPI_Init(&argc, &argv);
	}

	comm = MPI_COMM_WORLD;

	/* find out process rank */
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	/* find out number of processes */
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

	logger_init();

	cleanup_callback_num = 0;
	cleanup_callback = (int (**)(void)) malloc(sizeof(int (*)(void)) * MAX_CALLBACK_NUM);
	if (cleanup_callback == NULL ) {
		sprintf(log_buffer, "RUNTIME ERROR:Cannot allocate cleanup callback vector");
		runtime_log(ERROR_RUNTIME);
	}
	for (i = 0; i < MAX_CALLBACK_NUM; i++)
		cleanup_callback[i] = NULL;

	receive_callback_num = 0;
	receive_callback = (int (**)(int*, int, int)) malloc(sizeof(int (*)(int*, int, int)) * MAX_CALLBACK_NUM);
	if (receive_callback == NULL ) {
		sprintf(log_buffer, "RUNTIME ERROR:Cannot allocate receive callback vector");
		runtime_log(ERROR_RUNTIME);
	}
	for (i = 0; i < MAX_CALLBACK_NUM; i++)
		receive_callback[i] = NULL;

	request_callback_num = 0;
	request_callback = (int (**)(void)) malloc(sizeof(int (*)(void)) * MAX_CALLBACK_NUM);
	if (request_callback == NULL ) {
		sprintf(log_buffer, "RUNTIME ERROR:Cannot allocate request callback vector");
		runtime_log(ERROR_RUNTIME);
	}
	for (i = 0; i < MAX_CALLBACK_NUM; i++)
		request_callback[i] = NULL;

	timely_callback_num = 0;
	timely_callback = (int (**)(void)) malloc(sizeof(int (*)(void)) * MAX_CALLBACK_NUM);
	if (timely_callback == NULL ) {
		sprintf(log_buffer, "RUNTIME ERROR:Cannot allocate timely callback vector");
		runtime_log(ERROR_RUNTIME);
	}
	for (i = 0; i < MAX_CALLBACK_NUM; i++)
		timely_callback[i] = NULL;

	_event_queue_init();

	_runtime_register_request_callback(_runtime_request_callback, &runtime_receive_handle);
	_get_application_layer_message_id(&application_layer_message_id);
	_send_id_to_application(application_layer_message_id);

	initialized = 1;

	return 0;
}

int _runtime_cleanup() {
	int i = 0;
	for (i = 0; i < cleanup_callback_num; i++)
		if (cleanup_callback[i] != NULL )
			cleanup_callback[i]();

	_event_queue_cleanup();
	logger_clean();

	if(cleanup_callback != NULL)
	free(cleanup_callback);

	if(receive_callback != NULL)
	free(receive_callback);

	if(request_callback != NULL)
	free(request_callback);

	if(timely_callback != NULL)
	free(timely_callback);

	if (detected_num != 0) {
		printf("MPI Abort\n");
		MPI_Abort(comm, 1);
	}

	printf("MPI Finalize\n");
	MPI_Finalize();

//	runtime_send.message = 1;
//	printf("sending closed message\n");
//	_sent_to_application(&runtime_send, sizeof(runtime_send.message));

	return 0;
}

int _runtime() {
	MPI_Status probe_status;
	int probe_flag = 0;
	MPI_Status rcv_status;
	int i = 0;
	int *message_buffer;
	int message_buffer_size = 2;
	int message_size = 0;

	message_buffer = malloc(sizeof(MPI_INT ) * message_buffer_size);
	if (message_buffer == NULL ) {
		sprintf(log_buffer, "RUNTIME ERROR: Cannot allocate initial message_buffer.");
		runtime_log(ERROR_RUNTIME);
		return -1;
	}

	/* Barrier synch to check that all subsystems have been initialized */
	MPI_Barrier(comm);

	while (!close_application) {
		probe_flag = 0;
		MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &probe_flag, &probe_status);
		if (probe_flag) {
			if (!probe_status.MPI_ERROR) {
				MPI_Get_count(&probe_status, MPI_INT, &message_size);

				if (message_size > message_buffer_size) {
					free(message_buffer);
					message_buffer = malloc(sizeof(MPI_INT ) * message_size);
					if (message_buffer == NULL ) {
						sprintf(log_buffer, "RUNTIME ERROR: Cannot allocate resized message_buffer.");
						runtime_log(ERROR_RUNTIME);
						return -1;
					}
					message_buffer_size = message_size;
				}

				MPI_Recv(message_buffer, message_size, MPI_INT, probe_status.MPI_SOURCE, probe_status.MPI_TAG, comm,
						&rcv_status);
				perform_receive_callback(message_buffer, message_size, probe_status.MPI_SOURCE, probe_status.MPI_TAG);
			}
		}
		perform_request_callback();
		perform_timely_callbacks();
		_perform_event();
		i++;
		if (!probe_flag) {
			nanosleep(&tim, &tim2);
		}

	}
	free(message_buffer);
	_runtime_cleanup();
	return 0;
}

int perform_request_callback() {
	int i = 0;
	for (i = 0; i < request_callback_num; i++) {
		if (request_callback[i] != NULL ) {
			if (request_callback[i]() != -1)
				return 0;
		}
	}

	return 0;
}

int perform_receive_callback(int* message, int size, int sender, int tag) {
	if (tag > -1 && tag < receive_callback_num && receive_callback[tag] != NULL ) {
		return receive_callback[tag](message, size, sender);
	}
	return -1;
}

int perform_timely_callbacks() {
	int i = 0;
	int ret = 0;
	for (i = 0; i < timely_callback_num; i++)
		if (timely_callback[i] != NULL ) {
			if (timely_callback[i]() != 0)
				ret = -1;
		}
	return ret;
}

/*
 * Utility function to register a cleanup callbacks and getting the related handle
 */
int _runtime_register_cleanup_callback(int (*callback_func)(void), int* handle) {
	int i = 0;
	for (i = 0; i < MAX_CALLBACK_NUM; i++) {
		if (cleanup_callback[i] == NULL ) {
			cleanup_callback[i] = callback_func;
			*handle = i;
			cleanup_callback_num++;
			sprintf(log_buffer, "FD INFO: Cleanup callback %d registered", i);
			runtime_log(DEBUG_RUNTIME);
			return 0;
		}
	}
	sprintf(log_buffer, "RUNTIME ERROR: Cannot register cleanup callback, no slot available");
	runtime_log(ERROR_RUNTIME);
	return -1;
}

/*
 * Utility function to register a receive callbacks and getting the related handle
 */
int _runtime_register_receive_callback(int (*callback_func)(int*, int, int), int* handle) {
	int i = 0;
	for (i = 0; i < MAX_CALLBACK_NUM; i++) {
		if (receive_callback[i] == NULL ) {
			receive_callback[i] = callback_func;
			*handle = i;
			receive_callback_num++;
			sprintf(log_buffer, "RUNTIME INFO: Receive callback %d registered", i);
			runtime_log(DEBUG_RUNTIME);
			return 0;
		}
	}
	sprintf(log_buffer, "RUNTIME ERROR: Cannot register receive callback, no slot available");
	runtime_log(ERROR_RUNTIME);
	return -1;
}

/*
 * Utility function to register a request callbacks and getting the related handle
 */
int _runtime_register_request_callback(int (*callback_func)(void), int* handle) {
	int i = 0;
	for (i = 0; i < MAX_CALLBACK_NUM; i++) {
		if (request_callback[i] == NULL ) {
			request_callback[i] = callback_func;
			*handle = i;
			request_callback_num++;
			sprintf(log_buffer, "FD INFO: Request callback %d registered", i);
			runtime_log(DEBUG_RUNTIME);
			return 0;
		}
	}
	sprintf(log_buffer, "RUNTIME ERROR: Cannot register request callback, no slot available");
	runtime_log(ERROR_RUNTIME);
	return -1;
}

/*
 * Utility function to register a timely callbacks and getting the related handle
 */
int _runtime_register_timely_callback(int (*callback_func)(void), int* handle) {
	int i = 0;
	for (i = 0; i < MAX_CALLBACK_NUM; i++) {
		if (timely_callback[i] == NULL ) {
			timely_callback[i] = callback_func;
			*handle = i;
			timely_callback_num++;
			sprintf(log_buffer, "RUNTIME INFO: Timely callback %d registered", i);
			runtime_log(DEBUG_RUNTIME);
			return 0;
		}
	}
	sprintf(log_buffer, "RUNTIME ERROR: Cannot register timely callback, no slot available");
	runtime_log(ERROR_RUNTIME);
	return -1;
}

//TODO Dummy Callbacks
