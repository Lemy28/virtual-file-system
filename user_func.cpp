//
// Created by DiDi on 2022-06-06.
//

#include <algorithm>
#include "MingOS.h"
#include "dentry.h"
#include "rbtree.h"

using namespace std;



bool mkdir(int parinoAddr,char* name)	//Ŀ¼������������������һ��Ŀ¼�ļ�inode��ַ ,Ҫ������Ŀ¼���͵�ǰĿ¼��
{
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("�������Ŀ¼������\n");
        return false;
    }

    dentry dirlist[16];	//��ʱĿ¼�嵥

    //�������ַȡ��inode
    Inode cur;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    int i = 0;
    int cnt = cur.i_cnt+1;	//Ŀ¼����
    int posi = -1,posj = -1;
    while(i<160){
        //160��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
        int dno = i/16;	//�ڵڼ���ֱ�ӿ���

        if(cur.i_dirBlock[dno]==-1){
            i+=16;
            continue;
        }
        //ȡ�����ֱ�ӿ飬Ҫ�����Ŀ¼��Ŀ��λ��
        fseek(fr,cur.i_dirBlock[dno],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //����ô��̿��е�����Ŀ¼��
        int j;
        for(j=0;j<16;j++){

            if( strcmp(dirlist[j].itemName,name)==0 ){
                Inode tmp;
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&tmp,sizeof(Inode),1,fr);
                if( ((tmp.i_mode>>9)&1)==1 ){	//����Ŀ¼
                    printf("Ŀ¼�Ѵ���\n");
                    return false;
                }
            }
            else if( strcmp(dirlist[j].itemName,"")==0 ){
                //�ҵ�һ�����м�¼������Ŀ¼���������λ��
                //��¼���λ��
                if(posi==-1){
                    posi = dno;
                    posj = j;
                }

            }
            i++;
        }

    }

    if(posi!=-1){	//�ҵ��������λ��

        //ȡ�����ֱ�ӿ飬Ҫ�����Ŀ¼��Ŀ��λ��
        fseek(fr,cur.i_dirBlock[posi],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //�������Ŀ¼��
        strcpy(dirlist[posj].itemName,name);	//Ŀ¼��
        //д��������¼ "." ".."���ֱ�ָ��ǰinode�ڵ��ַ���͸�inode�ڵ�
        int chiinoAddr = ialloc();	//���䵱ǰ�ڵ��ַ
        if(chiinoAddr==0){
            printf("inode����ʧ��\n");
            return false;
        }
        dirlist[posj].inodeAddr = chiinoAddr; //������µ�Ŀ¼�����inode��ַ

        //��������Ŀ��inode
        Inode p;
        p.i_ino = (chiinoAddr-Inode_StartAddr)/superblock->s_INODE_SIZE;
        p.i_atime = time(NULL);
        p.i_ctime = time(NULL);
        p.i_mtime = time(NULL);
        strcpy(p.i_uname,Cur_User_Name);
        strcpy(p.i_gname,Cur_Group_Name);
        p.i_cnt = 2;	//�������ǰĿ¼,"."��".."

        //�������inode�Ĵ��̿飬�ڴ��̺���д��������¼ . �� ..
        int curblockAddr = balloc();
        if(curblockAddr==0){
            printf("block����ʧ��\n");
            return false;
        }
        dentry dirlist2[16] = {0};	//��ʱĿ¼���б� - 2
        strcpy(dirlist2[0].itemName,".");
        strcpy(dirlist2[1].itemName,"..");
        dirlist2[0].inodeAddr = chiinoAddr;	//��ǰĿ¼inode��ַ
        dirlist2[1].inodeAddr = parinoAddr;	//��Ŀ¼inode��ַ

        //д�뵽��ǰĿ¼�Ĵ��̿�
        fseek(fw,curblockAddr,SEEK_SET);
        fwrite(dirlist2,sizeof(dirlist2),1,fw);

        p.i_dirBlock[0] = curblockAddr;
        int k;
        for(k=1;k<10;k++){
            p.i_dirBlock[k] = -1;
        }
        p.i_size = superblock->s_BLOCK_SIZE;
        p.i_indirBlock_1 = -1;	//ûʹ��һ����ӿ�
        p.i_mode = MODE_DIR | DIR_DEF_PERMISSION;

        //��inodeд�뵽�����inode��ַ
        fseek(fw,chiinoAddr,SEEK_SET);
        fwrite(&p,sizeof(Inode),1,fw);

        //����ǰĿ¼�Ĵ��̿�д��
        fseek(fw,cur.i_dirBlock[posi],SEEK_SET);
        fwrite(dirlist,sizeof(dirlist),1,fw);

        //д��inode
        cur.i_cnt++;
        fseek(fw,parinoAddr,SEEK_SET);
        fwrite(&cur,sizeof(Inode),1,fw);
        fflush(fw);

        //����Ŀ¼��Ϣ���뵽�����

        string abs_name = Cur_Dir_Name;

        if(abs_name != "/")
            abs_name+="/";

        abs_name.append(name);
        rbtree_node* ptr = new rbtree_node;
        ptr->key = abs_name;
        ptr->value = chiinoAddr;
        rbtree_insert(&dir_tree,ptr);

        return true;
    }
    else{
        printf("û�ҵ�����Ŀ¼��,Ŀ¼����ʧ��");
        return false;
    }
}

void rmall(int parinoAddr)	//ɾ���ýڵ��������ļ���Ŀ¼
{
    //�������ַȡ��inode
    Inode cur;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //ȡ��Ŀ¼����
    int cnt = cur.i_cnt;
    if(cnt<=2){
        bfree(cur.i_dirBlock[0]);
        ifree(parinoAddr);
        return ;
    }

    //����ȡ�����̿�
    int i = 0;
    while(i<160){	//С��160
        dentry dirlist[16] = {0};

        if(cur.i_dirBlock[i/16]==-1){
            i+=16;
            continue;
        }
        //ȡ�����̿�
        int parblockAddr = cur.i_dirBlock[i/16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        //�Ӵ��̿�������ȡ��Ŀ¼��ݹ�ɾ��
        int j;
        bool f = false;
        for(j=0;j<16;j++){
            //Inode tmp;

            if( ! (strcmp(dirlist[j].itemName,".")==0 ||
                   strcmp(dirlist[j].itemName,"..")==0 ||
                   strcmp(dirlist[j].itemName,"")==0 ) ){
                f = true;
                rmall(dirlist[j].inodeAddr);	//�ݹ�ɾ��
            }

            cnt = cur.i_cnt;
            i++;
        }

        //�ô��̿��ѿգ�����
        if(f)
            bfree(parblockAddr);

    }
    //��inode�ѿգ�����
    ifree(parinoAddr);
    return ;

}

bool rmdir(int parinoAddr,char name[])	//Ŀ¼ɾ������
{
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("�������Ŀ¼������\n");
        return false;
    }

    if(strcmp(name,".")==0 || strcmp(name,"..")==0){
        printf("�������\n");
        return 0;
    }

    //�������ַȡ��inode
    Inode cur;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //ȡ��Ŀ¼����
    int cnt = cur.i_cnt;

    //�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
    int filemode;
    if(strcmp(Cur_User_Name,cur.i_uname)==0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)
        filemode = 3;
    else
        filemode = 0;

//    if( (((cur.i_mode>>filemode>>1)&1)==0) && (strcmp(Cur_User_Name,"root")!=0) ){
//        //û��д��Ȩ��
//        printf("Ȩ�޲��㣺��д��Ȩ��\n");
//        return false;
//    }


    //����ȡ�����̿�
    int i = 0;
    while(i<160){	//С��160
        dentry dirlist[16] = {0};

        if(cur.i_dirBlock[i/16]==-1){
            i+=16;
            continue;
        }
        //ȡ�����̿�
        int parblockAddr = cur.i_dirBlock[i/16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        //�ҵ�Ҫɾ����Ŀ¼
        int j;
        for(j=0;j<16;j++){
            Inode tmp;
            //ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
            fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
            fread(&tmp,sizeof(Inode),1,fr);

            if( strcmp(dirlist[j].itemName,name)==0){
                if( ( (tmp.i_mode>>9) & 1 ) == 1 ){	//�ҵ�Ŀ¼
                    //��Ŀ¼

                    rmall(dirlist[j].inodeAddr);

                    //ɾ����Ŀ¼��Ŀ��д�ش���
                    strcpy(dirlist[j].itemName,"");
                    dirlist[j].inodeAddr = -1;
                    fseek(fw,parblockAddr,SEEK_SET);
                    fwrite(&dirlist,sizeof(dirlist),1,fw);
                    cur.i_cnt--;
                    fseek(fw,parinoAddr,SEEK_SET);
                    fwrite(&cur,sizeof(Inode),1,fw);

                    fflush(fw);

                    //ɾ�����ں�����еĽڵ�
                    string abs_name = Cur_Dir_Name;
                    if(strcmp(Cur_Dir_Name,"/")!=0){
                        abs_name.append("/");
                    }
                    abs_name.append(name);
                    rbtree_node * p = rbtree_search(&dir_tree,abs_name);

                    if(p!=NULL){
                        rbtree_delete(&dir_tree,p);
                    }



                    return true;
                }
                else{
                    //����Ŀ¼������
                }
            }
            i++;
        }

    }

    printf("û���ҵ���Ŀ¼\n");
    return false;
}

bool create(int parinoAddr,char name[],char buf[])	//�����ļ��������ڸ�Ŀ¼�´����ļ����ļ����ݴ���buf
{
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("��������ļ�������\n");
        return false;
    }

    dentry dirlist[16];	//��ʱĿ¼�嵥

    //�������ַȡ��inode
    Inode cur;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    int i = 0;
    int posi = -1,posj = -1;	//�ҵ���Ŀ¼λ��
    int dno;
    int cnt = cur.i_cnt+1;	//Ŀ¼����
    while(i<160){
        //160��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
        dno = i/16;	//�ڵڼ���ֱ�ӿ���

        if(cur.i_dirBlock[dno]==-1){
            i+=16;
            continue;
        }
        fseek(fr,cur.i_dirBlock[dno],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //����ô��̿��е�����Ŀ¼��
        int j;
        for(j=0;j<16;j++){

            if( posi==-1 && strcmp(dirlist[j].itemName,"")==0 ){
                //�ҵ�һ�����м�¼�������ļ����������λ��
                posi = dno;
                posj = j;
            }
            else if(strcmp(dirlist[j].itemName,name)==0 ){
                //������ȡ��inode���ж��Ƿ����ļ�
                Inode cur2;
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&cur2,sizeof(Inode),1,fr);
                if( ((cur2.i_mode>>9)&1)==0 ){	//���ļ������������ܴ����ļ�
                    printf("�ļ��Ѵ���\n");
                    buf[0] = '\0';
                    return false;
                }
            }
            i++;
        }

    }
    if(posi!=-1){	//֮ǰ�ҵ�һ��Ŀ¼����
        //ȡ��֮ǰ�Ǹ�����Ŀ¼���Ӧ�Ĵ��̿�
        fseek(fr,cur.i_dirBlock[posi],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //�������Ŀ¼��
        strcpy(dirlist[posj].itemName,name);	//�ļ���
        int chiinoAddr = ialloc();	//���䵱ǰ�ڵ��ַ
        if(chiinoAddr==-1){
            printf("inode����ʧ��\n");
            return false;
        }
        dirlist[posj].inodeAddr = chiinoAddr; //������µ�Ŀ¼�����inode��ַ

        //��������Ŀ��inode
        Inode p;
        p.i_ino = (chiinoAddr-Inode_StartAddr)/superblock->s_INODE_SIZE;
        p.i_atime = time(NULL);
        p.i_ctime = time(NULL);
        p.i_mtime = time(NULL);
        strcpy(p.i_uname,Cur_User_Name);
        strcpy(p.i_gname,Cur_Group_Name);
        p.i_cnt = 1;	//ֻ��һ���ļ�ָ��


        //��buf���ݴ浽���̿�
        int k;
        int len = strlen(buf);	//�ļ����ȣ���λΪ�ֽ�
        for(k=0;k<len;k+=superblock->s_BLOCK_SIZE){	//���10�Σ�10�����̿죬�����5K
            //�������inode�Ĵ��̿飬�ӿ���̨��ȡ����
            int curblockAddr = balloc();
            if(curblockAddr==-1){
                printf("block����ʧ��\n");
                return false;
            }
            p.i_dirBlock[k/superblock->s_BLOCK_SIZE] = curblockAddr;
            //д�뵽��ǰĿ¼�Ĵ��̿�
            fseek(fw,curblockAddr,SEEK_SET);
            fwrite(buf+k,superblock->s_BLOCK_SIZE,1,fw);
        }


        for(k=len/superblock->s_BLOCK_SIZE+1;k<10;k++){
            p.i_dirBlock[k] = -1;
        }
        if(len==0){	//����Ϊ0�Ļ�Ҳ�ָ���һ��block
            int curblockAddr = balloc();
            if(curblockAddr==-1){
                printf("block����ʧ��\n");
                return false;
            }
            p.i_dirBlock[k/superblock->s_BLOCK_SIZE] = curblockAddr;
            //д�뵽��ǰĿ¼�Ĵ��̿�
            fseek(fw,curblockAddr,SEEK_SET);
            fwrite(buf,superblock->s_BLOCK_SIZE,1,fw);

        }
        p.i_size = len;
        p.i_indirBlock_1 = -1;	//ûʹ��һ����ӿ�
        p.i_mode = 0;
        p.i_mode = MODE_FILE | FILE_DEF_PERMISSION;

        //��inodeд�뵽�����inode��ַ
        fseek(fw,chiinoAddr,SEEK_SET);
        fwrite(&p,sizeof(Inode),1,fw);

        //����ǰĿ¼�Ĵ��̿�д��
        fseek(fw,cur.i_dirBlock[posi],SEEK_SET);
        fwrite(dirlist,sizeof(dirlist),1,fw);

        //д��inode
        cur.i_cnt++;
        fseek(fw,parinoAddr,SEEK_SET);
        fwrite(&cur,sizeof(Inode),1,fw);
        fflush(fw);

        return true;
    }
    else
        return false;
}

bool del(int parinoAddr,char name[])		//ɾ���ļ��������ڵ�ǰĿ¼��ɾ���ļ�
{
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("�������Ŀ¼������\n");
        return false;
    }

    //�������ַȡ��inode
    Inode cur;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //ȡ��Ŀ¼����
    int cnt = cur.i_cnt;

    //�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
    int filemode;
    if(strcmp(Cur_User_Name,cur.i_uname)==0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)
        filemode = 3;
    else
        filemode = 0;

    if( ((cur.i_mode>>filemode>>1)&1)==0 ){
        //û��д��Ȩ��
        printf("Ȩ�޲��㣺��д��Ȩ��\n");
        return false;
    }

    //����ȡ�����̿�
    int i = 0;
    while(i<160){	//С��160
        dentry dirlist[16] = {0};

        if(cur.i_dirBlock[i/16]==-1){
            i+=16;
            continue;
        }
        //ȡ�����̿�
        int parblockAddr = cur.i_dirBlock[i/16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        //�ҵ�Ҫɾ����Ŀ¼
        int pos;
        for(pos=0;pos<16;pos++){
            Inode tmp;
            //ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
            fseek(fr,dirlist[pos].inodeAddr,SEEK_SET);
            fread(&tmp,sizeof(Inode),1,fr);

            if( strcmp(dirlist[pos].itemName,name)==0){
                if( ( (tmp.i_mode>>9) & 1 ) == 1 ){	//�ҵ�Ŀ¼
                    //��Ŀ¼������
                }
                else{
                    //���ļ�

                    //�ͷ�block
                    int k;
                    for(k=0;k<10;k++)
                        if(tmp.i_dirBlock[k]!=-1)
                            bfree(tmp.i_dirBlock[k]);

                    //�ͷ�inode
                    ifree(dirlist[pos].inodeAddr);

                    //ɾ����Ŀ¼��Ŀ��д�ش���
                    strcpy(dirlist[pos].itemName,"");
                    dirlist[pos].inodeAddr = -1;
                    fseek(fw,parblockAddr,SEEK_SET);
                    fwrite(&dirlist,sizeof(dirlist),1,fw);
                    cur.i_cnt--;
                    fseek(fw,parinoAddr,SEEK_SET);
                    fwrite(&cur,sizeof(Inode),1,fw);

                    fflush(fw);
                    return true;
                }
            }
            i++;
        }

    }

    printf("û���ҵ����ļ�!\n");
    return false;
}

//todo:ʵ��һ���ܹ�����Ŀ¼�����Ŀ¼��ĺ���


void ls(int parinoAddr)		//��ʾ��ǰĿ¼�µ������ļ����ļ��С���������ǰĿ¼��inode�ڵ��ַ
{
    Inode curinode;
    //ȡ�����inode
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&curinode, sizeof(Inode), 1, fr);
    fflush(fr);

    //ȡ��Ŀ¼����
    int dentrycnt = curinode.i_cnt;

    //�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
    int filemode;
    if(strcmp(Cur_User_Name, curinode.i_uname) == 0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name, curinode.i_gname) == 0)
        filemode = 3;
    else
        filemode = 0;

    if(((curinode.i_mode >> filemode >> 2) & 1) == 0 ){
        //û�ж�ȡȨ��
        printf("Ȩ�޲��㣺�޶�ȡȨ��\n");
        return ;
    }

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

            if( strcmp(dirlist[j].itemName,"")==0 ){
                continue;
            }

            //�����Ϣ
            if( ( (tmp.i_mode>>9) & 1 ) == 1 ){
                printf("d");
            }
            else{
                printf("-");
            }

            tm *ptr;	//�洢ʱ��
            ptr=gmtime(&tmp.i_mtime);

            //���Ȩ����Ϣ
            int t = 8;
            while(t>=0){
                if( ((tmp.i_mode>>t)&1)==1){
                    if(t%3==2)	printf("r");
                    if(t%3==1)	printf("w");
                    if(t%3==0)	printf("x");
                }
                else{
                    printf("-");
                }
                t--;
            }
            printf("\t");

            //����
            printf("%d\t",tmp.i_cnt);	//����
            printf("%s\t",tmp.i_uname);	//�ļ������û���
            printf("%s\t",tmp.i_gname);	//�ļ������û���
            printf("%d B\t",tmp.i_size);	//�ļ���С
            printf("%d.%d.%d %02d:%02d:%02d  ",1900+ptr->tm_year,ptr->tm_mon+1,ptr->tm_mday,(8+ptr->tm_hour)%24,ptr->tm_min,ptr->tm_sec);	//��һ���޸ĵ�ʱ��
            printf("%s",dirlist[j].itemName);	//�ļ���
            printf("\n");
            i++;
        }

    }
    /*  δд�� */

}


void cd(int parinoAddr,char name[])	//����Ŀ���ַĿ¼��ָ����Ŀ¼
{
    //ȡ����ǰĿ¼��inode
    Inode curinode;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&curinode, sizeof(Inode), 1, fr);

    //����ȡ��inode��Ӧ�Ĵ��̿飬������û������Ϊname��Ŀ¼��
    int i = 0;

    //ȡ��Ŀ¼����
    int cnt = curinode.i_cnt;

    //�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
    int filemode;
    if(strcmp(Cur_User_Name, curinode.i_uname) == 0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name, curinode.i_gname) == 0)
        filemode = 3;
    else
        filemode = 0;

    while(i<160){
        dentry dirlist[16] = {0};
        if(curinode.i_dirBlock[i / 16] == -1){
            i+=16;
            continue;
        }
        //ȡ�����̿�
        int parblockAddr = curinode.i_dirBlock[i / 16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        //����ô��̿��е�����Ŀ¼��
        int j;
        for(j=0;j<16;j++){
            if(strcmp(dirlist[j].itemName,name)==0){
                Inode tmp;
                //ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&tmp,sizeof(Inode),1,fr);

                if( ( (tmp.i_mode>>9) & 1 ) == 1 ){
                    //�ҵ���Ŀ¼���ж��Ƿ���н���Ȩ��
                    if( ((tmp.i_mode>>filemode>>0)&1)==0 && strcmp(Cur_User_Name,"root")!=0 ){	//root�û�����Ŀ¼�����Բ鿴
                        //û��ִ��Ȩ��
                        printf("Ȩ�޲��㣺��ִ��Ȩ��\n");
                        return ;
                    }

                    //�ҵ���Ŀ¼������Ŀ¼��������ǰĿ¼

                    Cur_Dir_Addr = dirlist[j].inodeAddr;
                    if( strcmp(dirlist[j].itemName,".")==0){
                        //��Ŀ¼������
                    }
                    else if(strcmp(dirlist[j].itemName,"..")==0){
                        //��һ��Ŀ¼
                        int k;
                        for(k=strlen(Cur_Dir_Name);k>=0;k--)
                            if(Cur_Dir_Name[k]=='/')
                                break;
                        Cur_Dir_Name[k]='\0';
                        if(strlen(Cur_Dir_Name)==0)
                            Cur_Dir_Name[0]='/',Cur_Dir_Name[1]='\0';
                    }
                    else{
                        if(Cur_Dir_Name[strlen(Cur_Dir_Name)-1]!='/')
                            strcat(Cur_Dir_Name,"/");
                        strcat(Cur_Dir_Name,dirlist[j].itemName);
                    }

                    return ;
                }
                else{
                    //�ҵ���Ŀ¼��������Ŀ¼��������
                }

            }

            i++;
        }

    }

    //û�ҵ�
    printf("û�и�Ŀ¼\n");
    return ;

}


void gotoxy(HANDLE hOut, int x, int y)	//�ƶ���굽ָ��λ��
{
    COORD pos;
    pos.X = x;             //������
    pos.Y = y;            //������
    SetConsoleCursorPosition(hOut, pos);
}

void vi(int parinoAddr,char name[],char buf[])	//ģ��һ����vi�������ı���nameΪ�ļ���
{
    //���ж��ļ��Ƿ��Ѵ��ڡ�������ڣ�������ļ����༭
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("��������ļ�������\n");
        return ;
    }

    //��ջ�����
    memset(buf,0,sizeof(buf));
    int maxlen = 0;	//���������󳤶�

    //��������ͬ���ļ����еĻ�����༭ģʽ��û�н��봴���ļ�ģʽ
    dentry dirlist[16];	//��ʱĿ¼�嵥

    //�������ַȡ��inode
    Inode cur,fileInode;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
    int filemode;
    if(strcmp(Cur_User_Name,cur.i_uname)==0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)
        filemode = 3;
    else
        filemode = 0;

    int i = 0,j;
    int dno;
    int fileInodeAddr = -1;	//�ļ���inode��ַ
    bool isExist = false;	//�ļ��Ƿ��Ѵ���
    while(i<160){
        //160��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
        dno = i/16;	//�ڵڼ���ֱ�ӿ���

        if(cur.i_dirBlock[dno]==-1){
            i+=16;
            continue;
        }
        fseek(fr,cur.i_dirBlock[dno],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //����ô��̿��е�����Ŀ¼��
        for(j=0;j<16;j++){
            if(strcmp(dirlist[j].itemName,name)==0 ){
                //������ȡ��inode���ж��Ƿ����ļ�
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&fileInode,sizeof(Inode),1,fr);
                if( ((fileInode.i_mode>>9)&1)==0 ){	//���ļ���������������ļ������༭
                    fileInodeAddr = dirlist[j].inodeAddr;
                    isExist = true;
                    goto label;
                }
            }
            i++;
        }
    }
    label:

    //��ʼ��vi
    int cnt = 0;
    system("cls");	//����

    int winx,winy,curx,cury;

    HANDLE handle_out;                              //����һ�����
    CONSOLE_SCREEN_BUFFER_INFO screen_info;         //���崰�ڻ�������Ϣ�ṹ��
    COORD pos = {0, 0};                             //����һ������ṹ��

    if(isExist){	//�ļ��Ѵ��ڣ�����༭ģʽ�������֮ǰ���ļ�����

        //Ȩ���жϡ��ж��ļ��Ƿ�ɶ�
        if( ((fileInode.i_mode>>filemode>>2)&1)==0){
            //���ɶ�
            printf("Ȩ�޲��㣺û�ж�Ȩ��\n");
            return ;
        }

        //���ļ����ݶ�ȡ��������ʾ�ڣ�������
        i = 0;
        int sumlen = fileInode.i_size;	//�ļ�����
        int getlen = 0;	//ȡ�����ĳ���
        for(i=0;i<10;i++){
            char fileContent[1000] = {0};
            if(fileInode.i_dirBlock[i]==-1){
                continue;
            }
            //����ȡ�����̿������
            fseek(fr,fileInode.i_dirBlock[i],SEEK_SET);
            fread(fileContent,superblock->s_BLOCK_SIZE,1,fr);	//��ȡ��һ�����̿��С������
            fflush(fr);
            //����ַ���
            int curlen = 0;	//��ǰָ��
            while(curlen<superblock->s_BLOCK_SIZE){
                if(getlen>=sumlen)	//ȫ��������
                    break;
                printf("%c",fileContent[curlen]);	//�������Ļ
                buf[cnt++] = fileContent[curlen];	//�����buf
                curlen++;
                getlen++;
            }
            if(getlen>=sumlen)
                break;
        }
        maxlen = sumlen;
    }

    //������֮��Ĺ��λ��
    handle_out = GetStdHandle(STD_OUTPUT_HANDLE);   //��ñ�׼����豸���
    GetConsoleScreenBufferInfo(handle_out, &screen_info);   //��ȡ������Ϣ
    winx = screen_info.srWindow.Right - screen_info.srWindow.Left + 1;
    winy = screen_info.srWindow.Bottom - screen_info.srWindow.Top + 1;
    curx = screen_info.dwCursorPosition.X;
    cury = screen_info.dwCursorPosition.Y;


    //����vi
    //����vi��ȡ�ļ�����

    int mode = 0;	//viģʽ��һ��ʼ������ģʽ
    unsigned char c;
    while(1){
        if(mode==0){	//������ģʽ
            c=getch();

            if(c=='i' || c=='a'){	//����ģʽ
                if(c=='a'){
                    curx++;
                    if(curx==winx){
                        curx = 0;
                        cury++;

                        /*
                        if(cury>winy-2 || cury%(winy-1)==winy-2){
                            //������һ�������·�ҳ
                            if(cury%(winy-1)==winy-2)
                                printf("\n");
                            SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                            int i;
                            for(i=0;i<winx-1;i++)
                                printf(" ");
                            gotoxy(handle_out,0,cury+1);
                            printf(" - ����ģʽ - ");
                            gotoxy(handle_out,0,cury);
                        }
                        */
                    }
                }

                if(cury>winy-2 || cury%(winy-1)==winy-2){
                    //������һ�������·�ҳ
                    if(cury%(winy-1)==winy-2)
                        printf("\n");
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                    int i;
                    for(i=0;i<winx-1;i++)
                        printf(" ");
                    gotoxy(handle_out,0,cury+1);
                    printf(" - ����ģʽ - ");
                    gotoxy(handle_out,0,cury);
                }
                else{
                    //��ʾ "����ģʽ"
                    gotoxy(handle_out,0,winy-1);
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                    int i;
                    for(i=0;i<winx-1;i++)
                        printf(" ");
                    gotoxy(handle_out,0,winy-1);
                    printf(" - ����ģʽ - ");
                    gotoxy(handle_out,curx,cury);
                }

                gotoxy(handle_out,curx,cury);
                mode = 1;


            }
            else if(c==':'){
                //system("color 09");//�����ı�Ϊ��ɫ
                if(cury-winy+2>0)
                    gotoxy(handle_out,0,cury+1);
                else
                    gotoxy(handle_out,0,winy-1);
                _COORD pos;
                if(cury-winy+2>0)
                    pos.X = 0,pos.Y = cury+1;
                else
                    pos.X = 0,pos.Y = winy-1;
                SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                int i;
                for(i=0;i<winx-1;i++)
                    printf(" ");

                if(cury-winy+2>0)
                    gotoxy(handle_out,0,cury+1);
                else
                    gotoxy(handle_out,0,winy-1);

                WORD att = BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_INTENSITY; // �ı�����
                FillConsoleOutputAttribute(handle_out, att, winx, pos, NULL);	//����̨������ɫ
                SetConsoleTextAttribute(handle_out, FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_GREEN  );	//�����ı���ɫ
                printf(":");

                char pc;
                int tcnt = 1;	//������ģʽ������ַ�����
                while( c = getch() ){
                    if(c=='\r'){	//�س�
                        break;
                    }
                    else if(c=='\b'){	//�˸񣬴�������ɾ��һ���ַ�
                        //SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                        tcnt--;
                        if(tcnt==0)
                            break;
                        printf("\b");
                        printf(" ");
                        printf("\b");
                        continue;
                    }
                    pc = c;
                    printf("%c",pc);
                    tcnt++;
                }
                if(pc=='q'){
                    buf[cnt] = '\0';
                    //buf[maxlen] = '\0';
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                    system("cls");
                    break;	//vi >>>>>>>>>>>>>> �˳�����
                }
                else{
                    if(cury-winy+2>0)
                        gotoxy(handle_out,0,cury+1);
                    else
                        gotoxy(handle_out,0,winy-1);
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                    int i;
                    for(i=0;i<winx-1;i++)
                        printf(" ");

                    if(cury-winy+2>0)
                        gotoxy(handle_out,0,cury+1);
                    else
                        gotoxy(handle_out,0,winy-1);
                    SetConsoleTextAttribute(handle_out, FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_GREEN  );	//�����ı���ɫ
                    FillConsoleOutputAttribute(handle_out, att, winx, pos, NULL);	//����̨������ɫ
                    printf(" ��������");
                    //getch();
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                    gotoxy(handle_out,curx,cury);
                }
            }
            else if(c==27){	//ESC��������ģʽ����״̬��
                gotoxy(handle_out,0,winy-1);
                SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                int i;
                for(i=0;i<winx-1;i++)
                    printf(" ");
                gotoxy(handle_out,curx,cury);

            }

        }
        else if(mode==1){	//����ģʽ

            gotoxy(handle_out,winx/4*3,winy-1);
            int i = winx/4*3;
            while(i<winx-1){
                printf(" ");
                i++;
            }
            if(cury>winy-2)
                gotoxy(handle_out,winx/4*3,cury+1);
            else
                gotoxy(handle_out,winx/4*3,winy-1);
            printf("[��:%d,��:%d]",curx==-1?0:curx,cury);
            gotoxy(handle_out,curx,cury);

            c = getch();
            if(c==27){	// ESC����������ģʽ
                mode = 0;
                //��״̬��
                gotoxy(handle_out,0,winy-1);
                SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                int i;
                for(i=0;i<winx-1;i++)
                    printf(" ");
                continue;
            }
            else if(c=='\b'){	//�˸�ɾ��һ���ַ�
                if(cnt==0)	//�Ѿ��˵��ʼ
                    continue;
                printf("\b");
                printf(" ");
                printf("\b");
                curx--;
                cnt--;	//ɾ���ַ�
                if(buf[cnt]=='\n'){
                    //Ҫɾ��������ַ��ǻس������ص���һ��
                    if(cury!=0)
                        cury--;
                    int k;
                    curx = 0;
                    for(k = cnt-1;buf[k]!='\n' && k>=0;k--)
                        curx++;
                    gotoxy(handle_out,curx,cury);
                    printf(" ");
                    gotoxy(handle_out,curx,cury);
                    if(cury-winy+2>=0){	//��ҳʱ
                        gotoxy(handle_out,curx,0);
                        gotoxy(handle_out,curx,cury+1);
                        SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                        int i;
                        for(i=0;i<winx-1;i++)
                            printf(" ");
                        gotoxy(handle_out,0,cury+1);
                        printf(" - ����ģʽ - ");

                    }
                    gotoxy(handle_out,curx,cury);

                }
                else
                    buf[cnt] = ' ';
                continue;
            }
            else if(c==224){	//�ж��Ƿ��Ǽ�ͷ
                c = getch();
                if(c==75){	//���ͷ
                    if(cnt!=0){
                        cnt--;
                        curx--;
                        if(buf[cnt]=='\n'){
                            //��һ���ַ��ǻس�
                            if(cury!=0)
                                cury--;
                            int k;
                            curx = 0;
                            for(k = cnt-1;buf[k]!='\n' && k>=0;k--)
                                curx++;
                        }
                        gotoxy(handle_out,curx,cury);
                    }
                }
                else if(c==77){	//�Ҽ�ͷ
                    cnt++;
                    if(cnt>maxlen)
                        maxlen = cnt;
                    curx++;
                    if(curx==winx){
                        curx = 0;
                        cury++;

                        if(cury>winy-2 || cury%(winy-1)==winy-2){
                            //������һ�������·�ҳ
                            if(cury%(winy-1)==winy-2)
                                printf("\n");
                            SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                            int i;
                            for(i=0;i<winx-1;i++)
                                printf(" ");
                            gotoxy(handle_out,0,cury+1);
                            printf(" - ����ģʽ - ");
                            gotoxy(handle_out,0,cury);
                        }

                    }
                    gotoxy(handle_out,curx,cury);
                }
                continue;
            }
            if(c=='\r'){	//�����س�
                printf("\n");
                curx = 0;
                cury++;

                if(cury>winy-2 || cury%(winy-1)==winy-2){
                    //������һ�������·�ҳ
                    if(cury%(winy-1)==winy-2)
                        printf("\n");
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                    int i;
                    for(i=0;i<winx-1;i++)
                        printf(" ");
                    gotoxy(handle_out,0,cury+1);
                    printf(" - ����ģʽ - ");
                    gotoxy(handle_out,0,cury);
                }

                buf[cnt++] = '\n';
                if(cnt>maxlen)
                    maxlen = cnt;
                continue;
            }
            else{
                printf("%c",c);
            }
            //�ƶ����
            curx++;
            if(curx==winx){
                curx = 0;
                cury++;

                if(cury>winy-2 || cury%(winy-1)==winy-2){
                    //������һ�������·�ҳ
                    if(cury%(winy-1)==winy-2)
                        printf("\n");
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                    int i;
                    for(i=0;i<winx-1;i++)
                        printf(" ");
                    gotoxy(handle_out,0,cury+1);
                    printf(" - ����ģʽ - ");
                    gotoxy(handle_out,0,cury);
                }

                buf[cnt++] = '\n';
                if(cnt>maxlen)
                    maxlen = cnt;
                if(cury==winy){
                    printf("\n");
                }
            }
            //��¼�ַ�
            buf[cnt++] = c;
            if(cnt>maxlen)
                maxlen = cnt;
        }
        else{	//����ģʽ
        }
    }
    if(isExist){	//����Ǳ༭ģʽ
        //��buf����д���ļ��Ĵ��̿�

        if( ((fileInode.i_mode>>filemode>>1)&1)==1 ){	//��д
            writefile(fileInode,fileInodeAddr,buf);
        }
        else{	//����д
            printf("Ȩ�޲��㣺��д��Ȩ��\n");
        }

    }
    else{	//�Ǵ����ļ�ģʽ
        if( ((cur.i_mode>>filemode>>1)&1)==1){
            //��д�����Դ����ļ�
            create(parinoAddr,name,buf);	//�����ļ�
        }
        else{
            printf("Ȩ�޲��㣺��д��Ȩ��\n");
            return ;
        }
    }
}

void writefile(Inode fileInode,int fileInodeAddr,char buf[])	//��buf����д���ļ��Ĵ��̿�
{
    //��buf����д�ش��̿�
    int k;
    int len = strlen(buf);	//�ļ����ȣ���λΪ�ֽ�
    for(k=0;k<len;k+=superblock->s_BLOCK_SIZE){	//���10�Σ�10�����̿죬�����5K
        //�������inode�Ĵ��̿飬�ӿ���̨��ȡ����
        int curblockAddr;
        if(fileInode.i_dirBlock[k/superblock->s_BLOCK_SIZE]==-1){
            //ȱ�ٴ��̿飬����һ��
            curblockAddr = balloc();
            if(curblockAddr==-1){
                printf("block����ʧ��\n");
                return ;
            }
            fileInode.i_dirBlock[k/superblock->s_BLOCK_SIZE] = curblockAddr;
        }
        else{
            curblockAddr = fileInode.i_dirBlock[k/superblock->s_BLOCK_SIZE];
        }
        //д�뵽��ǰĿ¼�Ĵ��̿�
        fseek(fw,curblockAddr,SEEK_SET);
        fwrite(buf+k,superblock->s_BLOCK_SIZE,1,fw);
        fflush(fw);
    }
    //���¸��ļ���С
    fileInode.i_size = len;
    fileInode.i_mtime = time(NULL);
    fseek(fw,fileInodeAddr,SEEK_SET);
    fwrite(&fileInode,sizeof(Inode),1,fw);
    fflush(fw);
}



void gotoRoot()	//�ص���Ŀ¼
{
    memset(Cur_User_Name,0,sizeof(Cur_User_Name));		//��յ�ǰ�û���
    memset(Cur_User_Dir_Name,0,sizeof(Cur_User_Dir_Name));	//��յ�ǰ�û�Ŀ¼
    Cur_Dir_Addr = Root_Dir_Addr;	//��ǰ�û�Ŀ¼��ַ��Ϊ��Ŀ¼��ַ
    strcpy(Cur_Dir_Name,"/");		//��ǰĿ¼��Ϊ"/"
}


void touch(int parinoAddr,char name[],char buf[])	//touch������ļ��������ַ�
{
    //���ж��ļ��Ƿ��Ѵ��ڡ�������ڣ�������ļ����༭
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("��������ļ�������\n");
        return ;
    }
    //��������ͬ���ļ����еĻ���ʾ�����˳�����û�еĻ�������һ�����ļ�
    dentry dirlist[16];	//��ʱĿ¼����

    //�������ַȡ��inode
    Inode cur,fileInode;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
    int filemode;
    if(strcmp(Cur_User_Name,cur.i_uname)==0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)
        filemode = 3;
    else
        filemode = 0;

    int i = 0,j;
    int dno;
    while(i<160){
        //160��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
        dno = i/16;	//�ڵڼ���ֱ�ӿ���

        if(cur.i_dirBlock[dno]==-1){
            i+=16;
            continue;
        }
        fseek(fr,cur.i_dirBlock[dno],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //����ô��̿��е�����Ŀ¼��
        for(j=0;j<16;j++){
            if(strcmp(dirlist[j].itemName,name)==0 ){
                //������ȡ��inode���ж��Ƿ����ļ�
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&fileInode,sizeof(Inode),1,fr);
                if( ((fileInode.i_mode>>9)&1)==0 ){	//���ļ�����������ʾ�����˳�����
                    printf("�ļ��Ѵ���\n");
                    return ;
                }
            }
            i++;
        }
    }

    //�ļ������ڣ�����һ�����ļ�
    if( ((cur.i_mode>>filemode>>1)&1)==1){
        //��д�����Դ����ļ�
        buf[0] = '\0';
        create(parinoAddr,name,buf);	//�����ļ�
    }
    else{
        printf("Ȩ�޲��㣺��д��Ȩ��\n");
        return ;
    }

}

void help()	//��ʾ���������嵥
{
    printf("ls - ��ʾ��ǰĿ¼�嵥\n");
    printf("cd - ����Ŀ¼\n");
    printf("mkdir - ����Ŀ¼\n");
    printf("rmdir - ɾ��Ŀ¼\n");
    printf("tree - �鿴Ŀ¼��ĺ����\n");
    printf("vi - vi�༭��\n");
    printf("touch - ����һ�����ļ�\n");
    printf("rm - ɾ���ļ�\n");
    printf("cls - ����\n");
    printf("help - ��ʾ�����嵥\n");
    printf("exit - �˳�ϵͳ\n");
    return ;
}

void cmd(char str[])	//�������������
{
    char p1[10];
    char p2[100]={0};
    char buf[100000];	//���100K
    int tmp = 0;
    int i;
    sscanf(str,"%s",p1);
    if(strcmp(p1,"ls")==0){
        sscanf(str,"%s%s",p1,p2);
        if(p2[0]!=NULL) {//���ָ����Ŀ¼,��ʾָ����Ŀ¼

            rbtree_node *p = rbtree_search(&dir_tree, p2);

            if (p)//�ҵ��˸�Ŀ¼
                ls(p->value);
        }
        else{
            ls(Cur_Dir_Addr);	//������ʾ��ǰĿ¼
        }
    }
    else if(strcmp(p1,"cd")==0){
        sscanf(str,"%s%s",p1,p2);
        if(p2[0]!='/'){
            //û���þ���·��
            cd(Cur_Dir_Addr,p2);
        }

        else{
            //������Ǿ���·��
            rbtree_node *p = rbtree_search(&dir_tree,p2);

            if(p)//�ҵ���Ŀ¼
            {
                string target = p->key;
                strcpy(Cur_Dir_Name,target.c_str());
                Cur_Dir_Addr = p->value;

            }

        }
    }
    else if(strcmp(p1,"mkdir")==0){
        sscanf(str,"%s%s",p1,p2);
        mkdir(Cur_Dir_Addr,p2);
    }
    else if(strcmp(p1,"rmdir")==0){
        sscanf(str,"%s%s",p1,p2);
        rmdir(Cur_Dir_Addr,p2);
    }
    else if(strcmp(p1,"super")==0){
        printSuperBlock();
    }
    else if(strcmp(p1,"inode")==0){
        printInodeBitmap();
    }
    else if(strcmp(p1,"block")==0){
        sscanf(str,"%s%s",p1,p2);
        tmp = 0;
        if('0'<=p2[0] && p2[0]<='9'){
            for(i=0;p2[i];i++)
                tmp = tmp*10+p2[i]-'0';
            printBlockBitmap(tmp);
        }
        else
            printBlockBitmap();
    }
    else if(strcmp(p1,"vi")==0){	//����һ���ļ�
        sscanf(str,"%s%s",p1,p2);
        vi(Cur_Dir_Addr,p2,buf);	//��ȡ���ݵ�buf
    }
    else if(strcmp(p1,"touch")==0){
        sscanf(str,"%s%s",p1,p2);
        touch (Cur_Dir_Addr,p2,buf);	//��ȡ���ݵ�buf
    }
    else if(strcmp(p1,"rm")==0){	//ɾ��һ���ļ�
        sscanf(str,"%s%s",p1,p2);
        del(Cur_Dir_Addr,p2);
    }
    else if(strcmp(p1,"cls")==0){
        system("cls");
    }
    else if(strcmp(p1,"help")==0){
        help();
    }
    else if(strcmp(p1,"format")==0){
        Ready();
    }
    else if(strcmp(p1,"exit")==0){
        printf("bye~\n");
        exit(0);
    }
    else if(strcmp(p1,"tree")==0){
        rbtree_traversal(&dir_tree,dir_tree.root);
    }
    else{
        printf("����help�鿴����\n");
    }


    return ;
}
