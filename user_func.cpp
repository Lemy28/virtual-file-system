//
// Created by DiDi on 2022-06-06.
//

#include <algorithm>
#include "MingOS.h"
#include "dentry.h"
#include "rbtree.h"

using namespace std;



bool mkdir(int parinoAddr,char* name)	//目录创建函数。参数：上一层目录文件inode地址 ,要创建的目录名和当前目录名
{
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("超过最大目录名长度\n");
        return false;
    }

    dentry dirlist[16];	//临时目录清单

    //从这个地址取出inode
    Inode cur;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    int i = 0;
    int cnt = cur.i_cnt+1;	//目录项数
    int posi = -1,posj = -1;
    while(i<160){
        //160个目录项之内，可以直接在直接块里找
        int dno = i/16;	//在第几个直接块里

        if(cur.i_dirBlock[dno]==-1){
            i+=16;
            continue;
        }
        //取出这个直接块，要加入的目录条目的位置
        fseek(fr,cur.i_dirBlock[dno],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //输出该磁盘块中的所有目录项
        int j;
        for(j=0;j<16;j++){

            if( strcmp(dirlist[j].itemName,name)==0 ){
                Inode tmp;
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&tmp,sizeof(Inode),1,fr);
                if( ((tmp.i_mode>>9)&1)==1 ){	//不是目录
                    printf("目录已存在\n");
                    return false;
                }
            }
            else if( strcmp(dirlist[j].itemName,"")==0 ){
                //找到一个空闲记录，将新目录创建到这个位置
                //记录这个位置
                if(posi==-1){
                    posi = dno;
                    posj = j;
                }

            }
            i++;
        }

    }

    if(posi!=-1){	//找到这个空闲位置

        //取出这个直接块，要加入的目录条目的位置
        fseek(fr,cur.i_dirBlock[posi],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //创建这个目录项
        strcpy(dirlist[posj].itemName,name);	//目录名
        //写入两条记录 "." ".."，分别指向当前inode节点地址，和父inode节点
        int chiinoAddr = ialloc();	//分配当前节点地址
        if(chiinoAddr==0){
            printf("inode分配失败\n");
            return false;
        }
        dirlist[posj].inodeAddr = chiinoAddr; //给这个新的目录分配的inode地址

        //设置新条目的inode
        Inode p;
        p.i_ino = (chiinoAddr-Inode_StartAddr)/superblock->s_INODE_SIZE;
        p.i_atime = time(NULL);
        p.i_ctime = time(NULL);
        p.i_mtime = time(NULL);
        strcpy(p.i_uname,Cur_User_Name);
        strcpy(p.i_gname,Cur_Group_Name);
        p.i_cnt = 2;	//两个项，当前目录,"."和".."

        //分配这个inode的磁盘块，在磁盘号中写入两条记录 . 和 ..
        int curblockAddr = balloc();
        if(curblockAddr==0){
            printf("block分配失败\n");
            return false;
        }
        dentry dirlist2[16] = {0};	//临时目录项列表 - 2
        strcpy(dirlist2[0].itemName,".");
        strcpy(dirlist2[1].itemName,"..");
        dirlist2[0].inodeAddr = chiinoAddr;	//当前目录inode地址
        dirlist2[1].inodeAddr = parinoAddr;	//父目录inode地址

        //写入到当前目录的磁盘块
        fseek(fw,curblockAddr,SEEK_SET);
        fwrite(dirlist2,sizeof(dirlist2),1,fw);

        p.i_dirBlock[0] = curblockAddr;
        int k;
        for(k=1;k<10;k++){
            p.i_dirBlock[k] = -1;
        }
        p.i_size = superblock->s_BLOCK_SIZE;
        p.i_indirBlock_1 = -1;	//没使用一级间接块
        p.i_mode = MODE_DIR | DIR_DEF_PERMISSION;

        //将inode写入到申请的inode地址
        fseek(fw,chiinoAddr,SEEK_SET);
        fwrite(&p,sizeof(Inode),1,fw);

        //将当前目录的磁盘块写回
        fseek(fw,cur.i_dirBlock[posi],SEEK_SET);
        fwrite(dirlist,sizeof(dirlist),1,fw);

        //写回inode
        cur.i_cnt++;
        fseek(fw,parinoAddr,SEEK_SET);
        fwrite(&cur,sizeof(Inode),1,fw);
        fflush(fw);

        //将该目录信息加入到红黑树

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
        printf("没找到空闲目录项,目录创建失败");
        return false;
    }
}

void rmall(int parinoAddr)	//删除该节点下所有文件或目录
{
    //从这个地址取出inode
    Inode cur;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //取出目录项数
    int cnt = cur.i_cnt;
    if(cnt<=2){
        bfree(cur.i_dirBlock[0]);
        ifree(parinoAddr);
        return ;
    }

    //依次取出磁盘块
    int i = 0;
    while(i<160){	//小于160
        dentry dirlist[16] = {0};

        if(cur.i_dirBlock[i/16]==-1){
            i+=16;
            continue;
        }
        //取出磁盘块
        int parblockAddr = cur.i_dirBlock[i/16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        //从磁盘块中依次取出目录项，递归删除
        int j;
        bool f = false;
        for(j=0;j<16;j++){
            //Inode tmp;

            if( ! (strcmp(dirlist[j].itemName,".")==0 ||
                   strcmp(dirlist[j].itemName,"..")==0 ||
                   strcmp(dirlist[j].itemName,"")==0 ) ){
                f = true;
                rmall(dirlist[j].inodeAddr);	//递归删除
            }

            cnt = cur.i_cnt;
            i++;
        }

        //该磁盘块已空，回收
        if(f)
            bfree(parblockAddr);

    }
    //该inode已空，回收
    ifree(parinoAddr);
    return ;

}

bool rmdir(int parinoAddr,char name[])	//目录删除函数
{
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("超过最大目录名长度\n");
        return false;
    }

    if(strcmp(name,".")==0 || strcmp(name,"..")==0){
        printf("错误操作\n");
        return 0;
    }

    //从这个地址取出inode
    Inode cur;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //取出目录项数
    int cnt = cur.i_cnt;

    //判断文件模式。6为owner，3为group，0为other
    int filemode;
    if(strcmp(Cur_User_Name,cur.i_uname)==0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)
        filemode = 3;
    else
        filemode = 0;

//    if( (((cur.i_mode>>filemode>>1)&1)==0) && (strcmp(Cur_User_Name,"root")!=0) ){
//        //没有写入权限
//        printf("权限不足：无写入权限\n");
//        return false;
//    }


    //依次取出磁盘块
    int i = 0;
    while(i<160){	//小于160
        dentry dirlist[16] = {0};

        if(cur.i_dirBlock[i/16]==-1){
            i+=16;
            continue;
        }
        //取出磁盘块
        int parblockAddr = cur.i_dirBlock[i/16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        //找到要删除的目录
        int j;
        for(j=0;j<16;j++){
            Inode tmp;
            //取出该目录项的inode，判断该目录项是目录还是文件
            fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
            fread(&tmp,sizeof(Inode),1,fr);

            if( strcmp(dirlist[j].itemName,name)==0){
                if( ( (tmp.i_mode>>9) & 1 ) == 1 ){	//找到目录
                    //是目录

                    rmall(dirlist[j].inodeAddr);

                    //删除该目录条目，写回磁盘
                    strcpy(dirlist[j].itemName,"");
                    dirlist[j].inodeAddr = -1;
                    fseek(fw,parblockAddr,SEEK_SET);
                    fwrite(&dirlist,sizeof(dirlist),1,fw);
                    cur.i_cnt--;
                    fseek(fw,parinoAddr,SEEK_SET);
                    fwrite(&cur,sizeof(Inode),1,fw);

                    fflush(fw);

                    //删除其在红黑树中的节点
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
                    //不是目录，不管
                }
            }
            i++;
        }

    }

    printf("没有找到该目录\n");
    return false;
}

bool create(int parinoAddr,char name[],char buf[])	//创建文件函数，在该目录下创建文件，文件内容存在buf
{
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("超过最大文件名长度\n");
        return false;
    }

    dentry dirlist[16];	//临时目录清单

    //从这个地址取出inode
    Inode cur;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    int i = 0;
    int posi = -1,posj = -1;	//找到的目录位置
    int dno;
    int cnt = cur.i_cnt+1;	//目录项数
    while(i<160){
        //160个目录项之内，可以直接在直接块里找
        dno = i/16;	//在第几个直接块里

        if(cur.i_dirBlock[dno]==-1){
            i+=16;
            continue;
        }
        fseek(fr,cur.i_dirBlock[dno],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //输出该磁盘块中的所有目录项
        int j;
        for(j=0;j<16;j++){

            if( posi==-1 && strcmp(dirlist[j].itemName,"")==0 ){
                //找到一个空闲记录，将新文件创建到这个位置
                posi = dno;
                posj = j;
            }
            else if(strcmp(dirlist[j].itemName,name)==0 ){
                //重名，取出inode，判断是否是文件
                Inode cur2;
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&cur2,sizeof(Inode),1,fr);
                if( ((cur2.i_mode>>9)&1)==0 ){	//是文件且重名，不能创建文件
                    printf("文件已存在\n");
                    buf[0] = '\0';
                    return false;
                }
            }
            i++;
        }

    }
    if(posi!=-1){	//之前找到一个目录项了
        //取出之前那个空闲目录项对应的磁盘块
        fseek(fr,cur.i_dirBlock[posi],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //创建这个目录项
        strcpy(dirlist[posj].itemName,name);	//文件名
        int chiinoAddr = ialloc();	//分配当前节点地址
        if(chiinoAddr==-1){
            printf("inode分配失败\n");
            return false;
        }
        dirlist[posj].inodeAddr = chiinoAddr; //给这个新的目录分配的inode地址

        //设置新条目的inode
        Inode p;
        p.i_ino = (chiinoAddr-Inode_StartAddr)/superblock->s_INODE_SIZE;
        p.i_atime = time(NULL);
        p.i_ctime = time(NULL);
        p.i_mtime = time(NULL);
        strcpy(p.i_uname,Cur_User_Name);
        strcpy(p.i_gname,Cur_Group_Name);
        p.i_cnt = 1;	//只有一个文件指向


        //将buf内容存到磁盘块
        int k;
        int len = strlen(buf);	//文件长度，单位为字节
        for(k=0;k<len;k+=superblock->s_BLOCK_SIZE){	//最多10次，10个磁盘快，即最多5K
            //分配这个inode的磁盘块，从控制台读取内容
            int curblockAddr = balloc();
            if(curblockAddr==-1){
                printf("block分配失败\n");
                return false;
            }
            p.i_dirBlock[k/superblock->s_BLOCK_SIZE] = curblockAddr;
            //写入到当前目录的磁盘块
            fseek(fw,curblockAddr,SEEK_SET);
            fwrite(buf+k,superblock->s_BLOCK_SIZE,1,fw);
        }


        for(k=len/superblock->s_BLOCK_SIZE+1;k<10;k++){
            p.i_dirBlock[k] = -1;
        }
        if(len==0){	//长度为0的话也分给它一个block
            int curblockAddr = balloc();
            if(curblockAddr==-1){
                printf("block分配失败\n");
                return false;
            }
            p.i_dirBlock[k/superblock->s_BLOCK_SIZE] = curblockAddr;
            //写入到当前目录的磁盘块
            fseek(fw,curblockAddr,SEEK_SET);
            fwrite(buf,superblock->s_BLOCK_SIZE,1,fw);

        }
        p.i_size = len;
        p.i_indirBlock_1 = -1;	//没使用一级间接块
        p.i_mode = 0;
        p.i_mode = MODE_FILE | FILE_DEF_PERMISSION;

        //将inode写入到申请的inode地址
        fseek(fw,chiinoAddr,SEEK_SET);
        fwrite(&p,sizeof(Inode),1,fw);

        //将当前目录的磁盘块写回
        fseek(fw,cur.i_dirBlock[posi],SEEK_SET);
        fwrite(dirlist,sizeof(dirlist),1,fw);

        //写回inode
        cur.i_cnt++;
        fseek(fw,parinoAddr,SEEK_SET);
        fwrite(&cur,sizeof(Inode),1,fw);
        fflush(fw);

        return true;
    }
    else
        return false;
}

bool del(int parinoAddr,char name[])		//删除文件函数。在当前目录下删除文件
{
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("超过最大目录名长度\n");
        return false;
    }

    //从这个地址取出inode
    Inode cur;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //取出目录项数
    int cnt = cur.i_cnt;

    //判断文件模式。6为owner，3为group，0为other
    int filemode;
    if(strcmp(Cur_User_Name,cur.i_uname)==0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)
        filemode = 3;
    else
        filemode = 0;

    if( ((cur.i_mode>>filemode>>1)&1)==0 ){
        //没有写入权限
        printf("权限不足：无写入权限\n");
        return false;
    }

    //依次取出磁盘块
    int i = 0;
    while(i<160){	//小于160
        dentry dirlist[16] = {0};

        if(cur.i_dirBlock[i/16]==-1){
            i+=16;
            continue;
        }
        //取出磁盘块
        int parblockAddr = cur.i_dirBlock[i/16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        //找到要删除的目录
        int pos;
        for(pos=0;pos<16;pos++){
            Inode tmp;
            //取出该目录项的inode，判断该目录项是目录还是文件
            fseek(fr,dirlist[pos].inodeAddr,SEEK_SET);
            fread(&tmp,sizeof(Inode),1,fr);

            if( strcmp(dirlist[pos].itemName,name)==0){
                if( ( (tmp.i_mode>>9) & 1 ) == 1 ){	//找到目录
                    //是目录，不管
                }
                else{
                    //是文件

                    //释放block
                    int k;
                    for(k=0;k<10;k++)
                        if(tmp.i_dirBlock[k]!=-1)
                            bfree(tmp.i_dirBlock[k]);

                    //释放inode
                    ifree(dirlist[pos].inodeAddr);

                    //删除该目录条目，写回磁盘
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

    printf("没有找到该文件!\n");
    return false;
}

//todo:实现一个能够根据目录名输出目录项的函数


void ls(int parinoAddr)		//显示当前目录下的所有文件和文件夹。参数：当前目录的inode节点地址
{
    Inode curinode;
    //取出这个inode
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&curinode, sizeof(Inode), 1, fr);
    fflush(fr);

    //取出目录项数
    int dentrycnt = curinode.i_cnt;

    //判断文件模式。6为owner，3为group，0为other
    int filemode;
    if(strcmp(Cur_User_Name, curinode.i_uname) == 0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name, curinode.i_gname) == 0)
        filemode = 3;
    else
        filemode = 0;

    if(((curinode.i_mode >> filemode >> 2) & 1) == 0 ){
        //没有读取权限
        printf("权限不足：无读取权限\n");
        return ;
    }

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

            if( strcmp(dirlist[j].itemName,"")==0 ){
                continue;
            }

            //输出信息
            if( ( (tmp.i_mode>>9) & 1 ) == 1 ){
                printf("d");
            }
            else{
                printf("-");
            }

            tm *ptr;	//存储时间
            ptr=gmtime(&tmp.i_mtime);

            //输出权限信息
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

            //其它
            printf("%d\t",tmp.i_cnt);	//链接
            printf("%s\t",tmp.i_uname);	//文件所属用户名
            printf("%s\t",tmp.i_gname);	//文件所属用户名
            printf("%d B\t",tmp.i_size);	//文件大小
            printf("%d.%d.%d %02d:%02d:%02d  ",1900+ptr->tm_year,ptr->tm_mon+1,ptr->tm_mday,(8+ptr->tm_hour)%24,ptr->tm_min,ptr->tm_sec);	//上一次修改的时间
            printf("%s",dirlist[j].itemName);	//文件名
            printf("\n");
            i++;
        }

    }
    /*  未写完 */

}


void cd(int parinoAddr,char name[])	//进入目标地址目录下指定的目录
{
    //取出当前目录的inode
    Inode curinode;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&curinode, sizeof(Inode), 1, fr);

    //依次取出inode对应的磁盘块，查找有没有名字为name的目录项
    int i = 0;

    //取出目录项数
    int cnt = curinode.i_cnt;

    //判断文件模式。6为owner，3为group，0为other
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
        //取出磁盘块
        int parblockAddr = curinode.i_dirBlock[i / 16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        //输出该磁盘块中的所有目录项
        int j;
        for(j=0;j<16;j++){
            if(strcmp(dirlist[j].itemName,name)==0){
                Inode tmp;
                //取出该目录项的inode，判断该目录项是目录还是文件
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&tmp,sizeof(Inode),1,fr);

                if( ( (tmp.i_mode>>9) & 1 ) == 1 ){
                    //找到该目录，判断是否具有进入权限
                    if( ((tmp.i_mode>>filemode>>0)&1)==0 && strcmp(Cur_User_Name,"root")!=0 ){	//root用户所有目录都可以查看
                        //没有执行权限
                        printf("权限不足：无执行权限\n");
                        return ;
                    }

                    //找到该目录项，如果是目录，更换当前目录

                    Cur_Dir_Addr = dirlist[j].inodeAddr;
                    if( strcmp(dirlist[j].itemName,".")==0){
                        //本目录，不动
                    }
                    else if(strcmp(dirlist[j].itemName,"..")==0){
                        //上一次目录
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
                    //找到该目录项，如果不是目录，继续找
                }

            }

            i++;
        }

    }

    //没找到
    printf("没有该目录\n");
    return ;

}


void gotoxy(HANDLE hOut, int x, int y)	//移动光标到指定位置
{
    COORD pos;
    pos.X = x;             //横坐标
    pos.Y = y;            //纵坐标
    SetConsoleCursorPosition(hOut, pos);
}

void vi(int parinoAddr,char name[],char buf[])	//模拟一个简单vi，输入文本，name为文件名
{
    //先判断文件是否已存在。如果存在，打开这个文件并编辑
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("超过最大文件名长度\n");
        return ;
    }

    //清空缓冲区
    memset(buf,0,sizeof(buf));
    int maxlen = 0;	//到达过的最大长度

    //查找有无同名文件，有的话进入编辑模式，没有进入创建文件模式
    dentry dirlist[16];	//临时目录清单

    //从这个地址取出inode
    Inode cur,fileInode;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //判断文件模式。6为owner，3为group，0为other
    int filemode;
    if(strcmp(Cur_User_Name,cur.i_uname)==0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)
        filemode = 3;
    else
        filemode = 0;

    int i = 0,j;
    int dno;
    int fileInodeAddr = -1;	//文件的inode地址
    bool isExist = false;	//文件是否已存在
    while(i<160){
        //160个目录项之内，可以直接在直接块里找
        dno = i/16;	//在第几个直接块里

        if(cur.i_dirBlock[dno]==-1){
            i+=16;
            continue;
        }
        fseek(fr,cur.i_dirBlock[dno],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //输出该磁盘块中的所有目录项
        for(j=0;j<16;j++){
            if(strcmp(dirlist[j].itemName,name)==0 ){
                //重名，取出inode，判断是否是文件
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&fileInode,sizeof(Inode),1,fr);
                if( ((fileInode.i_mode>>9)&1)==0 ){	//是文件且重名，打开这个文件，并编辑
                    fileInodeAddr = dirlist[j].inodeAddr;
                    isExist = true;
                    goto label;
                }
            }
            i++;
        }
    }
    label:

    //初始化vi
    int cnt = 0;
    system("cls");	//清屏

    int winx,winy,curx,cury;

    HANDLE handle_out;                              //定义一个句柄
    CONSOLE_SCREEN_BUFFER_INFO screen_info;         //定义窗口缓冲区信息结构体
    COORD pos = {0, 0};                             //定义一个坐标结构体

    if(isExist){	//文件已存在，进入编辑模式，先输出之前的文件内容

        //权限判断。判断文件是否可读
        if( ((fileInode.i_mode>>filemode>>2)&1)==0){
            //不可读
            printf("权限不足：没有读权限\n");
            return ;
        }

        //将文件内容读取出来，显示在，窗口上
        i = 0;
        int sumlen = fileInode.i_size;	//文件长度
        int getlen = 0;	//取出来的长度
        for(i=0;i<10;i++){
            char fileContent[1000] = {0};
            if(fileInode.i_dirBlock[i]==-1){
                continue;
            }
            //依次取出磁盘块的内容
            fseek(fr,fileInode.i_dirBlock[i],SEEK_SET);
            fread(fileContent,superblock->s_BLOCK_SIZE,1,fr);	//读取出一个磁盘块大小的内容
            fflush(fr);
            //输出字符串
            int curlen = 0;	//当前指针
            while(curlen<superblock->s_BLOCK_SIZE){
                if(getlen>=sumlen)	//全部输出完毕
                    break;
                printf("%c",fileContent[curlen]);	//输出到屏幕
                buf[cnt++] = fileContent[curlen];	//输出到buf
                curlen++;
                getlen++;
            }
            if(getlen>=sumlen)
                break;
        }
        maxlen = sumlen;
    }

    //获得输出之后的光标位置
    handle_out = GetStdHandle(STD_OUTPUT_HANDLE);   //获得标准输出设备句柄
    GetConsoleScreenBufferInfo(handle_out, &screen_info);   //获取窗口信息
    winx = screen_info.srWindow.Right - screen_info.srWindow.Left + 1;
    winy = screen_info.srWindow.Bottom - screen_info.srWindow.Top + 1;
    curx = screen_info.dwCursorPosition.X;
    cury = screen_info.dwCursorPosition.Y;


    //进入vi
    //先用vi读取文件内容

    int mode = 0;	//vi模式，一开始是命令模式
    unsigned char c;
    while(1){
        if(mode==0){	//命令行模式
            c=getch();

            if(c=='i' || c=='a'){	//插入模式
                if(c=='a'){
                    curx++;
                    if(curx==winx){
                        curx = 0;
                        cury++;

                        /*
                        if(cury>winy-2 || cury%(winy-1)==winy-2){
                            //超过这一屏，向下翻页
                            if(cury%(winy-1)==winy-2)
                                printf("\n");
                            SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                            int i;
                            for(i=0;i<winx-1;i++)
                                printf(" ");
                            gotoxy(handle_out,0,cury+1);
                            printf(" - 插入模式 - ");
                            gotoxy(handle_out,0,cury);
                        }
                        */
                    }
                }

                if(cury>winy-2 || cury%(winy-1)==winy-2){
                    //超过这一屏，向下翻页
                    if(cury%(winy-1)==winy-2)
                        printf("\n");
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                    int i;
                    for(i=0;i<winx-1;i++)
                        printf(" ");
                    gotoxy(handle_out,0,cury+1);
                    printf(" - 插入模式 - ");
                    gotoxy(handle_out,0,cury);
                }
                else{
                    //显示 "插入模式"
                    gotoxy(handle_out,0,winy-1);
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                    int i;
                    for(i=0;i<winx-1;i++)
                        printf(" ");
                    gotoxy(handle_out,0,winy-1);
                    printf(" - 插入模式 - ");
                    gotoxy(handle_out,curx,cury);
                }

                gotoxy(handle_out,curx,cury);
                mode = 1;


            }
            else if(c==':'){
                //system("color 09");//设置文本为蓝色
                if(cury-winy+2>0)
                    gotoxy(handle_out,0,cury+1);
                else
                    gotoxy(handle_out,0,winy-1);
                _COORD pos;
                if(cury-winy+2>0)
                    pos.X = 0,pos.Y = cury+1;
                else
                    pos.X = 0,pos.Y = winy-1;
                SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                int i;
                for(i=0;i<winx-1;i++)
                    printf(" ");

                if(cury-winy+2>0)
                    gotoxy(handle_out,0,cury+1);
                else
                    gotoxy(handle_out,0,winy-1);

                WORD att = BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_INTENSITY; // 文本属性
                FillConsoleOutputAttribute(handle_out, att, winx, pos, NULL);	//控制台部分着色
                SetConsoleTextAttribute(handle_out, FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_GREEN  );	//设置文本颜色
                printf(":");

                char pc;
                int tcnt = 1;	//命令行模式输入的字符计数
                while( c = getch() ){
                    if(c=='\r'){	//回车
                        break;
                    }
                    else if(c=='\b'){	//退格，从命令条删除一个字符
                        //SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
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
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                    system("cls");
                    break;	//vi >>>>>>>>>>>>>> 退出出口
                }
                else{
                    if(cury-winy+2>0)
                        gotoxy(handle_out,0,cury+1);
                    else
                        gotoxy(handle_out,0,winy-1);
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                    int i;
                    for(i=0;i<winx-1;i++)
                        printf(" ");

                    if(cury-winy+2>0)
                        gotoxy(handle_out,0,cury+1);
                    else
                        gotoxy(handle_out,0,winy-1);
                    SetConsoleTextAttribute(handle_out, FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_GREEN  );	//设置文本颜色
                    FillConsoleOutputAttribute(handle_out, att, winx, pos, NULL);	//控制台部分着色
                    printf(" 错误命令");
                    //getch();
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                    gotoxy(handle_out,curx,cury);
                }
            }
            else if(c==27){	//ESC，命令行模式，清状态条
                gotoxy(handle_out,0,winy-1);
                SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                int i;
                for(i=0;i<winx-1;i++)
                    printf(" ");
                gotoxy(handle_out,curx,cury);

            }

        }
        else if(mode==1){	//插入模式

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
            printf("[行:%d,列:%d]",curx==-1?0:curx,cury);
            gotoxy(handle_out,curx,cury);

            c = getch();
            if(c==27){	// ESC，进入命令模式
                mode = 0;
                //清状态条
                gotoxy(handle_out,0,winy-1);
                SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                int i;
                for(i=0;i<winx-1;i++)
                    printf(" ");
                continue;
            }
            else if(c=='\b'){	//退格，删除一个字符
                if(cnt==0)	//已经退到最开始
                    continue;
                printf("\b");
                printf(" ");
                printf("\b");
                curx--;
                cnt--;	//删除字符
                if(buf[cnt]=='\n'){
                    //要删除的这个字符是回车，光标回到上一行
                    if(cury!=0)
                        cury--;
                    int k;
                    curx = 0;
                    for(k = cnt-1;buf[k]!='\n' && k>=0;k--)
                        curx++;
                    gotoxy(handle_out,curx,cury);
                    printf(" ");
                    gotoxy(handle_out,curx,cury);
                    if(cury-winy+2>=0){	//翻页时
                        gotoxy(handle_out,curx,0);
                        gotoxy(handle_out,curx,cury+1);
                        SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                        int i;
                        for(i=0;i<winx-1;i++)
                            printf(" ");
                        gotoxy(handle_out,0,cury+1);
                        printf(" - 插入模式 - ");

                    }
                    gotoxy(handle_out,curx,cury);

                }
                else
                    buf[cnt] = ' ';
                continue;
            }
            else if(c==224){	//判断是否是箭头
                c = getch();
                if(c==75){	//左箭头
                    if(cnt!=0){
                        cnt--;
                        curx--;
                        if(buf[cnt]=='\n'){
                            //上一个字符是回车
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
                else if(c==77){	//右箭头
                    cnt++;
                    if(cnt>maxlen)
                        maxlen = cnt;
                    curx++;
                    if(curx==winx){
                        curx = 0;
                        cury++;

                        if(cury>winy-2 || cury%(winy-1)==winy-2){
                            //超过这一屏，向下翻页
                            if(cury%(winy-1)==winy-2)
                                printf("\n");
                            SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                            int i;
                            for(i=0;i<winx-1;i++)
                                printf(" ");
                            gotoxy(handle_out,0,cury+1);
                            printf(" - 插入模式 - ");
                            gotoxy(handle_out,0,cury);
                        }

                    }
                    gotoxy(handle_out,curx,cury);
                }
                continue;
            }
            if(c=='\r'){	//遇到回车
                printf("\n");
                curx = 0;
                cury++;

                if(cury>winy-2 || cury%(winy-1)==winy-2){
                    //超过这一屏，向下翻页
                    if(cury%(winy-1)==winy-2)
                        printf("\n");
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                    int i;
                    for(i=0;i<winx-1;i++)
                        printf(" ");
                    gotoxy(handle_out,0,cury+1);
                    printf(" - 插入模式 - ");
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
            //移动光标
            curx++;
            if(curx==winx){
                curx = 0;
                cury++;

                if(cury>winy-2 || cury%(winy-1)==winy-2){
                    //超过这一屏，向下翻页
                    if(cury%(winy-1)==winy-2)
                        printf("\n");
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // 恢复原来的属性
                    int i;
                    for(i=0;i<winx-1;i++)
                        printf(" ");
                    gotoxy(handle_out,0,cury+1);
                    printf(" - 插入模式 - ");
                    gotoxy(handle_out,0,cury);
                }

                buf[cnt++] = '\n';
                if(cnt>maxlen)
                    maxlen = cnt;
                if(cury==winy){
                    printf("\n");
                }
            }
            //记录字符
            buf[cnt++] = c;
            if(cnt>maxlen)
                maxlen = cnt;
        }
        else{	//其他模式
        }
    }
    if(isExist){	//如果是编辑模式
        //将buf内容写回文件的磁盘块

        if( ((fileInode.i_mode>>filemode>>1)&1)==1 ){	//可写
            writefile(fileInode,fileInodeAddr,buf);
        }
        else{	//不可写
            printf("权限不足：无写入权限\n");
        }

    }
    else{	//是创建文件模式
        if( ((cur.i_mode>>filemode>>1)&1)==1){
            //可写。可以创建文件
            create(parinoAddr,name,buf);	//创建文件
        }
        else{
            printf("权限不足：无写入权限\n");
            return ;
        }
    }
}

void writefile(Inode fileInode,int fileInodeAddr,char buf[])	//将buf内容写回文件的磁盘块
{
    //将buf内容写回磁盘块
    int k;
    int len = strlen(buf);	//文件长度，单位为字节
    for(k=0;k<len;k+=superblock->s_BLOCK_SIZE){	//最多10次，10个磁盘快，即最多5K
        //分配这个inode的磁盘块，从控制台读取内容
        int curblockAddr;
        if(fileInode.i_dirBlock[k/superblock->s_BLOCK_SIZE]==-1){
            //缺少磁盘块，申请一个
            curblockAddr = balloc();
            if(curblockAddr==-1){
                printf("block分配失败\n");
                return ;
            }
            fileInode.i_dirBlock[k/superblock->s_BLOCK_SIZE] = curblockAddr;
        }
        else{
            curblockAddr = fileInode.i_dirBlock[k/superblock->s_BLOCK_SIZE];
        }
        //写入到当前目录的磁盘块
        fseek(fw,curblockAddr,SEEK_SET);
        fwrite(buf+k,superblock->s_BLOCK_SIZE,1,fw);
        fflush(fw);
    }
    //更新该文件大小
    fileInode.i_size = len;
    fileInode.i_mtime = time(NULL);
    fseek(fw,fileInodeAddr,SEEK_SET);
    fwrite(&fileInode,sizeof(Inode),1,fw);
    fflush(fw);
}



void gotoRoot()	//回到根目录
{
    memset(Cur_User_Name,0,sizeof(Cur_User_Name));		//清空当前用户名
    memset(Cur_User_Dir_Name,0,sizeof(Cur_User_Dir_Name));	//清空当前用户目录
    Cur_Dir_Addr = Root_Dir_Addr;	//当前用户目录地址设为根目录地址
    strcpy(Cur_Dir_Name,"/");		//当前目录设为"/"
}


void touch(int parinoAddr,char name[],char buf[])	//touch命令创建文件，读入字符
{
    //先判断文件是否已存在。如果存在，打开这个文件并编辑
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("超过最大文件名长度\n");
        return ;
    }
    //查找有无同名文件，有的话提示错误，退出程序。没有的话，创建一个空文件
    dentry dirlist[16];	//临时目录数组

    //从这个地址取出inode
    Inode cur,fileInode;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //判断文件模式。6为owner，3为group，0为other
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
        //160个目录项之内，可以直接在直接块里找
        dno = i/16;	//在第几个直接块里

        if(cur.i_dirBlock[dno]==-1){
            i+=16;
            continue;
        }
        fseek(fr,cur.i_dirBlock[dno],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //输出该磁盘块中的所有目录项
        for(j=0;j<16;j++){
            if(strcmp(dirlist[j].itemName,name)==0 ){
                //重名，取出inode，判断是否是文件
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&fileInode,sizeof(Inode),1,fr);
                if( ((fileInode.i_mode>>9)&1)==0 ){	//是文件且重名，提示错误，退出程序
                    printf("文件已存在\n");
                    return ;
                }
            }
            i++;
        }
    }

    //文件不存在，创建一个空文件
    if( ((cur.i_mode>>filemode>>1)&1)==1){
        //可写。可以创建文件
        buf[0] = '\0';
        create(parinoAddr,name,buf);	//创建文件
    }
    else{
        printf("权限不足：无写入权限\n");
        return ;
    }

}

void help()	//显示所有命令清单
{
    printf("ls - 显示当前目录清单\n");
    printf("cd - 进入目录\n");
    printf("mkdir - 创建目录\n");
    printf("rmdir - 删除目录\n");
    printf("tree - 查看目录项的红黑树\n");
    printf("vi - vi编辑器\n");
    printf("touch - 创建一个空文件\n");
    printf("rm - 删除文件\n");
    printf("cls - 清屏\n");
    printf("help - 显示命令清单\n");
    printf("exit - 退出系统\n");
    return ;
}

void cmd(char str[])	//处理输入的命令
{
    char p1[10];
    char p2[100]={0};
    char buf[100000];	//最大100K
    int tmp = 0;
    int i;
    sscanf(str,"%s",p1);
    if(strcmp(p1,"ls")==0){
        sscanf(str,"%s%s",p1,p2);
        if(p2[0]!=NULL) {//如果指定了目录,显示指定的目录

            rbtree_node *p = rbtree_search(&dir_tree, p2);

            if (p)//找到了该目录
                ls(p->value);
        }
        else{
            ls(Cur_Dir_Addr);	//否则显示当前目录
        }
    }
    else if(strcmp(p1,"cd")==0){
        sscanf(str,"%s%s",p1,p2);
        if(p2[0]!='/'){
            //没有用绝对路径
            cd(Cur_Dir_Addr,p2);
        }

        else{
            //输入的是绝对路径
            rbtree_node *p = rbtree_search(&dir_tree,p2);

            if(p)//找到了目录
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
    else if(strcmp(p1,"vi")==0){	//创建一个文件
        sscanf(str,"%s%s",p1,p2);
        vi(Cur_Dir_Addr,p2,buf);	//读取内容到buf
    }
    else if(strcmp(p1,"touch")==0){
        sscanf(str,"%s%s",p1,p2);
        touch (Cur_Dir_Addr,p2,buf);	//读取内容到buf
    }
    else if(strcmp(p1,"rm")==0){	//删除一个文件
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
        printf("输入help查看命令\n");
    }


    return ;
}
