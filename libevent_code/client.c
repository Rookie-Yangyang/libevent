#include <stdio.h>
#include <event.h>
#include <getopt.h>

#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
  
#include<event.h>
#include<event2/util.h>

void input_cb(int sockfd, short events, void* arg);
void read_cb(int sockfd, short events, void *arg);


void print_help(char *progname)
{
	printf("The project :%s\n",progname);
	printf("-i(--ipaddr):specify server ip\n");
	printf("-p(--port):specify derver port\n");
	printf("-h(--help):print help information!\n");
}
/*socket 初始化*/
int socket_connect(char *server_ip,int server_port)
{
	int           sockfd;
	int           connfd;

	struct sockaddr_in    servaddr;

	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if( sockfd<0 )
	{
		printf("Create socket failure:%s\n",strerror(errno));
		return -1;
	}
	printf("Create socket successfully!\n");

	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(server_port);
	inet_aton(server_ip,&servaddr.sin_addr);
	connfd=connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	if( connfd<0 )
	{
		printf("Connect to server failure:%s\n",strerror(errno));
		return -2;
	}
	printf("Connect to server successfully!\n");
	evutil_make_socket_nonblocking(sockfd);

	return sockfd;
}

int main(int argc, char** argv)
{
	int        ch;
	int        sockfd;
	char       *server_ip=NULL;
	int        server_port=0;

	struct option opt[]={
		{"server_ip",required_argument,NULL,'i'},
		{"server_port",required_argument,NULL,'p'},
		{"help",no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};


	while( (ch=getopt_long(argc,argv,"i:p:h",opt,NULL))!=-1 )
	{
		switch(ch)
		{
			case 'i':
				server_ip=optarg;
				break;
			case 'p':
				server_port=atoi(optarg);
				break;
			case 'h':
				print_help(argv[0]);
				return 0;
		}
	}

	if(!server_port)
	{
		print_help(argv[0]);
		return 0;
	}

	sockfd=socket_connect(server_ip,server_port);
	if( sockfd<0 )
	{
		printf("connect to server failure!\n");
		return -1;
	}
	printf("connect to server successfully!\n");

	struct event_base* base = event_base_new();  

	struct event *sockevent = event_new(base, sockfd,  EV_READ | EV_PERSIST,  read_cb, NULL);  
	event_add(sockevent, NULL);  

	//监听终端输入事件  
	struct event* ev_input = event_new(base, STDIN_FILENO,  EV_READ | EV_PERSIST, input_cb,  (void*)&sockfd);  


	event_add(ev_input, NULL);  

	event_base_dispatch(base);  
	printf("finished \n");  
	return 0;  
}

void read_cb(int fd, short events, void *arg)
{
	char       buf[1024];
	int        rv;
	
	rv=read(fd,buf,sizeof(buf));
	if( rv<=0 )
	{
		printf("read data from server %dfailure!\n",fd);
		exit(1);
	}

	printf("read %d data from server:%s\n",rv,buf);

}

void input_cb(int fd, short events, void* arg)
{
	char    buf[1024];
	int     rv;

	rv=read(fd,buf,sizeof(buf));
	if( rv<=0 )
	{
		printf("read failure!\n");
		exit(1);
	}

	int sockfd = *((int*)arg);
	write(sockfd,buf,rv);
}
