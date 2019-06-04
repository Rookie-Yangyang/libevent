#include <stdio.h>
#include <event.h>
#include <getopt.h>
#include <assert.h>
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

void accept_cb(int listenfd, short events, void* arg);
void read_cb(int clifd, short events, void* arg);

/*参数解析*/
void print_help(char *progname)
{
	printf("The project :%s\n",progname);
	printf("-p(--port):specify derver port\n");
	printf("-h(--help):print help information!\n");
}

/*socket初始化*/
int socket_init(char *server_ip,int server_port)
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

	/*允许多次绑定同一个地址。要用在socket和bind之间*/  
	evutil_make_listen_socket_reuseable(sockfd); 
	
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(server_port);
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY); 

	if( (bind(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr)))<0 )
	{
		printf("bind failure!\n");
		return -2;
	}

	listen(sockfd,64);

	/*跨平台统一接口，将套接字设置为非阻塞状态*/
	evutil_make_socket_nonblocking(sockfd);

	return sockfd;
}


int main(int argc,char **argv)
{
	int         listenfd;
	int         ch;
	int         port;

	struct option opt[]={
		{"port",required_argument,NULL,'p'},
		{"help",no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};

	while( (ch=getopt_long(argc,argv,"p:h",opt,NULL))!=-1 )
	{
		switch(ch)
		{
			case 'p':
				port=atoi(optarg);
				break;
			case 'h':
				print_help(argv[0]);
				return 0;
		}
	}
	printf("port:%d\n",port);

	if( !port )
	{
		print_help(argv[0]);
		return 0;
	}


	listenfd=socket_init(NULL,port);

	if( listenfd<0 )
	{
		printf("socket_init failure!\n");
		return -1;
	}
	printf("socket_init successfully!\n");
	
	/*创建一个event_base*/
	struct event_base *base = event_base_new();
	assert(base != NULL);

	/*创建并绑定一个event*/
	struct event* listen_event;

	listen_event=event_new(base,listenfd,EV_READ | EV_PERSIST,accept_cb,base);

	/*添加监听事件*/
	event_add(listen_event,NULL);

	/*启动循环，开始处理事件*/
	event_base_dispatch(base);

	return 0;
}

/*回调函数accept_cb*/
void accept_cb(int fd, short events, void* arg)
{
	struct sockaddr_in    cliaddr;
	evutil_socket_t       clifd;
	socklen_t             len=sizeof(cliaddr);

	clifd=accept(fd,(struct sockaddr*)&cliaddr,&len);
	if( clifd<0 )
	{
		printf("accept client %d failure!\n",clifd);
		close(clifd);
	}

	evutil_make_socket_nonblocking(clifd);
	printf("accept client %d successfully!\n",clifd);

	struct event_base* base = (struct event_base*)arg;
	/*动态创建一个event结构体，并将其作为回调参数传递给*/
	struct event* cli_event = event_new(NULL, -1, 0, NULL, NULL);
	event_assign(cli_event,base, clifd, EV_READ | EV_PERSIST,read_cb, (void*)cli_event);

	event_add(cli_event,NULL);



}

/*accept的回调函数read_cb*/
void read_cb(int fd, short events, void* arg)
{
	int        rv;
	char       buf[1024];
	struct event* ev = (struct event*)arg;

	rv=read(fd,buf,sizeof(buf));
	if( rv<=0 )
	{
		printf("read message failure!\n");
		event_free(ev);
		close(fd);
		return ;
	}

	printf("read message successfully:%s\n",buf);

	char reply_buf[1024] = "I have received the msg: ";
	strcat(reply_buf + strlen(reply_buf), buf);
	write(fd,reply_buf,sizeof(reply_buf));
}
