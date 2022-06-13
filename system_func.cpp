#include <deque>
#include <algorithm>
#include "MingOS.h"
#include "dentry.h"
#include "rbtree.h"
using namespace std;

deque<rbtree_node> dir_queue;
//传入目录地址和目录名,该函数可将目录内的所有目录加入红黑树
void dir_traversal(int parinoAddr,string dirname){

    Inode curinode;
    //取出inode
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&curinode, sizeof(Inode), 1, fr);
    fflush(fr);

    //取出目录项数
    int dentrycnt = curinode.i_cnt;
    if(curinode.i_cnt == 2)return;

    //依次取出磁盘块
    int i = 0;
    while(i < dentrycnt&&i<160){
        dentry dirlist[16] = {0};
        if(curinode.i_dirBlock[i / 16] == -1){
            i+=16;
            continue;
        }
        //取出磁盘块
        int parblockAddr = curinode.i_dirBlock[i / 16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //输出该磁盘块中的所有目录项
        int j;
        for(j=0;j<16 && i < dentrycnt; j++){


            Inode tmp;
            //取出该目录项的inode，判断该目录项是目录还是文件
            fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
            fread(&tmp,sizeof(Inode),1,fr);
            fflush(fr);

            if( strcmp(dirlist[j].itemName,"")==0 || strcmp(dirlist[j].itemName,".")==0 || strcmp(dirlist[j].itemName,"..")==0){
                continue;
            }
            //找到目录,加入到红黑树
            if( ( (tmp.i_mode>>9) & 1 ) == 1 ){

                rbtree_node* p = new rbtree_node;
                p->key = dirname+dirlist[j].itemName;
                p->value = dirlist[j].inodeAddr;
                rbtree_insert(&dir_tree,p);

                //将该目录入队列
                deque<rbtree_node>::iterator it = find(dir_queue.begin(),dir_queue.end(),*p);
                if(it == dir_queue.end()){
                    //队列中没有该目录,将其加入
                    dir_queue.push_back(*p);
                }


            }

            i++;
        }

    }

}





void Ready()	//登录系统前的准备工作,变量初始化+注册+安装
{
    //初始化变量
    nextUID = 0;
    nextGID = 0;
    strcpy(Cur_User_Name,"root");
    strcpy(Cur_Group_Name,"root");

    //获取主机名
    memset(Cur_Host_Name,0,sizeof(Cur_Host_Name));
    DWORD k= 100;
    GetComputerName(Cur_Host_Name,&k);

    //根目录inode地址 ，当前目录地址和名字
    Root_Dir_Addr = Inode_StartAddr;	//第一个inode地址
    Cur_Dir_Addr = Root_Dir_Addr;
    strcpy(Cur_Dir_Name,"/");


    char c;
    printf("是否格式化?[y/n]");
    while(c = getch()){
        fflush(stdin);
        if(c=='y'){
            printf("\n");
            printf("文件系统正在格式化……\n");
            if(!Format()){
                printf("文件系统格式化失败\n");
                return ;
            }
            printf("格式化完成\n");
            break;
        }
        else if(c=='n'){
            printf("\n");
            break;
        }
    }

    //printf("载入文件系统……\n");
    if(!Install()){
        printf("安装文件系统失败\n");
        return ;
    }
    //printf("载入完成\n");
}

bool Format()	//格式化一个虚拟磁盘文件
{
    int i,j;

    //初始化超级块
    superblock->s_INODE_NUM = INODE_NUM;
    superblock->s_BLOCK_NUM = BLOCK_NUM;
    superblock->s_SUPERBLOCK_SIZE = sizeof(SuperBlock);
    superblock->s_INODE_SIZE = INODE_SIZE;
    superblock->s_BLOCK_SIZE = BLOCK_SIZE;
    superblock->s_free_INODE_NUM = INODE_NUM;
    superblock->s_free_BLOCK_NUM = BLOCK_NUM;
    superblock->s_blocks_per_group = BLOCKS_PER_GROUP;
    superblock->s_free_addr = Block_StartAddr;	//空闲块堆栈指针为第一块block
    superblock->s_Superblock_StartAddr = Superblock_StartAddr;
    superblock->s_BlockBitmap_StartAddr = BlockBitmap_StartAddr;
    superblock->s_InodeBitmap_StartAddr = InodeBitmap_StartAddr;
    superblock->s_Block_StartAddr = Block_StartAddr;
    superblock->s_Inode_StartAddr = Inode_StartAddr;
    //空闲块堆栈在后面赋值

    //初始化inode位图
    memset(inode_bitmap,0,sizeof(inode_bitmap));
    fseek(fw,InodeBitmap_StartAddr,SEEK_SET);
    fwrite(inode_bitmap,sizeof(inode_bitmap),1,fw);

    //初始化block位图
    memset(block_bitmap,0,sizeof(block_bitmap));
    fseek(fw,BlockBitmap_StartAddr,SEEK_SET);
    fwrite(block_bitmap,sizeof(block_bitmap),1,fw);

    //初始化磁盘块区，根据成组链接法组织
    for(i=BLOCK_NUM/BLOCKS_PER_GROUP-1;i>=0;i--){	//一共INODE_NUM/BLOCKS_PER_GROUP组，一组FREESTACKNUM（128）个磁盘块 ，第一个磁盘块作为索引
        if(i==BLOCK_NUM/BLOCKS_PER_GROUP-1)
            superblock->s_free[0] = -1;	//没有下一个空闲块了
        else
            superblock->s_free[0] = Block_StartAddr + (i+1)*BLOCKS_PER_GROUP*BLOCK_SIZE;	//指向下一个空闲块
        for(j=1;j<BLOCKS_PER_GROUP;j++){
            superblock->s_free[j] = Block_StartAddr + (i*BLOCKS_PER_GROUP + j)*BLOCK_SIZE;
        }
        fseek(fw,Block_StartAddr+i*BLOCKS_PER_GROUP*BLOCK_SIZE,SEEK_SET);
        fwrite(superblock->s_free,sizeof(superblock->s_free),1,fw);	//填满这个磁盘块，512字节
    }
    //超级块写入到虚拟磁盘文件
    fseek(fw,Superblock_StartAddr,SEEK_SET);
    fwrite(superblock,sizeof(SuperBlock),1,fw);

    fflush(fw);

    //读取inode位图
    fseek(fr,InodeBitmap_StartAddr,SEEK_SET);
    fread(inode_bitmap,sizeof(inode_bitmap),1,fr);

    //读取block位图
    fseek(fr,BlockBitmap_StartAddr,SEEK_SET);
    fread(block_bitmap,sizeof(block_bitmap),1,fr);

    fflush(fr);

    //创建根目录 "/"
    Inode cur;

    //申请inode
    int inoAddr = ialloc();

    //给这个inode申请磁盘块
    int blockAddr = balloc();

    //在这个磁盘块里加入一个条目 "."
    dentry dirlist[16] = {0};
    strcpy(dirlist[0].itemName,".");
    dirlist[0].inodeAddr = inoAddr;

    //写回磁盘块
    fseek(fw,blockAddr,SEEK_SET);
    fwrite(dirlist,sizeof(dirlist),1,fw);

    //给inode赋值
    cur.i_ino = 0;
    cur.i_atime = time(NULL);
    cur.i_ctime = time(NULL);
    cur.i_mtime = time(NULL);
    strcpy(cur.i_uname,Cur_User_Name);
    strcpy(cur.i_gname,Cur_Group_Name);
    cur.i_cnt = 1;	//一个项，当前目录,"."
    cur.i_dirBlock[0] = blockAddr;
    for(i=1;i<10;i++){
        cur.i_dirBlock[i] = -1;
    }
    cur.i_size = superblock->s_BLOCK_SIZE;
    cur.i_indirBlock_1 = -1;	//没使用一级间接块
    cur.i_mode = MODE_DIR | DIR_DEF_PERMISSION;


    //写回inode
    fseek(fw,inoAddr,SEEK_SET);
    fwrite(&cur,sizeof(Inode),1,fw);
    fflush(fw);

    //创建目录及配置文件
    mkdir(Root_Dir_Addr,"home");	//用户目录
    cd(Root_Dir_Addr,"home");
    mkdir(Cur_Dir_Addr,"root");

    cd(Cur_Dir_Addr,"..");
    mkdir(Cur_Dir_Addr,"etc");	//配置文件目录
    cd(Cur_Dir_Addr,"etc");

    char buf[1000] = {0};

    sprintf(buf,"root:x:%d:%d\n",nextUID++,nextGID++);	//增加条目，用户名：加密密码：用户ID：用户组ID
    create(Cur_Dir_Addr,"passwd",buf);	//创建用户信息文件

    sprintf(buf,"root:root\n");	//增加条目，用户名：密码
    create(Cur_Dir_Addr,"shadow",buf);	//创建用户密码文件

    sprintf(buf,"root::0:root\n");	//增加管理员用户组，用户组名：口令（一般为空，这里没有使用）：组标识号：组内用户列表（用，分隔）
    sprintf(buf+strlen(buf),"user::1:\n");	//增加普通用户组，组内用户列表为空
    create(Cur_Dir_Addr,"group",buf);	//创建用户组信息文件


    cd(Cur_Dir_Addr,"..");	//回到根目录

    return true;
}

bool Install()	//安装文件系统，将虚拟磁盘文件中的关键信息如超级块读入到内存
{
    //读写虚拟磁盘文件，读取超级块，读取inode位图，block位图等

    //读取超级块
    fseek(fr,Superblock_StartAddr,SEEK_SET);
    fread(superblock,sizeof(SuperBlock),1,fr);

    //读取inode位图
    fseek(fr,InodeBitmap_StartAddr,SEEK_SET);
    fread(inode_bitmap,sizeof(inode_bitmap),1,fr);

    //读取block位图
    fseek(fr,BlockBitmap_StartAddr,SEEK_SET);
    fread(block_bitmap,sizeof(block_bitmap),1,fr);

    //todo:读取目录信息生成红黑树

    //插入根目录
    gotoRoot();
    rbtree_node *root = new rbtree_node;
    root->key = Cur_Dir_Name;
    root->value = Cur_Dir_Addr;
    rbtree_insert(&dir_tree,root);
    //遍历所有子目录,构建红黑树
    dir_traversal(Cur_Dir_Addr,"/");
    while(!dir_queue.empty()){
        rbtree_node temp =dir_queue.front();
        dir_queue.pop_front();
        dir_traversal(temp.value,temp.key + "/");
    }

    return true;
    }




void printSuperBlock()		//打印超级块信息
{
    printf("\n");
    printf("空闲inode数 / 总inode数 ：%d / %d\n",superblock->s_free_INODE_NUM,superblock->s_INODE_NUM);
    printf("空闲block数 / 总block数 ：%d / %d\n",superblock->s_free_BLOCK_NUM,superblock->s_BLOCK_NUM);
    printf("本系统 block大小：%d 字节，每个inode占 %d 字节（真实大小：%d 字节）\n",superblock->s_BLOCK_SIZE,superblock->s_INODE_SIZE,sizeof(Inode));
    printf("\t每磁盘块组（空闲堆栈）包含的block数量：%d\n",superblock->s_blocks_per_group);
    printf("\t超级块占 %d 字节（真实大小：%d 字节）\n",superblock->s_BLOCK_SIZE,superblock->s_SUPERBLOCK_SIZE);
    printf("磁盘分布：\n");
    printf("\t超级块开始位置：%d B\n",superblock->s_Superblock_StartAddr);
    printf("\tinode位图开始位置：%d B\n",superblock->s_InodeBitmap_StartAddr);
    printf("\tblock位图开始位置：%d B\n",superblock->s_BlockBitmap_StartAddr);
    printf("\tinode区开始位置：%d B\n",superblock->s_Inode_StartAddr);
    printf("\tblock区开始位置：%d B\n",superblock->s_Block_StartAddr);
    printf("\n");

    return ;
}

void printInodeBitmap()	//打印inode使用情况
{
    printf("\n");
    printf("inode使用表：[uesd:%d %d/%d]\n",superblock->s_INODE_NUM-superblock->s_free_INODE_NUM,superblock->s_free_INODE_NUM,superblock->s_INODE_NUM);
    int i;
    i = 0;
    printf("0 ");
    while(i<superblock->s_INODE_NUM){
        if(inode_bitmap[i])
            printf("*");
        else
            printf(".");
        i++;
        if(i!=0 && i%32==0){
            printf("\n");
            if(i!=superblock->s_INODE_NUM)
                printf("%d ",i/32);
        }
    }
    printf("\n");
    printf("\n");
    return ;
}

void printBlockBitmap(int num)	//打印block使用情况
{
    printf("\n");
    printf("block（磁盘块）使用表：[used:%d %d/%d]\n",superblock->s_BLOCK_NUM-superblock->s_free_BLOCK_NUM,superblock->s_free_BLOCK_NUM,superblock->s_BLOCK_NUM);
    int i;
    i = 0;
    printf("0 ");
    while(i<num){
        if(block_bitmap[i])
            printf("*");
        else
            printf(".");
        i++;
        if(i!=0 && i%32==0){
            printf("\n");
            if(num==superblock->s_BLOCK_NUM)
                getchar();
            if(i!=superblock->s_BLOCK_NUM)
                printf("%d ",i/32);
        }
    }
    printf("\n");
    printf("\n");
    return ;
}

int balloc()	//磁盘块分配函数
{
    //使用超级块中的空闲块堆栈
    //计算当前栈顶
    int top;	//栈顶指针
    if(superblock->s_free_BLOCK_NUM==0){	//剩余空闲块数为0
        printf("没有空闲块可以分配\n");
        return 0;	//没有可分配的空闲块，返回0
    }
    else{	//还有剩余块
        top = (superblock->s_free_BLOCK_NUM-1) % superblock->s_blocks_per_group;
    }
    //将栈顶取出
    //如果已是栈底，将当前块号地址返回，即为栈底块号，并将栈底指向的新空闲块堆栈覆盖原来的栈
    int retAddr;

    if(top==0){
        retAddr = superblock->s_free_addr;
        superblock->s_free_addr = superblock->s_free[0];	//取出下一个存有空闲块堆栈的空闲块的位置，更新空闲块堆栈指针

        //取出对应空闲块内容，覆盖原来的空闲块堆栈

        //取出下一个空闲块堆栈，覆盖原来的
        fseek(fr,superblock->s_free_addr,SEEK_SET);
        fread(superblock->s_free,sizeof(superblock->s_free),1,fr);
        fflush(fr);

        superblock->s_free_BLOCK_NUM--;

    }
    else{	//如果不为栈底，则将栈顶指向的地址返回，栈顶指针-1.
        retAddr = superblock->s_free[top];	//保存返回地址
        superblock->s_free[top] = -1;	//清栈顶
        top--;		//栈顶指针-1
        superblock->s_free_BLOCK_NUM--;	//空闲块数-1

    }

    //更新超级块
    fseek(fw,Superblock_StartAddr,SEEK_SET);
    fwrite(superblock,sizeof(SuperBlock),1,fw);
    fflush(fw);

    //更新block位图
    block_bitmap[(retAddr-Block_StartAddr)/BLOCK_SIZE] = 1;
    fseek(fw,(retAddr-Block_StartAddr)/BLOCK_SIZE+BlockBitmap_StartAddr,SEEK_SET);	//(retAddr-Block_StartAddr)/BLOCK_SIZE为第几个空闲块
    fwrite(&block_bitmap[(retAddr-Block_StartAddr)/BLOCK_SIZE],sizeof(bool),1,fw);
    fflush(fw);

    return retAddr;

}

bool bfree(int addr)	//磁盘块释放函数
{
    //判断
    //该地址不是磁盘块的起始地址
    if( (addr-Block_StartAddr) % superblock->s_BLOCK_SIZE != 0 ){
        printf("地址错误,该位置不是block（磁盘块）起始位置\n");
        return false;
    }
    unsigned int bno = (addr-Block_StartAddr) / superblock->s_BLOCK_SIZE;	//inode节点号
    //该地址还未使用，不能释放空间
    if(block_bitmap[bno]==0){
        printf("该block（磁盘块）还未使用，无法释放\n");
        return false;
    }

    //可以释放
    //计算当前栈顶
    int top;	//栈顶指针
    if(superblock->s_free_BLOCK_NUM==superblock->s_BLOCK_NUM){	//没有非空闲的磁盘块
        printf("没有非空闲的磁盘块，无法释放\n");
        return false;	//没有可分配的空闲块，返回-1
    }
    else{	//非满
        top = (superblock->s_free_BLOCK_NUM-1) % superblock->s_blocks_per_group;

        //清空block内容
        char tmp[BLOCK_SIZE] = {0};
        fseek(fw,addr,SEEK_SET);
        fwrite(tmp,sizeof(tmp),1,fw);

        if(top == superblock->s_blocks_per_group-1){	//该栈已满

            //该空闲块作为新的空闲块堆栈
            superblock->s_free[0] = superblock->s_free_addr;	//新的空闲块堆栈第一个地址指向旧的空闲块堆栈指针
            int i;
            for(i=1;i<superblock->s_blocks_per_group;i++){
                superblock->s_free[i] = -1;	//清空栈元素的其它地址
            }
            fseek(fw,addr,SEEK_SET);
            fwrite(superblock->s_free,sizeof(superblock->s_free),1,fw);	//填满这个磁盘块，512字节

        }
        else{	//栈还未满
            top++;	//栈顶指针+1
            superblock->s_free[top] = addr;	//栈顶放上这个要释放的地址，作为新的空闲块
        }
    }


    //更新超级块
    superblock->s_free_BLOCK_NUM++;	//空闲块数+1
    fseek(fw,Superblock_StartAddr,SEEK_SET);
    fwrite(superblock,sizeof(SuperBlock),1,fw);

    //更新block位图
    block_bitmap[bno] = 0;
    fseek(fw,bno+BlockBitmap_StartAddr,SEEK_SET);	//(addr-Block_StartAddr)/BLOCK_SIZE为第几个空闲块
    fwrite(&block_bitmap[bno],sizeof(bool),1,fw);
    fflush(fw);

    return true;
}

int ialloc()	//分配i节点区函数，返回inode地址
{
    //在inode位图中顺序查找空闲的inode，找到则返回inode地址。函数结束。
    if(superblock->s_free_INODE_NUM==0){
        printf("没有空闲inode可以分配\n");
        return 0;
    }
    else{

        //顺序查找空闲的inode
        int i;
        for(i=0;i<superblock->s_INODE_NUM;i++){
            if(inode_bitmap[i]==0)	//找到空闲inode
                break;
        }


        //更新超级块
        superblock->s_free_INODE_NUM--;	//空闲inode数-1
        fseek(fw,Superblock_StartAddr,SEEK_SET);
        fwrite(superblock,sizeof(SuperBlock),1,fw);

        //更新inode位图
        inode_bitmap[i] = 1;
        fseek(fw,InodeBitmap_StartAddr+i,SEEK_SET);
        fwrite(&inode_bitmap[i],sizeof(bool),1,fw);
        fflush(fw);

        return Inode_StartAddr + i*superblock->s_INODE_SIZE;
    }
}

bool ifree(int addr)	//释放i结点区函数
{
    //判断
    if( (addr-Inode_StartAddr) % superblock->s_INODE_SIZE != 0 ){
        printf("地址错误,该位置不是i节点起始位置\n");
        return false;
    }
    unsigned short ino = (addr-Inode_StartAddr) / superblock->s_INODE_SIZE;	//inode节点号
    if(inode_bitmap[ino]==0){
        printf("该inode还未使用，无法释放\n");
        return false;
    }

    //清空inode内容
    Inode tmp = {0};
    fseek(fw,addr,SEEK_SET);
    fwrite(&tmp,sizeof(tmp),1,fw);

    //更新超级块
    superblock->s_free_INODE_NUM++;
    //空闲inode数+1
    fseek(fw,Superblock_StartAddr,SEEK_SET);
    fwrite(superblock,sizeof(SuperBlock),1,fw);

    //更新inode位图
    inode_bitmap[ino] = 0;
    fseek(fw,InodeBitmap_StartAddr+ino,SEEK_SET);
    fwrite(&inode_bitmap[ino],sizeof(bool),1,fw);
    fflush(fw);

    return true;
}
