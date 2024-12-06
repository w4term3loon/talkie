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

static int TERMINATE = 0;

struct handler {
  int socket;
  scroll_window_t sw;
};

void *
listener(void *ctx) {
  struct handler han = *(struct handler *)ctx;
  for(char buffer[1024];!TERMINATE;) {
    if (0 < recv(han.socket, buffer, sizeof buffer, MSG_DONTWAIT)) {
      ncw_scroll_window_add_line(han.sw, buffer);
    }
  }
  printf("broke\n");
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

  if (argv < 3) {
    fprintf(stderr, "provide ip and port\n");
    return 1;
  }

  const char *addr = argc[1];
  const int port = atoi(argc[2]);
  printf(" * connecting to address: %s\n", addr);
  printf(" * selected port: %d\n", port);

  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  int convert_rv = inet_pton(AF_INET, addr, &server_address.sin_addr);
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
  ncw_input_window_init(&iw, 10, 17, 50, "type here", 0);
  scroll_window_t sw = NULL;
  ncw_scroll_window_init(&sw, 10, 2, 50, 15, "talkie");
  ctx.sw = sw;

  ncw_input_window_set_output(iw, sender, (void*)&ctx);

  pthread_t listen_thread;
  int pthread_create_rv = pthread_create(&listen_thread, NULL, listener, (void*)&ctx);
  if (pthread_create_rv != 0) {
    perror("type thread failed.");
    close(socket_fd);
    goto clean;
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
      TERMINATE = 1;
      goto clean;
    default:
      ncw_event_handler(event);
    }
  }

clean:
  pthread_join(listen_thread, NULL);
  ncw_input_window_close(&iw);
  ncw_scroll_window_close(&sw);

  ncw_close();
  close(socket_fd);

  return 0;
}
