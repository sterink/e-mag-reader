#include <stdio.h>
#include <sys/types.h>
#include "xmt_book.h"
#include "xmt_errno.h"
#include "sqlite3.h"
#include "xmt_util.h"

#include <assert.h>
#include <pthread.h>

extern int ipc_fd_a;

// page wanted 
extern page_info_tag wanted_page;
extern pthread_mutex_t page_mutex;

e_book::e_book(int i, int s, int n){
  isbn = i, issue = s, page_numb = n;
  // retrieve saved page last time
  pos = 0;
}

int e_book::get_main_cover(char name[]){
  sprintf(name, "%d_%d_%d.jpg", isbn, issue, pos); // cover page
  return 1;
}

int e_book::get_history_page(char name[]){
  sprintf(name, "%d_%d_%d.html", isbn, issue, pos); // bookmark page
  return 1;
}

int e_book::go_relative_page(int i, char name[]){
  int ii = pos + i;
  if(ii<0 || ii>page_numb) return -NOEXIST;
  return go_absolute_page(ii, name);
}

int e_book::go_absolute_page(int index, char name[]){
  if(index<0 || index>page_numb) return -NOEXIST;
  bool is_ready = false;
  int ret = 0;

  extern const char *data_file;
  extern const char *sql_create_table_ebooks_content;
  // retrieve page given [isbn, issue, pos]
  sqlite3 * db;
  sqlite3_open (data_file, &db);
  sqlite3_stmt * stmt;
  sqlite3_exec(db, sql_create_table_ebooks_content, NULL, NULL, NULL);

  int row_cnt = 0;
  bit_bool_set preview_set, total_set;

  char sql[256];
  sprintf(sql, "select preview_set,total_set from ebooks_content where isbn=%d and issue=%d;", isbn, issue);
  sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if(SQLITE_ROW == sqlite3_step(stmt)){
    preview_set.set_data(sqlite3_column_blob(stmt, 0));
    total_set.set_data(sqlite3_column_blob(stmt, 1));
    row_cnt++;
  }
  sqlite3_finalize(stmt);
  sqlite3_close(db);

  if(row_cnt > 0) is_ready = total_set[index];
  if(!is_ready){ // proxy request out
    pthread_mutex_lock(&page_mutex);
    wanted_page.isbn = isbn;
    wanted_page.issue = issue;
    wanted_page.num = index;
    pthread_mutex_unlock(&page_mutex);

    int code = 1;
    write(ipc_fd_a, &code, sizeof(code));

    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(ipc_fd_a, &readset);
    int maxfd = ipc_fd_a;
    timeval timeout={8,0};
    int nfds = select(maxfd+1, &readset, NULL, NULL, &timeout);
    if(nfds>0){
      ret = 0;
      read(ipc_fd_a, &code, 4);

      is_ready = true;
    }
    else if(nfds==0){
      pthread_mutex_lock(&page_mutex);
      wanted_page.isbn = -1; // invalid isbn
      pthread_mutex_unlock(&page_mutex);
      ret = -TIMEOUT;
      //printf("time out \n");
    }
  }
  if(is_ready){ // page available
    sprintf(name, "%d_%d_%d.html", isbn, issue, index);
    pos = index;
    ret = 0;
  }
  return ret;
}

