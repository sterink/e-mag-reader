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
  pages_set_tag pool[512];
  private:
  int item_num;

  public:
  int doit(int fd);
  protected:
  int handle_header();

  friend class book_stock_manager;
};

class local_magazine_request:public ebook_pkg{
  public:
    page_info_tag page;
  protected:
  int handle_header();
  public:
  int doit(int fd);
};

class remote_magazine_content:public ebook_pkg{
  int64_t f_size;
  page_info_tag page;
  int type; // 0 -- cover; 1 -- content
  // <<< ---------- cover-related field
  int page_size; // total page number
  bit_bool_set preview;

  public:
  remote_magazine_content();
  public:
    int sendit(int fd);

  public:
    void set_size(int t);
    void set_type(int t);
    void set_preview(bit_bool_set &p);
    void set_page(page_info_tag &tag);
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
