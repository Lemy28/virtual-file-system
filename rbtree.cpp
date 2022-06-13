//
// Created by DiDi on 2022/5/26.
//

#include <iostream>
#include "rbtree.h"


bool rbtree_node::operator==(_rbtree_node y){
    rbtree_node x = *this;
    if(
            x.key==y.key&&\
            x.value==y.value
            )
        return true;
    else
        return false;
}

rbtree_node *rbtree_mini(rbtree *T, rbtree_node *x) {
    while (x->left != T->nil) {
        x = x->left;
    }
    return x;
}

rbtree_node *rbtree_maxi(rbtree *T, rbtree_node *x) {
    while (x->right != T->nil) {
        x = x->right;
    }
    return x;
}

rbtree_node *rbtree_successor(rbtree *T, rbtree_node *x) {
    rbtree_node *y = x->parent;

    //找到右边最小的节点
    if (x->right != T->nil) {
        return rbtree_mini(T, x->right);
    }

    while ((y != T->nil) && (x == y->right)) {
        //x是最大的一个值
        x = y;
        y = y->parent;
    }
    return y;
}


//对x节点进行左旋
void rbtree_left_rotate(rbtree *T, rbtree_node *x) {

    //y节点为x的右节点
    rbtree_node *y = x->right;  // x  --> y  ,  y --> x,   right --> left,  left --> right

    x->right = y->left; //1 1
    if (y->left != T->nil) { //1 2
        y->left->parent = x;
    }

    y->parent = x->parent; //1 3
    if (x->parent == T->nil) { //1 4
        T->root = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }

    y->left = x; //1 5
    x->parent = y; //1 6
}


void rbtree_right_rotate(rbtree *T, rbtree_node *y) {

    rbtree_node *x = y->left;

    y->left = x->right;
    if (x->right != T->nil) {
        x->right->parent = y;
    }

    x->parent = y->parent;
    if (y->parent == T->nil) {
        T->root = x;
    } else if (y == y->parent->right) {
        y->parent->right = x;
    } else {
        y->parent->left = x;
    }

    x->right = y;
    y->parent = x;
}

/*红黑树的节点插入大概有两种情况，第二种情况又可以细分为两种，总的来说有三种
1.插入节点的双亲为黑色，没有破坏红黑树的性质，无需操作
2.插入节点的双亲为红色，破坏红黑树的性质：红节点的双亲一定是黑节点，需要调整
此时需要判断其叔叔节点的颜色
 (1)叔叔节点为红色，那么只需要将其祖父节点变为红色，叔叔和父节点变为黑色。再递归判断其祖父节点是否违反红黑树性质
 (2)叔叔节点为黑色，此时仅凭变色无法维持红黑树的性质，需要旋转。
 旋转具体的方法是：
 a.

 */

//需要修复的情况只有第二类,父节点为红色
void rbtree_insert_fixup(rbtree *T, rbtree_node *z) {

    while (z->parent!= NULL && z->parent->color == RED) { //z ---> RED
        //父节点为红色

        if (z->parent == z->parent->parent->left) {
            //因为叔叔节点可能是祖父的左子树或者右子树，所以需要判断一下。

            //父节点为祖父的左子树
            //y指向叔叔节点
            rbtree_node *y = z->parent->parent->right;

            if (y!=NULL&&y->color == RED) {
                //叔叔存在且为红色,则只需要祖父变为红色，叔叔和父亲变为黑色

                z->parent->color = BLACK;//父变黑色

                y->color = BLACK;//叔叔变黑色
                z->parent->parent->color = RED;//祖父变红色

                z = z->parent->parent; //z --> RED 祖父变为红色后，有可能破坏了红黑树性质，需要递归判断，这里用的是更改节点后循环
            } else {
                //叔叔为黑色(空节点也是黑色，即叶子节点),此时变色无法解决问题，需要旋转

                if (z == z->parent->right) {

                    //当前的节点为父节点的右孩子
                    //这是最麻烦的情况，需要两次旋转
                    z = z->parent;
                    rbtree_left_rotate(T, z);
                }

                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rbtree_right_rotate(T, z->parent->parent);
            }
        }else {
            //父节点为右子树

            //y还是指向叔叔节点
            rbtree_node *y = z->parent->parent->left;

            if (y!=NULL&&y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;

                z = z->parent->parent; //z --> RED
            }
            else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rbtree_right_rotate(T, z);
                }

                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rbtree_left_rotate(T, z->parent->parent);
            }
        }

    }

    //确保根节点为黑色
    T->root->color = BLACK;
}



void rbtree_insert(rbtree *T, rbtree_node *z) {

    if(z == NULL || T == NULL)return;

    //x用来找到z节点应该插入的位置，y节点用来找到插入节点的双亲节点
    rbtree_node *y = T->nil;
    rbtree_node *x = T->root;

    while (x != T->nil) {
        //y记录x的位置
        y = x;

        //x往下走
        if (z->key < x->key) {
            x = x->left;
        } else if (z->key > x->key) {
            x = x->right;
        } else { //Exist
            return ;
        }
    }
    //x已经找到位置,y为待插入节点的双亲节点

    //将待插入节点指向双亲
    z->parent = y;
    //判断树是否为空
    if (y == T->nil) {
        T->root = z;//树为空，将插入节点设置为根节点
       //否则根据值设置为左节点或者右节点
    } else if (z->key < y->key) {
        y->left = z;
    } else {
        y->right = z;
    }

    //将新插入的节点设置好属性
    z->left = T->nil;
    z->right = T->nil;
    z->color = RED;

    //确保红黑树的性质成立
    rbtree_insert_fixup(T, z);
}

void rbtree_delete_fixup(rbtree *T, rbtree_node *x) {

    while ((x != T->root) && (x->color == BLACK)) {
        if (x == x->parent->left) {

            rbtree_node *w= x->parent->right;
            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;

                rbtree_left_rotate(T, x->parent);
                w = x->parent->right;
            }

            if ((w->left->color == BLACK) && (w->right->color == BLACK)) {
                w->color = RED;
                x = x->parent;
            } else {

                if (w->right->color == BLACK) {
                    w->left->color = BLACK;
                    w->color = RED;
                    rbtree_right_rotate(T, w);
                    w = x->parent->right;
                }

                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->right->color = BLACK;
                rbtree_left_rotate(T, x->parent);

                x = T->root;
            }

        } else {

            rbtree_node *w = x->parent->left;
            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                rbtree_right_rotate(T, x->parent);
                w = x->parent->left;
            }

            if ((w->left->color == BLACK) && (w->right->color == BLACK)) {
                w->color = RED;
                x = x->parent;
            } else {

                if (w->left->color == BLACK) {
                    w->right->color = BLACK;
                    w->color = RED;
                    rbtree_left_rotate(T, w);
                    w = x->parent->left;
                }

                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->left->color = BLACK;
                rbtree_right_rotate(T, x->parent);

                x = T->root;
            }

        }
    }

    x->color = BLACK;
}

rbtree_node *rbtree_delete(rbtree *T, rbtree_node *z) {

    if(T==NULL||z==NULL)return NULL;

    rbtree_node *y = T->nil;
    rbtree_node *x = T->nil;

    if ((z->left == T->nil) || (z->right == T->nil)) {
        y = z;
    } else {
        y = rbtree_successor(T, z);
    }

    if (y->left != T->nil) {
        x = y->left;
    } else if (y->right != T->nil) {
        x = y->right;
    }



    if(x!=NULL)
    x->parent = y->parent;

    if (y->parent == T->nil) {
        T->root = x;
    } else if (y == y->parent->left) {
        y->parent->left = x;
    } else {
        y->parent->right = x;
    }

    if (y != z) {
        z->key = y->key;
        z->value = y->value;
    }

    if (y->color == BLACK) {
        rbtree_delete_fixup(T, x);
    }

    return y;
}

rbtree_node *rbtree_search(rbtree *T, string key) {

    rbtree_node *node = T->root;
    while (node != T->nil) {
        if (key < node->key) {
            node = node->left;
        } else if (key > node->key) {
            node = node->right;
        } else {
            return node;
        }
    }
    return T->nil;
}


//中序遍历
void rbtree_traversal(rbtree *T, rbtree_node *node) {
    if (node != NULL) {
        rbtree_traversal(T, node->left);
        if(node->color == BLACK){
            std::cout<<"name:"<<node->key<< "   color:  BLACK"<<endl;
        }
        else if(node->color == RED){cout<<"name:"<<node->key<< "   color:  RED"<<endl;}
        rbtree_traversal(T, node->right);
    }
}


