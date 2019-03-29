#include <stdio.h>

#define MAX_LEN 1024

/*
[题目]给定一个数组arr，数组可能非常大。在程序运行过程中，你可能要做好几次query和update操作：
query(arr, L, R) 表示计算数组arr中，从下标L到下标R之间的所有数字的和。
update(arr, i, val) 表示要把arr[i]中的数字改成val。
怎样尽可能快地完成一系列query和update的操作？
线段树可以在花费一些额外空间的情况下，把这两个操作的时间复杂度都控制在O(log(n)).
*/

void build_tree(int arr[], int tree[], int node, int start, int end){
	if(start == end) {
		tree[node] = arr[start];
	} else {
		int mid = (start + end) / 2;
		int left_node = 2 * node + 1;
		int right_node = 2 * node + 2;
		
		build_tree(arr, tree, left_node, start, mid);
		build_tree(arr, tree, right_node, mid + 1, end);
		tree[node] = tree[left_node] + tree[right_node];
	}
}

void update_tree(int arr[], int tree[], int node, int start, int end, int idx, int val) {
	if (start == end){
		arr[idx] = val;
		tree[node] = val;
	} 
	else {
		int mid = (start + end) / 2;
		int left_node = 2 * node + 1;
		int right_node = 2 * node + 2;
		
		if(idx >= start && idx <= mid) {
			update_tree(arr, tree, left_node, start, mid, idx, val);
		}
		else {
			update_tree(arr, tree, right_node, mid + 1, end, idx, val);
		}
		
		tree[node] = tree[left_node] + tree[right_node];		
	}
}

/*
* return L - R range sum
*/
int query_tree(int arr[], int tree[], int node, int start, int end, int L, int R){
	printf("start = %d, end = %d\n\n", start, end);
	if(R < start || L > end) {
		return 0;
	}
	else if(L <= start && end <= R) {
		return tree[node];
	}
	else if(start == end) {
		return tree[node];
	}
	else{
		int mid = (start + end) / 2;
		int left_node = 2 * node + 1;
		int right_node = 2 * node + 2;
		int sum_left  = query_tree(arr, tree, left_node,  start,   mid, L, R);
		int sum_right = query_tree(arr, tree, right_node, mid + 1, end, L, R);
		return sum_left + sum_right;
	}
}

void show_tree(int tree[], int len) {
	for(int i = 0; i < len; i++){
		printf("tree[%d] = %d\n",i ,tree[i]);
	}
}

int main(int argc, char** argv) {
	int arr[] = {1, 3, 5, 7, 9, 11};
	int len = sizeof(arr) / sizeof(arr[0]);
	int tree[MAX_LEN] = {0};
	int tlen = 15;
	
	build_tree(arr, tree, 0, 0, len - 1);
	show_tree(tree, tlen);
	
	update_tree(arr, tree, 0, 0, len - 1, 4, 6);
	printf("\n");
	show_tree(tree, tlen);
	
	int s = query_tree(arr, tree, 0, 0, len - 1, 2, 5);
	printf("s = %d\n", s);
	return 0;
}