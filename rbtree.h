//
// Created by DiDi on 2022/5/26.
//

#ifndef FILESYSTEM_RBTREE_H
#define FILESYSTEM_RBTREE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dentry.h"
#include <string>
//用于管理目录项的红黑树

#define RED				1
#define BLACK 			2
using namespace std;




//key为目录名，value 为绝对地址
typedef struct _rbtree_node {

    unsigned char color;
    struct _rbtree_node *right;
    struct _rbtree_node *left;
    struct _rbtree_node *parent;
    string key;
    int value;

    _rbtree_node(){
        color = RED;
        right = NULL;
        left = NULL;
        parent = NULL;
        value = 0;
    }
    bool operator==(_rbtree_node);
    ostream operator<<(ostream &out);

} rbtree_node;

typedef struct _rbtree {
    rbtree_node *root;
    rbtree_node *nil;

    _rbtree(){
        root = nullptr;
        nil = nullptr;
    }
} rbtree;

rbtree_node *rbtree_mini(rbtree *T, rbtree_node *x);


rbtree_node *rbtree_maxi(rbtree *T, rbtree_node *x) ;

rbtree_node *rbtree_successor(rbtree *T, rbtree_node *x) ;


void rbtree_left_rotate(rbtree *T, rbtree_node *x);



void rbtree_right_rotate(rbtree *T, rbtree_node *y) ;


void rbtree_insert_fixup(rbtree *T, rbtree_node *z);


void rbtree_insert(rbtree *T, rbtree_node *z);

void rbtree_delete_fixup(rbtree *T, rbtree_node *x) ;
rbtree_node *rbtree_delete(rbtree *T, rbtree_node *z) ;


rbtree_node *rbtree_search(rbtree *T, string key);


void rbtree_traversal(rbtree *T, rbtree_node *node);








#endif //FILESYSTEM_RBTREE_H
