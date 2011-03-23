#include "ebp_client.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

// client-end socket
int ipc_fd_f = -1;


static int init_client(){
  const char *serv_name = "localhost";
  const int serv_port = 33333;
  int client_fd;

  // retrieve latest version
  struct hostent *host = gethostbyname(serv_name);

  struct sockaddr_in serv_addr;
  client_fd = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family=AF_INET;
  serv_addr.sin_port=htons(serv_port);

  //memcpy(&server.sin_addr, he->h_addr_list[0], he->h_length);
  inet_aton("218.240.36.19", &serv_addr.sin_addr);
  //inet_aton("192.168.1.155", &serv_addr.sin_addr);

  bzero(&(serv_addr.sin_zero),8);
  int val = connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
  if(val==-1){
    close(client_fd);
    client_fd = -1;
  }
  // send retrieve pack
  //is_latest = true;
  return client_fd;
}

// backend data agent for transfer of e-magazine actively
void *data_bk_agent(void *){
  int fd[2];
  int r = socketpair(AF_LOCAL, SOCK_STREAM, 0, fd);
  ipc_fd_f = fd[0];
  int ipc_fd_b = fd[1];

  int sock_fd = init_client();
  printf("create client socket success\n");

  local_sys_info sys_info;
  sys_info.sendit(sock_fd);

  // dump local database info to server
  local_magazine_info magazine_info;
  magazine_info.sendit(sock_fd);

  local_magazine_request local_req;

  ebook_pkg *pkg = NULL;

  fd_set readset;
  while(true){
    FD_ZERO(&readset);
    FD_SET(sock_fd, &readset);
    FD_SET(ipc_fd_b, &readset);
    int maxfd = sock_fd>ipc_fd_b?sock_fd:ipc_fd_b;
    int nfds = select(maxfd+1, &readset, NULL, NULL, NULL);
    if (FD_ISSET(sock_fd, &readset)){
      if(pkg == NULL){ // create req
        printf("sock fd data available \n");
        int ret = create_ebp_pkg(&pkg, sock_fd);
      }
      if(pkg){
        int val = pkg->doit(sock_fd);
        if(val==1){
          pkg=NULL; // req finished then reset
        }
        else if(val<0) sock_fd = init_client();
      }
    }
    else if (FD_ISSET(ipc_fd_b, &readset)){
      // forward request
      local_magazine_request local_req;

      mag_tag wanted_tag;
      read(ipc_fd_b, &wanted_tag, sizeof(mag_tag));
      local_req.tag = wanted_tag;
      local_req.sendit(sock_fd);
    }
    else printf("engine error\n");
  }
}
