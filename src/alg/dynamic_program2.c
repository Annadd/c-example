#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEN  (20)

/*
* arr = {3, 34, 4, 12, 5, 2}
* s = 9
* select arr number equal s
*/

/*
* true = 1, false = 0 
*/
int rec_subset(int arr[], int i, int s) {
	if (s == 0)
		return 1;
	if (i == 0)
		return arr[i] == s;
	if (arr[i] > s)
		return rec_subset(arr, i - 1, s);
	else {
		int A = rec_subset(arr, i - 1, s - arr[i]);
		int B = rec_subset(arr, i - 1, s);
		return A || B;
	}
}

void matrix_row_set(int matrix[MAX_LEN][MAX_LEN], int row, int val) {
	for(int i = 0; i < row; i++) {
		matrix[i][0] = val;
	}
}

int dp_subset(int arr[], int len, int S) {
	int subset[MAX_LEN][MAX_LEN] = { 0 };
	subset[0][arr[0]] = 1;//true
	matrix_row_set(subset, len, 1);//true 
	
	int i = 0, s = 0;
	for (i = 1; i < len; i++) { 
		for (s = 1; s < S + 1; s++) { 
			if (arr[i] > s) {
				subset[i][s] = subset[i - 1][s];
			} 
			else {
				int A = subset[i - 1][s - arr[i]];
				int B = subset[i - 1][s];
				subset[i][s] = A || B;
			}
		}
	}
#if 0
	for(i = 0; i < len; i++) { 
		for(s = 0; s < S + 1; s++) { 
			printf("%d ", subset[i][s]);
		}
		printf("\n");
	}
#endif
	return subset[len - 1][S];
}

int main(int argc, char** argv) {
	int arr[] = {3, 34, 4, 12, 5, 2};
	int len = sizeof(arr) / sizeof(arr[0]);
	int ret = rec_subset(arr, len, 9);
	printf("rec_subset 9 = %d \n", ret);
	ret = rec_subset(arr, len, 10);
	printf("rec_subset 10 = %d \n", ret);
	ret = rec_subset(arr, len, 11);
	printf("rec_subset 11 = %d \n", ret);
	ret = rec_subset(arr, len, 12);
	printf("rec_subset 12 = %d \n", ret);
	ret = rec_subset(arr, len, 13);
	printf("rec_subset 13 = %d \n", ret);
	ret = rec_subset(arr, len, 14);
	printf("rec_subset 14 = %d \n", ret);	
	
	ret = dp_subset(arr, len, 9);
	printf("dp_subset 9 = %d \n", ret);
	ret = dp_subset(arr, len, 10);
	printf("dp_subset 10 = %d \n", ret);
	ret = dp_subset(arr, len, 11);
	printf("dp_subset 11 = %d \n", ret);
	ret = dp_subset(arr, len, 12);
	printf("dp_subset 12 = %d \n", ret);
	ret = dp_subset(arr, len, 13);
	printf("dp_subset 13 = %d \n", ret);
	ret = dp_subset(arr, len, 14);
	printf("dp_subset 14 = %d \n", ret);
	return 0;
}