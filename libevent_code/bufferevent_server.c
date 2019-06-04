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
#include<event2/bufferevent.h>  
#include<event2/buffer.h>


void accept_cb(int listenfd, short events, void* arg);
void read_cb(struct bufferevent* bev, void* arg);
void write_cb(struct bufferevent* bev,void* arg);
void event_cb(struct bufferevent *bev, short event, void *arg);


/*参数解析*/
void print_help(char *progname)
{
	printf("The project :%s\n",progname);
	printf("-p(--port):specify derver port\n");
	printf("-h(--help):print help information!\n");
}/*socket初始化*/
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
	struct event_base *base=event_base_new();


	/*创建一个事件*/
	struct event* listenevent;
	listenevent=event_new(base,listenfd,EV_READ | EV_PERSIST,accept_cb,base);

	event_add(listenevent,NULL);

	/*启动事件循环*/
	event_base_dispatch(base);

	return 0;
}

/*回调函数accept_cb*/
void accept_cb(int fd,short events,void* arg)
{
	struct sockaddr_in      cliaddr;
	evutil_socket_t         clifd;
	socklen_t               len=sizeof(cliaddr);


	clifd=accept(fd,(struct sockaddr*)&cliaddr,&len);
	if( clifd<0 )
	{
		printf("Accept new client failure!\n");
		close(clifd);
	}
	printf("Accept new client[%d] successfully!\n",clifd);

	struct event_base* base = (struct event_base*)arg;

	/*使用bufferevent_socket_new创建一个struct bufferevent* bev，关联上面的clifd*/
	struct bufferevent* bev=bufferevent_socket_new(base,clifd,BEV_OPT_CLOSE_ON_FREE);


	/*使用bufferevent_setcb(bev, read_cb, write_cb, error_cb, (void*)arg)*/
	bufferevent_setcb(bev, read_cb, write_cb, event_cb, (void*)arg);

	/*使用buffevent_enable(bev, EV_READ|EV_WRITE|EV_PERSIST)来启动read/write事件*/
	bufferevent_enable(bev, EV_READ|EV_WRITE|EV_PERSIST);

	/*printf("Finished!\n");*/
}

/*回调函数read_cb*/
void read_cb(struct bufferevent* bev, void* arg)
{
	char        buf[1024];
	
	size_t      rv;

	memset(buf,0,sizeof(buf));
	rv=bufferevent_read(bev,buf,sizeof(buf));

	printf("Read data from client is:%s\n",buf);

	char reply_buf[1024] = "I have recvieced the msg: ";

	strcat(reply_buf + strlen(reply_buf), buf);
	bufferevent_write(bev, reply_buf, strlen(reply_buf));

}

/*读取终端输入，发送给客户端*/
void write_cb(struct bufferevent* bev,void* arg)
{

}

/*回调函数event_cb*/
void event_cb(struct bufferevent *bev, short event, void *arg)
{
	if (event & BEV_EVENT_EOF)
		printf("connection closed\n");
	else if (event & BEV_EVENT_ERROR)
		printf("some other error\n");

	/*这将自动close套接字和free读写缓冲区*/
	bufferevent_free(bev);
}
