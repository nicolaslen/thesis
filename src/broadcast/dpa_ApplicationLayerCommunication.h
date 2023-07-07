/*
 ============================================================================
 Name        : dpa_ApplicationLayerCommunication.h
 Author      : Nicolò Rivetti
 Created on  : Aug 23, 2012
 Version     : 1.0
 Copyright   : Copyright (c) 2012 Nicolò Rivetti
 Description :
 ============================================================================
 */


#ifndef FRONTEND_H_
#define FRONTEND_H_

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

#endif /* FRONTEND_H_ */
