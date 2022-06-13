//
// Created by DiDi on 2022/5/26.
//

#ifndef FILESYSTEM_DENTRY_H
#define FILESYSTEM_DENTRY_H

#define MAX_NAME_SIZE 28					//最大名字长度，长度要小于这个大小

class dentry{								//32字节，一个磁盘块能存 512/32=16 个目录项
public:
    char itemName[MAX_NAME_SIZE];			//目录或者文件名
    int inodeAddr;                          //目录项对应的inode节点地址;
};



#endif //FILESYSTEM_DENTRY_H
