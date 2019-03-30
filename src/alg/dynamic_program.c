#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int get_max(int a, int b) {
	return a > b ? a : b;
}

int rec_opt(int arr[], int len, int idx) {
	if (idx == 0) {
		return arr[0];
	}
	else if (idx == 1) {
		return get_max(arr[1], arr[0]);
	}
	else {
		//A selected
		int A = rec_opt(arr, len, idx - 2) + arr[idx];
		//B abandon
		int B = rec_opt(arr, len, idx - 1);
		return get_max(A, B);
	}
}

int dp_opt(int arr[], int len) {
	int* opt = malloc(sizeof(int) * len);
	memset(opt, 0, sizeof(int) * len);
	opt[0] = arr[0];
	opt[1] = get_max(arr[0], arr[1]);
	
	for (int i = 2; i < len; i++) {
		int A = opt[i - 2] + arr[i];
		int B = opt[i - 1];
		opt[i] = get_max(A, B);
	}
	
	return opt[len - 1];
}

/*
* 不相邻的两个数相加值最大.
*/
int main(int argc, char** argv) {
	int arr[] = {1, 2, 4, 1, 7, 8, 3};
	int len = sizeof(arr) / sizeof(arr[0]);
	
	int max = rec_opt(arr, len, 6);
	printf("rec max=%d \n", max);
	
	max = dp_opt(arr, len);
	printf("dp max=%d \n", max);
	return 0;
}