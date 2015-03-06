/*
 *	CSE 533 Network Programming
 *	Assignment #2, Due: Friday, October 31st
 *	Name: Yiqi Yu, ID: 109215949
 *	File: server.h
 */

#ifndef SERVER_H_
#define SERVER_H_

#include "unp.h"
#include "unpifiplus.h"

struct srv_ifi_info {
  int sockfd;
  char if_name[16];
  struct sockaddr_in ip;
  struct sockaddr_in netmask;
  struct sockaddr_in subnet;

  struct cli_info* header;
};

struct cli_info {
  int fd_pipe;
  pid_t pid_child;
  struct sockaddr_in cliaddr;

  struct cli_info* next;
};
#endif
