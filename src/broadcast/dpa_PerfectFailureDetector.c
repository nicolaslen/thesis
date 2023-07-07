/*
 ============================================================================
 Name        : dpa_PerfectFailureDetector.c
 Author      : Nicolò Rivetti
 Created on  : Aug 10, 2012
 Version     : 1.0
 Copyright   : Copyright (c) 2012 Nicolò Rivetti
 Description : Implementation of a Perfect Failure Detector
 ============================================================================
 */
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "dpa_Abstraction.h"
#include "dpa_Common.h"
#include "dpa_PerfectFailureDetector.h"

//#define BUFFEREDSEND

// Logger Defines
#define DEBUG_PERFECTFAILUREDETECTOR  -1
#define INFO_PERFECTFAILUREDETECTOR  INFO_LOG_LEVEL
#define ERROR_PERFECTFAILUREDETECTOR ERROR_LOG_LEVEL

#define BUFFEREDSEND

// Limits Defines
#define MAX_CALLBACK_NUM 10

// Abstraction Logic Defines
#define TIMEOUT 5
#define HEARTBEAT_REQUEST 0
#define HEARTBEAT_REPLY 1

// Application Layer Interaction Data
static struct {
	long type;
	int rank;
} detected_msg;

static int application_layer_handle = -1;
static int application_layer_message_id = -1;

// Runtime Interaction Data
static int runtime_receive_handle = -1;
static int runtime_timely_handle = -1;
static int runtime_cleanup_handle = -1;

// Initialization Data
static int initialized = 0;

// Callback Data
static int (**callback)(int*, int) = NULL;
static int callback_num = 0;

static int *currently_detected = NULL;
static int currently_detected_num = 0;

// Abstraction Logic Data
static int heartbeat_request_message = HEARTBEAT_REQUEST;
static int heartbeat_reply_message = HEARTBEAT_REPLY;

static int timeout_interval; /* timeout interval in ms*/
static int current_time = 0;
static int previous_time = 0;

#ifndef BUFFEREDSEND
static MPI_Request *mpi_request_reply = NULL;
static MPI_Request *mpi_request_request = NULL;
#endif

#ifdef BUFFEREDSEND
static void *attached_buffer = NULL;
#endif

// Application Layer Interaction
int DPA_perfect_failure_detector_detected(int * rank);

// Application Layer Internal Management
int _application_layer_perfect_failure_detector_init();
int _perfect_failure_detector_notify(int* data, int size);
int _perfect_failure_detector_crash_to_application_callback(int * ranks, int num);

// Abstraction Logic Implementation
int _perfect_failure_detector_init(unsigned int flags);
int _perfect_failure_detector_cleanup();
int _perfect_failure_detector_receive(int *message, int size, int sender);
int _perfect_failure_detector_timeout();

// Commodity and Utility Functions
int _perfect_failure_detector_register_callback(int (*callback_func)(int*, int), int* handle);
int perfect_failure_detector_dummy_callback(int * ranks, int num);

// APPLICATION LAYER INTERACTION
int DPA_perfect_failure_detector_detected(int * rank) {
	if (application_layer_message_id == -1)
		return -1;
	if (_receive_from_runtime(&detected_msg, sizeof(detected_msg.rank), application_layer_message_id) == -1)
		return -1;

	*rank = detected_msg.rank;
	return 0;

}
// APPLICATION LAYER INTERNAL MANAGEMENT

/*
 * This function is called by the application layer, inside the library init,
 * to retrieve the id of the messages sent from this subsystem to the application
 * layer
 */
int _application_layer_perfect_failure_detector_init() {
	_receive_id_from_runtime(&application_layer_message_id);
	return 0;
}

/*
 * This function is the callback invoked when performing a crash event previously posted by the
 * _perfect_failure_detector_crash_to_application_callback() function.
 * It sends the a message for each process crashed, containing it's rank, and deallocates
 * the buffer containing the rank of the crashed processes
 */
int _perfect_failure_detector_notify(int* data, int size) {
	if (application_layer_message_id == -1)
		return -1;
	int i = 0;
	for (i = 0; i < size; i++) {
		detected_msg.type = application_layer_message_id;
		detected_msg.rank = ((int*) data)[i];
		_sent_to_application(&detected_msg, sizeof(detected_msg.rank));
	}
	free(data);
	return 0;
}

/*
 * This function is the callback invoked when a crash is detected and the application layer uses this
 * subsystem.
 * It allocates a buffer to which the set of crashed processes is copied, and then post a crashed event
 * which callback is _perfect_failure_detector_notify and the buffer as data
 */
int _perfect_failure_detector_crash_to_application_callback(int * ranks, int num) {
	if (application_layer_message_id == -1)
		return -1;

	int *buffer = (int*) malloc(sizeof(int) * num);
	if (buffer == NULL ) {
		sprintf(log_buffer, "FD ERROR: Cannot allocate data buffer when posting the crash event.");
		runtime_log(ERROR_PERFECTFAILUREDETECTOR);
		return -1;
	}
	copy_to_array(buffer, ranks, num);
	_add_event((void*) buffer, num, _perfect_failure_detector_notify);
	return 0;
}

// ABSTRACTION LOGIC IMPLEMENTATION

/*
 * Initialization function
 * Allocates the callback datastructure, the alive and detected array, the currently detected array,
 * the mpi_request_reply and mpi_request_request array (if #ifndef BUFFEREDSEND).
 * If necessary sets up the callback and communication with the application layer.
 * Finally register the callback to the runtime subsystem.
 */
int _perfect_failure_detector_init(unsigned int flags) {
	if (initialized) {
		return 0;
	}

	int i = 0;
	timeout_interval = TIMEOUT;

	sprintf(log_buffer, "FD INFO: Initializing failure detector");
	runtime_log(INFO_PERFECTFAILUREDETECTOR);

#ifndef BUFFEREDSEND

	// Allocating mpi_request_reply array
	mpi_request_reply = (MPI_Request*) malloc(sizeof(MPI_Request) * num_procs);
	if (mpi_request_reply == NULL ) {
		sprintf(log_buffer, "FD ERROR: Cannot allocate mpi_request_reply vector");
		runtime_log(ERROR_PERFECTFAILUREDETECTOR);
		return -1;
	}

	// Allocating mpi_request_request array
	mpi_request_request = (MPI_Request*) malloc(sizeof(MPI_Request) * num_procs);
	if (mpi_request_request == NULL ) {
		sprintf(log_buffer, "FD ERROR: Cannot allocate mpi_request_request vector");
		runtime_log(ERROR_PERFECTFAILUREDETECTOR);
		return -1;
	}

#endif

#ifdef BUFFEREDSEND
	int size = 0;
	MPI_Pack_size(1, MPI_INT, comm, &size);
	size = (size + MPI_BSEND_OVERHEAD) * num_procs;
	attached_buffer = (void*) malloc(size);
	MPI_Buffer_attach(attached_buffer, size);
#endif

	// Allocating alive array
	alive = (short *) malloc(sizeof(short) * num_procs);
	if (alive == NULL ) {
		sprintf(log_buffer, "FD ERROR: Cannot allocate alive vector");
		runtime_log(ERROR_PERFECTFAILUREDETECTOR);
		return -1;
	}

	// Initializing alive array
	for (i = 0; i < num_procs; i++)
		alive[i] = 1;

	// Allocating detected array
	detected = (short *) malloc(sizeof(short) * num_procs);
	if (detected == NULL ) {
		sprintf(log_buffer, "FD ERROR: Cannot allocate detected vector");
		runtime_log(ERROR_PERFECTFAILUREDETECTOR);
		return -1;
	}

	// Initializing detected array and detected_num
	for (i = 0; i < num_procs; i++)
		detected[i] = 0;
	detected_num = 0;

	// Allocating currently_detected array
	currently_detected = (int *) malloc(sizeof(int) * num_procs);
	if (currently_detected == NULL ) {
		sprintf(log_buffer, "FD ERROR: Cannot allocate currently_detected vector");
		runtime_log(ERROR_PERFECTFAILUREDETECTOR);
		return -1;
	}

	// Initializing currently_detected array and currently_detected_num
	for (i = 0; i < num_procs; i++)
		currently_detected[i] = -1;
	currently_detected_num = 0;

	// Allocating callback array
	callback = (int (**)(int*, int)) malloc(sizeof(int (*)(int*, int)) * MAX_CALLBACK_NUM);
	if (callback == NULL ) {
		sprintf(log_buffer, "FD ERROR:Cannot allocate callback vector");
		runtime_log(ERROR_PERFECTFAILUREDETECTOR);
		return -1;
	}

	// Initializing callback array and callback_num
	for (i = 0; i < MAX_CALLBACK_NUM; i++)
		callback[i] = perfect_failure_detector_dummy_callback;
	callback_num = 0;

	// Setting up, if necessary, the callback and communication with the application layer
	if (flags & USING_PERFECT_FAILURE_DETECTOR) {
		if (_perfect_failure_detector_register_callback(_perfect_failure_detector_crash_to_application_callback,
				&application_layer_handle) == -1)
			return -1;

		if (_get_application_layer_message_id(&application_layer_message_id) == -1)
			return -1;

		if (_send_id_to_application(application_layer_message_id) == -1)
			return -1;
	}

	// Registering callbacks to runtime
	if (_runtime_register_receive_callback(_perfect_failure_detector_receive, &runtime_receive_handle) == -1)
		return -1;

	if (_runtime_register_timely_callback(_perfect_failure_detector_timeout, &runtime_timely_handle) == -1)
		return -1;

	if (_runtime_register_cleanup_callback(_perfect_failure_detector_cleanup, &runtime_cleanup_handle) == -1)
		return -1;

	initialized = 1;
	return 0;
}

/*
 * Cleanup function
 * Deallocates the he callback datastructure, the alive and detected array, the currently detected array,
 * the mpi_request_reply and mpi_request_request array (if #ifndef BUFFEREDSEND).
 */

int _perfect_failure_detector_cleanup() {
	if (alive != NULL )
		free(alive);

	if (detected != NULL )
		free(detected);

	if (callback != NULL )
		free(callback);

#ifndef  BUFFEREDSEND
	if (mpi_request_reply != NULL )
	free(mpi_request_reply);

	if (mpi_request_request != NULL )
	free(mpi_request_request);
#endif
#ifdef BUFFEREDSEND
	if (attached_buffer != NULL )
		free(attached_buffer);
#endif

	if (currently_detected != NULL )
		free(currently_detected);

	return 0;
}

/*
 * This function is registered as timely callback to the runtime subsystem, hence it is called frequently
 * by the runtime.
 * It checks if the time interval has elapsed, if it has, then it checks if any process has crashed and
 * eventually notifies the registered subsystems.
 */
int _perfect_failure_detector_timeout() {
	current_time = time(NULL );
	// Check if the timeout interval has elapsed
	if (current_time - previous_time >= timeout_interval) {

		int i = 0;
		// Check for each process
		for (i = 0; i < num_procs; i++) {

			if ((detected[i] == 0) && (alive[i] == 0)) {
				// The process has not replied to the heartbeat request and was not already detected,
				// hence the process is flagged as failed and is added to the process crashed in this
				// period
				sprintf(log_buffer, "FD INFO: Failure of process %d", i);
				runtime_log(INFO_PERFECTFAILUREDETECTOR);
				detected[i] = 1;
				detected_num++;
				currently_detected[currently_detected_num] = i;
				currently_detected_num++;
			} else if (detected[i] == 0) {
				// If the process was not detected and has replied to the heartbeat request,
				// then the process is alive and a new heartbeat request is issued
				// sprintf(log_buffer, "FD INFO: Process %d alive", i);
				runtime_log(INFO_PERFECTFAILUREDETECTOR);
#ifdef BUFFEREDSEND
				MPI_Bsend(&heartbeat_request_message, 1, MPI_INT, i, runtime_receive_handle, comm);
#endif
#ifndef BUFFEREDSEND
				MPI_Isend(&heartbeat_request_message, 1, MPI_INT, i, runtime_receive_handle, comm,
						&(mpi_request_request[i]));
#endif
			}
		}

		if (currently_detected_num > 0) {
			// If at least one process is detected during this interval, then the list of process
			// crashed in this interval is notified through the callbacks
			sprintf(log_buffer, "FD DEBUG: Invoking crash callbacks");
			runtime_log(DEBUG_PERFECTFAILUREDETECTOR);
			for (i = 0; i < callback_num; i++)
				if (callback[i] != NULL )
					callback[i](currently_detected, currently_detected_num);

			// Resetting the set of currently detected processes
			for (i = 0; i < currently_detected_num; i++)
				currently_detected[i] = -1;
		}
		currently_detected_num = 0;

		// Resetting the set of currently alive processes
		for (i = 0; i < num_procs; i++)
			alive[i] = 0;

		// Updating the starting time for the following time interval
		previous_time = time(NULL );
	}
	return 0;
}

/*
 * This function is registered as receive callback to the runtime subsystem, hence it is called when a message
 * with the subsystem tag has been received.
 * It replyes to heartbeat requests and updates the alive array when a heartbeat reply is received.
 * This function does not post an event, as a receive callback should, to avoid delays in heartbeats replies.
 */
int _perfect_failure_detector_receive(int *message, int size, int sender) {
	switch (message[0]) {
	case HEARTBEAT_REPLY:
		alive[sender] = 1;
		break;
	case HEARTBEAT_REQUEST:
		if (detected[sender] == 0)
#ifdef BUFFEREDSEND
			MPI_Bsend(&heartbeat_reply_message, 1, MPI_INT, sender, runtime_receive_handle, comm);
#endif
#ifndef BUFFEREDSEND
		MPI_Isend(&heartbeat_reply_message, 1, MPI_INT, sender, runtime_receive_handle, comm,
				&(mpi_request_reply[sender]));

#endif

		break;
	default:
		//TODO Error
		break;
	}
	return 0;
}

// COMMODITY AND UTILITY FUNCTIONS

/*
 * Utility function to register a callback and getting the related handle
 */
int _perfect_failure_detector_register_callback(int (*callback_func)(int*, int), int* handle) {
	int i = 0;
	for (i = 0; i < MAX_CALLBACK_NUM; i++) {
		if (callback[i] == perfect_failure_detector_dummy_callback) {
			callback[i] = callback_func;
			*handle = i;
			callback_num++;
			sprintf(log_buffer, "FD INFO: Callback %d registered", i);
			runtime_log(DEBUG_PERFECTFAILUREDETECTOR);
			return 0;
		}
	}
	sprintf(log_buffer, "FD ERROR: Cannot register callback, no slot available");
	runtime_log(ERROR_PERFECTFAILUREDETECTOR);
	return -1;
}

/*
 * Place holder for the callback vector, should not be invoked
 */
int perfect_failure_detector_dummy_callback(int * ranks, int num) {
	sprintf(log_buffer, "FD ERROR: Referenced callback is not registered");
	runtime_log(ERROR_PERFECTFAILUREDETECTOR);
	return -1;
}
