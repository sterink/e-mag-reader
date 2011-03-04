#include <sys/stat.h>
#include "local_book_manager.h"

#include <string.h>
#include <sqlite3.h>

#include <pthread.h>
#include "ebp_client.h"

int ipc_fd_a = -1;
int ipc_fd_b = -1;
page_info_tag wanted_page;
pthread_mutex_t page_mutex = PTHREAD_MUTEX_INITIALIZER;

const char *data_file = "local_books_stock.db";
const char *sql_create_table_ebooks_info = "create table if not exists ebooks_info (isbn integer primary key not null, title text);";

const char *sql_create_table_ebooks_content = "create table if not exists ebooks_content (isbn integer not null, issue integer not null, cover text, page_numb integer, preview_set blob, total_set blob, primary key (isbn,issue));";

namespace{
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

  int init_client(){
    const char *serv_name = "localhost";
    const int serv_port = 33333;
    int client_fd;

    // retrieve latest version
    struct hostent *host = gethostbyname(serv_name);

    struct sockaddr_in serv_addr;
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(serv_port);
    //serv_addr.sin_addr = *((struct in_addr *)host->h_addr);
    inet_aton("192.168.1.155", &serv_addr.sin_addr);

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

  int whichisready(int fd1, int fd2, unsigned int timeout) {
    //if ((fd1 < 0) || (fd1 >= FD_SETSIZE) ||
    //    (fd2 < 0) || (fd2 >= FD_SETSIZE)) {
    //   errno = EINVAL;
    //   return -1;
    //}
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(fd1, &readset);
    FD_SET(fd2, &readset);
    int maxfd = (fd1 > fd2) ? fd1 : fd2;
    int nfds = select(maxfd+1, &readset, NULL, NULL, NULL);
    if (nfds == -1)
      return -1;
    if (FD_ISSET(fd1, &readset))
      return fd1;
    if (FD_ISSET(fd2, &readset))
      return fd2;
    errno = EINVAL;
    return -1;
  }

  // backend data agent for transfer of e-magazine actively
  void *data_bk_agent(void *){
    // fd1 is used for internal communication
    // fd2 is used for transfer of e-magazine

    int sock_fd = init_client();

    local_sys_info sys_info;
    sys_info.sendit(sock_fd);

    // dump local database info to server
    local_magazine_info magazine_info;
    magazine_info.sendit(sock_fd);

    local_magazine_request local_req;

    ebook_pkg *pkg = NULL;

    while(true){
      int fd = whichisready(ipc_fd_b, sock_fd, 0);
      if(fd == ipc_fd_b){
        // printf will trigger data flow into ipc_fd_b
        // retract msg, then resort to database
        int code = 0;
        read(fd, &code, sizeof(code));
        printf("msg from ipc (%d-%d) %d\n", ipc_fd_a, sock_fd, code);
        // parse code
        if(sock_fd==-1) sock_fd = init_client();

        if(sock_fd!=-1){
          local_req.set_page(wanted_page);
          local_req.sendit(sock_fd);
        }
        // look for document given by [isbn, serial, pagenum]
      }
      else if(fd == sock_fd){
        if(pkg == NULL){ // create req
          printf("sock fd data available \n");
          int ret = create_ebp_pkg(&pkg, fd);
        }
        if(pkg){
          int val = pkg->doit(fd);
          if(val==1){
            pkg=NULL; // req finished then reset
          }
          else if(val<0) sock_fd = init_client();
        }
      }
    }
  }
}

local_book_manager::local_book_manager(){
  int fd[2];
  int r = socketpair(AF_LOCAL, SOCK_STREAM, 0, fd);
  ipc_fd_a = fd[1];
  ipc_fd_b = fd[0];

  // launch data_bk_agent thread
  static pthread_t ntid;
  int err = pthread_create(&ntid, NULL, data_bk_agent, NULL);
}

int local_book_manager::notify_from_bk(remote_magazine_content *magazine){
  // verify wanted page
  page_info_tag page;
  magazine->get_page(page);
  pthread_mutex_lock(&page_mutex);
  int response = 0;
  if((page.isbn==wanted_page.isbn)&&(page.issue==wanted_page.issue)&&(page.num==wanted_page.num)) write(ipc_fd_b, &response, sizeof(response));
  pthread_mutex_unlock(&page_mutex);
}

int local_book_manager::notify_from_bk(remote_magazine_info *magazine){
}

e_book *local_book_manager::get_book(int i, int s, int n){
  e_book *book = new e_book(i, s, n);
  return book;
}

int local_book_manager::read_book_shelf(char name[], int &isbn, int &issue, int &numb){
  int ret = -1;
  enum {S0=1, S1, S2};
  static int state = S0;

  static sqlite3 *db;
  static sqlite3_stmt *stmt;

  if(state == S0){
    sqlite3_open (data_file, &db);
    sqlite3_exec(db, sql_create_table_ebooks_content, NULL, NULL, NULL);

    char sql[128];
    sprintf(sql, "select issue, page_numb from ebooks_content where isbn=%d order by isbn asc;", isbn);

    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    state = S1;
  }

  if(state == S1){
    if(SQLITE_ROW == sqlite3_step(stmt)){
      issue = sqlite3_column_int(stmt, 0);
      numb = sqlite3_column_int(stmt, 1);

      sprintf(name, "%d_%d_0.jpg", isbn, issue); // cover page

      ret = 0;
    }
    else{
      sqlite3_finalize(stmt);
      sqlite3_close(db);

      state = S0;
    }
  }

  return ret;
}

int local_book_manager::read_book_catalogue(char name[], int &isbn){
  int ret = -1;
  enum {S0=1, S1, S2};
  static int state = S0;

  static sqlite3 *db;
  static sqlite3_stmt *stmt;

  if(state == S0){
    sqlite3_open (data_file, &db);
    sqlite3_exec(db, sql_create_table_ebooks_content, NULL, NULL, NULL);

    const char *sql = "select isbn,title from ebooks_info;";
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    state = S1;
  }

  if(state == S1){
    if(SQLITE_ROW == sqlite3_step(stmt)){
      isbn = sqlite3_column_int(stmt, 0);
      const char *msg = (const char *)sqlite3_column_text(stmt, 1);
      strcpy(name, msg);

      ret = 0;
    }
    else{
      sqlite3_finalize(stmt);
      sqlite3_close(db);

      state = S0;
    }
  }

  return ret;
}
