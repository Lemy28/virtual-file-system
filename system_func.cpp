#include <deque>
#include <algorithm>
#include "MingOS.h"
#include "dentry.h"
#include "rbtree.h"
using namespace std;

deque<rbtree_node> dir_queue;
//����Ŀ¼��ַ��Ŀ¼��,�ú����ɽ�Ŀ¼�ڵ�����Ŀ¼��������
void dir_traversal(int parinoAddr,string dirname){

    Inode curinode;
    //ȡ��inode
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&curinode, sizeof(Inode), 1, fr);
    fflush(fr);

    //ȡ��Ŀ¼����
    int dentrycnt = curinode.i_cnt;
    if(curinode.i_cnt == 2)return;

    //����ȡ�����̿�
    int i = 0;
    while(i < dentrycnt&&i<160){
        dentry dirlist[16] = {0};
        if(curinode.i_dirBlock[i / 16] == -1){
            i+=16;
            continue;
        }
        //ȡ�����̿�
        int parblockAddr = curinode.i_dirBlock[i / 16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //����ô��̿��е�����Ŀ¼��
        int j;
        for(j=0;j<16 && i < dentrycnt; j++){


            Inode tmp;
            //ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
            fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
            fread(&tmp,sizeof(Inode),1,fr);
            fflush(fr);

            if( strcmp(dirlist[j].itemName,"")==0 || strcmp(dirlist[j].itemName,".")==0 || strcmp(dirlist[j].itemName,"..")==0){
                continue;
            }
            //�ҵ�Ŀ¼,���뵽�����
            if( ( (tmp.i_mode>>9) & 1 ) == 1 ){

                rbtree_node* p = new rbtree_node;
                p->key = dirname+dirlist[j].itemName;
                p->value = dirlist[j].inodeAddr;
                rbtree_insert(&dir_tree,p);

                //����Ŀ¼�����
                deque<rbtree_node>::iterator it = find(dir_queue.begin(),dir_queue.end(),*p);
                if(it == dir_queue.end()){
                    //������û�и�Ŀ¼,�������
                    dir_queue.push_back(*p);
                }


            }

            i++;
        }

    }

}





void Ready()	//��¼ϵͳǰ��׼������,������ʼ��+ע��+��װ
{
    //��ʼ������
    nextUID = 0;
    nextGID = 0;
    strcpy(Cur_User_Name,"root");
    strcpy(Cur_Group_Name,"root");

    //��ȡ������
    memset(Cur_Host_Name,0,sizeof(Cur_Host_Name));
    DWORD k= 100;
    GetComputerName(Cur_Host_Name,&k);

    //��Ŀ¼inode��ַ ����ǰĿ¼��ַ������
    Root_Dir_Addr = Inode_StartAddr;	//��һ��inode��ַ
    Cur_Dir_Addr = Root_Dir_Addr;
    strcpy(Cur_Dir_Name,"/");


    char c;
    printf("�Ƿ��ʽ��?[y/n]");
    while(c = getch()){
        fflush(stdin);
        if(c=='y'){
            printf("\n");
            printf("�ļ�ϵͳ���ڸ�ʽ������\n");
            if(!Format()){
                printf("�ļ�ϵͳ��ʽ��ʧ��\n");
                return ;
            }
            printf("��ʽ�����\n");
            break;
        }
        else if(c=='n'){
            printf("\n");
            break;
        }
    }

    //printf("�����ļ�ϵͳ����\n");
    if(!Install()){
        printf("��װ�ļ�ϵͳʧ��\n");
        return ;
    }
    //printf("�������\n");
}

bool Format()	//��ʽ��һ����������ļ�
{
    int i,j;

    //��ʼ��������
    superblock->s_INODE_NUM = INODE_NUM;
    superblock->s_BLOCK_NUM = BLOCK_NUM;
    superblock->s_SUPERBLOCK_SIZE = sizeof(SuperBlock);
    superblock->s_INODE_SIZE = INODE_SIZE;
    superblock->s_BLOCK_SIZE = BLOCK_SIZE;
    superblock->s_free_INODE_NUM = INODE_NUM;
    superblock->s_free_BLOCK_NUM = BLOCK_NUM;
    superblock->s_blocks_per_group = BLOCKS_PER_GROUP;
    superblock->s_free_addr = Block_StartAddr;	//���п��ջָ��Ϊ��һ��block
    superblock->s_Superblock_StartAddr = Superblock_StartAddr;
    superblock->s_BlockBitmap_StartAddr = BlockBitmap_StartAddr;
    superblock->s_InodeBitmap_StartAddr = InodeBitmap_StartAddr;
    superblock->s_Block_StartAddr = Block_StartAddr;
    superblock->s_Inode_StartAddr = Inode_StartAddr;
    //���п��ջ�ں��渳ֵ

    //��ʼ��inodeλͼ
    memset(inode_bitmap,0,sizeof(inode_bitmap));
    fseek(fw,InodeBitmap_StartAddr,SEEK_SET);
    fwrite(inode_bitmap,sizeof(inode_bitmap),1,fw);

    //��ʼ��blockλͼ
    memset(block_bitmap,0,sizeof(block_bitmap));
    fseek(fw,BlockBitmap_StartAddr,SEEK_SET);
    fwrite(block_bitmap,sizeof(block_bitmap),1,fw);

    //��ʼ�����̿��������ݳ������ӷ���֯
    for(i=BLOCK_NUM/BLOCKS_PER_GROUP-1;i>=0;i--){	//һ��INODE_NUM/BLOCKS_PER_GROUP�飬һ��FREESTACKNUM��128�������̿� ����һ�����̿���Ϊ����
        if(i==BLOCK_NUM/BLOCKS_PER_GROUP-1)
            superblock->s_free[0] = -1;	//û����һ�����п���
        else
            superblock->s_free[0] = Block_StartAddr + (i+1)*BLOCKS_PER_GROUP*BLOCK_SIZE;	//ָ����һ�����п�
        for(j=1;j<BLOCKS_PER_GROUP;j++){
            superblock->s_free[j] = Block_StartAddr + (i*BLOCKS_PER_GROUP + j)*BLOCK_SIZE;
        }
        fseek(fw,Block_StartAddr+i*BLOCKS_PER_GROUP*BLOCK_SIZE,SEEK_SET);
        fwrite(superblock->s_free,sizeof(superblock->s_free),1,fw);	//����������̿飬512�ֽ�
    }
    //������д�뵽��������ļ�
    fseek(fw,Superblock_StartAddr,SEEK_SET);
    fwrite(superblock,sizeof(SuperBlock),1,fw);

    fflush(fw);

    //��ȡinodeλͼ
    fseek(fr,InodeBitmap_StartAddr,SEEK_SET);
    fread(inode_bitmap,sizeof(inode_bitmap),1,fr);

    //��ȡblockλͼ
    fseek(fr,BlockBitmap_StartAddr,SEEK_SET);
    fread(block_bitmap,sizeof(block_bitmap),1,fr);

    fflush(fr);

    //������Ŀ¼ "/"
    Inode cur;

    //����inode
    int inoAddr = ialloc();

    //�����inode������̿�
    int blockAddr = balloc();

    //��������̿������һ����Ŀ "."
    dentry dirlist[16] = {0};
    strcpy(dirlist[0].itemName,".");
    dirlist[0].inodeAddr = inoAddr;

    //д�ش��̿�
    fseek(fw,blockAddr,SEEK_SET);
    fwrite(dirlist,sizeof(dirlist),1,fw);

    //��inode��ֵ
    cur.i_ino = 0;
    cur.i_atime = time(NULL);
    cur.i_ctime = time(NULL);
    cur.i_mtime = time(NULL);
    strcpy(cur.i_uname,Cur_User_Name);
    strcpy(cur.i_gname,Cur_Group_Name);
    cur.i_cnt = 1;	//һ�����ǰĿ¼,"."
    cur.i_dirBlock[0] = blockAddr;
    for(i=1;i<10;i++){
        cur.i_dirBlock[i] = -1;
    }
    cur.i_size = superblock->s_BLOCK_SIZE;
    cur.i_indirBlock_1 = -1;	//ûʹ��һ����ӿ�
    cur.i_mode = MODE_DIR | DIR_DEF_PERMISSION;


    //д��inode
    fseek(fw,inoAddr,SEEK_SET);
    fwrite(&cur,sizeof(Inode),1,fw);
    fflush(fw);

    //����Ŀ¼�������ļ�
    mkdir(Root_Dir_Addr,"home");	//�û�Ŀ¼
    cd(Root_Dir_Addr,"home");
    mkdir(Cur_Dir_Addr,"root");

    cd(Cur_Dir_Addr,"..");
    mkdir(Cur_Dir_Addr,"etc");	//�����ļ�Ŀ¼
    cd(Cur_Dir_Addr,"etc");

    char buf[1000] = {0};

    sprintf(buf,"root:x:%d:%d\n",nextUID++,nextGID++);	//������Ŀ���û������������룺�û�ID���û���ID
    create(Cur_Dir_Addr,"passwd",buf);	//�����û���Ϣ�ļ�

    sprintf(buf,"root:root\n");	//������Ŀ���û���������
    create(Cur_Dir_Addr,"shadow",buf);	//�����û������ļ�

    sprintf(buf,"root::0:root\n");	//���ӹ���Ա�û��飬�û����������һ��Ϊ�գ�����û��ʹ�ã������ʶ�ţ������û��б��ã��ָ���
    sprintf(buf+strlen(buf),"user::1:\n");	//������ͨ�û��飬�����û��б�Ϊ��
    create(Cur_Dir_Addr,"group",buf);	//�����û�����Ϣ�ļ�


    cd(Cur_Dir_Addr,"..");	//�ص���Ŀ¼

    return true;
}

bool Install()	//��װ�ļ�ϵͳ������������ļ��еĹؼ���Ϣ�糬������뵽�ڴ�
{
    //��д��������ļ�����ȡ�����飬��ȡinodeλͼ��blockλͼ��

    //��ȡ������
    fseek(fr,Superblock_StartAddr,SEEK_SET);
    fread(superblock,sizeof(SuperBlock),1,fr);

    //��ȡinodeλͼ
    fseek(fr,InodeBitmap_StartAddr,SEEK_SET);
    fread(inode_bitmap,sizeof(inode_bitmap),1,fr);

    //��ȡblockλͼ
    fseek(fr,BlockBitmap_StartAddr,SEEK_SET);
    fread(block_bitmap,sizeof(block_bitmap),1,fr);

    //todo:��ȡĿ¼��Ϣ���ɺ����

    //�����Ŀ¼
    gotoRoot();
    rbtree_node *root = new rbtree_node;
    root->key = Cur_Dir_Name;
    root->value = Cur_Dir_Addr;
    rbtree_insert(&dir_tree,root);
    //����������Ŀ¼,���������
    dir_traversal(Cur_Dir_Addr,"/");
    while(!dir_queue.empty()){
        rbtree_node temp =dir_queue.front();
        dir_queue.pop_front();
        dir_traversal(temp.value,temp.key + "/");
    }

    return true;
    }




void printSuperBlock()		//��ӡ��������Ϣ
{
    printf("\n");
    printf("����inode�� / ��inode�� ��%d / %d\n",superblock->s_free_INODE_NUM,superblock->s_INODE_NUM);
    printf("����block�� / ��block�� ��%d / %d\n",superblock->s_free_BLOCK_NUM,superblock->s_BLOCK_NUM);
    printf("��ϵͳ block��С��%d �ֽڣ�ÿ��inodeռ %d �ֽڣ���ʵ��С��%d �ֽڣ�\n",superblock->s_BLOCK_SIZE,superblock->s_INODE_SIZE,sizeof(Inode));
    printf("\tÿ���̿��飨���ж�ջ��������block������%d\n",superblock->s_blocks_per_group);
    printf("\t������ռ %d �ֽڣ���ʵ��С��%d �ֽڣ�\n",superblock->s_BLOCK_SIZE,superblock->s_SUPERBLOCK_SIZE);
    printf("���̷ֲ���\n");
    printf("\t�����鿪ʼλ�ã�%d B\n",superblock->s_Superblock_StartAddr);
    printf("\tinodeλͼ��ʼλ�ã�%d B\n",superblock->s_InodeBitmap_StartAddr);
    printf("\tblockλͼ��ʼλ�ã�%d B\n",superblock->s_BlockBitmap_StartAddr);
    printf("\tinode����ʼλ�ã�%d B\n",superblock->s_Inode_StartAddr);
    printf("\tblock����ʼλ�ã�%d B\n",superblock->s_Block_StartAddr);
    printf("\n");

    return ;
}

void printInodeBitmap()	//��ӡinodeʹ�����
{
    printf("\n");
    printf("inodeʹ�ñ�[uesd:%d %d/%d]\n",superblock->s_INODE_NUM-superblock->s_free_INODE_NUM,superblock->s_free_INODE_NUM,superblock->s_INODE_NUM);
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

void printBlockBitmap(int num)	//��ӡblockʹ�����
{
    printf("\n");
    printf("block�����̿飩ʹ�ñ�[used:%d %d/%d]\n",superblock->s_BLOCK_NUM-superblock->s_free_BLOCK_NUM,superblock->s_free_BLOCK_NUM,superblock->s_BLOCK_NUM);
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

int balloc()	//���̿���亯��
{
    //ʹ�ó������еĿ��п��ջ
    //���㵱ǰջ��
    int top;	//ջ��ָ��
    if(superblock->s_free_BLOCK_NUM==0){	//ʣ����п���Ϊ0
        printf("û�п��п���Է���\n");
        return 0;	//û�пɷ���Ŀ��п飬����0
    }
    else{	//����ʣ���
        top = (superblock->s_free_BLOCK_NUM-1) % superblock->s_blocks_per_group;
    }
    //��ջ��ȡ��
    //�������ջ�ף�����ǰ��ŵ�ַ���أ���Ϊջ�׿�ţ�����ջ��ָ����¿��п��ջ����ԭ����ջ
    int retAddr;

    if(top==0){
        retAddr = superblock->s_free_addr;
        superblock->s_free_addr = superblock->s_free[0];	//ȡ����һ�����п��п��ջ�Ŀ��п��λ�ã����¿��п��ջָ��

        //ȡ����Ӧ���п����ݣ�����ԭ���Ŀ��п��ջ

        //ȡ����һ�����п��ջ������ԭ����
        fseek(fr,superblock->s_free_addr,SEEK_SET);
        fread(superblock->s_free,sizeof(superblock->s_free),1,fr);
        fflush(fr);

        superblock->s_free_BLOCK_NUM--;

    }
    else{	//�����Ϊջ�ף���ջ��ָ��ĵ�ַ���أ�ջ��ָ��-1.
        retAddr = superblock->s_free[top];	//���淵�ص�ַ
        superblock->s_free[top] = -1;	//��ջ��
        top--;		//ջ��ָ��-1
        superblock->s_free_BLOCK_NUM--;	//���п���-1

    }

    //���³�����
    fseek(fw,Superblock_StartAddr,SEEK_SET);
    fwrite(superblock,sizeof(SuperBlock),1,fw);
    fflush(fw);

    //����blockλͼ
    block_bitmap[(retAddr-Block_StartAddr)/BLOCK_SIZE] = 1;
    fseek(fw,(retAddr-Block_StartAddr)/BLOCK_SIZE+BlockBitmap_StartAddr,SEEK_SET);	//(retAddr-Block_StartAddr)/BLOCK_SIZEΪ�ڼ������п�
    fwrite(&block_bitmap[(retAddr-Block_StartAddr)/BLOCK_SIZE],sizeof(bool),1,fw);
    fflush(fw);

    return retAddr;

}

bool bfree(int addr)	//���̿��ͷź���
{
    //�ж�
    //�õ�ַ���Ǵ��̿����ʼ��ַ
    if( (addr-Block_StartAddr) % superblock->s_BLOCK_SIZE != 0 ){
        printf("��ַ����,��λ�ò���block�����̿飩��ʼλ��\n");
        return false;
    }
    unsigned int bno = (addr-Block_StartAddr) / superblock->s_BLOCK_SIZE;	//inode�ڵ��
    //�õ�ַ��δʹ�ã������ͷſռ�
    if(block_bitmap[bno]==0){
        printf("��block�����̿飩��δʹ�ã��޷��ͷ�\n");
        return false;
    }

    //�����ͷ�
    //���㵱ǰջ��
    int top;	//ջ��ָ��
    if(superblock->s_free_BLOCK_NUM==superblock->s_BLOCK_NUM){	//û�зǿ��еĴ��̿�
        printf("û�зǿ��еĴ��̿飬�޷��ͷ�\n");
        return false;	//û�пɷ���Ŀ��п飬����-1
    }
    else{	//����
        top = (superblock->s_free_BLOCK_NUM-1) % superblock->s_blocks_per_group;

        //���block����
        char tmp[BLOCK_SIZE] = {0};
        fseek(fw,addr,SEEK_SET);
        fwrite(tmp,sizeof(tmp),1,fw);

        if(top == superblock->s_blocks_per_group-1){	//��ջ����

            //�ÿ��п���Ϊ�µĿ��п��ջ
            superblock->s_free[0] = superblock->s_free_addr;	//�µĿ��п��ջ��һ����ַָ��ɵĿ��п��ջָ��
            int i;
            for(i=1;i<superblock->s_blocks_per_group;i++){
                superblock->s_free[i] = -1;	//���ջԪ�ص�������ַ
            }
            fseek(fw,addr,SEEK_SET);
            fwrite(superblock->s_free,sizeof(superblock->s_free),1,fw);	//����������̿飬512�ֽ�

        }
        else{	//ջ��δ��
            top++;	//ջ��ָ��+1
            superblock->s_free[top] = addr;	//ջ���������Ҫ�ͷŵĵ�ַ����Ϊ�µĿ��п�
        }
    }


    //���³�����
    superblock->s_free_BLOCK_NUM++;	//���п���+1
    fseek(fw,Superblock_StartAddr,SEEK_SET);
    fwrite(superblock,sizeof(SuperBlock),1,fw);

    //����blockλͼ
    block_bitmap[bno] = 0;
    fseek(fw,bno+BlockBitmap_StartAddr,SEEK_SET);	//(addr-Block_StartAddr)/BLOCK_SIZEΪ�ڼ������п�
    fwrite(&block_bitmap[bno],sizeof(bool),1,fw);
    fflush(fw);

    return true;
}

int ialloc()	//����i�ڵ�������������inode��ַ
{
    //��inodeλͼ��˳����ҿ��е�inode���ҵ��򷵻�inode��ַ������������
    if(superblock->s_free_INODE_NUM==0){
        printf("û�п���inode���Է���\n");
        return 0;
    }
    else{

        //˳����ҿ��е�inode
        int i;
        for(i=0;i<superblock->s_INODE_NUM;i++){
            if(inode_bitmap[i]==0)	//�ҵ�����inode
                break;
        }


        //���³�����
        superblock->s_free_INODE_NUM--;	//����inode��-1
        fseek(fw,Superblock_StartAddr,SEEK_SET);
        fwrite(superblock,sizeof(SuperBlock),1,fw);

        //����inodeλͼ
        inode_bitmap[i] = 1;
        fseek(fw,InodeBitmap_StartAddr+i,SEEK_SET);
        fwrite(&inode_bitmap[i],sizeof(bool),1,fw);
        fflush(fw);

        return Inode_StartAddr + i*superblock->s_INODE_SIZE;
    }
}

bool ifree(int addr)	//�ͷ�i���������
{
    //�ж�
    if( (addr-Inode_StartAddr) % superblock->s_INODE_SIZE != 0 ){
        printf("��ַ����,��λ�ò���i�ڵ���ʼλ��\n");
        return false;
    }
    unsigned short ino = (addr-Inode_StartAddr) / superblock->s_INODE_SIZE;	//inode�ڵ��
    if(inode_bitmap[ino]==0){
        printf("��inode��δʹ�ã��޷��ͷ�\n");
        return false;
    }

    //���inode����
    Inode tmp = {0};
    fseek(fw,addr,SEEK_SET);
    fwrite(&tmp,sizeof(tmp),1,fw);

    //���³�����
    superblock->s_free_INODE_NUM++;
    //����inode��+1
    fseek(fw,Superblock_StartAddr,SEEK_SET);
    fwrite(superblock,sizeof(SuperBlock),1,fw);

    //����inodeλͼ
    inode_bitmap[ino] = 0;
    fseek(fw,InodeBitmap_StartAddr+ino,SEEK_SET);
    fwrite(&inode_bitmap[ino],sizeof(bool),1,fw);
    fflush(fw);

    return true;
}
