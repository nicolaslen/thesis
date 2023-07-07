/*
 * dpa_FloodingConsensus.h
 *
 *  Created on: Aug 18, 2012
 *      Author: Nicolò Rivetti
 */

#ifndef CONSENSUS_H_
#define CONSENSUS_H_

// Application Layer Interaction
int DPA_flooding_consensus_propose(int *data, int size, int id);
int DPA_flooding_consensus_decided(int** data, int *size, int *id) ;

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

#endif /* CONSENSUS_H_ */
