#include <stdio.h>
#include <sys/types.h>
#include "xmt_book.h"
#include "xmt_errno.h"
#include "sqlite3.h"
#include "xmt_util.h"

#include <assert.h>
#include <pthread.h>

extern int ipc_fd_f;

e_book::e_book(int i, int s, int n){
  isbn = i, issue = s, page_numb = n;
  // retrieve saved page last time
  pos = 0;
}

int e_book::get_main_cover(char name[]){
  sprintf(name, "%d/%d/main.jpg", isbn, issue);
  return 1;
}

int e_book::get_history_page(char name[]){
  sprintf(name, "%d/%d/%d.html", isbn, issue, pos); // bookmark page
  return 1;
}

int e_book::go_relative_page(int i, char name[]){
  int ii = pos + i;
  if(ii<0 || ii>page_numb) return -NOEXIST;
  return go_absolute_page(ii, name);
}

int e_book::go_absolute_page(int index, char name[]){
  int ret = 0;
  if(index<0 || index>page_numb) return -NOEXIST;

  sprintf(name, "%d/%d/%d.html", isbn, issue, index);
  // test if available
  ret = access(name, R_OK);
  if(ret!=0){
    int var[2];
    var[0] = isbn, var[1] = issue;
    // help via proxy
    write(ipc_fd_f, var, sizeof(var));

    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(ipc_fd_f, &readset);
    int maxfd = ipc_fd_f;
    timeval timeout={8,0};
    int nfds = select(maxfd+1, &readset, NULL, NULL, &timeout);
    if(nfds>0){
      read(ipc_fd_f, var, sizeof(var));
      if((var[0]==isbn)&&(var[1]==issue)) ret = 0;

    }
    else ret = -TIMEOUT;
    //printf("time out \n");
  }
  if(ret==0) pos = index;
  return ret;
}

