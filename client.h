/*
 *	CSE 533 Network Programming
 *	Assignment #2, Due: Friday, October 31st
 *	Name: Yiqi Yu, ID: 109215949
 *	File: client.h
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include "unp.h"
#include "unpifiplus.h"

struct cli_ifi_info {
  char if_name[16];
  struct sockaddr_in ip;
  struct sockaddr_in netmask;
};

#endif

