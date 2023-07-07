/*
 * dpa_Common.h
 *
 *  Created on: Aug 21, 2012
*      Author: Nicolò Rivetti
 */

#ifndef COMMON_H_
#define COMMON_H_

void delete_duplicate(int *array, int *size);
void sort(int *array, int size);
void merge_delete_duplicates(int *array_a, int size_a, int *array_b, int size_b,
		int *array_c, int *newSize);
void sort_delete_duplicate(int *array, int *size);
void copy_to_array(int *in_array, int* out_array, int size);

#endif /* COMMON_H_ */
