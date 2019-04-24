#include"binary_tree.h"

/*
           26
         /    \
      -1       55
     /  \     /  \
   -4    10 40    99
   /       /     /
 -9      33    88
 */

int main(){
    T *root = NULL;
    int arry[10] = {26, -1, -4, 10, 55, 40, 99, -9, 33, 88};
    int i = 0;
    for(; i < sizeof(arry)/sizeof(arry[0]); i++){
        root = create1_tree(root, arry[i]);
    }
    //前序遍历
    show1_tree(root);
    //putchar(10);
    return 0;
}
