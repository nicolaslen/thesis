/*
 ============================================================================
 Name        : dpa_BEBroadcast.c
 Author      : Nicolò Rivetti
 Created on  : Aug 17, 2012
 Version     : 1.0
 Copyright   : Copyright (c) 2012 Nicolò Rivetti
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "dpa_Abstraction.h"
#include "dpa_Common.h"
#include "dpa_PerfectFailureDetector.h"
#include "dpa_BEBroadcast.h"

// Logger Defines
#define DEBUG_BEBROADCAST -1
#define INFO_BEBROADCAST INFO_LOG_LEVEL
#define ERROR_BEBROADCAST ERROR_LOG_LEVEL

// Limits Defines
#define MAX_CALLBACK_NUM 10

// Application Layer Interaction Data
static struct {
	long type;
	int message;
} beb_send;

static struct {
	long type;
	struct {
		int sender;
		int message;
	} data;
} beb_deliver;

static int runtime_request_handle = -1;
static int application_layer_handle = -1;
static int application_layer_message_id = -1;

// Runtime Interaction Data
static int runtime_receive_handle = -1;

// Initialization Flag
static int initialized = 0;

// Callback Data
static int (**callback)(int*, int, int) = NULL;
static int callback_num = 0;

// Abstraction Logic Data

#ifndef BUFFEREDSEND

struct buffer_data_struct {
	int* buffer;
	MPI_Request **mpi_requests;
	struct buffer_data_struct * next;
};

typedef struct buffer_data_struct buffer_data;
static buffer_data *buffer_data_head = NULL;
static buffer_data *buffer_data_tail = NULL;

#endif

// Application Layer Interaction
int DPA_best_effort_broadcast_send(int data);
int DPA_best_effort_broadcast_deliver(int* data, int *sender);

// Application Layer Internal Management
int _application_layer_best_effort_broadcast_init();
int _best_effort_broadcast_request_callback();
int _best_effort_broadcast_notify(int* data, int size);
int _best_effort_broadcast_deliver_to_application_callback(int* message, int count, int sender);

// Abstraction Logic Implementation
int _best_effort_broadcast_init(unsigned int flags);
int _best_effort_broadcast_cleanup();
int _best_effort_broadcast_send(int *message, int count, int handle);
int _best_effort_broadcast_send_callback(int *data, int size);
int _best_effort_broadcast_receive(int *message, int count, int sender);

// Commodity and Utility Functions
int _best_effort_broadcast_register_callback(int (*callback_func)(int*, int, int), int* handle);
int best_effort_broadcast_dummy_callback(int* message, int count, int sender);

int _add_buffer(int* buffer, MPI_Request** mpi_requests);
int _remove_buffer();

// APPLICATION LAYER INTERACTION

int DPA_best_effort_broadcast_send(int data) {
	if (application_layer_message_id == -1)
		return -1;
	beb_send.type = application_layer_message_id;
	beb_send.message = data;

	return _send_to_runtime(&beb_send, sizeof(beb_send.message));
}

int DPA_best_effort_broadcast_deliver(int* data, int *sender) {
	if (application_layer_message_id == -1)
		return -1;

	if (_receive_from_runtime(&beb_deliver, sizeof(beb_deliver.data), application_layer_message_id) == -1)
		return -1;

	*data = beb_deliver.data.message;
	*sender = beb_deliver.data.sender;
	return 0;
}

// APPLICATION LAYER INTERNAL MANAGEMENT

/*
 * This function is called by the application layer, inside the library init,
 * to retrieve the id of the messages sent from this subsystem to the application
 * layer
 */
int _application_layer_best_effort_broadcast_init() {
	return _receive_id_from_runtime(&application_layer_message_id);
}

int _best_effort_broadcast_request_callback() {
	if (application_layer_handle == -1)
		return -1;

	if (_receive_from_application(&beb_send, sizeof(beb_send.message), application_layer_message_id) == -1)
		return -1;

	/*
	 int message[20];
	 int i = 0;
	 for (i = 0; i < my_rank + 1; i++)
	 message[i] = beb_send.message + i;
	 */
	return _best_effort_broadcast_send(&beb_send.message, 1, application_layer_handle);
}

int _best_effort_broadcast_notify(int* data, int size) {
	if (application_layer_message_id == -1)
		return -1;
	beb_deliver.type = application_layer_message_id;
	beb_deliver.data.sender = data[0];
	beb_deliver.data.message = data[1];
	free(data);
	return _sent_to_application(&beb_deliver, sizeof(beb_deliver.data));
}

int _best_effort_broadcast_deliver_to_application_callback(int* message, int count, int sender) {
	if (application_layer_message_id == -1)
		return -1;

	int *buffer = (int*) malloc(sizeof(int) * 2);
	if (buffer == NULL ) {
		sprintf(log_buffer, "BEB ERROR:Cannot allocate data buffer when posting the delivered event.");
		runtime_log(ERROR_BEBROADCAST);
		return -1;
	}
	buffer[0] = sender;
	buffer[1] = message[0];

	return _add_event(buffer, 2, _best_effort_broadcast_notify);
}

// ABSTRACTION LOGIC IMPLEMENTATION

/*
 * Initialization function
 * Allocates the callback datastructure and the the buffer data queue, which holds the buffer pointer and
 * MPI_Request array used for each sends until required by the MPI layer.
 */
int _best_effort_broadcast_init(unsigned int flags) {
	if (initialized)
		return 0;

	int i = 0;

	sprintf(log_buffer, "BEB INFO: Initializing BEBroadcast");
	runtime_log(INFO_BEBROADCAST);

	// Initializing required Abstractions
	if (_perfect_failure_detector_init(flags) == -1) {
		sprintf(log_buffer, "BEB ERROR:Cannot Initialize Perfect Failure Detector.");
		runtime_log(ERROR_BEBROADCAST);
		return -1;
	}

	// Allocating callback array
	callback = (int (**)(int*, int, int)) malloc(sizeof(int (*)(int*, int, int)) * MAX_CALLBACK_NUM);
	if (callback == NULL ) {
		sprintf(log_buffer, "BEB ERROR:Cannot allocate callback vector");
		runtime_log(ERROR_BEBROADCAST);
		return -1;
	}

	// Initializing callback array and callback_num
	for (i = 0; i < MAX_CALLBACK_NUM; i++)
		callback[i] = best_effort_broadcast_dummy_callback;
	callback_num = 0;

#ifndef BUFFEREDSEND

	// Allocate the buffer data queue head
	buffer_data_head = (buffer_data *) malloc(sizeof(buffer_data));
	if (buffer_data_head == NULL ) {
		sprintf(log_buffer, "BEB ERROR:Cannot allocate the buffer data queue head");
		runtime_log(ERROR_BEBROADCAST);
		return -1;
	}

	// Initialize the buffered data queue head and tail
	buffer_data_head->buffer = NULL;
	buffer_data_head->mpi_requests = NULL;
	buffer_data_head->next = NULL;
	buffer_data_tail = buffer_data_head;

#endif

	// Setting up, if necessary, the callback and communication with the application layer
	if (flags & USING_BEST_EFFORT_BROADCAST) {
		if (_best_effort_broadcast_register_callback(_best_effort_broadcast_deliver_to_application_callback,
				&application_layer_handle) == -1)
			return -1;

		if (_runtime_register_request_callback(_best_effort_broadcast_request_callback, &runtime_request_handle) == -1)
			return -1;

		if (_get_application_layer_message_id(&application_layer_message_id) == -1)
			return -1;

		if (_send_id_to_application(application_layer_message_id) == -1)
			return -1;
	}

	// Registering callbacks to runtime
	_runtime_register_receive_callback(_best_effort_broadcast_receive, &runtime_receive_handle);

	initialized = 1;

	return 0;
}

/*
 * Cleanup function
 * Deallocates the callback datastructure and the buffer data queue
 */
int _best_effort_broadcast_cleanup() {
	if (callback != NULL )
		free(callback);

	if (buffer_data_head != NULL && buffer_data_tail != NULL ) {

		buffer_data *parent = buffer_data_head->next;
		int j = 0;
		while (parent != NULL ) {
			buffer_data * bd = parent;
			parent = bd->next;

			free(bd->buffer);
			for (j = 0; j < num_procs; j++) {
				if (bd->mpi_requests[j] != NULL )
					free(bd->mpi_requests[j]);
			}

			free(bd->mpi_requests);
			free(bd);
		}

		free(buffer_data_head);
	}
	return 0;
}

/*
 * upon event <bebBroadcast | m>
 *
 * Given a pointer to the message array, the number of elements in the array and handle,
 * it packs the message and the handle into a new buffer and creates the related event,
 * which will cause the real communication to be performed when the event will be consumed
 */
int _best_effort_broadcast_send(int *message, int count, int handle) {
	if (count < 1) {
		sprintf(log_buffer, "BEB ERROR: message is empty.");
		runtime_log(ERROR_BEBROADCAST);
		return -1;
	}
	//printf("BEB INFO: Rank %d registering message \"%d\" with handle %d of size %d\n", my_rank, message[0], handle, count);
	sprintf(log_buffer, "BEB INFO: Registering message \"%d\" with handle %d of size %d", message[0], handle, count);
	runtime_log(DEBUG_BEBROADCAST);

// Copying the message, adding BEB callback.
	int * buffer = (int*) malloc(sizeof(int) * (count + 1));
	if (buffer == NULL ) {
		sprintf(log_buffer, "BEB ERROR:Cannot allocate data buffer when posting the send event.");
		runtime_log(ERROR_BEBROADCAST);
		return -1;
	}
	buffer[0] = handle;
	copy_to_array(&buffer[1], message, count);
	return _add_event(buffer, count + 1, _best_effort_broadcast_send_callback);
}

/*
 * Callback for: upon event <bebBroadcast | m>
 *
 * This function is called when the _best_effort_broadcast_send event is consumed by the runtime.
 * Since the data buffer has been created by the _best_effort_broadcast_send function, no worries
 * of accidental side effect. The buffer is freed when the sends are performed
 */

int _best_effort_broadcast_send_callback(int *message, int size) {
	if (size < 1)
		return -1;

	sprintf(log_buffer, "BEB INFO: Sending message \"%d\" with handle %d of size %d", message[1], message[0], size);
	runtime_log(DEBUG_BEBROADCAST);

//TODO: ERROR
	int current_process = 0;

#ifndef BUFFEREDSEND
	if (_remove_buffer() == -1)
		return -1;

	MPI_Request** mpi_requests = (MPI_Request**) malloc(sizeof(MPI_Request*) * num_procs);
	if (mpi_requests == NULL ) {
		sprintf(log_buffer, "BEB ERROR:Cannot allocate the mpi requests pointer table.");
		runtime_log(ERROR_BEBROADCAST);
		return -1;
	}
	if (_add_buffer(message, mpi_requests) == -1)
		return -1;

#endif

	for (current_process = 0; current_process < num_procs; current_process++) {
		if (detected[current_process] == 0) {
#ifdef BUFFEREDSEND
			MPI_Bsend(message, size, MPI_INT, current_process, runtime_receive_handle, comm);
#endif
#ifndef BUFFEREDSEND
			MPI_Isend(message, size, MPI_INT, current_process, runtime_receive_handle, comm,
					mpi_requests[current_process]);
#endif
			sprintf(log_buffer, "BEB INFO: Sending message \"%d\" of size %d with handle %d to process %d", message[1],
					size, message[0], current_process);
			runtime_log(DEBUG_BEBROADCAST);
		}
	}
	print_data("Sending Message", "value", message, size, DEBUG_BEBROADCAST);
#ifdef BUFFEREDSEND
	free(message);
#endif
	return 0;
}

/*
 * upon event <bebDeliver | p, m>
 *
 * Function called when a message with the BEST_EFFORT_BROADCAST tag is received.
 * It copied the message, replacing the handle value with the sender value, into a new buffer. This
 * way the destination subsystem will not have to worry about doind side effect
 * The callback associated with the handle is called passing the buffer and it's size as arguments
 */
int _best_effort_broadcast_receive(int *data, int count, int sender) {
	if (count < 1) {
		sprintf(log_buffer, "BEB ERROR: Message does not contain callback reference");
		runtime_log(ERROR_BEBROADCAST);
		return -1;
	}

	int callback_id = data[0];

	if (callback_id < 0 || callback_id > callback_num) {
		sprintf(log_buffer, "BEB ERROR: Handle out of range: %d", callback_id);
		runtime_log(ERROR_BEBROADCAST);
		return -1;
	}

	//printf("BEB INFO: Rank %d received message \"%d\" of size %d with handle %d from process %d\n", my_rank, data[1], count, callback_id, sender);
	sprintf(log_buffer, "BEB INFO: Received message \"%d\" of size %d with handle %d from process %d", data[1], count,
			callback_id, sender);
	runtime_log(DEBUG_BEBROADCAST);

	print_data("BEB Received Message", "value", data, count, DEBUG_BEBROADCAST);

	if ((count - 1) == 0) {
		callback[callback_id](NULL, 0, sender);
	} else {
		callback[callback_id](&(data[1]), count - 1, sender);
	}

	return 0;
}

// COMMODITY AND UTILITY FUNCTIONS

/*
 * Utility function to register a callback and getting the related handle
 */
int _best_effort_broadcast_register_callback(int (*callback_func)(int*, int, int), int* handle) {
	int i = 0;
	for (i = 0; i < MAX_CALLBACK_NUM; i++) {
		if (callback[i] == best_effort_broadcast_dummy_callback) {
			callback[i] = callback_func;
			*handle = i;
			callback_num++;
			sprintf(log_buffer, "BEB INFO: Callback %d registered", i);
			runtime_log(DEBUG_BEBROADCAST);
			return 0;
		}
	}
	sprintf(log_buffer, "BEB ERROR: Cannot register callback, no slot available");
	runtime_log(ERROR_BEBROADCAST);
	return -1;
}

/*
 * Place holder for the callback vector, should not be invoked
 */
int best_effort_broadcast_dummy_callback(int* message, int count, int sender) {
	sprintf(log_buffer, "BEB ERROR: Referenced callback is not registered");
	runtime_log(ERROR_BEBROADCAST);
	return -1;
}

int _add_buffer(int* buffer, MPI_Request** mpi_requests) {
	int i = 0;
	buffer_data *bd = (buffer_data*) malloc(sizeof(buffer_data));
	if (bd == NULL ) {
		sprintf(log_buffer, "BEB ERROR:Cannot allocate the buffer data node.");
		runtime_log(ERROR_BEBROADCAST);
		return -1;
	}
	bd->buffer = buffer;
	for (i = 0; i < num_procs; i++) {
		mpi_requests[i] = (MPI_Request*) malloc(sizeof(MPI_Request));
		if (mpi_requests[i] == NULL ) {
			sprintf(log_buffer, "BEB ERROR:Cannot allocate the mpi request.");
			runtime_log(ERROR_BEBROADCAST);
			return -1;
		}
	}
	bd->mpi_requests = mpi_requests;
	bd->next = NULL;

//	if (my_rank == 0)
//		printf("%d: Creating buffer\n", my_rank);

	buffer_data_tail->next = bd;
	buffer_data_tail = bd;
	return 0;
}

int _remove_buffer() {
	buffer_data *parent = buffer_data_head;
	int i = 0;
	int j = 0;
	int flag = 0;
	MPI_Status status;
	while (parent->next != NULL ) {
		buffer_data * bd = parent->next;
		flag = 0;
		for (i = 0; i < num_procs; i++) {
			if (detected[i] == 0) {
				MPI_Test(bd->mpi_requests[i], &flag, &status);
				if (!flag || (status.MPI_SOURCE == MPI_ANY_SOURCE && status.MPI_TAG == MPI_ANY_TAG)) {
					flag = 0;
					break;
				}
			} else {
				//MPI_Cancel(bd->mpi_requests[i]);
				//TODO Da errore
				//MPI_Request_free(bd->mpi_requests[i]);
				bd->mpi_requests[i] = NULL;
			}
		}
		if (flag) {
//			if (my_rank == 0)
//				printf("%d: Freeing buffer\n", my_rank);

			parent->next = bd->next;
			free(bd->buffer);
			for (j = 0; j < num_procs; j++) {
				if (bd->mpi_requests[j] != NULL )
					free(bd->mpi_requests[j]);
			}

			free(bd->mpi_requests);
			free(bd);
		} else {
			parent = parent->next;
		}
	}

	buffer_data_tail = parent;
	return 0;
}
