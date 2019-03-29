#include <stdio.h>

void swap(int arr[], int i, int j) {
	int temp = arr[i];
	arr[i] = arr[j];
	arr[j] = temp;
}

/*
* i current node
* node amount node
*/
void heapify(int tree[], int node, int i) {
	if (i >= node) {
		return;
	}
	
	int left_node  = 2 * i + 1;
	int right_node = 2 * i + 2;
	int max = i;
	if (left_node < node && tree[left_node] > tree[max]) {
		max = left_node;
	}
	if (right_node < node && tree[right_node] > tree[max]) {
		max = right_node;
	}
	if (max != i) {
		swap(tree, max, i);
		heapify(tree, node, max);
	}
}

void build_heap(int tree[], int n) {
	int last_node = n - 1;
	int parent = (last_node - 1) / 2;
	
	for(int i = parent; i >= 0; i--){
		heapify(tree, n, i);
	}
}

void heap_sort(int tree[], int n) {
	build_heap(tree, n);
	
	for(int i = n - 1; i >= 0; i--) {
		swap(tree, i, 0);
		heapify(tree, i, 0);
	}
}

void show_tree(int tree[], int n) {
	for (int i = 0; i < n; i++) {
		printf("tree[%d] = %d\n", i, tree[i]);
	}
}

int main(int argc, char** argv) {
	int tree[] = {4, 10, 3, 5, 1, 2};
	int n = 6;
	heapify(tree, n, 0);
	show_tree(tree, n);
	
	printf("\n");
	int tree2[] = {2, 5, 3, 1, 10, 4};
	build_heap(tree2, n);
	show_tree(tree2, n);
	
	printf("\n");
	int tree3[] = {2, 5, 3, 1, 10, 4};
	heap_sort(tree3, n);
	show_tree(tree3, n);
	return 0;
}