/*
 ============================================================================
 Name        : dpa.h
 Author      : Nicolò Rivetti
 Created on  : Aug 24, 2012
 Version     : 1.0
 Copyright   : Copyright (c) 2012 Nicolò Rivetti
 Description :
 ============================================================================
 */

#ifndef DPA_H_
#define DPA_H_


#define USING_PERFECT_FAILURE_DETECTOR 1
#define USING_BEST_EFFORT_BROADCAST 2
#define USING_EAGER_RELIABLE_BROADCAST 8
#define USING_FLOODING_CONSENSUS 16
#define USING_CONSENSUS_BASED_TOTAL_ORDER_BROADCAST 32

int DPA_Init(int argc, char **argv, unsigned int flags);
int DPA_Close();
int DPA_Get_Process_Number(int * proc_num);
int DPA_Get_Rank(int * rank);


#ifndef __DPA_INTERNAL__

// Perfect Failure Detector
int DPA_perfect_failure_detector_detected(int * rank);

// Best Effort Broadcast
int DPA_best_effort_broadcast_send(int data);
int DPA_best_effort_broadcast_deliver(int* data, int *sender);

// Eager Reliable Broadcast
int DPA_eager_reliable_broadcast_send(int data);
int DPA_eager_reliable_broadcast_deliver(int* data, int *sender);

// Flooding Consensus
int DPA_flooding_consensus_propose(int *data, int size, int id);
int DPA_flooding_consensus_decided(int** data, int *size, int *id);

// Consensus Based Total Order Broadcast
int DPA_consensus_based_tob_send(int data);
int DPA_consensus_based_tob_deliver(int* data, int *sender);

#endif

#endif /* DPA_H_ */
