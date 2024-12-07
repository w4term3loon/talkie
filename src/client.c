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
static char name[50];

struct handler {
  int socket;
  scroll_window_t sw;
};

void *
listener(void *ctx) {
  struct handler han = *(struct handler *)ctx;
  for (char buffer[1024]; !TERMINATE;) {
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

  // send message
  if (!strncmp(name, "", sizeof(name))) {
    send(han.socket, buf, buf_sz, 0);
  } else {
    const char delim[] = ": ";
    char send_buf[sizeof name + sizeof delim + buf_sz];
    strlcpy(send_buf, name, sizeof send_buf);
    strlcat(send_buf, delim, sizeof send_buf);
    strlcat(send_buf, buf, sizeof send_buf);
    send(han.socket, send_buf, sizeof send_buf, 0);
  }

  // display own message
  const char you[] = "You: ";
  char display[sizeof you + buf_sz];
  strlcpy(display, you, sizeof display);
  strlcat(display, buf, sizeof display);
  ncw_scroll_window_add_line(han.sw, display);
}

void
set_name(char *buf, size_t buf_sz, void *ctx) {
  struct handler han = *(struct handler *)ctx;
  strlcpy(name, buf, sizeof name);

  // create log
  const char log[] = "your name was set \'";
  char buffer[sizeof log + sizeof name + 1];
  strlcpy(buffer, log, sizeof buffer);
  strlcat(buffer, name, sizeof buffer);
  strlcat(buffer, "\'", sizeof buffer);
  ncw_scroll_window_add_line(han.sw, buffer);
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

  ncw_input_window_set_output(iw, sender, (void *)&ctx);

  pthread_t listen_thread;
  int pthread_create_rv = pthread_create(&listen_thread, NULL, listener, (void *)&ctx);
  if (pthread_create_rv != 0) {
    perror("type thread failed.");
    close(socket_fd);
    goto clean;
  }

  int event = 0;
  input_window_t namer = NULL;
  ncw_focus_step();
  for (;;) {
    ncw_update();
    event = ncw_getch();
    switch (event) {
    case ERR:
      break;
    case CTRL('n'):
      ncw_input_window_init(&namer, 10, 17, 50, "name", 1);         //< make window popup
      ncw_input_window_set_output(namer, set_name, (void *)&ctx); //< set output
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

  printf("name was: %s\n", name);

  return 0;
}
