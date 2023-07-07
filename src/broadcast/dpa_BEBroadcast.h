/*
 ============================================================================
 Name        : dpa_BEBroadcast.h
 Author      : Nicolò Rivetti
 Created on  : Aug 17, 2012
 Version     : 1.0
 Copyright   : Copyright (c) 2012 Nicolò Rivetti
 Description :
 ============================================================================
 */


#ifndef BEBROADCAST_H_
#define BEBROADCAST_H_

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

#endif /* BEBROADCAST_H_ */
