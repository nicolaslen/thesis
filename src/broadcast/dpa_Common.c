/*
 * dpa_Common.c
 *
 *  Created on: Aug 21, 2012
 *      Author: Nicolò Rivetti
 */

#include <stdlib.h>


/*
 * Given an array and it's size, it returns the same buffer without duplicates
 * and the new number of items
 */
void delete_duplicate(int *array, int *size) {
	int *aux = (int*) malloc(sizeof(int) * (*size));
	int i, k;
	k = 0;
	for (i = 0; i < *size; i++) {
		aux[k] = array[i];
		k++;
		while ((i < *size - 1) && (array[i + 1] == array[i]))
			i++;
	}

	*size = k;

	for (i = 0; i < k; i++) {
		array[i] = aux[i];
	}
	free(aux);

}

/*
 * Given an array and it's size, it returns the same buffer ordered in ascending order
 */
void sort(int *array, int size) {
	int temp, i, j = 0;
	for (i = 0; i < size; i++) {
		for (j = 0; j < size - i - 1; j++) {
			if (array[j] > array[j + 1]) {
				temp = array[j];
				array[j] = array[j + 1];
				array[j + 1] = temp;
			}
		}
	}
}

/*
 * Given two array in ascending order, their sizes and an array which can accomodates all items,
 * it returns a array which contains all non duplicate items of both array in ascending order,
 * and the number of item of the new array
 */
void merge_delete_duplicates(int *array_a, int size_a, int *array_b, int size_b, int *array_c, int *newSize) {
	int a = 0, b = 0, c = 0;

	while (a < size_a && b < size_b) {
		if (array_a[a] > array_b[b]) {
			array_c[c] = array_b[b];
			b++;

		} else if (array_a[a] < array_b[b]) {
			array_c[c] = array_a[a];
			a++;
		} else {
			array_c[c] = array_b[b];
			b++;
			a++;
		}
		c++;
	}
	while (a < size_a) {
		array_c[c] = array_a[a];
		a++;
		c++;
	}
	while (b < size_b) {
		array_c[c] = array_b[b];
		b++;
		c++;
	}
	*newSize = c;
}

/*
 * Given an array and it's size, it returns the same buffer without duplicates ordered in
 * ascending order and the new number of items
 */
void sort_delete_duplicate(int *array, int *size) {
	sort(array, *size);
	delete_duplicate(array, size);
}

/*
 * Given two arrays and the size of the smallest, it copies the values of the in_array
 * to the out_array
 */
void copy_to_array(int out_array[], int in_array[], int size) {
	int i = 0;

	for (i = 0; i < size; i++) {
		out_array[i] = in_array[i];
	}
}
