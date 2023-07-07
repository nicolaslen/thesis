/*
 * dpa_EventQueue.c
 *
 *  Created on: Aug 19, 2012
 *      Author: Nicolò Rivetti
 */
#include <stdio.h>
#include <stdlib.h>

#include "dpa_Runtime.h"
#include "dpa_EventQueue.h"
#include "dpa_Logger.h"

// Logger Defines
#define DEBUG_EVENTQUEUE DEBUG_LOG_LEVEL
#define INFO_EVENTQUEUE INFO_LOG_LEVEL
#define ERROR_EVENTQUEUE ERROR_LOG_LEVEL

// Event Queue Logic Data
typedef struct event_struct {
	int (*callback)(int * data, int size);
	void * data;
	int size;
	struct event_struct *next;
} event_t;

static event_t *event_head;
static event_t *event_tail;

static int event_num = 0;

// Event Queue Logic Implementation
int _event_queue_init();
int _event_queue_cleanup();
int _add_event(int * data, int size, int (*callback)(int *data, int size));
int _perform_event();
int event_queue_dummy_callback(int * data, int size);

/*
 * Event Queue inizializer, must be invoked before starting the runtime.
 */
int _event_queue_init() {
	sprintf(log_buffer, "RUNTIME INFO: Inizializing event queue.");
	runtime_log(INFO_EVENTQUEUE);

	// Allocating event queue head structure
	event_head = (event_t*) malloc(sizeof(event_t));
	if (event_head == NULL ) {
		sprintf(log_buffer, "RUNTIME ERROR:Cannot allocate event queue head.");
		runtime_log(ERROR_EVENTQUEUE);
		return -1;
	}
	// Setting up event queue tail and dummy callback
	event_tail = event_head;
	event_head->data = NULL;
	event_head->callback = event_queue_dummy_callback;
	event_head->size = 0;
	event_head->next = NULL;

	return 0;
}

int _event_queue_cleanup() {
	event_t * parent = event_head;
	event_t* current = NULL;
	do{
		current = parent->next;
		if(parent->data != NULL)
			free(parent->data);
		free(parent);
		parent = current;
	}while (parent != NULL );

	return 0;
}

/*
 * Adds a new event at the end of the event queue, given the function to be invoked to perform
 * the related operations, and a pointer to the related data.
 */
int _add_event(int * data, int size, int (*callback)(int *data, int size)) {
	sprintf(log_buffer, "RUNTIME INFO: Adding Event");
	runtime_log(DEBUG_EVENTQUEUE);

	if (callback == NULL ) {
		sprintf(log_buffer, "RUNTIME ERROR: Callback function is NULL.");
		runtime_log(ERROR_EVENTQUEUE);
		return -1;
	}

	// Allocating event_struct, pointed data should be handled by caller subsystem
	event_t *event = (event_t*) malloc(sizeof(event_t));
	if (event == NULL ) {
		sprintf(log_buffer, "RUNTIME ERROR:Cannot allocate new event (Events in queue:%d).", event_num);
		runtime_log(ERROR_EVENTQUEUE);
		return -1;
	}

	event->data = data;
	event->size = size;
	event->callback = callback;
	event->next = NULL;

	event_tail->next = event;
	event_tail = event;

	event_num++;

	return 0;
}

/*
 * Perform the first available event in the queue, invoking the related callback function on the
 * given data, and removes the event from the queue.
 */
int _perform_event() {

	// There are no events to perform
	if (event_head->next == NULL ) {
		return 0;
	}

	sprintf(log_buffer, "RUNTIME INFO: Performing Event (Events in queue:%d).", event_num);
	runtime_log(DEBUG_EVENTQUEUE);

	event_t * event = event_head->next;

	// Performing and removing the event
	int ret = event->callback(event->data, event->size);

	// If it's the last event in the queue, event_tail must point to the head
	if (event_tail == event_head->next) {
		event_tail = event_head;
		event_head->next = NULL;
	} else {
		event_head->next = event->next;
	}

	// Deallocating event_struct, pointed data should be handled by callback
	free(event);

	event_num--;

	return ret;
}

/*
 * Dummy handler to avoid invoking a null function and detect the issue.
 */
int event_queue_dummy_callback(int * data, int size) {
	sprintf(log_buffer, "RUNTIME ERROR: Referenced event callback is not registered");
	runtime_log(ERROR_EVENTQUEUE);
	return -1;
}

