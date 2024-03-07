#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/ioctl.h>

#include "ncwrap.h"

// TODO: implement an event loop (segfault after display)

typedef struct {
    int* socket_fd;
    input_window_t input;
    scroll_window_t output;
} input_ctx;

volatile unsigned int TERMINATE = 0;

size_t reader(char* buffer, size_t buffer_s, FILE* file)
{
    size_t count = 0;
    char ch = getc(file);
    while (ch != '\n')
    {
        buffer[count] = ch;
        ++count;
        ch = getc(file);
        
        if (count  == buffer_s - 1)
        {
            break;
        }
    }
    
    for (size_t i = count; i < buffer_s - 1; ++i)
    {
        buffer[i] = '\0';
    }
    
    return count;
}

void* input_task(void* ctx)
{
    char buffer[50];
    while (strcmp(buffer, "exit") != 0)
    {
        input_window_read(((input_ctx*)ctx)->input, buffer, sizeof buffer); // add cb that is called between every inputs
        send(*((input_ctx*)ctx)->socket_fd, buffer, sizeof buffer, 0); // some error if send failed
        scroll_window_add_line(((input_ctx*)ctx)->output, buffer);
    }
    
    TERMINATE = 1;
    pthread_exit(NULL);
}

int main(int argv, char* argc[]) {
    
    if (argv != 3)
    {
        fprintf(stderr, "ERROR: argument error\n");
        return 0;
    }
    
    printf(" * selected address: %s\n", argc[1]);
    int PORT = atoi(argc[2]);
    printf(" * selected port: %d\n", PORT);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    int convert_rv = inet_pton(AF_INET, argc[1], &server_address.sin_addr);
    if (convert_rv < 0)
    {
        fprintf(stderr, "ERROR: conversion error\n");
        return 1;
    }
    
    int status = connect(socket_fd, (struct sockaddr*)&server_address,
        sizeof(server_address));
    if (status < 0) {
        fprintf(stderr, "ERROR: connection error: %d\n", errno);
        return 1;
    }
    
    // draw
    ncwrap_init();
    scroll_window_t scroll_window = scroll_window_init(10, 2, 50, 15, "scroll");
    input_window_t input_window = input_window_init(10, 17, 50, "input");
    
    input_ctx* thread_ctx;
    thread_ctx->socket_fd = &socket_fd;
    thread_ctx->input = input_window;
    thread_ctx->output = scroll_window;
    
    pthread_t input_thread;
    int pthread_create_rv = pthread_create(&input_thread, NULL, input_task, (void*)thread_ctx);
    if (pthread_create_rv != 0)
    {
        perror("type thread failed.");
        close(socket_fd);
    }

    char buffer[50];
    while (!TERMINATE)
    {
        // blocking task {thread or non-blocking or work around}
        int recv_rv = recv(socket_fd, buffer, sizeof buffer, 0);
        if (recv_rv >= 0)
        {
            scroll_window_add_line(scroll_window, buffer);
        }
    }
    
    input_window_close(input_window);
    scroll_window_close(scroll_window);
    
    ncwrap_close();
    close(socket_fd);
    
    return 0;
}
