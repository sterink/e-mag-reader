#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/wait.h>

#include "ebp_server.h"
#include <string.h>
#include <stdlib.h>

int create_channel_mc(){
  struct sockaddr_in localSock;
  struct ip_mreq group;

  int serv_port = 4321;
  /* Create a datagram socket on which to receive. */
  int sd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sd < 0)
  {
    perror("Opening datagram socket error");
    exit(1);
  }
  else
    printf("Opening datagram socket....OK.\n");

  /* Enable SO_REUSEADDR to allow multiple instances of this */
  /* application to receive copies of the multicast datagrams. */
  {
    int reuse = 1;
    if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
    {
      perror("Setting SO_REUSEADDR error");
      close(sd);
      exit(1);
    }
    else
      printf("Setting SO_REUSEADDR...OK.\n");
  }

  /* Bind to the proper port number with the IP address */
  /* specified as INADDR_ANY. */
  memset((char *) &localSock, 0, sizeof(localSock));
  localSock.sin_family = AF_INET;
  localSock.sin_port = htons(serv_port);
  localSock.sin_addr.s_addr = INADDR_ANY;
  if(bind(sd, (struct sockaddr*)&localSock, sizeof(localSock)))
  {
    perror("Binding datagram socket error");
    close(sd);
    exit(1);
  }
  else
    printf("Binding datagram socket...OK.\n");

  /* Join the multicast group 226.1.1.1 on the local 203.106.93.94 */
  /* interface. Note that this IP_ADD_MEMBERSHIP option must be */
  /* called for each local interface over which the multicast */
  /* datagrams are to be received. */
  group.imr_multiaddr.s_addr = inet_addr("226.1.1.1");
  //group.imr_interface.s_addr = inet_addr("203.106.93.94");
  group.imr_interface.s_addr = htonl(INADDR_ANY);
  if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0)
  {
    perror("Adding multicast group error");
    close(sd);
    exit(1);
  }
  else
    printf("Adding multicast group...OK.\n");

  return sd;
}

// initialize server
int create_channel_ms(){
  const int qlen = 32;
  int serv_port = 33333;
  struct sockaddr_in my_addr;

  my_addr.sin_family=AF_INET;
  my_addr.sin_port=htons(serv_port);
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  bzero(&(my_addr.sin_zero),8);

  struct sockaddr *addr = (sockaddr *)&my_addr;

  int fd = socket(addr->sa_family, SOCK_STREAM, 0);
  if(fd < 0) return(-1);
  int reuse = 1;
  int err = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
  if(err < 0){
    err = errno;
    goto errout;
  }
  err = bind(fd, addr, sizeof(sockaddr_in));
  if(err < 0) {
    err = errno;
    goto errout;
  }
  err = listen(fd, qlen);
  if(err < 0) {
    err = errno;
    goto errout;
  }
  return(fd);
errout:
  close(fd);
  errno = err;
  return(-1);
}

int create_channel_ui(){
  const int qlen = 32;
  int serv_port = 33333;
  struct sockaddr_in my_addr;

  my_addr.sin_family=AF_INET;
  my_addr.sin_port=htons(serv_port);
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  bzero(&(my_addr.sin_zero),8);

  struct sockaddr *addr = (sockaddr *)&my_addr;

  int fd = socket(addr->sa_family, SOCK_DGRAM, 0);
  if(fd < 0) return(-1);
  int err = bind(fd, addr, sizeof(sockaddr_in));
  if(err < 0) {
    err = errno;
    goto errout;
  }
  return(fd);
errout:
  close(fd);
  errno = err;
  return -1;
}

const char *data_file = "server_books_stock.db";

void reaper(int sig){
  int status;
  while(wait3(&status, WNOHANG, 0) >= 0) ;
}

// multi cast sock
static int mc_sock = -1;
int main(int argc, char **argv){
  // multi-cast client port
  struct sockaddr_in fsin;

  bzero(&(fsin.sin_zero),8);
  fsin.sin_family=AF_INET;
  fsin.sin_port=htons(4321);
  fsin.sin_addr.s_addr = inet_addr("226.1.1.1");
  int mc_client = socket(AF_INET, SOCK_DGRAM, 0);


  mc_sock = create_channel_mc();
  int ui_sock = create_channel_ui();
  int ms_sock = create_channel_ms();

  signal(SIGCHLD, reaper);

  while(true){
    fd_set readset;
    FD_ZERO(&readset);
    int fd1 = ui_sock;
    int fd2 = ms_sock;
    FD_SET(fd1, &readset);
    FD_SET(fd2, &readset);
    int maxfd = (fd1 > fd2) ? fd1 : fd2;
    int nfds = select(maxfd+1, &readset, NULL, NULL, NULL);
    if (FD_ISSET(fd1, &readset)){
      char buf[256];
      int ret = recv(fd1, buf, 256, MSG_DONTWAIT);
      if(ret > 0) sendto(mc_client, buf, ret, 0, (struct sockaddr *)&fsin, sizeof(fsin));
    }
    else if (FD_ISSET(fd2, &readset)){
      struct sockaddr peer_addr;
      unsigned int peer_addr_len = sizeof(peer_addr);
      int ssock = accept(ms_sock, &peer_addr, &peer_addr_len);
      if(ssock < 0){
        printf("accept error %s\n", strerror(errno));
        exit(1);
      }
      pid_t pid = fork();
      switch(pid){
        case 0: // child
          printf("child begin \n");
          close(ui_sock);
          close(ms_sock);
          close(mc_client);
          void job_handle(int );
          job_handle(ssock);
          printf("child exit \n");
          exit(0);
        default: // parent
          printf("child pid %d \n", pid);
          close(ssock);
          break;
        case -1:
          printf("fork error\n");
          exit(1);
      }
    }
    else if (nfds == -1) return -1;
  }
  close(mc_client);
  close(mc_sock);
  close(ms_sock);
  close(ui_sock);
  return 0;
}

#include "xmt_book_stock_manager.h"

book_stock_manager stock_mg;
void job_handle(int fd){
  ebook_pkg *pkg = NULL;
  stock_mg.set_channel(fd);
  // duplicate book_stock info as starting point
  // main job
  while(true){
    fd_set readset;
    FD_ZERO(&readset);
    int fd1 = fd;
    int fd2 = mc_sock;
    FD_SET(fd1, &readset);
    FD_SET(fd2, &readset);
    int maxfd = (fd1 > fd2) ? fd1 : fd2;
    int nfds = select(maxfd+1, &readset, NULL, NULL, NULL);
    if (FD_ISSET(fd1, &readset)){
      // poll latest book stock
      //  // send if possible
      //  remote_magazine_content content;
      //  content.sendit();
      int val = create_ebp_pkg(&pkg, fd);
      printf("create_ebp_pkg return %d\n", val);
      if(val==0) break;
      //printf("create_ebp_pkg return val = %d\n", val);
      if(pkg){
        int val = pkg->doit(fd);
        if(val==1) pkg=NULL; // req finished then reset
      }
    }
    else if(FD_ISSET(fd2, &readset)){
      char buf[256];
      int ret = recvfrom(fd2, buf, 256, MSG_DONTWAIT, NULL, NULL);
      if(ret > 0) printf("job handle: %s\n", buf);
    }
    else if (nfds == -1) return ;
    stock_mg.try_push_content(fd);
  }
  close(fd);
}
