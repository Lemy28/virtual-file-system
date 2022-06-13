#include "MingOS.h"
#include "rbtree.h"
using namespace std;

//全局变量定义
const int Superblock_StartAddr = 0;																//超级块 偏移地址,占一个磁盘块
const int InodeBitmap_StartAddr = 1*BLOCK_SIZE;													//inode位图 偏移地址，占两个磁盘块，最多监控 1024 个inode的状态
const int BlockBitmap_StartAddr = InodeBitmap_StartAddr + 2*BLOCK_SIZE;							//block位图 偏移地址，占二十个磁盘块，最多监控 10240 个磁盘块（5120KB）的状态
const int Inode_StartAddr = BlockBitmap_StartAddr + 20*BLOCK_SIZE;								//inode节点区 偏移地址，占 INODE_NUM/(BLOCK_SIZE/INODE_SIZE) 个磁盘块
const int Block_StartAddr = Inode_StartAddr + INODE_NUM/(BLOCK_SIZE/INODE_SIZE) * BLOCK_SIZE;	//block数据区 偏移地址 ，占 INODE_NUM 个磁盘块

const int Sum_Size = Block_StartAddr + BLOCK_NUM * BLOCK_SIZE;									//虚拟磁盘文件大小


rbtree dir_tree;                            //目录树
int Root_Dir_Addr;							//根目录inode地址
int Cur_Dir_Addr;							//当前目录
char Cur_Dir_Name[100];						//当前目录名
char Cur_Host_Name[110];					//当前主机名
char Cur_User_Name[110];					//当前登陆用户名
char Cur_Group_Name[110];					//当前登陆用户组名
char Cur_User_Dir_Name[310];				//当前登陆用户目录名

int nextUID;								//下一个要分配的用户标识号
int nextGID;								//下一个要分配的用户组标识号


FILE* fw;									//虚拟磁盘文件 写文件指针
FILE* fr;									//虚拟磁盘文件 读文件指针
SuperBlock *superblock = new SuperBlock;	//超级块指针
bool inode_bitmap[INODE_NUM];				//inode位图
bool block_bitmap[BLOCK_NUM];				//磁盘块位图

char buffer[10000000] = {0};				//10M，缓存整个虚拟磁盘文件


int main()
{
	//打开虚拟磁盘文件
	if( (fr = fopen(FILESYSNAME,"rb"))==NULL){	//只读打开虚拟磁盘文件（二进制文件）
		//虚拟磁盘文件不存在，创建一个
		fw = fopen(FILESYSNAME,"wb");	//只写打开虚拟磁盘文件（二进制文件）
		if(fw==NULL){
			printf("虚拟磁盘文件打开失败\n");
			return 0;	//打开文件失败
		}
		fr = fopen(FILESYSNAME,"rb");


		//初始化变量
		nextUID = 0;
		nextGID = 0;
		strcpy(Cur_User_Name,"root");
		strcpy(Cur_Group_Name,"root");

		//获取主机名
		memset(Cur_Host_Name,0,sizeof(Cur_Host_Name));
		DWORD k= 100;//用于指定缓冲区大小
		GetComputerName(Cur_Host_Name,&k);

		//根目录inode地址 ，当前目录地址和名字
		Root_Dir_Addr = Inode_StartAddr;	//第一个inode地址
		Cur_Dir_Addr = Root_Dir_Addr;
		strcpy(Cur_Dir_Name,"/");

		printf("文件系统正在格式化……\n");
		if(!Format()){
			printf("文件系统格式化失败\n");
			return 0;
		}
		printf("格式化完成\n");
		printf("按任意键进入\n");
		system("pause");
		system("cls");


		if(!Install()){			
			printf("安装文件系统失败\n");
			return 0;
		}
	}
	else{	//虚拟磁盘文件已存在,将其缓存到内存中
		fread(buffer,Sum_Size,1,fr);

		//取出文件内容暂存到内容中，以写方式打开文件之后再写回文件（写方式打开会清空文件）
		fw = fopen(FILESYSNAME,"wb");	//只写打开虚拟磁盘文件（二进制文件）
		if(fw==NULL){
			printf("虚拟磁盘文件打开失败\n");
			return false;	//打开文件失败
		}
		fwrite(buffer,Sum_Size,1,fw);


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

		if(!Install()){
			printf("安装文件系统失败\n");
			return 0;
		}

	}

	while(1){
			char str[100];
			printf("[%s@%s ~%s]# ",Cur_Host_Name,Cur_User_Name,Cur_Dir_Name+strlen(Cur_User_Dir_Name));
			gets(str);
			cmd(str);

	}


}
