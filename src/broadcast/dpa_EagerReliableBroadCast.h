/*
 ============================================================================
 Name        : dpa_EagerReliableBroadCast.h
 Author      : Nicolò Rivetti
 Created on  : Aug 25, 2012
 Version     : 1.0
 Copyright   : Copyright (c) 2012 Nicolò Rivetti
 Description :
 ============================================================================
 */

#ifndef DPA_EAGERRELIABLEBROADCAST_H_
#define DPA_EAGERRELIABLEBROADCAST_H_

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

int _eager_reliable_broadcast_register_callback(int (*callback_func)(int*, int, int), int* handle);
int _eager_reliable_broadcast_unregister_callback(int handle);

#endif /* DPA_EAGERRELIABLEBROADCAST_H_ */
