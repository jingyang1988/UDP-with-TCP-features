/*
 *	CSE 533 Network Programming
 *	Assignment #2, Due: Friday, October 31st
 *	Name: Yiqi Yu, ID: 109215949
 *	File: cli_serv.c
 */

#include "cli_srv.h"

int start_timer(uint32_t tval) {
  struct itimerval it_val;

  it_val.it_value.tv_sec = (tval / 1000000);
  it_val.it_value.tv_usec = (tval % 1000000);
  it_val.it_interval = it_val.it_value;

  if(-1 == setitimer(ITIMER_REAL, &it_val, NULL)) {
    perror("setitimer()");
    return -1;
  }

  return 0;
}

int stop_timer() {
  struct itimerval it_val;

  it_val.it_value.tv_sec = 0;
  it_val.it_value.tv_usec = 0;
  it_val.it_interval = it_val.it_value;

  if(-1 == setitimer(ITIMER_REAL, &it_val, NULL)) {
    perror("setitimer()");
    return -1;
  }

  return 0;
}
