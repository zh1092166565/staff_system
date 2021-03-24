#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>        
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include <unistd.h>

#include <sqlite3.h>
#include <signal.h>

#include <time.h>
#define N 32


#define R 1          //user - register   注册
#define L 2          //user - login    登录
#define Q_M 3        //user - Query information    查询信息
#define CH_PW 4      //user - Change the password  修改密码
#define MY_INFOR  5  //uaer - Modify personal information修改个人信息
#define Q_H  6       //user - Query history查询历史记录

#define DATABASE "my.db"

//定义通信双方的结构体信息
/*
	操作类型
	名字
	年龄：
	性别：
	密码：
	工号：
	工资：
	部门
	数据
*/
typedef struct
{
	int type;
	int infor;
	char name[N];    
	char age[N];
	char gender[N];
	char password[N];
	char job_number[N];
	char salary[N];
	char department[256];
	char data[1024];
}MSG;

//管理员权限
int root_authority = 0;

int flag = 0;

int do_client(int acceptfd,sqlite3 *db);
void do_register(int acceptfd,MSG *msg,sqlite3 *db);
int do_login(int acceptfd,MSG *msg,sqlite3 *db);
int do_query_information(int acceptfd,MSG *msg,sqlite3 *db);
int do_change_password(int acceptfd,MSG *msg,sqlite3 *db);
int do_modify_information(int sockfd,MSG *msg,sqlite3 *db);
int do_query_history (int sockfd,MSG *msg,sqlite3 *db);
int callback(void *arg,int ncolumn,char **text,char **name);

// ./server 192.168.1.160 5001
int main(int argc,const char *argv[])
{
	
	int sockfd;
	
	struct sockaddr_in serveraddr;
	
	int n;
	MSG msg;
	sqlite3 *db;
	int acceptfd;
	pid_t pid;
	
	if(argc !=3)
	{
		printf("Usage:%s serverip port.\n",argv[0]);
		return -1;
	}
	
	//打开数据库
	if(sqlite3_open(DATABASE,&db) != SQLITE_OK)
	{
		printf("%s\n",sqlite3_errmsg(db));
		return -1;
	}
	else 
	{
		printf("open DATABASE success.\n");
	}
	
	if( (sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		perror("fail to socket.\n");
		return -1;
	}
	
	//允许快速重用
	int b_reuse =-1;
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&b_reuse,sizeof(int));
	
	bzero(&serveraddr,sizeof(serveraddr));
	
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
	serveraddr.sin_port = htons(atoi(argv[2]));
		
	if(bind(sockfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)) < 0)
	{
		perror("fail to bind.\n");
		return -1;
	}
	
	//将套接字设置为监听模式等待客户端请求到来
	if(listen(sockfd,5) <0)
	{
		printf("fail to listen.\n");
		return -1;
	}
	
	//处理僵尸进程
	signal(SIGCHLD,SIG_IGN);
	
	
	while(1)
	{
		if( (acceptfd = accept(sockfd,NULL,NULL)) < 0)
		{
			perror("fail to accept");
			return -1;
		}
		
		if((pid = fork()) < 0)
		{
			perror("fail to fork");
			return -1;
		}
		else if(pid ==0)  //子进程
		{
			//处理客户端具体消息
			close(sockfd);
			do_client(acceptfd,db);
		}
		else //父进程   用来接收客户端的请求
		{
			close(acceptfd);
		}
		
		
	}
	
	
	return 0;
}


int do_client(int acceptfd,sqlite3 *db)
{
	MSG msg;
	while(recv(acceptfd,&msg,sizeof(msg),0) >0)
	{
		printf("type:%d ",msg.type);
		switch(msg.type)
		{
			case R:
				do_register(acceptfd,&msg,db);
				break;
			case L:
				do_login(acceptfd,&msg,db);
				break;
			case Q_M:
				do_query_information(acceptfd,&msg,db);
				break;
			case CH_PW:
				do_change_password(acceptfd,&msg,db);
				break;
			case MY_INFOR:
				do_modify_information(acceptfd,&msg,db);
				break;
			case Q_H:
				do_query_history (acceptfd,&msg,db);
				break;
			default:
				printf("invalid data cmd.\n");
		}
			
	}
	
	printf("client exit.\n");
	close(acceptfd);
	exit(0);
	
	return 0;
}


void do_register(int acceptfd,MSG *msg,sqlite3 *db)
{
	char *errmsg;
	char sql[128];
	
	sprintf(sql,"insert into usr values('%s',%s,'%s',%s,%s,%s,%s);",msg->name,msg->age,msg->gender,msg->password,msg->job_number,NULL,NULL);
	printf("%s\n",sql);
	if( sqlite3_exec(db,sql,NULL,NULL,&errmsg ) != SQLITE_OK )
	{
		printf("%s\n",errmsg);
		strcpy(msg->data,"usr name already exist.");
	}
	else
	{
		printf("client register ok !\n");
		strcpy(msg->data,"ok!");
	}
	
	if (send(acceptfd,msg ,sizeof(MSG),0) <0)
	{
		perror("fail to send.");
		return ;
	}
	return ;
}

int do_login(int acceptfd,MSG *msg,sqlite3 *db)
{
	char sql[128] = {};
	char *errmsg;
	int nrow;
	int ncloumn;
	char **resultp;
	char root[32] = {"root"};
	char pasword[256] = {"1"};
	
	//判定是否是管理员登录
	if( *(msg->name) == *root && *(msg->password) == *pasword)
	{
		nrow =1;
		root_authority = 1;
		printf("root_authority = %d\n",root_authority);
	}
	else
	{
		sprintf(sql,"select *from usr where name = '%s' and password = '%s';",msg->name,msg->password);
		printf("%s\n",sql);

			
		if(sqlite3_get_table(db,sql,&resultp,&nrow,&ncloumn,&errmsg) != SQLITE_OK )
		{
			printf("%s\n",errmsg);
			return -1;
		}
		else
		{
			printf("get_table ok!.\n");
		}
	}
	

	
	//查询成功，数据库中拥有此用户
	if(nrow ==1)
	{
		strcpy(msg->data,"ok");
		send(acceptfd,msg,sizeof(MSG),0);
		return 1;
	}
	else // 密码或者用户名错误
	{
		strcpy(msg->data,"usr/passwd wrong.");
		send(acceptfd,msg,sizeof(MSG),0);
		return 0;
	}
	return 1;
}

int do_query_information(int acceptfd,MSG *msg,sqlite3 *db)
{
	return 0;
}

int do_change_password(int acceptfd,MSG *msg,sqlite3 *db)
{
	char *errmsg;
	char sql[128];
	
	sprintf(sql,"update usr set password=%s where name='%s';",msg->password,msg->name);
	printf("%s\n",sql);
	if( sqlite3_exec(db,sql,NULL,NULL,&errmsg ) != SQLITE_OK )
	{
		printf("%s\n",errmsg);
		strcpy(msg->data,"usr name already exist.");
	}
	else
	{
		printf("Change the password is ok !\n");
		strcpy(msg->data,"ok!");
	}
	
	if (send(acceptfd,msg ,sizeof(MSG),0) <0)
	{
		perror("fail to send.");
		return ;
	}
	return ;
}
int do_modify_information(int sockfd,MSG *msg,sqlite3 *db)
{
	char *errmsg;
	char sql[128];
	
	if(msg->infor == 1)
	{
		sprintf(sql,"update usr set name='%s' where job_number=%s;",msg->name,msg->job_number);
		printf("%s\n",sql);
		if( sqlite3_exec(db,sql,NULL,NULL,&errmsg ) != SQLITE_OK )
		{
			printf("%s\n",errmsg);
			strcpy(msg->data,"usr name already exist.");
		}
		else
		{
			printf("Change the password is ok !\n");
			strcpy(msg->data,"ok!");
		}
	}
	if(msg->infor == 2)
	{
		sprintf(sql,"update usr set age=%s where job_number=%s;",msg->age,msg->job_number);
		printf("%s\n",sql);
		if( sqlite3_exec(db,sql,NULL,NULL,&errmsg ) != SQLITE_OK )
		{
			printf("%s\n",errmsg);
			strcpy(msg->data,"usr name already exist.");
		}
		else
		{
			printf("Change the password is ok !\n");
			strcpy(msg->data,"ok!");
		}
		
	}
	if(msg->infor == 3)
	{
		sprintf(sql,"update usr set gender='%s' where job_number=%s;",msg->gender,msg->job_number);
		printf("%s\n",sql);
		if( sqlite3_exec(db,sql,NULL,NULL,&errmsg ) != SQLITE_OK )
		{
			printf("%s\n",errmsg);
			strcpy(msg->data,"usr name already exist.");
		}
		else
		{
			printf("Change the password is ok !\n");
			strcpy(msg->data,"ok!");
		}
	}
	if (send(sockfd,msg ,sizeof(MSG),0) <0)
		{
			perror("fail to send.");
			return ;
		}
	
	return 0;
}
int do_query_history (int sockfd,MSG *msg,sqlite3 *db)
{
	char *errmsg;
	char sql[128];
	
	sprintf(sql,"select * from usr;");
	printf("%s\n",sql);
	if(sqlite3_exec(db,sql,callback,(void *)&sockfd,&errmsg) != SQLITE_OK)
		printf("error:%s\n",errmsg);
	else
		printf("select OK\n");
	
	//所有的记录查询发送完毕之后，给客户端发送一个额结束信息
	msg->data[0] ='\0';
	send(sockfd,msg,sizeof(MSG),0);
	return 0;
}

int callback(void *arg,int ncolumn,char **text,char **name)
{
	//执行 sqlite3_exec 此回调函数会被调用多次
	//调用的次数是行数+1
	int i;
	
	int sockfd;
	MSG msg;
	sockfd = *((int*)arg);

	//printf("ncolumn = %d\n",ncolumn);
	//由于第一行是表头，将其通过标志位打印隔离出来
	if(flag == 0){
		for(i = 0; i < ncolumn;i++)
			//注意此处是name -- 字段名称
			printf("%-10s",name[i]);
		putchar(10);
		printf("------------------------------------\n");
		flag = 1;
	}

	for(i = 0;i < ncolumn;i++)
		//注意此处是text -- 字段值
		printf("%-10s",text[i]);
	putchar(10);
	
	sprintf(msg.data,"%s , %s,%s , %s,%s , %s",text[0],text[1],text[2],text[3],text[4],text[5]);
	send(sockfd,&msg ,sizeof(MSG),0);

	return 0;

}







