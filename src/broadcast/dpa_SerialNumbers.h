/*
 * dpa_SerialNumbers.h
 *
 *  Created on: Sep 26, 2012
 *      Author: helrohir
 */

#ifndef DPA_SERIALNUMBERS_H_
#define DPA_SERIALNUMBERS_H_

struct serialNumber_struct {
	unsigned int treshold;
	unsigned int max;
	unsigned int guard;
	unsigned int guards[4];
	long timestamps[4];
};

typedef struct serialNumber_struct serialNumber;

int initializeSerialNumber(serialNumber *sn);
int isNextSerialNumber(serialNumber *sn, unsigned int id);
int isAllowedSerialNumber(serialNumber *sn, unsigned int id);
int incrementNextSerialNumber(serialNumber *sn, unsigned int id, long ts);
int isLowerSerialNumber(serialNumber *sn, unsigned int id);

#endif /* DPA_SERIALNUMBERS_H_ */
