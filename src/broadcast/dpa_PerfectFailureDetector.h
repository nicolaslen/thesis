/*
 ============================================================================
 Name        : dpa_PerfectFailureDetector.c
 Author      : Nicolò Rivetti
 Created on  : Aug 10, 2012
 Version     : 1.0
 Copyright   : Copyright (c) 2012 Nicolò Rivetti
 Description : Implementation of a Perfect Failure Detector
 ============================================================================
 */
#ifndef Perfect_Failure_Detector_H_
#define Perfect_Failure_Detector_H_

short * alive; /* bitmap of currently alive process*/
short * detected; /* bitmap of currently crashed process*/
int detected_num; /* number of currently crashed process*/

// Application Layer Interaction
int DPA_perfect_failure_detector_detected(int * rank);

// Application Layer Internal Management
int _application_layer_perfect_failure_detector_init();
int _perfect_failure_detector_notify(int* data, int size);
int _perfect_failure_detector_crash_to_application_callback(int * ranks, int num);

// Abstraction Logic Implementation
int _perfect_failure_detector_init(unsigned int flags);
int _perfect_failure_detector_cleanup();
int _perfect_failure_detector_receive(int *message, int size, int sender);
int _perfect_failure_detector_timeout();

// Commodity and Utility Functions
int _perfect_failure_detector_register_callback(int (*callback_func)(int*, int), int* handle);

#endif /* Perfect_Failure_Detector_H_ */
