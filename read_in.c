/*
 *	CSE 533 Network Programming
 *	Assignment #2, Due: Friday, October 31st
 *	Name: Yiqi Yu, ID: 109215949
 *	File: read_in.c
 */

#include "cli_srv.h"

int chomp(char* str) {
  if (str == NULL)
    return -1;

  int len = strlen(str);
  while (str[len--] != '\n')
    ;
  str[len + 1] = '\0';

  return 0;
}

int read_in(const char* filename, const char* type, void* ptr) {
  FILE* fp;
  char line[BUFFER_SIZE_FUNC];

  fp = fopen(filename, "r");
  if(NULL == fp) {
    perror("fopen() error");
    return -1;
  }

  if (0 == strcmp("client", type)) {
    if(NULL != fgets(line, BUFFER_SIZE_FUNC, fp)) {
      chomp(line);
      strcpy(((struct arg_client*) ptr)->srv_ip, line);
    }
    else {
      perror("fget() error");
      return -1;
    }

    if(NULL != fgets(line, BUFFER_SIZE_FUNC, fp))
      ((struct arg_client*) ptr)->port_num = atoi(line);
    else {
      perror("fget() error");
      return -1;
    }

    if(NULL != fgets(line, BUFFER_SIZE_FUNC, fp)) {
      chomp(line);
      strcpy(((struct arg_client*) ptr)->filename, line);
    }
    else {
      perror("fget() error");
      return -1;
    }

    if(NULL != fgets(line, BUFFER_SIZE_FUNC, fp))
      ((struct arg_client*) ptr)->window_size = atoi(line);
    else {
      perror("fget() error");
      return -1;
    }
    
    if(NULL != fgets(line, BUFFER_SIZE_FUNC, fp))
      ((struct arg_client*) ptr)->seed = atoi(line);
    else {
      perror("fget() error");
      return -1;
    }
    
    if(NULL != fgets(line, BUFFER_SIZE_FUNC, fp))
      ((struct arg_client*) ptr)->prob = atof(line);
    else {
      perror("fget() error");
      return -1;
    }

    if(NULL != fgets(line, BUFFER_SIZE_FUNC, fp))
      ((struct arg_client*) ptr)->mu= atoi(line);
    else {
      perror("fget() error");
      return -1;
    }
  }
  else if (0 == strcmp("server", type)) {
    if(NULL != fgets(line, BUFFER_SIZE_FUNC, fp))
      ((struct arg_server*) ptr)->port_num = atoi(line);
    else {
      perror("fget() error");
      return -1;
    }

    if(NULL != fgets(line, BUFFER_SIZE_FUNC, fp))
      ((struct arg_server*) ptr)->window_size = atoi(line);
    else {
      perror("fget() error");
      return -1;
    }    
  }  else {
    printf("%s: no match", __func__);
    return -1;
  }
  fclose(fp);
  
  return 0;
}
