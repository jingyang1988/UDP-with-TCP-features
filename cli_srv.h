/*
 *	CSE 533 Network Programming
 *	Assignment #2, Due: Friday, October 31st
 *	Name: Yiqi Yu, ID: 109215949
 *	File: cli_serv.h
 */

#ifndef CLI_SRV_H_
#define CLI_SRV_H_

#include <setjmp.h>               // siglongjmp()
#include <sys/time.h>             // setitimer()
#include "unp.h"                  // header from Steven's
#include "unprtt_plus.h"          // modify RTT and RTO mechanisms


#define BUFFER_SIZE_FUNC 128
#define BUFFER_SIZE_MESG 512

#define TIME_INTERVAL 500

// define bool type
typedef enum {
  false,
  true
} bool;

struct arg_client {
  // from config file
  char srv_ip[16];
  uint16_t port_num;
  char filename[64];
  uint16_t window_size;
  uint16_t seed;
  float prob;
  uint16_t mu;

  // from kernel
  uint16_t cli_port_num;
  uint16_t srv_port_num;
};

struct arg_server {
  uint16_t port_num;
  uint16_t window_size;
};

#define MSG_TYPE_CONN 1
#define MSG_TYPE_ACK  2
#define MSG_TYPE_DATA 3
#define MSG_TYPE_FIN  4
#define MSG_TYPE_ERR  5 

// message fromat 
struct mesghdr {
	uint32_t mesg_type;
	uint32_t seq_num;
	uint32_t window_size;
  uint32_t timestamp;
    uint32_t ackn;
    
};

struct mesg {
  struct mesghdr header;
  char payload[512 - sizeof(struct mesghdr)];
};

int read_in(const char*, const char*, void*);
bool if_drop(float);
int start_timer(uint32_t);
int stop_timer();

#endif
