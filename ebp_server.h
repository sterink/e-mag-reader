#ifndef EBOOK_PROC_H_
#define EBOOK_PROC_H_
#include <sys/types.h>
#include "xmt_util.h"

class ebook_pkg{
  protected:
  int code, h_size;

  public:
    virtual int sendit(int fd);
    virtual int doit(int fd);
  protected:
    virtual int handle_header();
    friend int create_ebp_pkg(ebook_pkg **pkg, int fd);
};

class local_sys_info:public ebook_pkg{
  int machine, soft_version, pkg_version;
  public:
  int doit(int );
  protected:
  int handle_header();
};

class local_magazine_info:public ebook_pkg{
  mag_set pool[50];
  int item_num;

  public:
  int doit(int fd);
  protected:
  int handle_header();

  friend class book_stock_manager;
};

class local_magazine_request:public ebook_pkg{
  public:
    mag_tag tag;
  protected:
  int handle_header();
  public:
  int doit(int fd);
};

class remote_magazine_content:public ebook_pkg{
  int64_t f_size;
  int type; // 0 -- cover; 1 -- content
  // <<< ---------- cover-related field
  int page_size; // total page number

  public:
  mag_tag tag;
  public:
  remote_magazine_content();
  public:
  int sendit(int fd);

  public:
  void set_size(int t);
  void set_type(int t);
  void set_tag(mag_tag &);
};

// magazine brand info
class remote_magazine_info:public ebook_pkg{
  public:
    int isbn;
    char title[64]; // brand title

  public:
    remote_magazine_info();
  public:
    int sendit(int fd);
};

int create_ebp_pkg(ebook_pkg **pkg, int fd);
#endif
