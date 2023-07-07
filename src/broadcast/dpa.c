/*
 ============================================================================
 Name        : dpa.c
 Author      : Nicolò Rivetti
 Created on  : Aug 24, 2012
 Version     : 1.0
 Copyright   : Copyright (c) 2012 Nicolò Rivetti
 Description :
 ============================================================================
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <wait.h>

#include "dpa.h"
#include "dpa_Runtime.h"
#include "dpa_ApplicationLayerCommunication.h"

#include "dpa_PerfectFailureDetector.h"
#include "dpa_BEBroadcast.h"
#include "dpa_EagerReliableBroadCast.h"
#include "dpa_FloodingConsensus.h"
#include "dpa_ConsensusBasedTOB.h"

static int pid = -1;

int DPA_Init(int argc, char **argv, unsigned int flags);
int DPA_Close();

int application_layer_subsystems_init(unsigned int flags);
int subsystems_init(unsigned int flags);

int DPA_Init(int argc, char **argv, unsigned int flags) {

	if (_application_layer_communication_init() == -1)
		return -1;

	pid = fork();
	printf("%d: dpa.c - DPA_INIT - fork\n", pid);
	if (pid == -1) {
		return -1;
	}

	if (pid == 0) {
		/* CALL TO RUNTIME INIT */
		_runtime_init(argc, argv, flags);
		subsystems_init(flags);
		_runtime();
		_application_layer_communication_cleanup();
		exit(1);
	} else {
		int childpid = getpid();
	}

	if (_application_layer_runtime_init() == -1)
		return -1;

	if (application_layer_subsystems_init(flags) == -1)
		return -1;

	return 0;
}

int DPA_Get_Process_Number(int * proc_num) {
	return _runtime_send_command(GET_PROC_NUM, proc_num);
}

int DPA_Get_Rank(int * rank) {
	return _runtime_send_command(GET_RANK, rank);
}

int DPA_Close() {
	int answer = -1;
	int status = -1;

	_runtime_send_command(SHUTDOWN, &answer);
	waitpid(pid, &status, 0);
	return 0;
}

int application_layer_subsystems_init(unsigned int flags) {

	if (flags & USING_PERFECT_FAILURE_DETECTOR) {
		_application_layer_perfect_failure_detector_init();
	}
	if (flags & USING_BEST_EFFORT_BROADCAST) {
		_application_layer_best_effort_broadcast_init();
	}
	if (flags & USING_EAGER_RELIABLE_BROADCAST) {
		_application_layer_eager_reliable_broadcast_init();
	}
	if (flags & USING_FLOODING_CONSENSUS) {
		_application_layer_flooding_consensus_init();
	}
	if (flags & USING_CONSENSUS_BASED_TOTAL_ORDER_BROADCAST) {
		_application_layer_consensus_based_tob_init();
	}

	return 0;
}

int subsystems_init(unsigned int flags) {
	if (flags & USING_PERFECT_FAILURE_DETECTOR) {
		_perfect_failure_detector_init(flags);
	}
	if (flags & USING_BEST_EFFORT_BROADCAST) {
		_best_effort_broadcast_init(flags);
	}
	if (flags & USING_EAGER_RELIABLE_BROADCAST) {
		_eager_reliable_broadcast_init(flags);
	}
	if (flags & USING_FLOODING_CONSENSUS) {
		_flooding_consensus_init(flags);
	}
	if (flags & USING_CONSENSUS_BASED_TOTAL_ORDER_BROADCAST) {
		_consensus_based_tob_init(flags);
	}
	return 0;
}
