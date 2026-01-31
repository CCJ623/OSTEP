#ifndef __UDP_h__
#define __UDP_h__

#ifdef __cplusplus
// 如果这个头文件被 C++ 编译器（如 g++）处理，请使用 C 链接约定
extern "C" {
#endif

//
// includes
//

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

//
// prototypes
//

int UDP_Open(int port);
int UDP_Close(int fd);

int UDP_Read(int fd, struct sockaddr_in *addr, char *buffer, int n);
int UDP_Write(int fd, struct sockaddr_in *addr, const char *buffer, int n);

int UDP_FillSockAddr(struct sockaddr_in *addr, const char *hostName, int port);

#ifdef __cplusplus
}
#endif

#endif // __UDP_h__
