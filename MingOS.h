#include <iostream>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>
#include "SuperBlock.h"
#include "Inode.h"
#include "rbtree.h"

//�궨��
#define BLOCK_SIZE	512						//��Ŵ�СΪ512B
#define INODE_SIZE	128						//inode�ڵ��СΪ128B��ע��sizeof(Inode)���ܳ�����ֵ

#define INODE_NUM	640						//inode�ڵ���,���64���ļ�
#define BLOCK_NUM	10240					//�������10240 * 512B = 5120KB

#define MODE_DIR	01000					//Ŀ¼��ʶ
#define MODE_FILE	00000					//�ļ���ʶ
#define OWNER_R	4<<6						//���û���Ȩ��
#define OWNER_W	2<<6						//���û�дȨ��
#define OWNER_X	1<<6						//���û�ִ��Ȩ��
#define GROUP_R	4<<3						//���û���Ȩ��
#define GROUP_W	2<<3						//���û�дȨ��
#define GROUP_X	1<<3						//���û�ִ��Ȩ��
#define OTHERS_R	4						//�����û���Ȩ��
#define OTHERS_W	2						//�����û�дȨ��
#define OTHERS_X	1						//�����û�ִ��Ȩ��
#define FILE_DEF_PERMISSION 0664			//�ļ�Ĭ��Ȩ��
#define DIR_DEF_PERMISSION	0755			//Ŀ¼Ĭ��Ȩ��

#define FILESYSNAME	"virtualdisk"			//��������ļ���



//ȫ�ֱ�������
extern SuperBlock *superblock;
extern const int Inode_StartAddr;
extern const int Superblock_StartAddr;		//������ ƫ�Ƶ�ַ,ռһ�����̿�
extern const int InodeBitmap_StartAddr;		//inodeλͼ ƫ�Ƶ�ַ��ռ�������̿飬����� 1024 ��inode��״̬
extern const int BlockBitmap_StartAddr;		//blockλͼ ƫ�Ƶ�ַ��ռ��ʮ�����̿飬����� 10240 �����̿飨5120KB����״̬
extern const int Inode_StartAddr;			//inode�ڵ��� ƫ�Ƶ�ַ��ռ INODE_NUM/(BLOCK_SIZE/INODE_SIZE) �����̿�
extern const int Block_StartAddr;			//block������ ƫ�Ƶ�ַ ��ռ INODE_NUM �����̿�
extern const int File_Max_Size;				//�����ļ�����С
extern const int Sum_Size;					//��������ļ���С


//ȫ�ֱ�������
extern rbtree dir_tree;                     //Ŀ¼��
extern int Root_Dir_Addr;					//��Ŀ¼inode��ַ
extern int Cur_Dir_Addr;					//��ǰĿ¼
extern char Cur_Dir_Name[100];				//��ǰĿ¼��
extern char Cur_Host_Name[110];				//��ǰ������
extern char Cur_User_Name[110];				//��ǰ��½�û���
extern char Cur_Group_Name[110];			//��ǰ��½�û�����
extern char Cur_User_Dir_Name[310];			//��ǰ��½�û�Ŀ¼��

extern int nextUID;							//��һ��Ҫ������û���ʶ��
extern int nextGID;							//��һ��Ҫ������û����ʶ��


extern FILE* fw;							//��������ļ� д�ļ�ָ��
extern FILE* fr;							//��������ļ� ���ļ�ָ��
extern SuperBlock *superblock;				//������ָ��
extern bool inode_bitmap[INODE_NUM];		//inodeλͼ
extern bool block_bitmap[BLOCK_NUM];		//���̿�λͼ

extern char buffer[10000000];				//10M������������������ļ�


//��������
void Ready();													//��¼ϵͳǰ��׼������,ע��+��װ
bool Format();													//��ʽ��һ����������ļ�
bool Install();													//��װ�ļ�ϵͳ������������ļ��еĹؼ���Ϣ�糬������뵽�ڴ�
void printSuperBlock();											//��ӡ��������Ϣ
void printInodeBitmap();										//��ӡinodeʹ�����
void printBlockBitmap(int num = superblock->s_BLOCK_NUM);		//��ӡblockʹ�����
int	 balloc();													//���̿���亯��
bool bfree(int);													//���̿��ͷź���
int  ialloc();													//����i�ڵ�������
bool ifree(int);													//�ͷ�i���������
bool mkdir(int parinoAddr,char name[]);							//Ŀ¼������������������һ��Ŀ¼�ļ�inode��ַ ,Ҫ������Ŀ¼��
bool rmdir(int parinoAddr,char name[]);							//Ŀ¼ɾ������
bool create(int parinoAddr,char name[],char buf[]);				//�����ļ�����
bool del(int parinoAddr,char name[]);							//ɾ���ļ�����
void ls(int parinoaddr);										//��ʾ��ǰĿ¼�µ������ļ����ļ���
void cd(int parinoaddr,char name[]);							//���뵱ǰĿ¼�µ�nameĿ¼
void gotoxy(HANDLE hOut, int x, int y);							//�ƶ���굽ָ��λ��
void vi(int parinoaddr,char name[],char buf[]);					//ģ��һ����vi�������ı�
void writefile(Inode fileInode,int fileInodeAddr,char buf[]);	//��buf����д���ļ��Ĵ��̿�
void gotoRoot();												//�ص���Ŀ¼
void touch(int parinoAddr,char name[],char buf[]);				//touch������ļ��������ַ�
void help();													//��ʾ���������嵥

void cmd(char str[]);											//�������������
