/*
 ============================================================================
 Name        : dpa_Runtime.h
 Author      : Nicolò Rivetti
 Created on  : Aug 10, 2012
 Version     : 1.0
 Copyright   : Copyright (c) 2012 Nicolò Rivetti
 Description :
 ============================================================================
 */
#ifndef RUNTIME_H_
#define RUNTIME_H_

#include <mpi.h>

#define SHUTDOWN 0
#define GET_PROC_NUM 1
#define GET_RANK 2

//#define BUFFEREDSEND

int num_procs;
int my_rank;
MPI_Comm comm;

// Application Layer Internal Interaction
int _runtime_send_command(int command, int * answer);

// Application Layer Internal Management
int _application_layer_runtime_init();
int _runtime_request_callback();

// Runtime Logic Implementation
int _runtime_init(int argc, char **argv, unsigned int flags);
int _runtime_cleanup();
int _runtime();

// Commodity and Utility Functions
int _runtime_register_cleanup_callback(int (*callback_func)(void), int* handle);
int _runtime_register_receive_callback(int (*callback_func)(int*, int, int), int* handle);
int _runtime_register_request_callback(int (*callback_func)(void), int* handle);
int _runtime_register_timely_callback(int (*callback_func)(void), int* handle);

#endif /* RUNTIME_H_ */
