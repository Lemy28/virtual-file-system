//
// Created by DiDi on 2022/5/26.
//

#ifndef FILESYSTEM_SUPERBLOCK_H
#define FILESYSTEM_SUPERBLOCK_H

#define BLOCKS_PER_GROUP	64				//空闲块堆栈大小，一个空闲堆栈最多能存多少个磁盘块地址


class SuperBlock{
public:
    unsigned short s_INODE_NUM;				//inode节点数，最多 65535
    unsigned int s_BLOCK_NUM;				//逻辑块总数量，最多 4294967294




    unsigned short s_free_INODE_NUM;		//空闲inode节点数
    unsigned int s_free_BLOCK_NUM;			//空闲磁盘块数
    int s_free_addr;						//空闲块堆栈指针
    int s_free[BLOCKS_PER_GROUP];			//空闲块堆栈

    unsigned short s_BLOCK_SIZE;			//磁盘块大小
    unsigned short s_INODE_SIZE;			//inode大小
    unsigned short s_SUPERBLOCK_SIZE;		//超级块大小
    unsigned short s_blocks_per_group;		//每 blockgroup 的block数量

    //磁盘分布
    int s_Superblock_StartAddr;
    int s_InodeBitmap_StartAddr;
    int s_BlockBitmap_StartAddr;
    int s_Inode_StartAddr;
    int s_Block_StartAddr;
};


#endif //FILESYSTEM_SUPERBLOCK_H
