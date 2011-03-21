#include "ebp_server.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <sqlite3.h>

#include "xmt_book_stock_manager.h"

extern book_stock_manager stock_mg;

// database file path
extern const char *data_file;

const int meta_len = 8;
static int meta_tag[meta_len/sizeof(int)]; // holder for header tag
static char buff[1024]; // buffer for residual header

// read pkg header into buffer
// 0 -- to be continued
// pasitive -- pkg code
// negative -- error, such as in case of connection closed
static int buffer_header_pkg(int fd){
  int size=0;
  int code = -1;
  int ret = 0;
  ret=recv(fd, &meta_tag, meta_len, MSG_WAITALL);
  if(ret==8) {
    code = meta_tag[0];
    size = meta_tag[1];
    ret=recv(fd, &buff, size, MSG_WAITALL);
    if(ret != size){
      printf("recv data error\n");
      code = -1;
    }
  }
  return code;
}

// returned obj is stored in data section
int create_ebp_pkg(ebook_pkg **ppp, int fd){
  static remote_magazine_content r_m_c;
  static remote_magazine_info r_m_i;
  static local_magazine_info l_m_i;
  static local_magazine_request l_m_r;
  static local_sys_info l_s_i;

  ebook_pkg *pkg = NULL;

  *ppp = pkg;
  int code = buffer_header_pkg(fd);
  if(code <= 0) return 0; // decide if channel broken
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
  pkg->h_size = meta_tag[1];
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
  printf("not implemented\n");
  return -1;
}

int local_sys_info::doit(int fd){
  return 1;
}

int local_sys_info::handle_header(){
  printf("local_sys_info \n");
  int offset = 0;
  {int *p = (int *)(buff+offset); machine = p[0]; offset+=sizeof(int);}
  {int *p = (int *)(buff+offset); soft_version = p[0]; offset+=sizeof(int);}
  {int *p = (int *)(buff+offset); pkg_version = p[0]; offset+=sizeof(int);}
  stock_mg.handle_request(this);
}

int local_magazine_request::handle_header(){
  printf("local_magazine_request :");
  int offset = 0;
  {mag_tag *p = (mag_tag *)(buff+offset); tag = p[0]; offset+=sizeof(mag_tag);}
  printf("local request %d %d \n", tag.isbn, tag.issue);
  stock_mg.handle_request(this);
}

int local_magazine_request::doit(int fd){
  return 1;
}

int local_magazine_info::handle_header(){
  printf("local_magazine_info \n");
  item_num = h_size/sizeof(struct mag_set);
  memcpy(pool, buff, h_size);
  stock_mg.handle_request(this);
}

int local_magazine_info::doit(int fd){
  return 1;
}

remote_magazine_content::remote_magazine_content(){
  // write magic word
  code = OP_REMOTE_MAGAZINE_CONTENT;
}

int remote_magazine_content::sendit(int fd){
  // calculate head section size
  h_size = sizeof(f_size) + sizeof(type) + sizeof(tag);
  if(type==MAGAZINE_FREE) h_size += sizeof(page_size);

  ebook_pkg::sendit(fd);

  int isbn, issue;
  isbn = tag.isbn; issue = tag.issue;
  // coin filename
  char name[64];
  sprintf(name, "%d_%d_%d.zip", isbn, issue, type);

  int ret;
  f_size = 0;
  struct stat sb;
  ret = stat(name, &sb);
  if(ret==0) f_size = sb.st_size;

  ret = write(fd, &f_size, sizeof(f_size));
  ret = write(fd, &type, sizeof(type));
  ret = write(fd, &tag, sizeof(tag));
  if(type==MAGAZINE_FREE){
    ret = write(fd, &page_size, sizeof(page_size));
  }

  int infile = open(name, O_RDONLY, 0);
  int cnt = 0;
  char line[1024];
  while((cnt = read(infile, line, 1024))>0) ret = write(fd, line, cnt);
  close(infile);
}

void remote_magazine_content::set_size(int t){
  page_size = t;
}

void remote_magazine_content::set_type(int t){
  type = t;
}

void remote_magazine_content::set_tag(mag_tag &t){
  tag = t;
}

remote_magazine_info::remote_magazine_info(){
  // write magic word
  code = OP_REMOTE_MAGAZINE_INFO;
}

int remote_magazine_info::sendit(int fd){
  h_size = sizeof(isbn) + sizeof(title);

  ebook_pkg::sendit(fd);

  write(fd, &isbn, sizeof(isbn));
  write(fd, title, sizeof(title));
}
