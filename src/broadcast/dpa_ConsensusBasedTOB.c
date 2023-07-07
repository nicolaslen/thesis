/*
 ============================================================================
 Name        : dpa_ConsensusBasedTOB.c
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
#include "dpa_EagerReliableBroadCast.h"
#include "dpa_FloodingConsensus.h"
#include "dpa_ConsensusBasedTOB.h"

// Logger Defines
#define DEBUG_TOB DEBUG_LOG_LEVEL
#define INFO_TOB INFO_LOG_LEVEL
#define ERROR_TOB ERROR_LOG_LEVEL

// Limits Defines
#define MAX_CALLBACK_NUM 10

// Abstraction Logic Defines
#define SERIAL_NUMBER_BITS 22
#define SERIAL_NUMBER_MASK 4194303

// Application Layer Interaction Data
static struct {
	long type;
	int message;
} tob_send;

static struct {
	long type;
	struct {
		int sender;
		int message;
	} data;
} tob_deliver;

static int runtime_request_handle = -1;
static int application_layer_handle = -1;
static int application_layer_message_id = -1;

// Best Effort Broadcast Interaction Data
static int eager_reliable_broadcast_handle = -1;

// Flooding Consensus Interaction Data
static int flooding_consensus_handle = -1;

// Initialization Flag
static int initialized = 0;

// Callback Data
static int (**callback)(int*, int, int);
static int callback_num = 0;

// Abstraction Logic Data

struct unordered_struct {
	int rank;
	int callback_id;
	int id;
	int size;
	int *message;
	struct unordered_struct *next;
};

typedef struct unordered_struct unordered;
static unordered *unordered_set;
static int unordered_size = 0;

struct delivered_struct {
	int id;
	struct delivered_struct *next;
};

typedef struct delivered_struct delivered;

typedef struct {
	serialNumber *gc_threshold;
	delivered *delivered_set;
} proc_data;

static struct timeval gettimeofdayArg;

static proc_data ** proc_data_set;

static unsigned int serial_number = 0;

struct consensus_struct {
	int round;
	int* decided_set;
	int decided_set_size;
	struct consensus_struct * next;
};

typedef struct consensus_struct consensus;
static consensus * consensus_set;

static int current_consensus_round = 0;
static int next_deciding_consensus_round = 0;

static int wait = 0;

// Application Layer Interaction
int DPA_consensus_based_tob_send(int data);
int DPA_consensus_based_tob_deliver(int* data, int *sender);

// Application Layer Internal Management
int _application_layer_consensus_based_tob_init();
int _consensus_based_tob_request_callback();
int _consensus_based_tob_notify();
int _consensus_based_tob_deliver_to_application_callback(int* message, int count, int sender);

// Abstraction Logic Implementation
int _consensus_based_tob_init(unsigned int flags);
int _consensus_based_tob_cleanup();
int _consensus_based_tob_send(int *message, int count, int handle);
int _consensus_based_tob_new_consensus_istance();
int _consensus_based_tob_rb_deliver(int *message, int count);

// Commodity and Utility Functions
int consensus_based_tob_rb_deliver_callback(int *data, int count, int sender);
int consensus_based_tob_decided_callback(int *data, int count, int istance);
int _consensus_based_tob_register_callback(int (*callback_func)(int*, int, int), int* handle);
int _consensus_based_tob_unregister_callback(int handle);
static int consensus_based_tob_dummy_callback(int* message, int count, int sender);
static int is_in_delivered(int id, int real_sender);
static int add_to_delivered(int id, int real_sender);
static int add_to_unordered(int id, int real_sender, int *message, int size, int handle);
static int remove_from_unordered(int id, int real_sender, int **message, int * size, int *handle);
static int add_to_consensus_set(int round, int* set, int size);
static int perform_deliver(int *set, int size);

// APPLICATION LAYER INTERACTION

int DPA_consensus_based_tob_send(int data) {
	if (application_layer_message_id == -1)
		return -1;
	tob_send.type = application_layer_message_id;
	tob_send.message = data;
	_send_to_runtime(&tob_send, sizeof(tob_send.message));
	return 0;
}

int DPA_consensus_based_tob_deliver(int* data, int *sender) {
	if (application_layer_message_id == -1)
		return -1;

	if (_receive_from_runtime(&tob_deliver, sizeof(tob_deliver.data), application_layer_message_id) == -1)
		return -1;

	*data = tob_deliver.data.message;
	*sender = tob_deliver.data.sender;
	return 0;
}

// APPLICATION LAYER INTERNAL MANAGEMENT

int _application_layer_consensus_based_tob_init() {
	_receive_id_from_runtime(&application_layer_message_id);
	printf("App Layer Init with message id: %d\n", application_layer_message_id);
	return 0;
}

int _consensus_based_tob_request_callback() {
	if (application_layer_handle == -1)
		return -1;
	if (_receive_from_application(&tob_send, sizeof(tob_send.message), application_layer_message_id) == -1)
		return -1;
	_consensus_based_tob_send(&tob_send.message, 1, application_layer_handle);
	return 0;
}

int _consensus_based_tob_notify(int* data, int size) {
	if (application_layer_message_id == -1)
		return -1;
	tob_deliver.type = application_layer_message_id;
	tob_deliver.data.sender = ((int*) data)[0];
	tob_deliver.data.message = ((int*) data)[1];
	_sent_to_application(&tob_deliver, sizeof(tob_deliver.data));
	return 0;
}

int _consensus_based_tob_deliver_to_application_callback(int* message, int count, int sender) {
	if (application_layer_message_id == -1)
		return -1;
	int *buffer = (int*) malloc(sizeof(int) * 2);
	buffer[0] = sender;
	buffer[1] = message[0];
	_add_event(buffer, 2, _consensus_based_tob_notify);
	return 0;
}

// ABSTRACTION LOGIC IMPLEMENTATION

/*
 * Initialization function
 * Allocates the callback and unordered datastructures
 */
int _consensus_based_tob_init(unsigned int flags) {
	if (initialized)
		return -1;

	sprintf(log_buffer, "CONSENSUS BASED TOB INFO: Initializing Total Order Broadcast");
	runtime_log(INFO_TOB);

	_eager_reliable_broadcast_init(flags);
	_flooding_consensus_init(flags);

	callback_num = 0;

	callback = (int (**)(int*, int, int)) malloc(sizeof(int (*)(int*, int, int)) * MAX_CALLBACK_NUM);

	if (callback == NULL ) {
		sprintf(log_buffer, "CONSENSUS BASED TOB ERROR:Cannot allocate callback vector");
		runtime_log(ERROR_TOB);
	}

	int i = 0;
	for (i = 0; i < MAX_CALLBACK_NUM; i++)
		callback[i] = consensus_based_tob_dummy_callback;

	proc_data_set = (proc_data**) malloc(sizeof(proc_data*) * num_procs);
	for (i = 0; i < num_procs; i++) {
		proc_data_set[i] = (proc_data*) malloc(sizeof(proc_data));
		proc_data_set[i]->gc_threshold = (serialNumber*) malloc(sizeof(serialNumber));
		initializeSerialNumber(proc_data_set[i]->gc_threshold);
		proc_data_set[i]->delivered_set = (delivered*) malloc(sizeof(delivered));
		proc_data_set[i]->delivered_set->id = -1;
		proc_data_set[i]->delivered_set->next = NULL;
	}

	serial_number = 0;

	unordered_set = (unordered *) malloc(sizeof(unordered));
	unordered_set->id = -1;
	unordered_set->callback_id = -1;
	unordered_set->size = 0;
	unordered_set->message = NULL;
	unordered_set->next = NULL;

	consensus_set = (consensus *) malloc(sizeof(consensus));
	consensus_set->round = -1;
	consensus_set->decided_set_size = 0;
	consensus_set->decided_set = NULL;
	consensus_set->next = NULL;

	current_consensus_round = 0;
	next_deciding_consensus_round = 0;

	if (flags & USING_CONSENSUS_BASED_TOTAL_ORDER_BROADCAST) {
		_consensus_based_tob_register_callback(_consensus_based_tob_deliver_to_application_callback,
				&application_layer_handle);
		_runtime_register_request_callback(_consensus_based_tob_request_callback, &runtime_request_handle);

		_get_application_layer_message_id(&application_layer_message_id);
		_send_id_to_application(application_layer_message_id);
	}
	_eager_reliable_broadcast_register_callback(consensus_based_tob_rb_deliver_callback,
			&eager_reliable_broadcast_handle);
	_flooding_consensus_register_callback(consensus_based_tob_decided_callback, &flooding_consensus_handle);

	initialized = 1;
	return 0;
}

/*
 * Cleanup function
 * Deallocates the callback datastructure
 */
//TODO
int _consensus_based_tob_cleanup() {
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
int _consensus_based_tob_send(int *message, int count, int handle) {
	//printf("CONSENSUS BASED TOB INFO: Rank %d registering message \"%d\" with handle %d\n", my_rank, message[0], handle);
	sprintf(log_buffer, "CONSENSUS BASED TOB INFO: Registering message \"%d\" with handle %d", message[0], handle);
	runtime_log(DEBUG_TOB);

	// Copying the message, adding CONSENSUS BASED TOB callback.
	int * buffer = (int*) malloc(sizeof(int) * (count + 3));
	buffer[0] = handle;
	buffer[1] = serial_number++;
	buffer[2] = my_rank;
	copy_to_array(&buffer[3], message, count);
	_eager_reliable_broadcast_send(buffer, count + 3, eager_reliable_broadcast_handle);
	free(buffer);
	return 0;
}

/*
 * upon event <bebDeliver | p, [Data, s, m]>
 * This function is posted as the event callback by the callback function registered to the Best Effort
 * Broadcast subsystem. It is called when a  message has been unordered by the BEB layer.
 * It checks if the message has already been unordered. If it has the message is ignored. If it has not
 * the message id is added to the unordered set of the real sender and the message is forwarded
 * to the other processes throuhg a Best Effort Broadcast
 */

int _consensus_based_tob_rb_deliver(int *data, int count) {

	int* message = (int*) data;

	if (count < 3) {
		sprintf(log_buffer, "CONSENSUS BASED TOB ERROR: Message bad format");
		runtime_log(ERROR_TOB);
		return -1;
	}

	int handle = message[0];
	int id = message[1];
	int real_sender = message[2];
	int message_size = count - 3;
	if (handle < 0 || handle > callback_num) {
		sprintf(log_buffer, "CONSENSUS BASED TOB ERROR: Handle out of range: %d", handle);
		runtime_log(ERROR_TOB);
		return -1;
	}

	if (real_sender < 0 || real_sender >= num_procs) {
		sprintf(log_buffer, "CONSENSUS BASED TOB ERROR: Message sent by unknown sender.");
		runtime_log(ERROR_TOB);
		return -1;
	}

	//printf("CONSENSUS BASED TOB INFO: Rank %d received message \"%d\" of size %d with handle %d from process %d\n", my_rank, message[1], count, handle, real_sender);
	sprintf(log_buffer, "CONSENSUS BASED TOB INFO: Received message \"%d\" of size %d with handle %d from process %d",
			message[1], count, handle, real_sender);
	runtime_log(DEBUG_TOB);

	print_data("CONSENSUS BASED TOB Received Message", "value", data, count, DEBUG_LOG_LEVEL);

	if (is_in_delivered(id, real_sender) == -1)
		return -1;

	add_to_unordered(id, real_sender, &message[3], message_size, handle);

	fflush(stdout);
	free(data);

	_consensus_based_tob_new_consensus_istance();

	return 0;
}

/*
 * upon event |unordered| > 0 && Wait = False
 */
int _consensus_based_tob_new_consensus_istance() {
	fflush(stdout);
	if (wait || unordered_size < 1)
		return 0;
	wait = 1;

	int * set = (int*) malloc(sizeof(int) * unordered_size);
	int i = 0;
	unordered * father = unordered_set;
	while (father->next != NULL ) {
		unordered * current = father->next;
		int sn = current->id & SERIAL_NUMBER_MASK;
		int rank = current->rank << SERIAL_NUMBER_BITS;
		set[i] = rank + sn;
		i++;
		father = father->next;
	}

	_flooding_consensus_propose(set, unordered_size, current_consensus_round, flooding_consensus_handle);
	current_consensus_round++;
	return 0;
}

/*
 * upon event <cDecide |decided>
 */
int _consensus_based_tob_fc_decided(int *data, int size) {
	int res = 0;
	int consensus_round = ((int*) data)[0];
	int set_size = size - 1;
	int *set = &((int*) data)[1];

	sort(set, set_size);

	//TODO Work in Progress

	if (consensus_round == next_deciding_consensus_round) {
		perform_deliver(set, set_size);
		next_deciding_consensus_round++;

		consensus * father = consensus_set;
		while (father->next != NULL && father->next->round == next_deciding_consensus_round) {
			if (perform_deliver(father->next->decided_set, father->next->decided_set_size) == -1)
				res = -1;
			consensus * curr = father->next;
			father->next = father->next->next;
			free(curr->decided_set);
			free(curr);
			next_deciding_consensus_round++;
		}
	} else {
		int * new_set = (int*) malloc(sizeof(int) * set_size);
		copy_to_array(new_set, set, set_size);
		add_to_consensus_set(consensus_round, new_set, set_size);
	}
	free(data);

	wait = 0;
	if (_consensus_based_tob_new_consensus_istance() == -1)
		res = -1;

	return res;
}

// COMMODITY AND UTILITY FUNCTIONS

int consensus_based_tob_fc_decided_callback(int *data, int count, int id) {
	int * buffer = (int*) malloc(sizeof(int) * (count + 1));
	buffer[0] = id;
	copy_to_array(&buffer[1], data, count);
	_add_event(buffer, count + 1, _consensus_based_tob_fc_decided);
	return 0;
}

/*
 * This function is registered as callback to the Eager Realiable Broadcast subsystem. It is called when
 * any message is delivered by the Eager Reliable Broadcast  layer for the Total Order Broadcast layer.
 * It posts a new event with the _consensus_based_tob_tob_deliver function as callback.
 */
int consensus_based_tob_rb_deliver_callback(int *data, int count, int sender) {
	int * buffer = (int*) malloc(sizeof(int) * count);
	copy_to_array(buffer, data, count);
	_add_event(buffer, count, _consensus_based_tob_rb_deliver);
	return 0;
}

int consensus_based_tob_decided_callback(int *data, int count, int istance) {
	int * buffer = (int*) malloc(sizeof(int) * (count + 1));
	buffer[0] = istance;
	copy_to_array(&buffer[1], data, count);
	_add_event(buffer, count + 1, _consensus_based_tob_fc_decided);
	return 0;
}

/*
 * Utility function to register a callback and getting the related handle
 */
int _consensus_based_tob_register_callback(int (*callback_func)(int*, int, int), int* handle) {
	int i = 0;
	for (i = 0; i < MAX_CALLBACK_NUM; i++) {
		if (callback[i] == consensus_based_tob_dummy_callback) {
			callback[i] = callback_func;
			*handle = i;
			callback_num++;
			sprintf(log_buffer, "CONSENSUS BASED TOB INFO: Callback %d registered", i);
			runtime_log(DEBUG_TOB);
			return 0;
		}
	}
	sprintf(log_buffer, "CONSENSUS BASED TOB ERROR: Cannot register callback, no slot available");
	runtime_log(ERROR_TOB);
	return -1;
}

/*
 * Utility function to unregister a callback given it's handle
 */
int _consensus_based_tob_unregister_callback(int handle) {
	if ((callback[handle] == consensus_based_tob_dummy_callback) || (handle >= callback_num)) {
		sprintf(log_buffer,
				"CONSENSUS BASED TOB ERROR: Cannot unregister callback, handle does not correspond to any registered callback");
		runtime_log(ERROR_TOB);
		return -1;
	}

	callback[handle] = consensus_based_tob_dummy_callback;
	sprintf(log_buffer, "CONSENSUS BASED TOB INFO: Callback %d unregistered", handle);
	runtime_log(DEBUG_TOB);
	return 0;
}

/*
 * Place holder for the callback vector, should not be invoked
 */
int consensus_based_tob_dummy_callback(int* message, int count, int sender) {
	sprintf(log_buffer, "CONSENSUS BASED TOB ERROR: Referenced callback is not registered");
	runtime_log(ERROR_TOB);
	return -1;
}

int is_in_delivered(int id, int real_sender) {
// Delivered (removed ids)
	if (isAllowedSerialNumber(proc_data_set[real_sender]->gc_threshold, id) == -1) {
		return -1;
	}
	delivered * father = proc_data_set[real_sender]->delivered_set;

// Reading all the set looking for a node with the same id
	while (father->next != NULL ) {
		if (father->next->id == id) {
			return -1;
		}
		father = father->next;
	}
// Message is not in the delivered
	return 0;
}

int add_to_delivered(int id, int real_sender) {
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

int add_to_unordered(int id, int real_sender, int *message, int size, int handle) {
	unordered * d = (unordered *) malloc(sizeof(unordered));
	d->rank = real_sender;
	d->callback_id = handle;
	d->id = id;
	d->size = size;
	d->message = (int*) malloc(sizeof(int) * size);
	copy_to_array(d->message, message, size);
	d->next = unordered_set->next;
	unordered_set->next = d;
// Increasing global unordered size
	unordered_size++;
	return 0;
}

int remove_from_unordered(int id, int real_sender, int ** message, int * size, int * handle) {
	unordered * father = unordered_set;

// Reading all the set looking for the message
	while (father->next != NULL ) {
		if (father->next->rank == real_sender && father->next->id == id) {
			// Removing found message
			unordered * u = father->next;
			father->next = father->next->next;
			*message = u->message;
			*size = u->size;
			*handle = u->callback_id;
			free(u);

			// Decreasing global unordered size
			unordered_size--;
			return 0;
		}
		father = father->next;
	}

	return -1;
}

int add_to_consensus_set(int round, int* set, int size) {

	consensus * father = consensus_set;

	// Reading all the set looking for greatest consensus id lower than received id
	while (father->next != NULL ) {
		if (father->next->round > round) {
			// father is the greatest consensus round lower than the delivered round
			consensus * c = (consensus *) malloc(sizeof(consensus));
			c->round = round;
			c->decided_set = set;
			c->decided_set_size = size;
			c->next = father->next;
			father->next = c;
			return 0;
		} else if (father->next->round == round) {
			// ERROR, this round was already in the set
			return -1;
		}
		father = father->next;
	}
	// Current round is the highest round in the consensus set
	consensus * c = (consensus *) malloc(sizeof(consensus));
	c->round = round;
	c->decided_set = set;
	c->decided_set_size = size;
	c->next = father->next;
	father->next = c;
	return 0;
}

int perform_deliver(int *set, int size) {
	int message_id = -1;
	int sender = -1;
	int **message = (int**) malloc(sizeof(int*));
	int message_size = -1;
	int handle = -1;
	int i = 0;
	int res = 0;
	for (i = 0; i < size; i++) {
		message_id = set[i] & SERIAL_NUMBER_MASK;
		sender = set[i] >> SERIAL_NUMBER_BITS;
		remove_from_unordered(message_id, sender, message, &message_size, &handle);
		add_to_delivered(message_id, sender);
		if (handle > -1 && handle < callback_num)
			if (callback[handle](*message, message_size, sender) == -1)
				res = -1;
	}
	return res;
}
