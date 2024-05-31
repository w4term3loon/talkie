// TODO::daemonize
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0
#define NCLIENT 10
#define PORT 6942

int
main(int argv, char *argc[]) {
  int opt = 1;              //< option setter where memaddr is needed
  uint8_t backlog = 4;      //< the max queue lenght on the socket
  char buffer[1024] = {0};    //< message buffer

  struct sockaddr_in address; //< socket address if listener
  struct pollfd fds[NCLIENT]; //< the fds being polled

  int nfds = 1;                  // number of fds being polled
  int timeout = (5 * 60 * 1000); // after 5 minutes no active end

  int f_end_server = FALSE;
  int f_close_connection = FALSE;
  int f_compress_array = FALSE;

  int listen_socket_fd;
  int new_socket_fd;

  printf(" * LOG: selected port: %d\n", PORT);

  // create socket
  listen_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  // set address to be reuseable
  int setsockopt_rv = setsockopt(listen_socket_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
  if (setsockopt_rv < 0) {
    perror("setsockopt failed");
    return 1;
  }

  // set the socket non-blocking
  int ioctl_rv = ioctl(listen_socket_fd, FIONBIO, (char *)&opt);
  if (ioctl_rv < 0) {
    perror("ioctl failed");
    close(listen_socket_fd);
    return 1;
  }

  // set address attributes
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // bind the socket
  int bind_rv = bind(listen_socket_fd, (struct sockaddr *)&address, sizeof(address));
  if (bind_rv < 0) {
    perror("bind failed");
    close(listen_socket_fd);
    return 1;
  }

  // start listening on the socket
  int listen_rv = listen(listen_socket_fd, backlog);
  if (listen_rv < 0) {
    perror("listen failed");
    close(listen_socket_fd);
    return 1;
  }

  // set up the fd list for polling
  memset(fds, 0, sizeof fds);
  uint8_t LISTEN = 0;
  fds[LISTEN].fd = listen_socket_fd;
  fds[LISTEN].events = POLLIN;

  // main event loop
  // for new connections and data
  do {
    // poll
    printf(" * Waiting for poll...\n");
    int poll_rv = poll(fds, nfds, timeout);
    if (poll_rv < 0) {
      perror("poll failed");
      break;

    } else if (poll_rv == 0) {
      perror("poll timeout");
      break;
    }

    int current_nfds = nfds;
    for (int i = 0; i < current_nfds; ++i) {
      // no event
      if (fds[i].revents == 0x0000) {
        continue;
      }

      else if (fds[i].revents == 0x0001) {
        // event
      }

      else if (fds[i].revents == 0x0011) {
        // socket was closed on client side
      }

      // unexpected event, change later
      else {
        printf(" * LOG: unexpected revents: %d\n", fds[i].revents);
        f_end_server = TRUE;
        break;
      }

      // listen socket event
      if (fds[i].fd == listen_socket_fd) {
        printf(" * LOG: listening socket is readable.\n");

        do {
          // accept the incoming connection
          new_socket_fd = accept(listen_socket_fd, NULL, NULL);
          if (new_socket_fd < 0) {
            if (errno != EWOULDBLOCK) {
              perror("accept failed");
              f_end_server = TRUE;
            }
            break;
          }

          // set the socket non-blocking
          // in case it does not inherit attribute from listening socket
          int ioctl_rv = ioctl(new_socket_fd, FIONBIO, (char *)&opt);
          if (ioctl_rv < 0) {
            perror("ioctl failed");
            close(new_socket_fd);
            return 1;
          }

          printf(" * LOG: new incoming connection %d\n", new_socket_fd);
          fds[nfds].fd = new_socket_fd;
          fds[nfds].events = POLLIN;
          ++nfds;

        } while (new_socket_fd != -1);

        // other socket event
      } else {

        printf(" * LOG: connected socket %d is readable.\n", fds[i].fd);

        do {
          // receive message
          int recv_rv = recv(fds[i].fd, buffer, sizeof buffer, 0);
          if (recv_rv < 0) {
            if (errno != EWOULDBLOCK) {
              perror("recv failed");
              f_close_connection = TRUE;
            }
            break;
          }

          // connection closed by client
          if (recv_rv == 0) {
            printf(" * LOG: connection closed client side.\n");
            f_close_connection = TRUE;
            break;
          }

          printf(" * REC: from %d %d bytes.\n", fds[i].fd, recv_rv);
          printf(" * LOG: message: %s\n", buffer);

          // leave out listening socket
          for (int j = 1; j < current_nfds; ++j) {
            if (j == i)
              continue;
            printf(" * LOG: updating: %d\n", fds[j].fd);

            // echo back to clients
            int send_rv = send(fds[j].fd, buffer, recv_rv, 0);
            if (send_rv < 0) {
              perror(" send failed.");
              f_close_connection = TRUE;
              break;
            }
          }
        } while (TRUE);

        // connection is closed
        if (f_close_connection) {
          f_close_connection = FALSE;
          f_compress_array = TRUE;

          close(fds[i].fd);
          printf(" * LOG: connected socket %d was closed.\n", fds[i].fd);
          fds[i].fd = -1;
        }
      }
    }

    // remove closed connection fd
    if (f_compress_array) {
      f_compress_array = FALSE;

      for (int i = 0; i < nfds; ++i) {
        if (fds[i].fd == -1) {
          for (int j = i; j < nfds; ++j) {
            fds[j].fd = fds[j + 1].fd;
          }
          --i;
          --nfds;
        }
      }
    }

  } while (f_end_server == FALSE);

  // cleanup
  for (int i = 0; i < nfds; ++i) {
    if (fds[i].fd >= 0)
      close(fds[i].fd);
  }

  return 0;
}
