/*
 *	CSE 533 Network Programming
 *	Assignment #2, Due: Friday, October 31st
 *	Name: Yiqi Yu, ID: 109215949
 *	File: udpsrv.c
 */

#include "server.h"
#include "cli_srv.h"
#include "unprtt_plus.h"
#include	<setjmp.h>
#include "unp.h"
#include "unpifiplus.h"
#include <stdlib.h>
#include <stdio.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
//variables for transfer file

int timeout=0;
int connfd;
int allSeq=0;
int cwnd=1,ssthresh=127;
//slowstart is valid at beginning
int slowstart=1;
//the first packet we haven't got ack yet
int waitackp=0;
//the next to be transmitted packet
int nextp=0;
//ack global
int acknow=0;
int finACK=0;
int finsend=0;
int sendLen=0;
int lastACK=0;
int currACK=0;
int dupACK=0;
int tobeSent=0;
int dgSize=512-sizeof(struct mesghdr);//datagram packet size
int srvwin;
//all seg sent
int sentSeg=0;

int adWinNow;// the intial advertised window size
//send window size avaliable
int sendWin;
//persist timer
int pstTimer=0;

struct mesg msgsend,msgrecv;

//this struct saved all datapacket in one sender window size
//in case need to retransmit
typedef struct{
    uint32_t seq; /* sequence # */
    uint32_t timestamp;
    char data[MAXLINE];
}save;

save *backup;

static int				 rttinit = 0;
static struct rtt_info_plus rttinfo;
static sigjmp_buf	jmpbuf;
static void sig_alrm( int signo )
{
	siglongjmp( jmpbuf, 1 );
}
//variables for transfer file

//begin of data transfer function

void sendAgain(int sockfd);
//we get ACK and deal with it
int handleACK(int fd){
    ssize_t			n;
    
    Recvfrom(fd, &msgrecv, sizeof(struct mesg), 0,NULL, NULL);
    
    //we got the right ack waiting for
    //if the ack number is even bigger,means packets before this ack arrive!CHANGED
    
    
    if(msgrecv.header.ackn==-2){
        fprintf(stderr,"Probing packet is back........\n\n");
        if(msgrecv.header.window_size>0){
            fprintf(stderr,"Receive window is open now, windowsize is %d\n\n",msgrecv.header.window_size);
            adWinNow=msgrecv.header.window_size;
        }
        else{
            fprintf(stderr,"Receive window is still 0, resend probe packet after 4 seconds.....\n\n");
            sleep(4);
            sendAgain(fd);
        }
    }
    
    else if(msgrecv.header.ackn==-9){
        //we got the fin
        fprintf(stderr,"Get FIN ACK# %d......\n\n",msgrecv.header.ackn);
        finACK=1;
    }
    else if(msgrecv.header.ackn==-10){
        //we got the fin
        fprintf(stderr,"Get Closing ACK# %d......\n\n",msgrecv.header.ackn);
       stop_timer();
    
    }
    else if((waitackp+1<=msgrecv.header.ackn)&&(msgrecv.header.ackn>=0)){
        //stop_timer();
        timeout=0;
        rtt_stop(&rttinfo, rtt_ts(&rttinfo) - ntohl(msgrecv.header.timestamp));
        
        if(waitackp+1==msgrecv.header.ackn){
            fprintf(stderr,"Get right ACK# %d.....\n\n",msgrecv.header.ackn);
        
        }
        else if(waitackp+1<msgrecv.header.ackn){

            fprintf(stderr,"Get bigger ACK# %d than expected,all packet before this received!\n\n",msgrecv.header.ackn);
        }
        
        while(waitackp<msgrecv.header.ackn){
            //backup[waitackp%srvwin].data=NULL;
            
            
            backup[waitackp%srvwin].data[0]='\0';
            waitackp++;
        }
        
        // rtt_stop(&rttinfo,rtt_ts(&rttinfo) - msgrecv.header.timestamp);
        // rtt_newpack( &rttinfo );
        
        //update the client window avaliable;ack
        adWinNow=msgrecv.header.window_size;
        waitackp=msgrecv.header.ackn;
        //fprintf(stderr,"waitACKp is # %d\n\n",msgrecv.header.ackn);
        
        //slowstart detect
        if(cwnd>=ssthresh){
            slowstart=0;
            cwnd=cwnd+1/cwnd;
            fprintf(stderr,"Congestion avoidance stage.\nCWND increase by 1/cwnd for each right ack, CWND is now %d \n\n",cwnd);
        }
        else if(slowstart==1&&cwnd<ssthresh){
            cwnd++;
            fprintf(stderr,"Slow start stage,CWND increase by 1 for each right ack,cwnd size is %d\n\n",cwnd);
        }
    }
    else{
        fprintf(stderr,"Didn't get right ACK,got ACK # %d instead\n\n",msgrecv.header.ackn);
        
    }
    
    currACK=msgrecv.header.ackn;
    
    if(lastACK!=currACK){
        dupACK==0;
        lastACK=currACK;
    }
    else{
        dupACK++;
    }
    
    
    //update new ACK to sender function
    return msgrecv.header.ackn;
}

//retransfer datagram with given ACK
void retransmit(int fd,int ack){

    int position;
    position=(ack%srvwin);
    
    //we get normal ack
    
    if(ack>=0){
     strcpy(msgsend.payload,backup[position].data);
        
          }
        
    //backup[position].timestamp=msgsend.header.timestamp;
    //use wrapped or not...
    //retransmit completed
    if(ack>=0){
        fprintf(stderr,"we are retransmitting sequence # %d.....\n\n",ack);
    }
    
    else{
        if(ack==-8){
        fprintf(stderr,"we are transmitting FIN sequence......\n\n");
        }
        else if(ack==-9){
        fprintf(stderr,"we got FIN ACK, now transmitting closing FIN ......\n\n");
        }
        else if(ack==-2){
            fprintf(stderr,"Probing packet is send......\n\n");
        }
    }
    
    msgsend.header.seq_num=ack;
    msgsend.header.timestamp = rtt_ts(&rttinfo);
    
    sendto(fd, &msgsend,512, 0, NULL, NULL);
    start_timer(rtt_start(&rttinfo));
}

//send datagram again, But will judge which to send
void sendAgain(int sockfd){
    
    //TIMEOUT happened
    if(adWinNow>0&&finsend==0){
        fprintf(stderr,"Timeout,we enter sendAgain()\n\n");
        retransmit(sockfd,waitackp);
	}
    //do probing
    else if(adWinNow==0){
        //usleep(10);
        retransmit(sockfd,-2);
    }
    
    //means time out,and it's the time out for the FIN server sent.keep send -8 as fin until ack -9 send back
    if(finsend==1&&finACK==0){
        retransmit(sockfd,-8);
    }
    //means time out,and may be client dropped the #-9 sequence it get.
    if(finACK==1){
        retransmit(sockfd,-9);
    }
    //reset cwnd, ssthresh
    
    cwnd=1;
    ssthresh=MIN(cwnd,adWinNow);
    ssthresh=MAX(ssthresh/2,2);
       slowstart=1;
}



//this method set header and packet all info together
//outbytes is length send out
//we can return sth for this method
void sendDatagram(int fd, char* outbuff,size_t outbytes){
    ssize_t	n;
    //bzero(&msgsend, 512);
    msgsend.header.seq_num=allSeq;
    int position;
    position=(allSeq%srvwin);
    
    strcpy(msgsend.payload,outbuff);
    
    //fprintf(stderr,"%s\n\n",msgsend.payload);
    
    
    //put it here or in transfer()?
    Signal(SIGALRM, sig_alrm);
    rtt_newpack( &rttinfo );
    msgsend.header.timestamp = rtt_ts(&rttinfo);
    
    //back up the content we're sending and its ts
    strcpy(backup[position].data,msgsend.payload);
    backup[position].timestamp=msgsend.header.timestamp;
    
    
    int err = 0;
    // Sendto(fd, &msgsend,sizeof(struct mesg), 0,NULL, NULL);
    fprintf(stderr,"******we are sending sequence # %d******\n\n",allSeq);
    err = sendto(fd, &msgsend, sizeof(msgsend), 0, NULL, NULL);
    nextp++;
    //fprintf(stderr,"Next package to be sent is %d\n\n",nextp);
    
    if(err < 0) {
        perror("sendto()");
        exit(EXIT_FAILURE);
    }
    
    allSeq++;
}//end of sendDatagram



static int server_init(struct srv_ifi_info*, struct arg_server*);
static bool is_new_client(struct srv_ifi_info*, struct sockaddr_in*);
static int register_client(struct srv_ifi_info*, struct sockaddr_in*,
                           int, pid_t);
// static bool is_local(struct srv_ifi_info*, int);
static int handle_client(struct srv_ifi_info*, struct sockaddr_in*,
                         int, int, int);


int main(int argc, char** argv) {
    int err;
    struct arg_server srv;
    
    err = read_in("server.in", "server", (void*)&srv);
    if(-1 == err) {
        printf("invalid input in server.in\n");
        exit(EXIT_FAILURE);
    }
    printf("\n[server config]\n");
    printf("well-known port numebr for server: %d\n", srv.port_num);
    printf("maximum sending sliding window size: %d\n", srv.window_size);
    srvwin=srv.window_size;
    // get the number of interfaces
    int if_cnt = 0;
    struct ifi_info *ifi, *ifihead;
	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 1);
         ifi != NULL; ifi = ifi->ifi_next)
        if_cnt++;
    
    // fill the interface info array
    struct srv_ifi_info conns[if_cnt];
    err = server_init(conns, &srv);
    if(err < 0) {
        ;
    }
    
    // monitor all interfaces
    int i, len = sizeof(struct sockaddr_in), max_fd;
    fd_set set_read, master;
    struct mesg message;
    struct sockaddr_in cliaddr;
    
    struct timeval tval;
    tval.tv_sec = 2;
    tval.tv_usec = 0;
    
    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sin_family = AF_INET;
    
    for(i = 0; i < if_cnt; i++) {
        FD_SET(conns[i].sockfd, &set_read);
        if(max_fd < conns[i].sockfd) {
            max_fd = conns[i].sockfd;
        }
    }
    master = set_read;
    
    printf("\n[server listening]\n");
    for(;;) {
        set_read = master;
    select_restart:
        //err = select(max_fd + 1, &set_read, NULL, NULL, &tval);
        err = select(max_fd + 1, &set_read, NULL, NULL, NULL);
        if(err < 0) {
            if(errno == EINTR)
                goto select_restart;
            perror("select()");
            exit(EXIT_FAILURE);
        }
        
        for(i = 0; i < if_cnt; ++i) {
            if(FD_ISSET(conns[i].sockfd, &set_read)) {
                bzero(&message, sizeof(struct mesg));
                recvfrom(conns[i].sockfd, (void*)&message, sizeof(struct mesg), 0,
                         (SA*)&cliaddr, &len);
                printf("\n[first handshake: client -> server]\n");
                
                if(is_new_client(&conns[i], &cliaddr)) {
                    
                    printf("client ip %s\n",
                           Sock_ntop_host((SA*)&cliaddr, sizeof(cliaddr)));
                    printf("client port number %d\n", cliaddr.sin_port);
                    printf("client request filename: %s\n", message.payload);
                    
                    int fd_pipe[2];
                    pid_t pid_child;
                    pipe(fd_pipe);
                    
                    pid_child = fork();
                    if(pid_child > 0) {
                        printf("in parent process\n");
                        close(fd_pipe[1]);
                        register_client(&conns[i], &cliaddr, fd_pipe[0], pid_child);
                        
                    }
                    else if(pid_child == 0) {
                        fprintf(stderr,"in child process\n");
                        close(fd_pipe[0]);
                        handle_client(&conns[0], &cliaddr, if_cnt, i, fd_pipe[1]);
                        
                        exit(EXIT_SUCCESS);
                    }
                    else {
                        perror("fork()");
                        exit(EXIT_FAILURE);
                    }
                }
                else {
                    printf("already spawn a child process for handling\n");
                } // end of if_new_client()
            } // end of FD_ISSET()
        } // end of polling interfaces
    }  // end of for loop
    
    exit(EXIT_SUCCESS);
}

int server_init(struct srv_ifi_info conns[], struct arg_server* srv) {
    int err = 0, i = 0, optval = 1;
    struct sockaddr* sa;
    struct ifi_info *ifi, *ifihead;
    
    printf("\n[server network interface info]\n");
	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 1);
         ifi != NULL; ifi = ifi->ifi_next, i++) {
        
        bzero(&conns[i], sizeof(struct srv_ifi_info));
        
        conns[i].sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        
        err = setsockopt(conns[i].sockfd, SOL_SOCKET, SO_REUSEADDR,
                         &optval, sizeof(optval));
        if(err < 0) {
            perror("setsockopt()");
            exit(EXIT_FAILURE);
        }
        
        sa = ifi->ifi_addr;
        ((struct sockaddr_in*)sa)->sin_family = AF_INET;
        ((struct sockaddr_in*)sa)->sin_port = htons(srv->port_num);
        Bind(conns[i].sockfd, sa, sizeof(*sa));
        
        strcpy(conns[i].if_name, ifi->ifi_name);
		printf("name: %s, ", conns[i].if_name);
        
		if ( (sa = ifi->ifi_addr) != NULL) {
            memcpy(&(conns[i].ip), sa, sizeof(*sa));
			printf("IP address: %s, ",
                   Sock_ntop_host((struct sockaddr*)&(conns[i].ip), sizeof(*sa)));
        }
        
		if ( (sa = ifi->ifi_ntmaddr) != NULL) {
			memcpy(&(conns[i].netmask), sa, sizeof(*sa));
            printf("network mask: %s, ",
                   Sock_ntop_host((struct sockaddr*)&(conns[i].netmask),
                                  sizeof(*sa)));
        }
        
        conns[i].subnet.sin_family = AF_INET;
        conns[i].subnet.sin_addr.s_addr =
        conns[i].ip.sin_addr.s_addr &
        conns[i].netmask.sin_addr.s_addr;
        printf("subnet: %s\n",
               Sock_ntop_host((struct sockaddr*)&(conns[i].subnet),
                              sizeof(*sa)));
        
        conns[i].header = NULL;
	}
	free_ifi_info_plus(ifihead);
    
    return err;
}

bool is_new_client(struct srv_ifi_info* conn, struct sockaddr_in* cliaddr) {
    // printf("%s()\n", __func__);
    if(conn->header == NULL) {
        return true;
    }
    
    struct cli_info *tmp = conn->header;
    while(tmp) {
        if(tmp->cliaddr.sin_addr.s_addr == cliaddr->sin_addr.s_addr &&
           tmp->cliaddr.sin_port == cliaddr->sin_port) {
            return false;
        }
        else {
            // printf("tmp ip %s\n",
            // Sock_ntop_host((SA*)&(tmp->cliaddr), sizeof(tmp->cliaddr)));
            // printf("tmp port number %d\n", tmp->cliaddr.sin_port);
            
            tmp = tmp->next;
        }
    }
    
    return true;
}

int register_client(struct srv_ifi_info* conn, struct sockaddr_in* cliaddr,
                    int fd_pipe, pid_t pid_child) {
    // printf("%s()\n", __func__);
    struct cli_info* tmp = (struct cli_info*) malloc(sizeof(struct cli_info));
    
    tmp->fd_pipe = fd_pipe;
    tmp->pid_child = pid_child;
    memcpy(&(tmp->cliaddr), cliaddr, sizeof(*cliaddr));
    tmp->next = NULL;
    
    if(conn->header == NULL) {
        conn->header = tmp;
    }
    else {
        struct cli_info* tail = conn->header;
        
        while(tail->next) {
            tail = tail->next;
        }
        tail->next = tmp;
    }
    
    fprintf(stderr,"new client registered\n");
    
    return 0;
}


int handle_client(struct srv_ifi_info* conn, struct sockaddr_in* cliaddr,
                  int if_cnt, int index, int fd_pipe) {
    // printf("%s()\n", __func__);
    int err, i;
    for(i = 0; i < if_cnt; ++i) {
        if(i == index){
            connfd=conn[i].sockfd;
            continue;
        }
        close(conn[i].sockfd);
    }
    
    int fd_sock;
    int len = sizeof(struct sockaddr_in);
    struct sockaddr_in srvaddr, tmpaddr;
    bzero(&srvaddr, sizeof(struct sockaddr_in));
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(0);
    Inet_pton(AF_INET,
              Sock_ntop_host((SA*)&(conn[index].ip),
                             sizeof(struct sockaddr_in)),
              &srvaddr.sin_addr);
    
    fd_sock = Socket(AF_INET, SOCK_DGRAM, 0);
    Bind(fd_sock, (SA*)&srvaddr, sizeof(struct sockaddr_in));
    
    bzero(&tmpaddr, sizeof(tmpaddr));
    //get file name from client
    Getsockname(fd_sock, (SA*)&tmpaddr, &len);
    printf("server ip: %s\n", Sock_ntop_host((SA*)&tmpaddr, sizeof(tmpaddr)));
    printf("server port number: %d\n", ntohs(tmpaddr.sin_port));
    
    unsigned long ip, mask;
    bool samehost = false, islocal = false;
    if(((struct sockaddr_in*)cliaddr)->sin_addr.s_addr ==
       ((struct sockaddr_in*)&tmpaddr)->sin_addr.s_addr) {
        printf("client and server are on the same host\n");
        samehost = true;
    }
    
    if(samehost == false) {
        ip = ((struct sockaddr_in*)&(conn[index].ip))->sin_addr.s_addr;
        mask = ((struct sockaddr_in*)&(conn[index].netmask))->sin_addr.s_addr;
        
        if((ip & mask) == (cliaddr->sin_addr.s_addr & mask)) {
            printf("client and server are local\n");
            islocal = true;
        }
    }
    
    if(islocal || samehost) {
        int optval = 1;
        setsockopt(fd_sock, SOL_SOCKET, SO_DONTROUTE, &optval, sizeof(optval));
    }
    else {
        printf("client and server are NOT local\n");
    }
    
    //missing connect()
    // Connect(fd_sock,(SA*)cliaddr, sizeof(struct sockaddr_in));
    // fprintf(stderr,"connect works");
    
    struct mesg message;
    bzero(&message, sizeof(struct mesg));
    sprintf(message.payload, "%d", ntohs(tmpaddr.sin_port));
    //TELL CLIENT SERVER PORT 3
send_again_1:
    Sendto(conn[index].sockfd, &message,sizeof(struct mesg), 0,
           (SA *)cliaddr, sizeof(struct sockaddr_in));
    printf("\n[second handshake: server(child) -> client]\n");
    
    
    int maxfd;
    fd_set set_read, set_master;
    FD_ZERO(&set_master);
    
    FD_SET(fd_sock, &set_master);
    FD_SET(fd_pipe, &set_master);
    maxfd = fd_sock > fd_pipe? fd_sock : fd_pipe;
    
    for(; ;) {
    restart_select:
        set_read = set_master;
        err = select(maxfd + 1, &set_read, NULL, NULL, NULL);
        if (err < 0) {
            if (errno == EINTR)
                goto restart_select;
            else {
                perror("select() error");
                return -1;
            }
        }
        
        if (FD_ISSET(fd_sock, &set_read)) {
            bzero(&message, sizeof(struct mesg));
            err = recvfrom(fd_sock, &message, sizeof(struct mesg), 0,
                           (SA*)&tmpaddr, &len);
            // printf("client ip: %s\n", Sock_ntop_host((SA*)&tmpaddr, sizeof(tmpaddr)));
            // printf("client port number: %d\n", ntohs(tmpaddr.sin_port));
            
            err = connect(fd_sock, (SA*)&tmpaddr, sizeof(tmpaddr));
            if(err < 0) {
                perror("connect()");
                return -1;
            }
            
            close(conn[index].sockfd);
            
            break;
        }
        
        if (FD_ISSET(fd_pipe, &set_read)) {
            char buf[16];
            bzero(buf, 16);
            
            read(fd_pipe, buf, 16);
            printf("receive message from parent process: %s\n", buf);
            
            goto send_again_1;
        }
    }
    printf("\n[third handshake: client -> server(child)]\n");
    printf("client request filename: %s\n", message.payload);
     printf("client intial receive window: %d\n", message.header.window_size);
    printf("\n[Connection established, data transmission begins]\n");
    // struct mesg message;
    // bzero(&message, sizeof(struct mesg));
    // sprintf(message.payload, "%d", ntohs(tmpaddr.sin_port));
    //TELL CLIENT SERVER PORT 3
    // Sendto(conn[index].sockfd, &message,sizeof(struct mesg), 0,
    // (SA *)cliaddr, sizeof(struct sockaddr_in));
    // printf("\n[second handshake: server(child) -> client]\n");
    
    // bzero(&message, sizeof(struct mesg));
    // RECEIVE CLIENT'S ACK,use select
    // fd_set rset;
    // FD_ZERO(&rset);
    // FD_SET(fd_sock, &rset);
    // Recvfrom(fd_sock, &message, sizeof(struct mesg), 0,(SA *)&tmpaddr, &len);
    // Recvfrom(fd_sock, &message, sizeof(struct mesg), 0,NULL, NULL);
    /*Select(fd_sock +1, &rset, NULL, NULL, NULL);
     fprintf(stderr,"pass select");
     if (FD_ISSET(fd_sock, &rset)){
     if( message.header.ackn==1){
     printf("\n[third handshake: client -> server(child)]\n");
     printf("client request filename: %s\n", message.payload);
     }
     }*/
    if(message.header.ackn==1){
        
        printf("\n[third handshake: client -> server(child)]\n");
        printf("client request filename: %s\n", message.payload);
    }
    
    //get filename
    //char *filename;
    //strcpy(filename,message.payload);
    adWinNow=message.header.window_size;
    
    FILE *fp;
    fp=fopen(message.payload,"r");
    
    uint32_t seq_num = 0;
    printf("\n[Connection established, data transmission begins]\n");
    
    //the following part we're transfer file content
    bzero(&message, sizeof(struct mesg));
    message.header.mesg_type = MSG_TYPE_DATA;
    message.header.seq_num = seq_num;
    
    
    
    
    //open file
    //fp=fopen(filename,"r");
    //check if open success
    int filePtr;
    int seqNumber;
    int fileLength;
    
    int loopc=0;
    
    //segs sent in one wave
    int counter=0;
    backup= malloc(srvwin*sizeof(save) );
    
    char sendText[MAXLINE], recvText[MAXLINE + 1];
    int receiveFIN=0;
    int j;
    fseek(fp, 0, SEEK_END);
    fileLength=ftell(fp);
    fseek(fp,0,SEEK_SET);
    int totalSeg=fileLength/dgSize;
    //fprintf(stderr,"total length: %d\n",fileLength);
    //fprintf(stderr,"dg size: %d\n",dgSize);
    //fprintf(stderr,"mesg size: %d\n",sizeof(struct mesghdr));
    if(fileLength%dgSize!=0){
        totalSeg++;
    }
    fprintf(stderr,"total FILE Segments: %d\n",totalSeg);
    
    //intialize send buffer
    
	
	for( j = 0 ; i<srvwin; i++)
	{
        backup[i].data[0] = '\0';
	}
    
    //formly start data transfer
    for(;;){
        loopc++;
        //we first examine cwnd,sendWin,adWinNow to make sure we can transfer packet
        /*if(loopc>80){
            fprintf(stderr,"not finish transfering but has to stop\n");
            retransmit(fd_sock,-8);
            handleACK(fd_sock);
            retransmit(fd_sock,-9);
            exit(0);
        }*/
        
        if(cwnd-(nextp-waitackp)<=0){
            sendWin=0;
            fprintf(stderr,"Sending window have no extra slot now,stop sending\n\n");
        }
        else{
            sendWin=cwnd-(nextp-waitackp);
        }
        tobeSent=min(sendWin,adWinNow);
        fprintf(stderr,"**********Window size report start*********\n");
        
        fprintf(stderr,"* CWND now is %d\n",cwnd);
        fprintf(stderr,"* client advertise window now is %d\n",adWinNow);
        fprintf(stderr,"* Avaliable sending window size now is %d\n",tobeSent);
        fprintf(stderr,"* SSTHRESH now is %d\n",ssthresh);
        //fprintf(stderr,"* counter is now is %d\n",counter);
        //fprintf(stderr,"* waitackp is now is %d\n",waitackp);
        //fprintf(stderr,"* acknow is now is %d\n",acknow);
        //fprintf(stderr,"* nextp is now is %d\n",nextp);
        fprintf(stderr,"* All seq# sent now is %d\n",allSeq);
        
        
        fprintf(stderr,"**********Window size report end*********\n\n");
        
        
        //when there's still packets to be sent in this wave,send 1 packet
        //need to add
        if(adWinNow==0){
            
            for(;;){
                
                //no window avaliable in client,do probing
                if(adWinNow==0&&pstTimer==0){
                    //try to get recv window condition
                    fprintf(stderr,"receive advistised window is 0, probing...\n");
                    //if(pstTimer==0){
                    //-2 is probing signal
                    //usleep(10);
                    retransmit(fd_sock,-2);
                    //pstTimer=1;
                    //}
                }
                //do not update ack
                acknow=handleACK(fd_sock);
                //handleACK(fd_sock);
                
                //packet lost,we need to retransmit
                if(dupACK>=3){
                    
                    //reset cwnd, ssthresh
                    ssthresh=MIN(cwnd,adWinNow);
                    ssthresh=MAX(ssthresh/2,2);
                    //cwnd=ssthresh;
                    fprintf(stderr,"Duplicate ACK>3,retransmit package %d \n\n",lastACK);
                    retransmit(fd_sock,lastACK);
                    dupACK=0;
                }
                //transmit recover
                if(adWinNow>0){
                    fprintf(stderr,"Probing finish\n\n");
                    pstTimer=0;
                    //if we got all ack after probing,reset counter
                    if(waitackp==nextp){
                        counter=0;
                    }
                    break;
                }
                
                
            }//for loop end
            
            
            
            
        }//if end
        
        else if((allSeq<totalSeg)&&(counter<tobeSent)&&MIN(sendWin,adWinNow)!=0){
            
            //we send package allowed in one wave
            //intialize rtt,following adapt from 22.8
            if( rttinit == 0 )
            {
                rtt_init(&rttinfo);
                rtt_newpack(&rttinfo);
                
                rttinit = 1;
                //rtt_d_flag = 1;
            }
            
            //need to check
            fread(sendText,dgSize,1,fp );
            sendLen=strlen(sendText);
            //fprintf(stderr,"sendtext%s",sendText);
            //this is first place send file data
            //send_again_2:
            
            
            sendDatagram(fd_sock, sendText, sendLen);
            
            memset(sendText,'\0',MAXLINE);
            
        send_again_2:
            if(timeout==1){
                sendAgain(fd_sock);
                
            }
            start_timer(rtt_start(&rttinfo));
            // alarm(rtt_start(&rttinfo));
            //check timeout
            if(sigsetjmp(jmpbuf, 1) != 0) {
                if (rtt_timeout(&rttinfo) < 0) {
                    err_msg("no response from server, giving up\n\n");
                    rttinit = 0; /* reinit in case we're called again */
                    errno = ETIMEDOUT;
                    return (-1);
                }
                
                
                //we'll resend later when time out
                timeout = 1;
                
                
                // fprintf(stderr,"sendagain\n");
                goto send_again_2;
                
            }
            
            //if success,no timeout,update #package sent&packets sent in this wave
            //all sent seq#
            sentSeg++;
            counter++;
        }//end if,finish sending data
        

        
        
        //this else follow each wave,we deal with ACK and reset sender count
        else{
            if(finsend==1&&finACK==0){
                goto fin;
            }
            //means time out,and may be client dropped the #-9 sequence it get.
            if(finACK==1){
                goto close;
            }
            
            //if we haven't got all ack for that wave,keep get them,or can't start next send wave
            while(waitackp<nextp){
                
                fprintf(stderr,"Waiting to get ACK%d......\n\n",waitackp+1);
                acknow=handleACK(fd_sock);
                
                if(nextp==acknow&&pstTimer==0){
                    //fprintf(stderr,"We get all ACK needed in this wave\n");
                    counter=0;
                    //get out of while loop
                    break;
                }
                
 
                //retransmit if more than 3 dups
                if(dupACK>=3){
                    
                    ssthresh=MIN(cwnd,adWinNow);
                    ssthresh=MAX(ssthresh/2,2);
                    //cwnd=ssthresh;
                    //fprintf(stderr,"We get all ACK needed in this wave\n");
                    fprintf(stderr,"Duplicate ACK>3,retransmit package %d \n\n",lastACK);
                    retransmit(fd_sock,lastACK);
                    dupACK=0;
                }
                           }
            
            
            //final condition
            if(allSeq+1>=totalSeg&&acknow>=totalSeg){
                fprintf(stderr,"Got enough ack\n");
                fprintf(stderr,"Finally finish file transfering\n\n");
                fprintf(stderr,"Now tell client file is all sent\n\n");
                retransmit(fd_sock,-8);
                finsend=1;
            fin:
                while(acknow!=-9){
                    acknow=handleACK(fd_sock);
                }  
                if(acknow==-9){
                fprintf(stderr,"Client send FIN ACK back\n\n");
                }
            close:
                retransmit(fd_sock,-9);
                //enter time_wait to make sure client got the package.
                //settimer here need to check
                //start_timer(2*rtt_start(&rttinfo));
                acknow=handleACK(fd_sock);
                while(acknow!=-10){
                    acknow=handleACK(fd_sock);
                }
                
                if(acknow==-10){
                    //only if we confirmed that client is closed.
                
                fprintf(stderr,"We got confirmation that client is closing,exit.....\n");
                exit(0);
                }
            }
            //while loop end
            //we finish sending file already,send fin and safely close
            //need to consider if these ACK lost    
        }
    }//end of infinite for loop
}


