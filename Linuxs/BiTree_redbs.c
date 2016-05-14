/******************************************************************************
 * Function:	A example of red-black tree operation.
 * Author: 	forwarding2012@yahoo.com.cn			  		 
 * Date:		2012.01.01				  		  
 * Complie:	gcc -Wall BiTree_redbs.c -o BiTree_redbs
******************************************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>

typedef enum color_s { RED, BLACK } color_t;

typedef struct RBTNode {
	int key;
	struct RBTNode *left;
	struct RBTNode *right;
	struct RBTNode *parent;
	color_t color;
	char data;
} RBTree;

RBTree *rb_new_node(int key, char data)
{
	RBTree *node;

	if ((node = (RBTree *) malloc(sizeof(RBTree))) == NULL)
		return NULL;

	node->key = key;
	node->data = data;

	return node;
}

RBTree *rb_rotate_left(RBTree * node, RBTree * root)
{
	RBTree *right = node->right;

	if ((node->right = right->left))
		right->left->parent = node;
	right->left = node;

	if ((right->parent = node->parent)) {
		if (node == node->parent->right)
			node->parent->right = right;
		else
			node->parent->left = right;
	} else {
		root = right;
	}
	node->parent = right;

	return root;
}

RBTree *rb_rotate_right(RBTree * node, RBTree * root)
{
	RBTree *left = node->left;

	if ((node->left = left->right))
		left->right->parent = node;
	left->right = node;

	if ((left->parent = node->parent)) {
		if (node == node->parent->right)
			node->parent->right = left;
		else
			node->parent->left = left;
	} else {
		root = left;
	}
	node->parent = left;

	return root;
}

RBTree *rb_search_auxiliary(int key, RBTree * root, RBTree ** save)
{
	int ret;
	RBTree *node = root, *parent = NULL;

	while (node) {
		parent = node;
		ret = node->key - key;
		if (0 < ret)
			node = node->left;
		else if (0 > ret)
			node = node->right;
		else
			return node;
	}

	if (save)
		*save = parent;

	return NULL;
}

RBTree *rb_insert_rebalance(RBTree * node, RBTree * root)
{
	RBTree *parent, *gparent, *uncle, *tmp;

	while ((parent = node->parent) && parent->color == RED) {
		gparent = parent->parent;

		if (parent == gparent->left) {
			uncle = gparent->right;

			if (uncle && uncle->color == RED) {
				uncle->color = BLACK;
				parent->color = BLACK;
				gparent->color = RED;
				node = gparent;
			} else {
				if (parent->right == node) {
					root = rb_rotate_left(parent, root);
					tmp = parent;
					parent = node;
					node = tmp;
				}

				parent->color = BLACK;
				gparent->color = RED;
				root = rb_rotate_right(gparent, root);
			}
		} else {
			uncle = gparent->left;
			if (uncle && uncle->color == RED) {
				uncle->color = BLACK;
				parent->color = BLACK;
				gparent->color = RED;
				node = gparent;
			} else {
				if (parent->left == node) {
					root = rb_rotate_right(parent, root);
					tmp = parent;
					parent = node;
					node = tmp;
				}
				parent->color = BLACK;
				gparent->color = RED;
				root = rb_rotate_left(gparent, root);
			}
		}
	}
	root->color = BLACK;

	return root;
}

RBTree *rb_insert(int key, char data, RBTree * root)
{
	RBTree *parent = NULL, *node;

	if ((node = rb_search_auxiliary(key, root, &parent)))
		return root;

	node = rb_new_node(key, data);
	node->parent = parent;
	node->left = node->right = NULL;
	node->color = RED;

	if (parent) {
		if (parent->key > key)
			parent->left = node;
		else
			parent->right = node;
	} else {
		root = node;
	}

	return rb_insert_rebalance(node, root);
}

RBTree *rb_erase_rebalance(RBTree * node, RBTree * parent, RBTree * root)
{
	RBTree *other, *o_left, *o_right;

	while ((!node || node->color == BLACK) && node != root) {
		if (parent->left == node) {
			other = parent->right;
			if (other->color == RED) {
				other->color = BLACK;
				parent->color = RED;
				root = rb_rotate_left(parent, root);
				other = parent->right;
			}

			if ((!other->left || other->left->color == BLACK) &&
			    (!other->right || other->right->color == BLACK)) {
				other->color = RED;
				node = parent;
				parent = node->parent;
			} else {
				if (!other->right
				    || other->right->color == BLACK) {
					if ((o_left = other->left))
						o_left->color = BLACK;
					other->color = RED;
					root = rb_rotate_right(other, root);
					other = parent->right;
				}

				other->color = parent->color;
				parent->color = BLACK;
				if (other->right)
					other->right->color = BLACK;

				root = rb_rotate_left(parent, root);
				node = root;
				break;
			}
		} else {
			other = parent->left;
			if (other->color == RED) {
				other->color = BLACK;
				parent->color = RED;
				root = rb_rotate_right(parent, root);
				other = parent->left;
			}

			if ((!other->left || other->left->color == BLACK) &&
			    (!other->right || other->right->color == BLACK)) {
				other->color = RED;
				node = parent;
				parent = node->parent;
			} else {
				if (!other->left || other->left->color == BLACK) {
					if ((o_right = other->right))
						o_right->color = BLACK;

					other->color = RED;
					root = rb_rotate_left(other, root);
					other = parent->left;
				}
				other->color = parent->color;
				parent->color = BLACK;
				if (other->left)
					other->left->color = BLACK;

				root = rb_rotate_right(parent, root);
				node = root;
				break;
			}
		}
	}

	if (node)
		node->color = BLACK;

	return root;
}

RBTree *rb_erase(int key, RBTree * root)
{
	RBTree *child, *parent, *old, *left, *node;
	color_t color;

	if ((node = rb_search_auxiliary(key, root, NULL)) == NULL)
		return root;

	old = node;
	if (node->left && node->right) {
		node = node->right;
		while ((left = node->left) != NULL)
			node = left;

		child = node->right;
		parent = node->parent;
		color = node->color;

		if (child)
			child->parent = parent;

		if (parent) {
			if (parent->left == node)
				parent->left = child;
			else
				parent->right = child;
		} else {
			root = child;
		}

		if (node->parent == old)
			parent = node;
		node->parent = old->parent;
		node->color = old->color;
		node->right = old->right;
		node->left = old->left;

		if (old->parent) {
			if (old->parent->left == old)
				old->parent->left = node;
			else
				old->parent->right = node;
		} else {
			root = node;
		}

		old->left->parent = node;
		if (old->right)
			old->right->parent = node;
	} else {
		if (!node->left)
			child = node->right;
		else if (!node->right)
			child = node->left;

		parent = node->parent;
		color = node->color;

		if (child)
			child->parent = parent;

		if (parent) {
			if (parent->left == node)
				parent->left = child;
			else
				parent->right = child;
		} else {
			root = child;
		}
	}
	free(old);

	if (color == BLACK)
		root = rb_erase_rebalance(child, parent, root);

	return root;
}

int main(void)
{
	int i, key, count = 20;
	RBTree *root = NULL, *node = NULL;

	srand(time(NULL));
	for (i = 1; i < count; ++i) {
		key = rand() % count;
		if ((root = rb_insert(key, i, root)))
			printf("[i-%2d] insert key %2d success.\n", i, key);
		else
			return -1;

		if ((node = rb_search_auxiliary(key, root, NULL)))
			printf("[i-%2d] search key %2d success.\n", i, key);
		else
			return -1;

		if ((i % 10) == 0) {
			if ((root = rb_erase(key, root)))
				printf("[i-%2d] erases key %2d success.\n", i, key);
			else
				printf("[i-%2d] erases key %2d error.\n", i, key);
		}
	}

	return 0;
}
