/*
 * CounterTest2.c
 *
 *  Created on: Sep 20, 2012
 *      Author: helrohir
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MIN_DELAY 0 //((unsigned int) 2000)
#define MAX_UINT ((unsigned int ) ~0)
#define QUARTER0 ((unsigned int ) 0)
#define QUARTER1 ((unsigned int ) (MAX_UINT - 1) / 4)
#define QUARTER2 ((unsigned int ) QUARTER1 * 2 + 1)
#define QUARTER3 ((unsigned int ) QUARTER1 * 3 + 2)

#include "dpa_SerialNumbers.h"

static void incrHighTreshold(serialNumber *sn);
static void incrLowTreshold(serialNumber *sn, unsigned long ts);
int initializeSerialNumber(serialNumber *sn);
int isNextSerialNumber(serialNumber *sn, unsigned int id);
int isAllowedSerialNumber(serialNumber *sn, unsigned int id);
int incrementNextSerialNumber(serialNumber *sn, unsigned int id, long ts);
int isLowerSerialNumber(serialNumber *sn, unsigned int id);

int initializeSerialNumber(serialNumber *sn) {
	sn->timestamps[0] = sn->timestamps[1] = sn->timestamps[2] = sn->timestamps[3] = 0;
	sn->max = MAX_UINT;
	sn->guards[0] = QUARTER0;
	sn->guards[1] = QUARTER1;
	sn->guards[2] = QUARTER2;
	sn->guards[3] = QUARTER3;
	sn->treshold = 0;
	sn->guard = 3;
	return 0;
}

int isLowerSerialNumber(serialNumber *sn, unsigned int id) {
	if (id < sn->treshold)
		return 0;

	return -1;
}

int isNextSerialNumber(serialNumber *sn, unsigned int id) {
	if (sn->treshold < sn->guards[sn->guard]) {
		if (id >= sn->treshold && id < sn->guards[sn->guard]) {
			if (id == sn->treshold) {
				return 0;
			}
		}
	} else if (sn->treshold > sn->guards[sn->guard]) {
		if ((id >= sn->treshold && id <= sn->max) || (id >= 0 && id < sn->guards[sn->guard])) {
			if (id == sn->treshold) {
				return 0;
			}
		}
	}
	return -1;
}

int isAllowedSerialNumber(serialNumber *sn, unsigned int id) {
	if (sn->treshold < sn->guards[sn->guard]) {
		if (id >= sn->treshold && id < sn->guards[sn->guard]) {
			return 0;
		}
	} else if (sn->treshold > sn->guards[sn->guard]) {
		if ((id >= sn->treshold && id <= sn->max) || (id >= 0 && id < sn->guards[sn->guard])) {
			return 0;
		}
	}
	return -1;
}

int incrementNextSerialNumber(serialNumber *sn, unsigned int id, long ts) {
	if (sn->treshold < sn->guards[sn->guard]) {
		if (id >= sn->treshold && id < sn->guards[sn->guard]) {
			if (id == sn->treshold) {
				incrHighTreshold(sn);
				incrLowTreshold(sn, ts);
			}
			return 0;
		}
	} else if (sn->treshold > sn->guards[sn->guard]) {
		if ((id >= sn->treshold && id <= sn->max) || (id >= 0 && id < sn->guards[sn->guard])) {
			if (id == sn->treshold) {
				incrHighTreshold(sn);
				incrLowTreshold(sn, ts);
			}
			return 0;
		}
	}
	printf("ERROR! threshold: %u, id: %u, ts: %u, guard:%u, guardValue: %u\n", sn->treshold, id, ts, sn->guard,
			sn->guards[sn->guard]);
	return -1;
}

void incrHighTreshold(serialNumber *sn) {
	sn->treshold++;
}

void incrLowTreshold(serialNumber *sn, unsigned long ts) {

	switch (sn->guard) {
	case 0:
		if (sn->treshold > sn->guards[2] && sn->treshold < sn->max && ts > (sn->timestamps[0] + MIN_DELAY)) {
			sn->guard = 1;
			sn->timestamps[0] = ts;
			printf("From Q0 to Q1\n");
			printf("treshold: %u, old guard: %u, new guard: %u, new ts: %u\n", sn->treshold, sn->guards[0],
					sn->guards[1], sn->timestamps[0]);
			//			sleep(5);
		}
		break;

	case 1:
		if (((sn->treshold > sn->guards[3] && sn->treshold < sn->max)
				|| (sn->treshold > sn->guards[0] && sn->treshold < sn->guards[1]))
				&& ts > (sn->timestamps[1] + MIN_DELAY)) {
			sn->guard = 2;
			sn->timestamps[1] = ts;
		}
		break;

	case 2:
		if (sn->treshold > sn->guards[0] && sn->treshold < sn->guards[2] && ts > (sn->timestamps[2] + MIN_DELAY)) {
			sn->guard = 3;
			sn->timestamps[2] = ts;
		}
		break;

	case 3:
		if (sn->treshold > sn->guards[1] && sn->treshold < sn->guards[3] && ts > (sn->timestamps[3] + MIN_DELAY)) {
			sn->guard = 0;
			sn->timestamps[3] = ts;
		}
		break;
	default:
		break;
	}

}

