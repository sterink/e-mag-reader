#include <sys/stat.h>
#include <pthread.h>
#include <stdio.h>

#include "local_book_manager.h"

#include <string.h>
#include <sqlite3.h>

const char *data_file = "local_books_stock.db";
const char *sql_create_table_ebooks_info = "create table if not exists ebooks_info (isbn integer primary key not null, title text);";

const char *sql_create_table_ebooks_content = "create table if not exists ebooks_content (isbn integer not null, issue integer not null, page_numb integer, primary key (isbn,issue));";

local_book_manager::local_book_manager(){
  extern void *data_bk_agent(void *);

  // launch data_bk_agent thread
  static pthread_t ntid;
  int err = pthread_create(&ntid, NULL, data_bk_agent, NULL);
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
