/*
 *	CSE 533 Network Programming
 *	Assignment #2, Due: Friday, October 31st
 *	Name: Yiqi Yu, ID: 109215949
 *	File: udpcli.c
 */

#include "client.h"
#include "cli_srv.h"
#include	"unpifiplus.h"
#include	<string.h>
#include 	<arpa/inet.h>
#include	<setjmp.h>
#include 	<sys/socket.h>
#include 	<unpthread.h>
#include 	<math.h>

pthread_t pid;
struct arg_client cli1;

int n;

//this struct saved all datapacket in one sender window size
//in case need to retransmit
typedef struct{
    char data[MAXLINE];
}clibuf;

clibuf *rwnd;


pthread_mutex_t	mutex=PTHREAD_MUTEX_INITIALIZER;
struct sockaddr_in* finsrvaddr, fincliaddr;

struct mesg msgsend,msgrecv;
int dgsize=512-sizeof(struct mesghdr);//datagram packet size
int totalconsume=0;
int totalsrvsent=0;
int expectseq=0;
int bufposition;
//available client window length
int cliWinNow;
//fixed client recv window length
int cliWin;
int finACK=0;
int probe=0;
int getfin=0;
uint32_t recvTS;
char sendline[MAXLINE], recvline[MAXLINE];
int a;
float p;
//dropped=0 means not drop,=1 means drop
int dropped=0;

bool if_drop(float prob){
	float val = (float)(rand() % 100) / 100;
    // printf("%f\n", val);
	if(val >= prob) {
        //printf("client processed package\n");
        return true;
	}
    else {
        //printf("client dropped package\n");
		return false;
	}
}

//here we anaylze the packet we get.
//If it's exactly the packet wanted, we handle it and put content in to rwndto print,update all global ack and seq, send back to server,tell next right packet.
//if it's not the packtet wanted,simply ignore it,but send mesg back to tell server"I want right packet" by use ackn as correct expectseq
ssize_t handleACK(int* fd, void *inbuff, size_t inbytes, struct sockaddr_in* srvaddr){
    
    char output[MAXLINE];
    
    Recvfrom(*fd,&msgrecv,512,0,NULL, NULL);
    
    
    
    if(if_drop(p)){
        dropped=0;
        //Recvfrom(*fd,&msgrecv,512,0,NULL, NULL);
        fprintf(stderr,"we get package # %d\n\n",msgrecv.header.seq_num);
        // fprintf(stderr,"%s\n\n",msgrecv.payload);
        //special cases
        if(msgrecv.header.seq_num==-2){
            //probing
            probe=1;
            printf("Get probing packet\n\n");
        }
        else if(msgrecv.header.seq_num==-8){
            getfin=1;
            printf("File sent is completed.Get FIN from server \n\n");
        }
        
        else if(msgrecv.header.seq_num==-9){
            finACK=1;
            printf("Final handshake completed,Closing...\n\n");
            
        }
        
        else if(msgrecv.header.seq_num==expectseq){
            n=expectseq+1;
            fprintf(stderr,"Got correct packet %d\n\n",msgrecv.header.seq_num);
            
            Pthread_mutex_lock(&mutex);
            
            if(rwnd[msgrecv.header.seq_num%cliWin].data[0]=='\0'){
                fprintf(stderr,"The slot %d in receive window is still availiable,put data in receiver buffer\n\n",msgrecv.header.seq_num%cliWin);
                
                strcpy(rwnd[(msgrecv.header.seq_num)%cliWin].data,msgrecv.payload);
                //fprintf(stderr,"rwnd data %s",rwnd[(msgrecv.header.seq_num)%cliWin].data);
                //cliWinNow--;
                expectseq++;
                recvTS=msgrecv.header.timestamp;
                //msgsend.header.seq_num=expectseq;
                //msgsend.header.window_size=cliWinNow;
            }
            
            else{
                fprintf(stderr,"Packets in rwnd not consumed yet\n\n");
            }
            
            Pthread_mutex_unlock(&mutex);
            
            
        }
        
    }
    
    else{
        dropped=1;
        fprintf(stderr,"Received packet %d is dropped,wait for server to send another\n\n",msgrecv.header.seq_num);
    }
    
    
    return n;
}

//here we send datagram back to server
//If last time we got exactly the packet wanted, we handled it and send packetback to server,tell next right packet.
//if didn't get right packet in last one,still send require for right packet.
//if keeps send request for same packet,don't need to change recvpacket much.
ssize_t sendACK(int* fd){
    
    
    ssize_t	n;
    //if is the right seq,send ack to server
    
    //fprintf(stderr,"received msg seq is #%d\n",msgrecv.header.seq_num);
    //fprintf(stderr,"Expected seq is now %d\n",expectseq);
    
    
    Pthread_mutex_lock(&mutex);
    
    if(probe==1){
        msgsend.header.ackn=-2;//send probe packets
        msgsend.header.window_size=cliWin-expectseq+totalconsume;
        if(msgsend.header.window_size>0){
            probe=0;
        }
        //NOTICE THIS LINE SHOULD BE DELETED,IT'S HERE BECAUSE reader thread doesn't work...
        //cliWinNow=5;
    }
    //send out packet,ackn is -9,tell server cli is waiting for fin
    else if(getfin==1&&finACK==0){
        msgsend.header.ackn=-9;
    }
    else if(finACK==1){
        msgsend.header.ackn=-10;
    }
    
    else if((msgrecv.header.seq_num+1)==expectseq)
    {
        
        msgsend.header.ackn=msgrecv.header.seq_num+1;
        msgsend.header.window_size=cliWin-expectseq+totalconsume;
        msgsend.header.timestamp=recvTS;
        fprintf(stderr,"Now send ack %d to server\n\n",msgsend.header.ackn);
    }
    //didn't get right packet,request the right packet again
    else{
        
        msgsend.header.ackn=expectseq;
        msgsend.header.window_size=cliWin-expectseq+totalconsume;
        msgsend.header.timestamp=recvTS;
        fprintf(stderr,"Didn't got right packet,keeps send ack to server\n\n");    }
    
    Pthread_mutex_unlock(&mutex);
    
    
    //test part
    if(if_drop(p)){
        dropped=0;
        Sendto(*fd, &msgsend,sizeof(msgsend), 0, NULL, NULL);
        fprintf(stderr,"******we are sending ACK # %d******\n\n",msgsend.header.ackn);
        //fprintf(stderr,"The client receive window is now # %d\n\n",msgsend.header.window_size);
    }
    //drop packet
    else{
        dropped=1;
        fprintf(stderr,"The sending ACK # %d is dropped,waiting server response...\n\n",msgsend.header.ackn);
    }
    
    
    return n;
}

float mu;

static void* readthread(){
    // fprintf(stderr,"\n[in read thread]\n");
    
    float r = ( drand48() / (double)(RAND_MAX / 2 + 1) );
    // printf("random num: %f\n", r);
    // printf("log(r): %f\n", log(r));
    float sleeptime = -1 * mu * (log(r));
    // printf("sleeptime: %f\n", sleeptime);
    
    for(;;){
        
              
        
           // printf("\n[in readthread()]\n");
        //we also need to set random()here,later...
        usleep(sleeptime);
        //usleep(30000);
        
        Pthread_mutex_lock(&mutex);
        
        while(totalconsume < expectseq){
            //still have packet to be print
            bufposition=totalconsume%cliWin;
            
            r = ( drand48() / (double)(RAND_MAX / 2 + 1) );
            sleeptime = -1 * mu * (log(r));
            
            if(rwnd[bufposition].data[0]!='\0'){
                printf("\n[readthread processing data packet # %d]\n",totalconsume);
                printf("\n[print data begin]\n%s\n",rwnd[bufposition].data);
                //clean up data
                rwnd[bufposition].data==NULL;
                rwnd[bufposition].data[0]='\0';
                totalconsume++;
                printf("\n[print data finish]\n\n");            }
            
        }
        Pthread_mutex_unlock(&mutex);
        
        
        //only when finish printing.exit
        if(finACK){
            if(totalconsume==expectseq){
                printf("Final handshake, reader thread close\n\n");
                break;
            }
            
        }
    }

}

static struct rtt_info_plus rttinfo;
static sigjmp_buf jmpbuf;
static void sigalrm_handler(int);

static bool check_local(char*, char*);
static int client_init(int*, struct arg_client*, char*,
                       struct sockaddr_in*, struct sockaddr_in*);

int main(int argc, char** argv) {
    int err;
    struct arg_client cli;
    
    err = read_in("client.in", "client", (void*)&cli);
    if(-1 == err) {
        printf("invalid input in client.in\n");
        exit(EXIT_FAILURE);
    }
    printf("\n[client config]\n");
    printf("ip address of server: %s\n", cli.srv_ip);
    printf("well-known port number of server: %d\n", cli.port_num);
    printf("filename to be transfered: %s\n", cli.filename);
    printf("receiving sliding window size: %d\n", cli.window_size);
    printf("random generator seed value: %d\n", cli.seed);
    //here we generate random seed
    srand(cli.seed);
    //srand(time(NULL));
    cliWin=cli.window_size;
    struct timeval time;
    gettimeofday(&time,NULL);
    srand48((unsigned int) time.tv_usec);
    printf("probability p of datagram loss: %f\n", cli.prob);
    p=cli.prob;
    if(cli.prob > 1.0 || cli.prob < 0.0) {
        printf("invalid probablity of datagram loss, quit program\n");
        return 0;
    }
    printf("mean u for exponential distribution: %d\n", cli.mu);
    mu = cli.mu;
    
    // for timeout handler
    if(SIG_ERR == signal(SIGALRM, sigalrm_handler)) {
        perror("signal()");
        exit(EXIT_FAILURE);
    }
    
    char cli_ip[16];
    bool islocal;
    int sockfd, optval = 1;
    struct sockaddr_in srvaddr, cliaddr;
    
    islocal = check_local(cli.srv_ip, cli_ip);
    printf("\n[setting up client UDP socket]\n");
    printf("resolved IPserver: %s\n", cli.srv_ip);
    printf("resolved IPclient: %s\n", cli_ip);
    
    if(islocal == true) {
        printf("client and server are on the same network\n");
        setsockopt(sockfd, SOL_SOCKET, SO_DONTROUTE, &optval, sizeof(optval));
    }
    else {
        printf("client and server are NOT local\n");
    }
    
    err = client_init(&sockfd, &cli, cli_ip, &srvaddr, &cliaddr);
    if(err == -1) {
        ;
    }
    
    exit(EXIT_SUCCESS);
}

void sigalrm_handler(int signo) {
    // printf("%s\n", __func__);
    
    siglongjmp(jmpbuf, 1);
    // longjmp(jmpbuf, 1);
}

bool check_local(char* srv_ip, char* cli_ip) {
    bool islocal = false, samehost = false;
    char *cptr;
    unsigned long ip, mask, max;
    struct ifi_info	*ifi, *ifihead;
	struct sockaddr	*sa;
    
    struct sockaddr_in tmp;
    inet_pton(AF_INET, srv_ip, &(tmp.sin_addr));
    
    printf("\n[client network interface info]\n");
	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 1);
		 ifi != NULL; ifi = ifi->ifi_next) {
        
		printf("name: %s, ", ifi->ifi_name);
        
		if ( (sa = ifi->ifi_addr) != NULL) {
            cptr = Sock_ntop_host(sa, sizeof(*sa));
			printf("ip address: %s, ", cptr);
            if(strcmp(srv_ip, cptr) == 0)
                samehost = true;
        }
        
		if ( (sa = ifi->ifi_ntmaddr) != NULL) {
            cptr = Sock_ntop_host(sa, sizeof(*sa));
			printf("network mask: %s\n", cptr);
        }
        
        ip = ((struct sockaddr_in*)ifi->ifi_addr)->sin_addr.s_addr;
        mask = ((struct sockaddr_in*)ifi->ifi_ntmaddr)->sin_addr.s_addr;
        
        if((ip & mask) == (tmp.sin_addr.s_addr & mask) ) {
            islocal = true;
            
            if(mask > max) {
                max = mask;
                sa = ifi->ifi_addr;
                cptr = Sock_ntop_host(sa, sizeof(*sa));
                strcpy(cli_ip, cptr);
            }
        }
	}
	free_ifi_info_plus(ifihead);
    
    if (samehost == true) {
        printf("client and server are on the same host\n");
        strcpy(srv_ip, "127.0.0.1");
        strcpy(cli_ip, "127.0.0.1");
        return true;
    }
    
    if (islocal == true) {
        printf("client recognizes server as local\n");
        return true;
    }
    
    // not local, pick up a ip for client
    for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 1);
         ifi != NULL; ifi = ifi->ifi_next) {
        if ( (sa = ifi->ifi_addr) != NULL) {
            cptr = Sock_ntop_host(sa, sizeof(*sa));
            if(strcmp(cptr, "127.0.0.1") != 0) {
                strcpy(cli_ip, cptr);
                break;
            }
        }
    }
    
    return false;
}

int client_init(int* sockfd, struct arg_client* cli, char* cli_ip,
                struct sockaddr_in* srvaddr, struct sockaddr_in* cliaddr) {
    int err;
    int len = sizeof(struct sockaddr_in);
    struct sockaddr_in tmpaddr;
    
    // set client sockaddr_in
    bzero(cliaddr, sizeof(*cliaddr));
    cliaddr->sin_family = AF_INET;
    cliaddr->sin_port = htons(0);
    Inet_pton(AF_INET, cli_ip, &cliaddr->sin_addr);
    
    // get sockfd as UDP socket
    *sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
    
    // bind sockfd
    Bind(*sockfd, (SA*)cliaddr, sizeof(*cliaddr));
    
    // use getsockname() to get client port number
    bzero(&tmpaddr, sizeof(tmpaddr));
    Getsockname(*sockfd, (SA*)&tmpaddr, &len);
    printf("client ip: %s\n", Sock_ntop_host((SA*)&tmpaddr, sizeof(tmpaddr)));
    cli->cli_port_num = ntohs(tmpaddr.sin_port);
    printf("client port number: %d\n", cli->cli_port_num);
    
    // set server sockaddr_in
    bzero(srvaddr, sizeof(*srvaddr));
    srvaddr->sin_family = AF_INET;
    srvaddr->sin_port = htons(cli->port_num);
    Inet_pton(AF_INET, cli->srv_ip, &srvaddr->sin_addr);
    //here's connect coresponding to server's connect
    Connect(*sockfd, (SA*)srvaddr, sizeof(*srvaddr));
    if(err != 0) {
        ;
    }
    
    // getpeername()
    bzero(&tmpaddr, sizeof(tmpaddr));
    Getpeername(*sockfd, (SA*)&tmpaddr, &len);
    printf("server ip: %s\n", Sock_ntop_host((SA*)&tmpaddr, sizeof(tmpaddr)));
    printf("server port number: %d\n", ntohs(tmpaddr.sin_port));
    
    // init rttinfo
    rtt_init(&rttinfo);
    rtt_newpack(&rttinfo);
    struct mesg message;
    bzero(&message, sizeof(struct mesg));
    strcpy(message.payload, cli->filename);
    message.header.timestamp = htonl(rtt_ts(&rttinfo));
    
send_again_1:
    printf("\n[first handshake: client -> server]\n");
    if(if_drop(cli->prob))
        Sendto(*sockfd, &message, sizeof(struct mesg), 0, NULL, NULL);
    
    start_timer(rtt_start(&rttinfo));
    
    if(0 != sigsetjmp(jmpbuf, 1)) {
        if(rtt_timeout(&rttinfo) < 0) {
            printf("no response from server, give up\n");
            return -ETIMEDOUT;
        }
        printf("requst timeout, retransmitting\n");
        goto send_again_1;
    }
    
    for(; ;) {
        bzero(&message, sizeof(struct mesg));
        err = recvfrom(*sockfd, &message, sizeof(struct mesg), 0,
                       (SA*)&tmpaddr, &len);
        if(err < 0){
            if(errno == EINTR) {
                continue;
            }
            else {
                perror("recvfrom()");
                return -1;
            }
        }
        else {
            printf("\n[received package: server(child) -> client]\n");
            if(if_drop(cli->prob)) {
                stop_timer();
                rtt_stop(&rttinfo, rtt_ts(&rttinfo) - ntohl(message.header.timestamp));
                break;
            }
            else {
                bzero(&message, sizeof(struct mesg));
                continue;
            }
        }
    }
    printf("\n[second handshake: server(child) -> client]\n");
    printf("server(child) port num: %s\n", message.payload);
    srvaddr->sin_port = htons(atoi(message.payload));
    err = connect(*sockfd, (SA*)srvaddr, sizeof(*srvaddr));
    if(err < 0) {
        perror("connect()");
        return -1;
    }
    
    /*
     //RECEIVE SERVER PORT 3
     fd_set rset;
     FD_ZERO(&rset);
     FD_SET(*sockfd, &rset);
     Recvfrom(*sockfd, &message, sizeof(struct mesg), 0,(SA*)&tmpaddr, &len);
     
     printf("\n[second handshake: server(child) -> client]\n");  printf("server(child) port num: %s\n", message.payload);
     srvaddr->sin_port = htons(atoi(message.payload));
     //FINALLY SET SERVER ADDRESS,CONNECT()
     Connect(*sockfd, (SA*)srvaddr, sizeof(*srvaddr));
     if(err < 0) {
     ;
     }
     */
    
    bzero(&message, sizeof(struct mesg));
    strcpy(message.payload, cli->filename);
    message.header.window_size=cliWin;
    //this is the ack reply to server's send
    //message.header.ackn=1;
    //strcpy(message.payload, "we're finally there!!!");
    Sendto(*sockfd, &message, sizeof(struct mesg), 0, NULL, NULL);
    
    printf("\n[third handshake: client -> server(child)]\n");
    printf("\n[Connection established, data transmission begins]\n");
    
    rwnd=malloc(cliWin*sizeof(clibuf));
    for(a=0;a<cliWin;a++){
        rwnd[a].data[0]='\0';
    }
    //creat a thread to print
    Pthread_create(&pid,NULL,&readthread,NULL);
    bzero(&message, sizeof(struct mesg));
    
    
    //formly start data transfer
    while(1){
        
        handleACK(sockfd,recvline,MAXLINE,srvaddr);
        
    
        if(dropped==0){
            
            sendACK(sockfd);
        }
    fin:
        if(finACK==1){
            if(dropped==0){
            //sleep for a few seconds so reader can print all contents
            sleep(3);
            printf("Final handshake,Client close\n");
                exit(0);}
            else{
                sendACK(sockfd);
                goto fin;
            }
            
        }
    }
}
