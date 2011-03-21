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
  public:
  local_sys_info();
  public:
  int sendit(int);
};

class local_magazine_info:public ebook_pkg{
  mag_set pool[50];
  int item_num;

  public:
  local_magazine_info();
  public:
  int sendit(int fd);
  private:
  void retrieve_snapshot();
};

class local_magazine_request:public ebook_pkg{
  public:
    mag_tag tag;
  public:
    local_magazine_request();
  public:
    int sendit(int fd);
};

class remote_magazine_content:public ebook_pkg{
  int64_t f_size;
  int type; // 0 -- cover; 1 -- content
  // <<< ---------- cover-related field
  int page_size; // total page number
  /// --------------->>>

  public:
  mag_tag tag;
  private:
  char tmp_name[64];
  int save_fd; // outfile
  int64_t remain;

  public:
  int doit(int fd);
  int handle_header();

  private:
  void update_db();
  void notify();
};

// magazine brand info
class remote_magazine_info:public ebook_pkg{
  int isbn;
  char title[64]; // brand title

  public:
  int doit(int fd);
  int handle_header();
  private:
  void update_db();
  void notify();
};

// return -1 if error
// return 0 for success
int create_ebp_pkg(ebook_pkg **pkg, int fd);
#endif
