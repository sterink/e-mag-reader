#include "ebp_client.h"
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sqlite3.h>
#include "xmt_version.h"
#include "xmt_zlib.h"
#include "local_book_manager.h"

extern const char *sql_create_table_ebooks_info;

extern const char *sql_create_table_ebooks_content;
extern int ipc_fd_b;

extern int notify_fd;
extern mag_tag wanted_tag;

// database file path
extern const char *data_file;

const int meta_len = 8;
static int meta_tag[meta_len/sizeof(int)]; // holder for header tag
static char buff[1024]; // buffer for residual header

// read pkg header into buffer aggressively
// 0 -- to be continued
// pasitive -- pkg code
// negative -- error, such as in case of connection closed
static int buffer_header_pkg(int fd){
  enum{
    S0, S1, S2, S3, S4
  };
  int ret = 0;
  static int state = S0;
  static int remain, size;
  static int code=-1;
  static char *pp;
  const int meta_len = 8;
  while(true){
    if(state==S0){
      remain = size = meta_len; pp = (char *)meta_tag;
      state = S1;
    } // code
    else if(state == S2) { // header
      code = meta_tag[0];
      remain = size = meta_tag[1];
      pp = buff;
      state = S3;
    }
    else if(state == S4){
      ret = code;
      state = S0;
      break;
    }
    if(state == S1 || state == S3){
      while(true){ // read as more as possible
        if(remain == 0) break;

        char *p=pp+size-remain;
        ret=recv(fd, p, remain, MSG_DONTWAIT);
        if(ret==0) goto out;  // socket shutdown 
        else if(ret<0){
          if(errno==EAGAIN) goto out; // time out
          else{
            printf("head pkg error %s\n", strerror(errno));
            goto out;
          }
        }
        else remain -= ret;
      }
    }
    // state transition
    if(state==S1 && remain==0) state = S2;
    if(state==S3 && remain==0) state = S4;
  }

out:
  return ret;
}

// returned obj is stored in data section
int create_ebp_pkg(ebook_pkg **ppp, int fd){
  static remote_magazine_content r_m_c;
  static remote_magazine_info r_m_i;
  static local_magazine_info l_m_i;
  static local_magazine_request l_m_r;
  static local_sys_info l_s_i;

  ebook_pkg *pkg = NULL;

  *ppp = NULL;
  int code = buffer_header_pkg(fd);
  if(code <= 0) return code; // decide if channel broken
  switch(code){
    case OP_LOCAL_SYS_INFO:
      pkg = &l_s_i;
      break;
    case OP_LOCAL_MAGAZINE_INFO:
      pkg = &l_m_i;
      break;
    case OP_LOCAL_MAGAZINE_REQUEST:
      pkg = &l_m_r;
      break;
    case OP_REMOTE_MAGAZINE_CONTENT:
      //pkg = new remote_magazine_content;
      pkg = &r_m_c;
      break;
    case OP_REMOTE_MAGAZINE_INFO:
      //pkg = new remote_magazine_info;
      pkg = &r_m_i;
      break;
    default:
      printf("pkg protocol error\n");
  }
  pkg->handle_header();
  *ppp = pkg;
  return code>0;
}

// return value
// 1 -- done
// 0 -- not done
// negative -- error code
int ebook_pkg::doit(int fd){
  printf("not implemented\n");
}

// send header & file section size
int ebook_pkg::sendit(int fd){
  if(fd<0) return -1;
  write(fd, &code, sizeof(code));
  write(fd, &h_size, sizeof(h_size));
  return -1;
}

int ebook_pkg::handle_header(){
  h_size = meta_tag[1];
  return -1;
}

local_sys_info::local_sys_info(){
  code = OP_LOCAL_SYS_INFO;
}

extern const int machine, soft_version, protocol_version;
int local_sys_info::sendit(int fd){
  if(fd<0) return -1;
  h_size = sizeof(machine) + sizeof(soft_version) + sizeof(protocol_version);
  ebook_pkg::sendit(fd);

  int ret = 0;
  ret = write(fd, &machine, sizeof(machine));
  ret = write(fd, &soft_version, sizeof(soft_version));
  ret = write(fd, &protocol_version, sizeof(protocol_version));
}

local_magazine_request::local_magazine_request(){
  code = OP_LOCAL_MAGAZINE_REQUEST;
}

int local_magazine_request::sendit(int fd){
  if(fd<0) return -1;
  h_size = sizeof(tag);
  ebook_pkg::sendit(fd);

  int ret = 0;
  ret = write(fd, &tag, sizeof(tag));
}

local_magazine_info::local_magazine_info(){
  // write magic word
  code = OP_LOCAL_MAGAZINE_INFO;
}

int local_magazine_info::sendit(int fd){
  if(fd<0) return -1;
  retrieve_snapshot();

  h_size = item_num * sizeof(mag_set);
  ebook_pkg::sendit(fd);

  write(fd, pool, h_size);
}

void local_magazine_info::retrieve_snapshot(){
  mag_set cur = {-1, 0, 0};
  item_num = -1;
  int pre_issue = -1;
  // retrieve magazine snapshot
  sqlite3 * db;
  sqlite3_open(data_file, &db);
  sqlite3_stmt * stmt;
  sqlite3_exec(db, sql_create_table_ebooks_content, NULL, NULL, NULL);

  const char *sql = "select isbn, issue from ebooks_content order by isbn asc, issue asc;";
  sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

  while(sqlite3_step (stmt)==SQLITE_ROW){
    int isbn, issue;
    isbn = sqlite3_column_int(stmt, 0);
    issue = sqlite3_column_int(stmt, 1);
    if(isbn!=cur.isbn){
      if(item_num==-1) item_num++;
      else{
        cur.max_issue = pre_issue; pool[item_num++] = cur;
      }
      cur.isbn = isbn; cur.min_issue = issue;
    }
    pre_issue = issue;
  }
  sqlite3_finalize(stmt);

  sqlite3_close(db);
  if(item_num>=0){
    cur.max_issue = pre_issue; pool[item_num++] = cur;
  }
}

int remote_magazine_content::handle_header(){
  ebook_pkg::handle_header();

  int offset = 0;
  {int64_t *p = (int64_t *)(buff+offset); f_size = p[0]; offset+=sizeof(int64_t);}
  {int *p = (int *)(buff+offset); type = p[0]; offset+=sizeof(int);}
  {mag_tag *p = (mag_tag *)(buff+offset); tag = p[0]; offset+=sizeof(mag_tag);}
  if(type==MAGAZINE_FREE) {
    {int *p = (int *)(buff+offset); page_size = p[0]; offset+=sizeof(int);}
  }
  remain = f_size;
  save_fd = -1;
  if(f_size>0){
    strcpy(tmp_name, "tempfile_XXXXXX");
    save_fd = mkstemp(tmp_name);
  }
  //save_fd = open(name, O_CREAT | O_TRUNC | O_WRONLY, 0666);
}

int remote_magazine_content::doit(int fd){
  if(save_fd>0){
    int fildes = save_fd;
    const int max_s = 1024;
    char buff[max_s];
    int size = remain>max_s?max_s:remain;
    int n=recv(fd, buff, size, MSG_DONTWAIT);
    if(n>0)  write(fildes, buff, n);
    else if(n==0) return -1; // shutdown elegantly
    else{
      if(errno==EAGAIN) return 0;
      else{
        printf("something is wrong %d-%d-%s\n", n,errno,strerror(errno));
        return -1;
      }
    }
    remain -= n;
    if(remain == 0){
      // decomp
      int status;
      sprintf(buff, "%d", tag.isbn);
      status = mkdir(buff, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      status = chdir(buff);

      sprintf(buff, "../%s", tmp_name);
      my_zip_extract(buff, NULL);
      unlink(buff);

      // restore path
      status = chdir("..");

      close(save_fd); save_fd = -1;

      update_db();
      notify();
      return 1;
    }
  }
  return 0;
}

void remote_magazine_content::notify(){
  // verify wanted page
  if(tag == wanted_tag) write(ipc_fd_b, &wanted_tag, sizeof(wanted_tag));
  const char *str = "local book manager content\n";
  write(notify_fd, str, strlen(str));
}

void remote_magazine_content::update_db(){
  int isbn, issue, numb;
  isbn=tag.isbn, issue=tag.issue;
  numb=page_size;
  sqlite3 * db = NULL;
  if(type == MAGAZINE_FREE){
    sqlite3_open(data_file, &db);
    sqlite3_stmt * stmt;

    // adjust set
    const char *sql = "insert or replace into ebooks_content (isbn, issue, page_numb) values (?,?,?);";
    //sprintf(sql, "insert or replace ebooks_content set isbn=?, issue=?, preview_set=?");
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, isbn);
    sqlite3_bind_int(stmt, 2, issue);
    sqlite3_bind_int(stmt, 3, numb);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  }
  sqlite3_close(db);
}

int remote_magazine_info::handle_header(){
  ebook_pkg::handle_header();

  int offset = 0;
  {int *p = (int *)(buff+offset); isbn = p[0]; offset+=sizeof(int);}
  {char *p = (char *)(buff+offset); strncpy(title, p, sizeof(title)); offset+=sizeof(title);}
}

int remote_magazine_info::doit(int fd){
  update_db();
  notify();
  return 1;
}

void remote_magazine_info::notify(){
  const char *str = "local book manager info\n";
  write(notify_fd, str, strlen(str));
}

void  remote_magazine_info::update_db(){
  sqlite3 * db;
  sqlite3_open(data_file, &db);
  sqlite3_stmt * stmt;

  sqlite3_exec(db, sql_create_table_ebooks_info, NULL, NULL, NULL);

  const char *sql="insert into ebooks_info (isbn, title) values (?,?);";
  sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  sqlite3_bind_int(stmt, 1, isbn);
  sqlite3_bind_text(stmt, 2, title, -1, SQLITE_STATIC);

  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  sqlite3_close(db);
}

