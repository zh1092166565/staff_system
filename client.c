#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>        
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>




#define N 32


#define R 1          //user - register   注册
#define L 2          //user - login    登录
#define Q_M 3        //user - Query information    查询信息
#define CH_PW 4      //user - Change the password  修改密码
#define MY_INFOR  5  //uaer - Modify personal information修改个人信息
#define Q_H  6       //user - Query history查询历史记录

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

int directory(int sockfd,MSG *msg);


int do_register(int sockfd,MSG *msg)
{
	char employee_number =0;
	
	msg->type = R;
	printf("input name:");
	scanf("%s",msg->name);
	getchar();
	
	printf("input age:");
	scanf("%s",msg->age);
	getchar();
	
	printf("input gender:");
	scanf("%s",msg->gender);
	getchar();
	
	printf("input passwd:");
	scanf("%s",msg->password);
	getchar();

	printf("input your job_number:");
	scanf("%s",msg->job_number);
	getchar();
	
	/*
	printf("input your department:");
	scanf("%s",msg->department);
	getchar();
	*/


	
	if (send(sockfd,msg,sizeof(MSG),0) < 0)
	{
		printf("fail to send.\n");
		return -1;
	}
	if(recv(sockfd ,msg ,sizeof(MSG),0) <0)
	{
		printf("fail to recv");
		return -1;
	}
	
	//ok ! or usr alread exist
	printf("%s\n",msg->data);
	
	return 0;
}

int do_login(int sockfd,MSG *msg)
{
	msg->type = L;
	printf("input name:");
	scanf("%s",msg->name);
	getchar();
	
	printf("input passwd:");
	scanf("%s",msg->password);
	getchar();
	
	
	if (send(sockfd,msg,sizeof(MSG),0) < 0)
	{
		printf("fail to send.\n");
		return -1;
	}
	if(recv(sockfd ,msg ,sizeof(MSG),0) <0)
	{
		printf("fail to recv");
		return -1;
	}
	
	if(strncmp(msg->data,"ok",3) == 0)
	{
		printf("login ok!\n");
		return 1;
	}
	else
	{
		printf("%s\n",msg->data);
	}
	
	return 0;
}

int do_query_information(int sockfd,MSG *msg)
{
	msg->type =Q_M;
	
	puts("------------------");
	printf("Query information is ok \n");
	
	return 0;
}


int do_change_password(int sockfd,MSG *msg)
{
	msg->type = CH_PW;
	
	printf("Confirm account modification:");
	scanf("%s",msg->name);
	getchar();
	
	printf("input new passwd:");
	scanf("%s",msg->password);
	getchar();
	
	printf("Please confirm the new password:%s\n",msg->password);
	
	if (send(sockfd,msg,sizeof(MSG),0) < 0)
	{
		printf("fail to send.\n");
		return -1;
	}
	if(recv(sockfd ,msg ,sizeof(MSG),0) <0)
	{
		printf("fail to recv");
		return -1;
	}
	
	if(strncmp(msg->data,"ok",3) == 0)
	{
		printf("login ok!\n");
		return 1;
	}
	else
	{
		printf("%s\n",msg->data);
	}	
	
	return 0;
}

int do_modify_information(int sockfd,MSG *msg)
{
	msg->type = MY_INFOR;
	int n;
	printf("Please enter employee number:");
	scanf("%s",msg->job_number);
	getchar();
	
	printf("Options to allow modifications: 1.name  2.age 3.gender\n");
	
	printf("Select the options you want to modify:");
	scanf("%d",n);
	getchar();
	
	//printf("data= %d\n",n);
	//msg->data = (char)n;
	n =0;
	printf("data1= %d\n",msg->infor);
	if (n == 1)
	{
		msg->infor = 1;
		printf("Please enter a new name:");
		scanf("%s",msg->name);
		getchar();
	}
	
	if (n == 2)
	{
		msg->infor = 2;
		printf("Please enter a new age:");
		scanf("%s",msg->age);
		getchar();
	}
	
	if (n == 3)
	{
		msg->infor = 3;
		printf("Please enter a new gender:");
		scanf("%s",msg->gender);
		getchar();
	}
	
	
	
	if (send(sockfd,msg,sizeof(MSG),0) < 0)
	{
		printf("fail to send.\n");
		return -1;
	}
	if(recv(sockfd ,msg ,sizeof(MSG),0) <0)
	{
		printf("fail to recv");
		return -1;
	}
	
	if(strncmp(msg->data,"ok",3) == 0)
	{
		printf("login ok!\n");
		return 1;
	}
	else
	{
		printf("%s\n",msg->data);
	}	
	
	return 0;
	
}

int do_query_history (int sockfd,MSG *msg)
{
	msg->type = Q_H;
	
	send(sockfd,msg,sizeof(MSG),0);
	
	//接收服务器传递胡来的历史记录信息
	while(1)
	{
		recv(sockfd ,msg,sizeof(MSG),0);
		
		if( msg->data[0] == '\0')
			break;
		//打印输出历史记录信息
		printf("%s\n",msg->data);
	}
	return 1;

}

// ./server 192.168.1.160 5001
int main(int argc,const char *argv[])
{
	
	int sockfd;
	
	struct sockaddr_in serveraddr;
	
	int n;
	MSG msg;
	
	if(argc !=3)
	{
		printf("Usage:%s serverip port.\n",argv[0]);
		return -1;
	}
	
	if( (sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		perror("fail to socket.\n");
		return -1;
	}
	
	bzero(&serveraddr,sizeof(serveraddr));
	
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
	serveraddr.sin_port = htons(atoi(argv[2]));
	
	if (connect(sockfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)) < 0)
	{
		perror("fail to connect");
		return -1;
	}
	
	
	while(1)
	{
		printf("**************************************************\n");
		printf("*         1.register    2.login     3.quit       *\n");
		printf("**************************************************\n");
		printf("please choose: ");
		
		scanf("%d",&n);
		getchar();
		
		switch(n)
		{
			case 1:
				do_register(sockfd,&msg);
				break;
			case 2:
				if( do_login(sockfd,&msg) == 1)
				{
					directory(sockfd,&msg);
				}
				break;
			case 3:
				close(sockfd);
				exit(0);
				break;
			default:
				printf("invalid data cmd.\n");
			
				
		}
	}	
	
	return 0;
}

int directory(int sockfd,MSG *msg)
{
	int n;
	while(1)
		{
			printf("*************************************************************\n");
			printf("*        1.Query information    2.Change the password       *\n");
			printf("* 3.Modify personal information   4.Query history   5.exit  *\n");
			printf("*************************************************************\n");
			printf("please choose:");
			scanf("%d",&n);
			getchar();
			
			switch(n)
			{
				case 1:
					do_query_information(sockfd,&msg);
					break;
				case 2:
					if (do_change_password(sockfd,&msg) == 1)
					{
						directory(sockfd,&msg);
					}
					break;
				case 3:
					do_modify_information(sockfd,&msg);
					break;
				case 4:
					if (do_query_history (sockfd,&msg) == 1)
					{
						directory(sockfd,&msg);
					}
					break;
				case 5:
					close(sockfd);
					exit(0);
					break;
				default:
					printf("invalid data cmd.\n");
			}
		}
		
	return ;
}