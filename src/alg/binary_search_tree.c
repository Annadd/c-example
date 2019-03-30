#include <stdio.h>
#include <stdlib.h>

#define Element  int 

typedef struct node {
	Element data;
	struct node* left;
	struct node* right;
} Node;

typedef struct { 
	Node* root;
} Tree;

void insert(Tree* tree, Element value) {
	Node* node = malloc(sizeof(Node));
	node->data = value;
	node->left = NULL;
	node->right = NULL;
	
	if (tree->root == NULL) {
		tree->root = node;
	}
	else {
		Node* temp = tree->root;
		while (temp != NULL) {
			if (value < temp->data) { 
				if (!temp->left) {
					temp->left = node;
					break;
				} else {
					temp = temp->left;
				}
			} 
			else if (value > temp->data){
				if (!temp->right) { 
					temp->right = node;
					break;
				} else {
					temp = temp->right;
				}
			}
			else {
				printf("value is duplicate\n");
			}
		}
		
	}
}

int get_height(Node* node) {
	if (node == NULL) {
		return 0;
	} 
	else {
		int left  = get_height(node->left);
		int right = get_height(node->right);
		int max = left;
		if(right > max) {
			max = right;
		}
		return max + 1;
	}
}

int get_maximum(Node* node) {
	if (node == NULL) {
		return -1;
	}
	else {
		int m1 = get_maximum(node->left);
		int m2 = get_maximum(node->right);
		int m3 = node->data;
		int max = m1;
		if (m2 > max) { max = m2; }
		if (m3 > max) { max = m3; }
		return max;
	}
}

void preorder(Node* node) {
	if (node != NULL) {
		printf("%d \n", node->data);
		preorder(node->left);
		preorder(node->right);
	}
}

void inorder(Node* node) {
	if (node != NULL) {
		inorder(node->left);
		printf("%d \n", node->data);
		inorder(node->right);
	}
}

void postorder(Node* node) {
	if (node != NULL) {
		postorder(node->left);
		postorder(node->right);
		printf("%d \n", node->data);
	}
}

int main(int argc, char** argv) {
	int arr[] = {6, 3, 8, 2, 5, 1, 7};
	int len = sizeof(arr) / sizeof(arr[0]);
	Tree tree;
	tree.root = NULL;
	
	for (int i = 0; i < len; i++) {
		insert(&tree, arr[i]);
	}
	
	preorder(tree.root);
	printf("\n\n");
	inorder(tree.root);
	
	int heigth = get_height(tree.root);
	printf("heigth=%d \n", heigth);
	
	int maximum = get_maximum(tree.root);
	printf("maximum=%d \n", maximum);
	return 0;
}