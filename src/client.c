#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ncwrap.h"

volatile unsigned int TERMINATE = 0;

struct handler {
  int socket;
  scroll_window_t sw;
};

void *
listener(void *ctx) {
  char buffer[50];
  struct handler han = *(struct handler *)ctx;
  while (strcmp(buffer, "exit") != 0) {
    int recv_rv = recv(han.socket, buffer, sizeof buffer, 0);
    if (recv_rv >= 0) {
      ncw_scroll_window_add_line(han.sw, buffer);
    }
  }

  TERMINATE = 1;
  pthread_exit(NULL);
}

void
sender(char *buf, size_t buf_sz, void *ctx) {
  struct handler han = *(struct handler *)ctx;
  send(han.socket, buf, buf_sz, 0);
  ncw_scroll_window_add_line(han.sw, buf);
}

int
main(int argv, char *argc[]) {

  if (argv != 3) {
    fprintf(stderr, "ERROR: argument error\n");
    return 0;
  }

  int PORT = atoi(argc[2]);
  printf(" * selected address: %s\n", argc[1]);
  printf(" * selected port: %d\n", PORT);

  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  int convert_rv = inet_pton(AF_INET, argc[1], &server_address.sin_addr);
  if (convert_rv < 0) {
    fprintf(stderr, "ERROR: conversion error\n");
    return 1;
  }

  int status = connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address));
  if (status < 0) {
    fprintf(stderr, "ERROR: connection error: %d\n", errno);
    return 1;
  }

  struct handler ctx;
  ctx.socket = socket_fd;

  (void)ncw_init();
  input_window_t iw = NULL;
  ncw_input_window_init(&iw, 10, 17, 50, "input", 0);
  scroll_window_t sw = NULL;
  ncw_scroll_window_init(&sw, 10, 2, 50, 15, "scroll");
  ctx.sw = sw;

  ncw_input_window_set_output(iw, sender, (void*)&ctx);

  pthread_t listen_thread;
  int pthread_create_rv = pthread_create(&listen_thread, NULL, listener, (void*)&ctx);
  if (pthread_create_rv != 0) {
    perror("type thread failed.");
    close(socket_fd);
  }

  int event = 0;
  ncw_focus_step();
  for (;;) {
    ncw_update();
    event = ncw_getch();
    switch (event) {

    case ERR:
      break;

    case CTRL('x'):
      goto clean;

    default:
      ncw_event_handler(event);
    }
  }

clean:
  ncw_input_window_close(&iw);
  ncw_scroll_window_close(&sw);

  ncw_close();
  close(socket_fd);

  return 0;
}
