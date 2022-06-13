//
// Created by DiDi on 2022/5/26.
//

#ifndef FILESYSTEM_INODE_H
#define FILESYSTEM_INODE_H


class Inode{
public:
    unsigned short i_ino;					//inode标识（编号）
    unsigned short i_mode;					//存取权限。r--读取，w--写，x--执行
    unsigned short i_cnt;					//链接数。有多少文件名指向这个inode
    char i_uname[20];						//文件所属用户
    char i_gname[20];						//文件所属用户组
    unsigned int i_size;					//文件大小，单位为字节（B）
    time_t  i_ctime;						//inode上一次变动的时间
    time_t  i_mtime;						//文件内容上一次变动的时间
    time_t  i_atime;						//文件上一次打开的时间
    int i_dirBlock[10];						//10个直接块。10*512B = 5120B = 5KB
    int i_indirBlock_1;						//一级间接块。512B/4 * 512B = 128 * 512B = 64KB
};



#endif //FILESYSTEM_INODE_H
