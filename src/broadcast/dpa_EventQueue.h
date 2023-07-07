/*
 * dpa_EventQueue.h
 *
 *  Created on: Aug 20, 2012
 *      Author: Nicolò Rivetti
 */

#ifndef EVENTQUEUE_H_
#define EVENTQUEUE_H_

int _event_queue_init();
int _event_queue_cleanup();

int _add_event(int * data, int size, int (*callback)(int *data, int size));
int _perform_event();

#endif /* EVENTQUEUE_H_ */
