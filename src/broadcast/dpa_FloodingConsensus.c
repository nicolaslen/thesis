/*
 * dpa_FloodingConsensus.c
 *
 *  Created on: Aug 19, 2012
 *      Author: Nicolò Rivetti
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dpa_Abstraction.h"
#include "dpa_Common.h"
#include "dpa_PerfectFailureDetector.h"
#include "dpa_BEBroadcast.h"
#include "dpa_FloodingConsensus.h"

// Logger Defines
#define DEBUG_FLOODINGCONSENSUS -1
#define INFO_FLOODINGCONSENSUS INFO_LOG_LEVEL
#define ERROR_FLOODINGCONSENSUS ERROR_LOG_LEVEL

// Limits Defines
#define MAX_CALLBACK_NUM 10

// Abstraction Logic Defines
#define PROPOSAL 100
#define DECIDED 101

#define ROUND 1
#define CALLBACK 2
#define PROPOSALS 4
#define DECISION 8
#define RECEIVEDFROM 16

#define MIN_DELAY 10

// Application Layer Interaction Data
static struct {
	long type;
	struct {
		int id;
		int size;
	} data;
} propose, decided;

static struct {
	long type;
	int value;
} propose_value, decided_value;

static int runtime_request_handle = -1;
static int application_layer_handle = -1;
static int application_layer_message_id = -1;

// Runtime Interaction Data

// Failure Detector Interaction Data
static int perfect_failure_detector_crash_handle = -1;

// Best Effort Broadcast Interaction Data
static int best_effort_broadcast_deliver_handle = -1;

// Initialization Flag
static int initialized = 0;

// Callback Data
static int (**callback)(int*, int, int);
static int callback_num = 0;

// Abstraction Logic Data
struct proposal_struct {
	int * proposalSet;
	int proposalSetSize;
};

typedef struct proposal_struct proposal;

struct instance_struct {
	int instance_id;
	int callback;
	int upper_layer_id;
	int round;
	proposal **proposals;
	proposal *decision;
	unsigned long decidedTs;
	int **receivedFrom;
	struct instance_struct *next;
};

typedef struct instance_struct instance;
static instance* head;

static int current_instance_id = 0;

static struct timeval gettimeofdayArg;

/* Structure of Proposal message array
 * message[0] = callback id
 * message[1] = upper_layer_id
 * message[2] = Proposal tag
 * message[3] = instance id
 * message[4] = round
 * message[5] = start of proposal set
 */

/* Structure of Proposal data array
 * data[0] = sender
 * data[1] = instance id
 * data[2] = round
 * data[3] = start of proposal set
 */

/* Structure of Decided message array
 * message[0] = callback id
 * message[1] = upper_layer_id
 * message[2] = Decided tag
 * message[3] = instance id
 * message[4] = start of proposal set
 */

/* Structure of Decided data array
 * data[0] = sender
 * data[1] = instance id
 * data[2] = start of proposal set
 */

// Application Layer Interaction
int DPA_flooding_consensus_propose(int *data, int size, int id);
int DPA_flooding_consensus_decided(int** data, int *size, int *id);

// Application Layer Internal Management
int _application_layer_flooding_consensus_init();
int _flooding_consensus_request_callback();
int _flooding_consensus_notify(int* data, int size);
int _flooding_consensus_decided_to_application_callback(int* data, int size, int id);

// Abstraction Logic Implementation
int _flooding_consensus_init(unsigned int flags);
int _flooding_consensus_cleanup();
int _flooding_consensus_crash_callback(int* ranks, int num);
int _flooding_consensus_propose(int* proposedSet, int proposedSetSize, int instance_id, int handle);
int _flooding_consensus_delivered_proposal(int *data, int count);
int _flooding_conensus_received_from_all(int *data, int size);
int _flooding_consensus_delivered_decided(int *data, int count);

// Commodity and Utility Functions
int _flooding_consensus_beb_deliver_callback(int *data, int count, int sender);
int _flooding_consensus_register_callback(int (*callback_func)(int*, int, int), int* handle);
int _flooding_consensus_unregister_callback(int handle);
int flooding_consensus_dummy_callback(int * data, int size, int id);
int init_new_instance(int id, int callback_id, int upper_layer_id);
static instance * getinstance(int id);

// APPLICATION LAYER INTERACTION
int DPA_flooding_consensus_propose(int *data, int size, int id) {
	if (application_layer_message_id == -1)
		return -1;

	propose.type = application_layer_message_id;
	propose.data.size = size;
	propose.data.id = id;
	_send_to_runtime(&propose, sizeof(propose.data));
	int i = 0;
	for (i = 0; i < size; i++) {
		propose_value.type = application_layer_message_id;
		propose_value.value = data[i];
		_send_to_runtime(&propose_value, sizeof(propose_value.value));
	}
	return 0;
}

int DPA_flooding_consensus_decided(int** data, int *size, int *id) {
	if (application_layer_message_id == -1)
		return -1;

	if (_receive_from_runtime(&decided, sizeof(decided.data), application_layer_message_id) == -1)
		return -1;

	*size = decided.data.size;
	*id = decided.data.id;
	*data = (int*) malloc(sizeof(int) * (*size));
	int i = 0;
	for (i = 0; i < *size; i++) {
		if (_receive_from_runtime_blck(&decided_value, sizeof(decided_value.value), application_layer_message_id) == -1)
			return -1;
		(*data)[i] = decided_value.value;
	}
	return 0;
}

// APPLICATION LAYER INTERNAL MANAGEMENT

int _application_layer_flooding_consensus_init() {
	_receive_id_from_runtime(&application_layer_message_id);
	return 0;
}

int _flooding_consensus_request_callback() {
	if (application_layer_handle == -1)
		return -1;
	if (_receive_from_application(&propose, sizeof(propose.data), application_layer_message_id) == -1)
		return -1;

	int size = propose.data.size;
	int id = propose.data.id;
	int *buffer = (int*) malloc(sizeof(int) * size);
	int i = 0;
	for (i = 0; i < size; i++) {
		if (_receive_from_application_blck(&propose_value, sizeof(propose_value.value), application_layer_message_id)
				== -1)
			return -1;
		buffer[i] = propose_value.value;
	}
	_flooding_consensus_propose(buffer, size, id, application_layer_handle);
	//TODO Added
	free(buffer);
	return 0;
}

int _flooding_consensus_notify(int* data, int size) {
	if (application_layer_message_id == -1)
		return -1;

	decided.type = application_layer_message_id;
	decided.data.size = size - 1;
	decided.data.id = ((int*) data)[0];
	_sent_to_application(&decided, sizeof(decided.data));
	int i = 0;
	for (i = 1; i < size; i++) {
		decided_value.type = application_layer_message_id;
		decided_value.value = ((int*) data)[i];
		if (_sent_to_application(&decided_value, sizeof(decided_value.value)) == -1)
			printf("ERROR\n");

	}
	fflush(stdout);
	return 0;
}

int _flooding_consensus_decided_to_application_callback(int* data, int size, int id) {
	if (application_layer_message_id == -1)
		return -1;

	int *buffer = (int*) malloc(sizeof(int) * (size + 1));
	buffer[0] = id;
	copy_to_array(&buffer[1], data, size);
	_add_event((void*) buffer, size + 1, _flooding_consensus_notify);
	fflush(stdout);
	return 0;
}

// ABSTRACTION LOGIC IMPLEMENTATION

/*
 * Inizialization function
 * Allocates the instance data structure head, registers Best Effort Broadcast Broadcast callback
 * and Perfect Failure Detector callback
 */
int _flooding_consensus_init(unsigned int flags) {
	if (initialized)
		return 0;

	int i = 0;

	sprintf(log_buffer, "CONSENSUS INFO: Initializing flooding consensus");
	runtime_log(INFO_FLOODINGCONSENSUS);

	// Initializing required Abstractions
	if (_perfect_failure_detector_init(flags) == -1) {
		sprintf(log_buffer, "CONSENSUS ERROR:Cannot Initialize Perfect Failure Detector.");
		runtime_log(ERROR_FLOODINGCONSENSUS);
		return -1;
	}

	if (_best_effort_broadcast_init(flags) == -1) {
		sprintf(log_buffer, "CONSENSUS ERROR:Cannot Initialize Best Effort Broadcast.");
		runtime_log(ERROR_FLOODINGCONSENSUS);
		return -1;
	}

	// Allocating istance queue head
	head = (instance*) malloc(sizeof(instance));
	if (head == NULL ) {
		sprintf(log_buffer, "CONSENSUS ERROR:Cannot allocate instance queue head.");
		runtime_log(ERROR_FLOODINGCONSENSUS);
	}

	// Initializing istance queue head
	head->instance_id = -1;
	head->callback = -1;
	head->upper_layer_id = -1;
	head->round = 0;
	head->decision = NULL;
	head->proposals = NULL;
	head->decidedTs = 0;
	head->receivedFrom = NULL;
	head->next = NULL;

	// Allocating callback array
	callback = (int (**)(int*, int, int)) malloc(sizeof(int (*)(int*, int, int)) * MAX_CALLBACK_NUM);
	if (callback == NULL ) {
		sprintf(log_buffer, "CONSENSUS ERROR:Cannot allocate callback vector");
		runtime_log(ERROR_FLOODINGCONSENSUS);
	}

	// Initializing callback array and callback_num
	for (i = 0; i < MAX_CALLBACK_NUM; i++)
		callback[i] = flooding_consensus_dummy_callback;
	callback_num = 0;

	// Setting up, if necessary, the callback and communication with the application layer
	if (flags & USING_FLOODING_CONSENSUS) {
		if (_flooding_consensus_register_callback(_flooding_consensus_decided_to_application_callback,
				&application_layer_handle) == -1)
			return -1;

		if (_runtime_register_request_callback(_flooding_consensus_request_callback, &runtime_request_handle) == -1)
			return -1;

		if (_get_application_layer_message_id(&application_layer_message_id) == -1)
			return -1;
		if (_send_id_to_application(application_layer_message_id) == -1)
			return -1;
	}

	// Registering callback to Best Effort Broadcast
	_best_effort_broadcast_register_callback(_flooding_consensus_beb_deliver_callback,
			&best_effort_broadcast_deliver_handle);

	// Registering callback to Perfect Failure Detector
	_perfect_failure_detector_register_callback(_flooding_consensus_crash_callback,
			&perfect_failure_detector_crash_handle);

	initialized = 1;
	return 0;
}

/*
 * Cleanup Function
 * Deallocates all the instance data structure
 */
int _flooding_consensus_cleanup() {
	//TODO
	return 0;
}

/*
 * upon event <PCrash | p>
 * This function is the callback registered at the Perfect Failure Detector.
 * This function is called when a process is detected.
 * It checks for all existing instances if the "upon event correct in receivedFrom[round]
 * && decision == NULL" event condition verifies. For each instances for which it does, an event
 * is posted with the function _flooding_conensus_received_from_all as callback
 */
int _flooding_consensus_crash_callback(int* ranks, int num) {
	sprintf(log_buffer, "CONSENSUS INFO: Crash Event.");
	runtime_log(DEBUG_FLOODINGCONSENSUS);

	// Foreach instance struct in the instances queue
	instance *parent = head;
	while (parent->next != NULL ) {
		int ** receivedFrom = parent->next->receivedFrom;
		int i = 0;
		int flag = 1;
		if (parent->next->decision == NULL ) {
			for (i = 0; i < num_procs; i++) {
				if (receivedFrom[parent->next->round][i] == 0 && detected[i] == 0) {
					flag = 0;
					break;
				}
			}
			if (flag) {
				int *data = (int*) malloc(sizeof(int));
				data[0] = parent->next->instance_id;
				_add_event(data, 1, _flooding_conensus_received_from_all);
			}
		}
		parent = parent->next;
	}
	return 0;
}

/*
 * upon event <cPropose, set>
 * This function is called when the caller wants to propose a set of values. The value of instance_id
 * is left to the caller which must be sure that all process choose the same for the same instances of
 * Flooding Consensus.
 * This function allocates a message buffer where the proposed values are stored together with
 * the PROPOSAL tag, the round (1) and the instance id. The message is sent through Best Effort Broadcast
 */
int _flooding_consensus_propose(int* proposedSet, int proposedSetSize, int upper_layer_id, int handle) {
	sprintf(log_buffer, "CONSENSUS INFO: Propose set of size %d for instance %d", proposedSetSize, upper_layer_id);
	runtime_log(DEBUG_FLOODINGCONSENSUS);

	print_data("Propose Set:", "value", proposedSet, proposedSetSize, DEBUG_FLOODINGCONSENSUS);

	int size = proposedSetSize;
	int *message = (int*) malloc(sizeof(int) * (proposedSetSize + 5));

	// Copy the proposed values, leaving place for the metadata
	copy_to_array(&message[5], proposedSet, proposedSetSize);
	sort_delete_duplicate(&message[5], &size);
	// If the set of proposed values contained some duplicates, then the message buffer must shrink

	if (size < proposedSetSize) {
		int *tmp = (int*) malloc(sizeof(int) * (size + 5));
		copy_to_array(&tmp[5], &message[5], size);
		free(message);
		message = tmp;
	}

	// Populate the message metadata
	message[0] = handle;
	message[1] = upper_layer_id;
	message[2] = PROPOSAL;
	message[3] = current_instance_id;
	message[4] = 1;

	_best_effort_broadcast_send(message, size + 5, best_effort_broadcast_deliver_handle);
	current_instance_id++;

	free(message);
	return 0;
}

/*
 * upon event <bebDeliver> | p, [Proposal], r, ps]>
 * This function is posted as the event callback by the callback function registered to the Best Effort
 * Broadcast subsystem. It is called when a Proposal message has been delivered by the BEB layer.
 * It retrieves (if it exists) the related instance and merges the proposed set with the new values.
 * It updates the receivedFrom data structure and checks if the "upon event correct in receivedFrom[round]
 * && decision == NULL" is verified, if it is, then the event is posted
 */
int _flooding_consensus_delivered_proposal(int *data, int count) {
	// Rename the data buffer entries
	int data_sender = ((int*) data)[0];
	int data_id = ((int*) data)[1];
	int data_round = ((int*) data)[2];
	int *data_set = &((int*) data)[3];

	sprintf(log_buffer, "CONSENSUS INFO: Delivered Proposal instance %d", data_id);
	runtime_log(DEBUG_FLOODINGCONSENSUS);

	instance * ist = getinstance(data_id);

	if (ist == NULL ) {
		free(data);
		return -1;
	}

	// Retrieves receivedDrom, proposals and decision from the instance struct
	int **receivedFrom = ist->receivedFrom;
	proposal **proposals = ist->proposals;
	proposal *decision = ist->decision;

	// Merges the proposals set with the new values, eliminating duplicates
	int proposed_set_size = count - 3;
	int current_size = proposals[data_round]->proposalSetSize;
	int size = 0;
	int *new_proposalSet = (int*) malloc(sizeof(int) * (current_size + proposed_set_size));

	sprintf(log_buffer, "CONENSUS INFO: Delivered proposal set \"%d\" of size %d for round %d from %d.", data_set[0],
			proposed_set_size, data_round, data_sender);
	runtime_log(DEBUG_FLOODINGCONSENSUS);

	print_data("Delivered Propose Set:", "value", data_set, proposed_set_size, DEBUG_FLOODINGCONSENSUS);

	merge_delete_duplicates(proposals[data_round]->proposalSet, current_size, data_set, proposed_set_size,
			new_proposalSet, &size);

	if (size < (current_size + proposed_set_size)) {
		int *tmp = (int*) malloc(sizeof(int) * size);
		copy_to_array(tmp, new_proposalSet, size);
		free(new_proposalSet);
		new_proposalSet = tmp;
	}

	free(proposals[data_round]->proposalSet);
	proposals[data_round]->proposalSet = new_proposalSet;
	proposals[data_round]->proposalSetSize = size;

	// Updates receivedFrom data structure
	receivedFrom[data_round][data_sender] = 1;
	free(data);

	sprintf(log_buffer, "CONSENSUS INFO: Reiceved from %d", data_sender);
	runtime_log(DEBUG_FLOODINGCONSENSUS);

	// Checks if the "upon event correct in receivedFrom[round] && decision == NULL" is verified
	// and if it its, posts a new event with the related handler function
	int i = 0;
	int flag = 1;
	if (decision == NULL ) {
		for (i = 0; i < num_procs; i++) {
			if (receivedFrom[data_round][i] == 0 && detected[i] == 0) {
				flag = 0;
				break;
			}
		}
		if (flag) {
			int *data = (int*) malloc(sizeof(int));
			data[0] = data_id;
			_add_event(data, 1, _flooding_conensus_received_from_all);
		}
	}

	return 0;
}

/*
 * upon event correct in receivedFrom[round] && decision == NULL
 * This function is posted as the event callback by the function _consensus_beb_deliver_proposal
 * or the _flooding_consensus_crash_callback when the event condition verifies.
 * The function retrieves the instance data, checks if the set of previous proposers matches the current
 * one. If it does, then the decision can be taken, an appropriate event is posted and the decision
 * is notified to the other process through Best Effort Broadcast.
 * If it does not, then a new round is started and the current proposals are sent to the other process
 * through Best Effort Broadcast.
 */
int _flooding_conensus_received_from_all(int *data, int size) {
	sprintf(log_buffer, "CONENSUS INFO: _flooding_conensus_received_from_all_callback");
	runtime_log(DEBUG_FLOODINGCONSENSUS);

	// Retrieves the instance id and the related instance data.
	int id = ((int*) data)[0];
	free(data);

	instance * ist = getinstance(id);

	if (ist == NULL ) {
		return -1;
	}

	proposal *decision = ist->decision;

	// If the decision has already been taken, it returns.
	if (decision != NULL )
		return 0;

	int **receivedFrom = ist->receivedFrom;
	proposal **proposals = ist->proposals;
	int round = ist->round;
	int callback_id = ist->callback;
	int upper_layer_id = ist->upper_layer_id;

	// If not then it checks if the previous set of proposers matches the current one
	int i = 0;
	int flag = 1;
	int *message;

	for (i = 0; i < num_procs; i++) {
		if (receivedFrom[round][i] != receivedFrom[round - 1][i]) {
			flag = 0;
			break;
		}
	}

	if (flag) {
		// If it does, the decision can be taken

		sprintf(log_buffer, "CONSENSUS INFO: DECIDED at round %d, instance %d", round, id);
		runtime_log(DEBUG_FLOODINGCONSENSUS);

		// The current proposal set is copied as decided set
		proposal *p = (proposal *) malloc(sizeof(proposal));
		p->proposalSetSize = proposals[round]->proposalSetSize;
		p->proposalSet = (int*) malloc(sizeof(int) * p->proposalSetSize);
		copy_to_array(p->proposalSet, proposals[round]->proposalSet, p->proposalSetSize);

		ist->decision = p;

		gettimeofday(&gettimeofdayArg, NULL );
		ist->decidedTs = gettimeofdayArg.tv_sec;

		print_data("Decided Set", "value", p->proposalSet, p->proposalSetSize, DEBUG_FLOODINGCONSENSUS);

		size = p->proposalSetSize;

		// The decided set is copied, together with some metadata, to a message buffer

		message = (int*) malloc(sizeof(int) * (size + 4));
		message[0] = callback_id;
		message[1] = upper_layer_id;
		message[2] = DECIDED;
		message[3] = id;

		copy_to_array(&message[4], p->proposalSet, size);

		print_data("FLOODING CONSENSUS Decided message", "value", message, size + 4, DEBUG_FLOODINGCONSENSUS);

		// The decision is broadcasted
		_best_effort_broadcast_send(message, size + 4, best_effort_broadcast_deliver_handle);
		if (callback_id > -1 && callback_id < callback_num)
			callback[callback_id](ist->decision->proposalSet, ist->decision->proposalSetSize, ist->upper_layer_id);
		//TODO: DECIDED EVENT TOWARDS UPPER LAYER

	} else {
		// If not, then the following round is started
		round++;
		ist->round++;

		sprintf(log_buffer, "CONENSUS INFO: Next Round %d, instance %d", round, id);
		runtime_log(DEBUG_FLOODINGCONSENSUS);

		// The current proposal set is sent as proposal for the new round
		size = proposals[round - 1]->proposalSetSize;
		message = (int*) malloc(sizeof(int) * (size + 5));
		message[0] = callback_id;
		message[1] = upper_layer_id;
		message[2] = PROPOSAL;
		message[3] = id;
		message[4] = round;

		copy_to_array(&message[5], proposals[round - 1]->proposalSet, size);

		_best_effort_broadcast_send(message, size + 5, best_effort_broadcast_deliver_handle);

	}
	if (message != NULL )
		free(message);
	return 0;
}

/*
 * upon event <bebDeliver | p, [Decided, set]>
 * This function is posted as the event callback by the callback function registered to the Best Effort
 * Broadcast subsystem. It is called when a Decided message has been delivered by the BEB layer.
 * It retrieves (if it exists) the related instance and checks if a decision has been already taken.
 * If it has the message is simply ignored. If it has not, the decided delivered values become the decided
 * values, and the message is forwarded to the other process through Best Effort Broadcast
 */
//TODO COMMENTS
int _flooding_consensus_delivered_decided(int *data, int count) {
	int data_sender = ((int*) data)[0];
	int data_id = ((int*) data)[1];
	int *data_set = &((int*) data)[2];
	int proposal_set_size = count - 2;

	instance *ist = getinstance(data_id);

	if (ist == NULL ) {
		free(data);
		return -1;
	}

	proposal * decision = ist->decision;

	sprintf(log_buffer, "CONSENSUS INFO: DELIVER DECIDED");
	runtime_log(DEBUG_FLOODINGCONSENSUS);

	if ((decision != NULL )|| (detected[data_sender] ==1)){
	free(data);
	return 0;
}

	int round = ist->round;
	int callback_id = ist->callback;
	int upper_layer_id = ist->upper_layer_id;

	proposal *p = (proposal *) malloc(sizeof(proposal));
	p->proposalSet = (int*) malloc(sizeof(int) * proposal_set_size);
	copy_to_array(p->proposalSet, data_set, proposal_set_size);
	p->proposalSetSize = proposal_set_size;
	ist->decision = p;

	gettimeofday(&gettimeofdayArg, NULL );
	ist->decidedTs = gettimeofdayArg.tv_sec;

	int *message = (int*) malloc(sizeof(int) * (proposal_set_size + 4));
	message[0] = callback_id;
	message[1] = upper_layer_id;
	message[2] = DECIDED;
	message[3] = data_id;
	copy_to_array(&message[4], data_set, proposal_set_size + 4);
	_best_effort_broadcast_send(message, proposal_set_size + 4, best_effort_broadcast_deliver_handle);

	free(message);

	sprintf(log_buffer, "CONESNSUS INFO: DECIDED after delivery of Decided event at round %d, instance %d", round,
			data_id);
	runtime_log(DEBUG_FLOODINGCONSENSUS);

	print_data("FLOODING CONSENSUS Decided Set", "value", ist->decision->proposalSet, ist->decision->proposalSetSize,
			DEBUG_FLOODINGCONSENSUS);

	if (callback_id > -1 && callback_id < callback_num)
		callback[callback_id](ist->decision->proposalSet, ist->decision->proposalSetSize, ist->upper_layer_id);

	free(data);
	return 0;
}

// COMMODITY AND UTILITY FUNCTIONS

/*
 * This function is registered as callback to the Best Effort Broadcast subsystem. It is called when
 * any message is delivered by the Best Effort Broadcast layer for the Flooding Consensus layer.
 * It retrieves the instance id and creates a new instance if necessary. It then post a new event
 * with either the _flooding_consensus_delivered_proposal function as callback, or the
 * _flooding_consensus_delivered_decided as callback, depending on the message flag.
 */
int _flooding_consensus_beb_deliver_callback(int *message, int count, int sender) {
	if (count < 0)
		return -1;
	//TODO: ERROR

	int callback_id = message[0];
	int upper_layer_id = message[1];
	int tag = message[2];
	int id = message[3];
	int * data = &message[4];
	int data_size = count - 4;

	print_data("CONSENSUS Delivered", "value", message, count, DEBUG_FLOODINGCONSENSUS);
	sprintf(log_buffer, "CONSENSUS instance %d", id);
	runtime_log(DEBUG_FLOODINGCONSENSUS);

	init_new_instance(id, callback_id, upper_layer_id);

	int * new_message = (int*) malloc(sizeof(int) * (data_size + 2));
	new_message[0] = sender;
	new_message[1] = id;
	copy_to_array(&new_message[2], data, data_size);

	switch (tag) {
	case PROPOSAL:
		_add_event(new_message, data_size + 2, _flooding_consensus_delivered_proposal);
		break;

	case DECIDED:
		_add_event(new_message, data_size + 2, _flooding_consensus_delivered_decided);
		break;
	default:
		break;
	}

	return 0;
}

/*
 * Utility function to register a callback and getting the related handle
 */
int _flooding_consensus_register_callback(int (*callback_func)(int*, int, int), int* handle) {
	int i = 0;
	for (i = 0; i < MAX_CALLBACK_NUM; i++) {
		if (callback[i] == flooding_consensus_dummy_callback) {
			callback[i] = callback_func;
			*handle = i;
			callback_num++;
			sprintf(log_buffer, "FLOODING CONSENSUS INFO: Callback %d registered", i);
			runtime_log(DEBUG_FLOODINGCONSENSUS);
			return 0;
		}
	}
	sprintf(log_buffer, "FLOODING CONSENSUS ERROR: Cannot register callback, no slot available");
	runtime_log(ERROR_FLOODINGCONSENSUS);
	return -1;
}

/*
 * Utility function to unregister a callback given it's handle
 */
int _flooding_consensus_unregister_callback(int handle) {
	if ((callback[handle] == flooding_consensus_dummy_callback) || (handle >= callback_num)) {
		sprintf(log_buffer,
				"FLOODING CONSENSUS ERROR: Cannot unregister callback, handle does not correspond to any registered callback");
		runtime_log(ERROR_FLOODINGCONSENSUS);
		return -1;
	}

	callback[handle] = flooding_consensus_dummy_callback;
	sprintf(log_buffer, "FLOODING CONSENSUS INFO: Callback %d unregistered", handle);
	runtime_log(DEBUG_FLOODINGCONSENSUS);
	return 0;
}

/*
 * Place holder for the callback vector, should not be invoked
 */
int flooding_consensus_dummy_callback(int * ranks, int num, int id) {
	sprintf(log_buffer, "FLOODING CONSENSUS ERROR: Referenced callback is not registered");
	runtime_log(ERROR_FLOODINGCONSENSUS);
	return -1;
}

/*
 * Utility function that creates an instance with the given id, if it does not exist already.
 * It allocates the instance struct, allocates receivedFrom and proposals data structures and
 * initialize instance_id, round and decision values. Finally it links the new instances to the
 * instances data structure
 */
//TODO COMMENTS
int init_new_instance(int id, int callback_id, int upper_layer_id) {
	if (getinstance(id) != NULL )
		return 0;

	sprintf(log_buffer, "CONSENSUS Initilizing new instance %d", id);
	runtime_log(DEBUG_FLOODINGCONSENSUS);

	instance *new_instance = (instance*) malloc(sizeof(instance));

	new_instance->instance_id = id;
	new_instance->callback = callback_id;
	new_instance->upper_layer_id = upper_layer_id;
	new_instance->round = 1;
	new_instance->decision = NULL;

	new_instance->receivedFrom = (int**) malloc(sizeof(int*) * (num_procs + 1));

	int i = 0;
	int j = 0;

	for (i = 0; i < num_procs + 1; i++) {
		new_instance->receivedFrom[i] = (int*) malloc(sizeof(int) * num_procs);
		for (j = 0; j < num_procs; j++)
			new_instance->receivedFrom[i][j] = 0;
	}

	for (j = 0; j < num_procs; j++)
		if (detected[j] == 0)
			new_instance->receivedFrom[0][j] = 1;

	new_instance->proposals = (proposal **) malloc(sizeof(proposal*) * (num_procs + 1)); //TODO
	for (i = 0; i < num_procs + 1; i++) {
		proposal *p = (proposal *) malloc(sizeof(proposal));
		p->proposalSet = NULL;
		p->proposalSetSize = 0;
		new_instance->proposals[i] = p; //TODO
	}

	new_instance->next = NULL;

	instance *parent = head;
	while (parent->next != NULL )
		parent = parent->next;

	parent->next = new_instance;

	return 0;
}

/*
 * Commodity function to retrieve the instance struct (if it exists) given it's id
 */
instance * getinstance(int id) {
	instance *parent = head;

	gettimeofday(&gettimeofdayArg, NULL );
	unsigned long currentTS = gettimeofdayArg.tv_sec;

	while (parent->next != NULL ) {
		if ((parent->next->decision != NULL )&&(parent->next->decidedTs < currentTS - MIN_DELAY)){
		instance *ist = parent->next;
		free(ist->decision);

		int i = 0;
		if(ist->receivedFrom != NULL) {
			for (i = 0; i < num_procs + 1; i++) {
				free(ist->receivedFrom[i]);
			}
			free(ist->receivedFrom);
		}

		if(ist->proposals != NULL) {
			for (i = 0; i < num_procs + 1; i++) {
				free(ist->proposals[i]);
			}
			free(ist->proposals);
		}

		parent->next = parent->next->next;
		free(ist);
	} else {

		if (parent->next->instance_id == id) {
			return parent->next;
		}
		parent = parent->next;
	}
}
	return NULL ;
}
