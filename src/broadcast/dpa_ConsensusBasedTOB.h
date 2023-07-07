/*
 ============================================================================
 Name        : dpa_ConsensusBasedTOB.h
 Author      : Nicolò Rivetti
 Created on  : Aug 25, 2012
 Version     : 1.0
 Copyright   : Copyright (c) 2012 Nicolò Rivetti
 Description :
 ============================================================================
 */
#ifndef DPA_CONSENSUSBASEDTOB_H_
#define DPA_CONSENSUSBASEDTOB_H_

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

#endif /* DPA_CONSENSUSBASEDTOB_H_ */
