/*
 ============================================================================
 Name        : dpa_EagerReliableBroadCast.c
 Author      : Nicolò Rivetti
 Created on  : Aug 25, 2012
 Version     : 1.0
 Copyright   : Copyright (c) 2012 Nicolò Rivetti
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "dpa_Abstraction.h"
#include "dpa_Common.h"
#include "dpa_BEBroadcast.h"
#include "dpa_EagerReliableBroadCast.h"

// Logger Defines
#define DEBUG_EAGERRELIABLEBROADCAST -1
#define INFO_EAGERRELIABLEBROADCAST INFO_LOG_LEVEL
#define ERROR_EAGERRELIABLEBROADCAST ERROR_LOG_LEVEL

// Limits Defines
#define MAX_CALLBACK_NUM 10

// Application Layer Interaction Data
static struct {
	long type;
	int message;
} rb_send;

static struct {
	long type;
	struct {
		int sender;
		int message;
	} data;
} rb_deliver;

static int runtime_request_handle = -1;
static int application_layer_handle = -1;
static int application_layer_message_id = -1;

// Best Effort Broadcast Interaction Data
static int best_effort_broadcast_handle = -1;

// Initialization Flag
static int initialized = 0;

// Callback Data
static int (**callback)(int*, int, int);
static int callback_num = 0;

// Abstraction Logic Data

struct delivered_struct {
	unsigned int id;
	struct delivered_struct *next;
};

typedef struct delivered_struct delivered;

typedef struct {
	serialNumber *gc_threshold;
	delivered *delivered_set;
} proc_data;

static struct timeval gettimeofdayArg;

static proc_data ** proc_data_set;
static unsigned int serial_number;

/* Message array
 * message[0] = handle
 * message[1] = message id
 * message[2] = real sender
 * message[3] = message data
 */

// Application Layer Interaction
int DPA_eager_reliable_broadcast_send(int data);
int DPA_eager_reliable_broadcast_deliver(int* data, int *sender);

// Application Layer Internal Management
int _application_layer_eager_reliable_broadcast_init();
int _eager_reliable_broadcast_request_callback();
int _eager_reliable_broadcast_notify(int* data, int size);
int _eager_reliable_broadcast_deliver_to_application_callback(int* message, int count, int sender);

// Abstraction Logic Implementation
int _eager_reliable_broadcast_init(unsigned int flags);
int _eager_reliable_broadcast_cleanup();
int _eager_reliable_broadcast_send(int *message, int count, int handle);
int _eager_reliable_broadcast_beb_deliver(int *message, int count);

// Commodity and Utility Functions
static int eager_reliable_broadcast_beb_deliver_callback(int *data, int count, int sender);
int _eager_reliable_broadcast_register_callback(int (*callback_func)(int*, int, int), int* handle);
int _eager_reliable_broadcast_unregister_callback(int handle);
static int eager_reliable_broadcast_dummy_callback(int* message, int count, int sender);
static int add_to_delivered(unsigned int id, int real_sender);

// APPLICATION LAYER INTERACTION

int DPA_eager_reliable_broadcast_send(int data) {
	if (application_layer_message_id == -1)
		return -1;
	rb_send.type = application_layer_message_id;
	rb_send.message = data;
	_send_to_runtime(&rb_send, sizeof(rb_send.message));
	return 0;
}

int DPA_eager_reliable_broadcast_deliver(int* data, int *sender) {
	if (application_layer_message_id == -1)
		return -1;

	if (_receive_from_runtime(&rb_deliver, sizeof(rb_deliver.data), application_layer_message_id) == -1)
		return -1;

	*data = rb_deliver.data.message;
	*sender = rb_deliver.data.sender;
	return 0;
}

// APPLICATION LAYER INTERNAL MANAGEMENT

int _application_layer_eager_reliable_broadcast_init() {
	_receive_id_from_runtime(&application_layer_message_id);
	return 0;
}

int _eager_reliable_broadcast_request_callback() {
	if (application_layer_handle == -1)
		return -1;
	if (_receive_from_application(&rb_send, sizeof(rb_send.message), application_layer_message_id) == -1)
		return -1;
	_eager_reliable_broadcast_send(&rb_send.message, 1, application_layer_handle);
	return 0;
}

int _eager_reliable_broadcast_notify(int* data, int size) {
	if (application_layer_message_id == -1)
		return -1;
	rb_deliver.type = application_layer_message_id;
	rb_deliver.data.sender = ((int*) data)[0];
	rb_deliver.data.message = ((int*) data)[1];
	_sent_to_application(&rb_deliver, sizeof(rb_deliver.data));
	free(data);
	return 0;
}

int _eager_reliable_broadcast_deliver_to_application_callback(int* message, int count, int sender) {
	if (application_layer_message_id == -1)
		return -1;

	print_data("EAGER", "message", message, count, DEBUG_EAGERRELIABLEBROADCAST);

	int *buffer = (int*) malloc(sizeof(int) * 2);
	buffer[0] = sender;
	buffer[1] = message[0];
	_add_event(buffer, 2, _eager_reliable_broadcast_notify);
	return 0;
}

// ABSTRACTION LOGIC IMPLEMENTATION

/*
 * Initialization function
 * Allocates the callback and delivered datastructures
 */
int _eager_reliable_broadcast_init(unsigned int flags) {
	if (initialized)
		return -1;

	sprintf(log_buffer, "EAGER RELIABLE BROADCAST INFO: Initializing best effort broadcast");
	runtime_log(INFO_EAGERRELIABLEBROADCAST);

	_best_effort_broadcast_init(flags);

	callback = (int (**)(int*, int, int)) malloc(sizeof(int (*)(int*, int, int)) * MAX_CALLBACK_NUM);
	if (callback == NULL ) {
		sprintf(log_buffer, "EAGER RELIABLE BROADCAST ERROR:Cannot allocate callback vector");
		runtime_log(ERROR_EAGERRELIABLEBROADCAST);
	}

	int i = 0;
	for (i = 0; i < MAX_CALLBACK_NUM; i++)
		callback[i] = eager_reliable_broadcast_dummy_callback;
	callback_num = 0;

	proc_data_set = (proc_data**) malloc(sizeof(proc_data*) * num_procs);
	if (proc_data_set == NULL ) {
		sprintf(log_buffer, "EAGER RELIABLE BROADCAST ERROR:Cannot allocate proc data queue head vector");
		runtime_log(ERROR_EAGERRELIABLEBROADCAST);
	}
	for (i = 0; i < num_procs; i++) {
		proc_data_set[i] = (proc_data*) malloc(sizeof(proc_data));
		proc_data_set[i]->gc_threshold = (serialNumber*) malloc(sizeof(serialNumber));
		initializeSerialNumber(proc_data_set[i]->gc_threshold);
		proc_data_set[i]->delivered_set = (delivered*) malloc(sizeof(delivered));
		proc_data_set[i]->delivered_set->id = -1;
		proc_data_set[i]->delivered_set->next = NULL;
	}

	serial_number = 0;

	if (flags & USING_EAGER_RELIABLE_BROADCAST) {
		_eager_reliable_broadcast_register_callback(_eager_reliable_broadcast_deliver_to_application_callback,
				&application_layer_handle);
		_runtime_register_request_callback(_eager_reliable_broadcast_request_callback, &runtime_request_handle);

		_get_application_layer_message_id(&application_layer_message_id);
		_send_id_to_application(application_layer_message_id);
	}
	_best_effort_broadcast_register_callback(eager_reliable_broadcast_beb_deliver_callback,
			&best_effort_broadcast_handle);
	initialized = 1;
	return 0;
}

/*
 * Cleanup function
 * Deallocates the callback datastructure
 */
//TODO
int _eager_reliable_broadcast_cleanup() {
	free(callback);
	return 0;
}

/*
 * upon event <rbBroadcast | m>
 *
 * Given a pointer to the message array, the number of elements in the array and handle,
 * it packs the message, self id and the handle into a new buffer. The message is sent to the other
 * processes through Best Effort BroadCast
 */
int _eager_reliable_broadcast_send(int *message, int count, int handle) {
	//printf("EAGER RELIABLE BROADCAST INFO: Rank %d registering message \"%d\" with handle %d\n", my_rank, message[0], handle);
	sprintf(log_buffer, "EAGER RELIABLE BROADCAST INFO: Registering message \"%d\" with handle %d", message[0], handle);
	runtime_log(DEBUG_EAGERRELIABLEBROADCAST);

	// Copying the message, adding EAGER RELIABLE BROADCAST callback.
	int * buffer = (int*) malloc(sizeof(int) * (count + 3));
	buffer[0] = handle;
	buffer[1] = serial_number++;
	buffer[2] = my_rank;
	copy_to_array(&buffer[3], message, count);
	_best_effort_broadcast_send(buffer, count + 3, best_effort_broadcast_handle);
	free(buffer);
	return 0;
}

/*
 * upon event <bebDeliver | p, [Data, s, m]>
 * This function is posted as the event callback by the callback function registered to the Best Effort
 * Broadcast subsystem. It is called when a  message has been delivered by the BEB layer.
 * It checks if the message has already been delivered. If it has the message is ignored. If it has not
 * the message id is added to the delivered set of the real sender and the message is forwarded
 * to the other processes throuhg a Best Effort Broadcast
 */

int _eager_reliable_broadcast_beb_deliver(int *data, int count) {

	int* message = (int*) data;

	if (count < 3) {
		sprintf(log_buffer, "EAGER RELIABLE BROADCAST ERROR: Message bad format");
		runtime_log(ERROR_EAGERRELIABLEBROADCAST);
		return -1;
	}

	int handle = message[0];
	int id = message[1];
	int real_sender = message[2];
	int message_size = count - 3;
	if (handle < 0 || handle > callback_num) {
		sprintf(log_buffer, "EAGER RELIABLE BROADCAST ERROR: Handle out of range: %d", handle);
		runtime_log(ERROR_EAGERRELIABLEBROADCAST);
		return -1;
	}

	if (real_sender < 0 || real_sender >= num_procs) {
		sprintf(log_buffer, "EAGER RELIABLE BROADCAST ERROR: Message sent by unkown sender.");
		runtime_log(ERROR_EAGERRELIABLEBROADCAST);
		return -1;
	}

	sprintf(log_buffer,
			"EAGER RELIABLE BROADCAST INFO: Received message \"%d\" of size %d with handle %d from process %d",
			message[1], count, handle, real_sender);
	//printf("AGER RELIABLE BROADCAST INFO: Rank %d received message \"%d\" of size %d with handle %d from process %d\n", my_rank, message[1], count, handle, real_sender);
	runtime_log(DEBUG_EAGERRELIABLEBROADCAST);

	print_data("EAGER RELIABLE BROADCAST Received Message", "value", data, count, DEBUG_EAGERRELIABLEBROADCAST);

	if (add_to_delivered(id, real_sender) == -1)
		return -1;

	// Copying the message, replacing EAGER RELIABLE BROADCAST callback with the sender Id, this way the subsystem callback
	// can do side effect on the message without interfering with the caller message buffer
	int* new_message = (int*) malloc(sizeof(int) * message_size);
	copy_to_array(new_message, &message[3], message_size);

	if (callback[handle] != NULL )
		callback[handle](new_message, message_size, real_sender);

	_best_effort_broadcast_send(message, count, best_effort_broadcast_handle);

	free(new_message);
	free(data);

	return 0;
}

// COMMODITY AND UTILITY FUNCTIONS

/*
 * This function is registered as callback to the Best Effort Broadcast subsystem. It is called when
 * any message is delivered by the Best Effort Broadcast layer for the Eager Reliable Broadcast layer.
 * It posts a new event with the _eager_reliable_broadcast_beb_deliver function as callback.
 */
int eager_reliable_broadcast_beb_deliver_callback(int *data, int count, int sender) {
	int * buffer = (int*) malloc(sizeof(int) * count);
	copy_to_array(buffer, data, count);
	_add_event(buffer, count, _eager_reliable_broadcast_beb_deliver);
	return 0;
}

/*
 * Utility function to register a callback and getting the related handle
 */
int _eager_reliable_broadcast_register_callback(int (*callback_func)(int*, int, int), int* handle) {
	int i = 0;
	for (i = 0; i < MAX_CALLBACK_NUM; i++) {
		if (callback[i] == eager_reliable_broadcast_dummy_callback) {
			callback[i] = callback_func;
			*handle = i;
			callback_num++;
			sprintf(log_buffer, "EAGER RELIABLE BROADCAST INFO: Callback %d registered", i);
			runtime_log(DEBUG_EAGERRELIABLEBROADCAST);
			return 0;
		}
	}
	sprintf(log_buffer, "EAGER RELIABLE BROADCAST ERROR: Cannot register callback, no slot available");
	runtime_log(ERROR_EAGERRELIABLEBROADCAST);
	return -1;
}

/*
 * Utility function to unregister a callback given it's handle
 */
int _eager_reliable_broadcast_unregister_callback(int handle) {
	if ((callback[handle] == eager_reliable_broadcast_dummy_callback) || (handle >= callback_num)) {
		sprintf(log_buffer,
				"EAGER RELIABLE BROADCAST ERROR: Cannot unregister callback, handle does not correspond to any registered callback");
		runtime_log(ERROR_EAGERRELIABLEBROADCAST);
		return -1;
	}

	callback[handle] = eager_reliable_broadcast_dummy_callback;
	sprintf(log_buffer, "EAGER RELIABLE BROADCAST INFO: Callback %d unregistered", handle);
	runtime_log(DEBUG_EAGERRELIABLEBROADCAST);
	return 0;
}

/*
 * Place holder for the callback vector, should not be invoked
 */
int eager_reliable_broadcast_dummy_callback(int* message, int count, int sender) {
	sprintf(log_buffer, "EAGER RELIABLE BROADCAST ERROR: Referenced callback is not registered");
	runtime_log(ERROR_EAGERRELIABLEBROADCAST);
	return -1;
}

int add_to_delivered(unsigned int id, int real_sender) {
	// Delivered (removed ids)
	if (isAllowedSerialNumber(proc_data_set[real_sender]->gc_threshold, id) == -1) {
		return -1;
	}

	delivered * father = proc_data_set[real_sender]->delivered_set;

	// Skips messages with id < thresold, until the list ends or the message with id = 0 is found
	while (father->next != NULL && isLowerSerialNumber(proc_data_set[real_sender]->gc_threshold, father->next->id) == 0) {
		father->next = father->next->next;
	}

	// Removes oldest contiguous messages from delivered set
	while (father->next != NULL && isNextSerialNumber(proc_data_set[real_sender]->gc_threshold, father->next->id) == 0) {
		delivered * d = father->next;

		gettimeofday(&gettimeofdayArg, NULL );
		incrementNextSerialNumber(proc_data_set[real_sender]->gc_threshold, father->next->id, gettimeofdayArg.tv_sec);

		father->next = father->next->next;
		free(d);
	}

	if (isAllowedSerialNumber(proc_data_set[real_sender]->gc_threshold, id) == -1) {
		return -1;
	}

	father = proc_data_set[real_sender]->delivered_set;

	// Reading all the set looking for greatest delivered id lower than received id
	while (father->next != NULL ) {
		if (father->next->id > id) {
			// father is the greatest delivered message lower than the received id
			delivered * d = (delivered *) malloc(sizeof(delivered));
			d->id = id;
			d->next = father->next;
			father->next = d;
			return 0;
		} else if (father->next->id == id) {
			// Message is already in the delivered set
			return -1;
		}
		father = father->next;

	}
	// Current id is the highest id in the delivered set
	delivered * d = (delivered *) malloc(sizeof(delivered));
	d->id = id;
	d->next = NULL;
	father->next = d;

	return 0;
}
