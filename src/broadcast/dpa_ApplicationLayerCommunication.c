/*
 ============================================================================
 Name        : dpa_ApplicationLayerCommunication.c
 Author      : Nicolò Rivetti
 Created on  : Aug 23, 2012
 Version     : 1.0
 Copyright   : Copyright (c) 2012 Nicolò Rivetti
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#include "dpa_Logger.h"
#include "dpa_ApplicationLayerCommunication.h"

// Logger Defines
#define DEBUG_APPCOMM -1
#define INFO_APPCOMM INFO_LOG_LEVEL
#define ERROR_APPCOMM ERROR_LOG_LEVEL

// Application Layer Communication Defines
#define RUNTIME_MESSAGE_ID 99999

// Application Layer Communication Data
static int queue_id_in;
static int queue_id_out;

static int next_application_layer_message_id = 1;

static struct {
	long type;
	int registeredMessageType;
} msg;

// Application Layer Internal Management
int _application_layer_communication_init();
int _application_layer_communication_cleanup();

// Application Layer Communication Implementation
int _get_application_layer_message_id(int *id);
int _receive_id_from_runtime(int *id);
int _send_id_to_application(int id);
int _receive_from_application(void *buff, int size, int tag);
int _receive_from_application_blck(void *buff, int size, int tag);
int _sent_to_application(void *buff, int size);
int _send_to_runtime(void *buff, int size);
int _receive_from_runtime(void *buff, int size, int tag);
int _receive_from_runtime_blck(void *buff, int size, int tag);

// APPLICATION LAYER INTERNAL MANAGEMENT
int _application_layer_communication_init() {
	int key = IPC_PRIVATE;
	int creation_flags = IPC_CREAT | IPC_EXCL | 0666;

	queue_id_in = msgget(key, creation_flags);
	if (queue_id_in == -1) {
		sprintf(log_buffer, "APPLICATION COMM ERROR: Cannot create in message queue.");
		runtime_log(ERROR_APPCOMM);
		return -1;
	}

	queue_id_out = msgget(key, creation_flags);
	if (queue_id_out == -1) {
		sprintf(log_buffer, "APPLICATION COMM ERROR: Cannot create out message queue.");
		runtime_log(ERROR_APPCOMM);
		return -1;
	}
	return 0;
}

int _application_layer_communication_cleanup() {
	int ret = 0;
	if (msgctl(queue_id_in, IPC_RMID, NULL ) == -1)
		ret = -1;

	if (msgctl(queue_id_out, IPC_RMID, NULL ) == -1)
		ret = -1;

	return ret;
}

// APPLICATION LAYER COMMUNICATION IMPLEMENTATION
int _get_application_layer_message_id(int *id) {
	if (next_application_layer_message_id > 0 && next_application_layer_message_id < RUNTIME_MESSAGE_ID) {
		*id = next_application_layer_message_id;
		next_application_layer_message_id++;
		return 0;
	}
	sprintf(log_buffer, "APPLICATION COMM ERROR: application ids exhausted.");
	runtime_log(ERROR_APPCOMM);
	*id = -1;
	return -1;
}

int _receive_id_from_runtime(int *id) {
	if (_receive_from_runtime_blck(&msg, sizeof(msg.registeredMessageType), RUNTIME_MESSAGE_ID) == -1) {
		sprintf(log_buffer, "APPLICATION COMM ERROR: Error while receiving application id from runtime.");
		runtime_log(ERROR_APPCOMM);
		return -1;
	}
	*id = msg.registeredMessageType;
	return 0;

}

int _send_id_to_application(int id) {
	msg.type = RUNTIME_MESSAGE_ID;
	msg.registeredMessageType = id;
	if (_sent_to_application(&msg, sizeof(msg.registeredMessageType)) == -1) {
		sprintf(log_buffer, "APPLICATION COMM ERROR: Error while sending application id to application layer.");
		runtime_log(ERROR_APPCOMM);
		return -1;
	}
	return 0;
}

int _receive_from_application(void *buff, int size, int tag) {
	if (msgrcv(queue_id_in, buff, size, tag, IPC_NOWAIT) == -1) {
		if (errno != ENOMSG) {
			sprintf(log_buffer, "APPLICATION COMM ERROR: Error while receiving from runtime.");
			runtime_log(ERROR_APPCOMM);
		}
		return -1;
	}
	return 0;
}

int _receive_from_application_blck(void *buff, int size, int tag) {
	if (msgrcv(queue_id_in, buff, size, tag, 0) == -1) {
		sprintf(log_buffer, "APPLICATION COMM ERROR: Error while receiving (blck) from application layer.");
		runtime_log(ERROR_APPCOMM);
		return -1;
	}
	return 0;
}

int _sent_to_application(void *buff, int size) {
	if (msgsnd(queue_id_out, buff, size, IPC_NOWAIT) == -1) {
		sprintf(log_buffer, "APPLICATION COMM ERROR: Error while sending to application layer.");
		runtime_log(ERROR_APPCOMM);
		return -1;
	}
	return 0;
}

int _send_to_runtime(void *buff, int size) {
	if (msgsnd(queue_id_in, buff, size, IPC_NOWAIT) == -1) {
		sprintf(log_buffer, "APPLICATION COMM ERROR: Error while sending to runtime.");
		runtime_log(ERROR_APPCOMM);
		return -1;
	}
	return 0;
}

int _receive_from_runtime(void *buff, int size, int tag) {
	if (msgrcv(queue_id_out, buff, size, tag, IPC_NOWAIT) == -1) {
		if (errno != ENOMSG) {
			sprintf(log_buffer, "APPLICATION COMM ERROR: Error while receiving from runtime.");
			runtime_log(ERROR_APPCOMM);
		}
		return -1;
	}
	return 0;
}

int _receive_from_runtime_blck(void *buff, int size, int tag) {
	if (msgrcv(queue_id_out, buff, size, tag, 0) == -1) {
		sprintf(log_buffer, "APPLICATION COMM ERROR: Error while receiving (blck) from runtime.");
		runtime_log(ERROR_APPCOMM);
		return -1;
	}
	return 0;
}
