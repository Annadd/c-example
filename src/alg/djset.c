#include <stdio.h>
#include <stdlib.h>

#define VERTICES 6

void initialise(int parent[], int rank[]) {
	for(int i = 0; i < VERTICES; i++) {
		rank[i] = parent[i] = -1;
	}
}

int find_root(int x, int parent[]) {
	int x_root = x;
	while(parent[x_root] != -1) {
		x_root = parent[x_root];
	}
	return x_root;
}

/*
* @return 1 - union successfully, 0 - failed.
*/
int union_vertices(int x, int y, int parent[], int rank[]) {
	int x_root = find_root(x, parent);
	int y_root = find_root(y, parent);
	
	if (x_root == y_root) {
		return 0;
	}
	else {
		if (rank[x_root] > rank[y_root]) {
			parent[y_root] = x_root;
		}
		else if(rank[y_root] > rank[x_root]){
			parent[y_root] = x_root;
		}
		else {
			parent[x_root] = y_root;
			rank[y_root]++;
		}
		return 1;
	}
}

int test() {
	int parent[VERTICES] = {0};
	int rank[VERTICES] = {0};
	int edges[VERTICES][2] = {
		{0, 1}, {1, 2}, {1, 3},
		{2, 4}, {3, 4}, {2, 5}
	}; 
	initialise(parent, rank);
	
	for(int i = 0; i < VERTICES; i++) {
		int x = edges[i][0];
		int y = edges[i][1];
		if (union_vertices(x, y, parent, rank)  == 0) { 
			printf("Cycle detected!\n");
			return 0;
		}
	}
	
	printf("No Cycle found!\n");
	return -1;
}

int test2() {
	int parent[VERTICES] = {0};
	int rank[VERTICES] = {0};
	int edges[5][2] = {
		{0, 1}, {1, 2}, {1, 3},
		{3, 4}, {2, 5}
	}; 
	initialise(parent, rank);
	
	for(int i = 0; i < 5; i++) {
		int x = edges[i][0];
		int y = edges[i][1];
		if (union_vertices(x, y, parent, rank) == 0) { 
			printf("Cycle detected!\n");
			return 0;
		}
	}
	
	printf("No Cycle found!\n");
}

int test3() {
	int parent[VERTICES] = {0};
	int rank[VERTICES] = {0};
	int edges[VERTICES][2] = {
		{0, 1}, {1, 2}, {1, 3},
		{5, 4}, {3, 4}, {2, 5}
	}; 
	initialise(parent, rank);
	
	for(int i = 0; i < VERTICES; i++) {
		int x = edges[i][0];
		int y = edges[i][1];
		if (union_vertices(x, y, parent, rank)  == 0) { 
			printf("Cycle detected!\n");
			return 0;
		}
	}
	
	printf("No Cycle found!\n");
	return -1;
}

int main(int argc, char** argv) {
	test();
	printf("test 2\n");
	test2();
	printf("test 3\n");
	test3();
	return 0;
}